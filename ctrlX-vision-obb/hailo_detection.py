"""
Hailo-accelerated YOLO object detection for the ctrlX AI Detector.

Supports both common HEF flavors:

1. **On-chip NMS** (Hailo Model Zoo default for detect models): a single
   output in HAILO_NMS_BY_CLASS format; per class an array of
   [ymin, xmin, ymax, xmax, score] normalized to the model input.
2. **Raw end nodes** (6 conv outputs: per scale 8/16/32 a 64ch bbox-DFL
   head and a class head — same compile workflow as the OBB recipe,
   just without the angle nodes). Decoding + NMS run here on the CPU.

The flavor is picked automatically from the number of HEF outputs.
Reuses infrastructure (device driver, letterbox, sync inference) from
hailo_obb.HailoOBB.
"""

import cv2
import numpy as np

from hailo_obb import HailoOBB, _sigmoid, _softmax, _name_key


class HailoDetection(HailoOBB):
    """YOLO detection on Hailo. Same call interface as yolodet.YOLODet."""

    def __init__(self, hef_path, conf_thres=0.3, iou_thres=0.5,
                 timeout_ms=10000):
        super().__init__(hef_path, conf_thres=conf_thres,
                         iou_thres=iou_thres, timeout_ms=timeout_ms)
        if len(self.output_names) == 1:
            self.flavor = "nms_on_chip"
        elif len(self.output_names) == 6:
            self.flavor = "raw_heads"
        else:
            raise RuntimeError(
                f"Unexpected number of HEF outputs "
                f"({len(self.output_names)}). Expected 1 (model-zoo HEF "
                f"with on-chip NMS) or 6 (raw bbox+cls end nodes)")
        print(f"HailoDetection: using '{self.flavor}' HEF flavor", flush=True)

    def detect_objects(self, image):
        """
        Returns:
            boxes:     (n, 4) [cx, cy, w, h] in original image pixels
            scores:    (n,) confidences
            class_ids: (n,) class indices
        """
        tensor = self.prepare_input(image)
        outputs = self._infer_sync(tensor)

        if self.flavor == "nms_on_chip":
            self.boxes, self.scores, self.class_ids = \
                self._parse_nms_output(outputs)
        else:
            boxes, cls_scores = self._decode_raw_heads(outputs)
            self.boxes, self.scores, self.class_ids = \
                self._filter_and_nms_axis(boxes, cls_scores)

        return self.boxes, self.scores, self.class_ids

    # ------------------------------------------------------------------
    # Flavor 1: on-chip NMS output
    # ------------------------------------------------------------------

    def _parse_nms_output(self, outputs):
        """Per class: array (n_dets, 5) of [ymin, xmin, ymax, xmax, score]
        normalized to the model input. Un-letterbox to original pixels."""
        result = outputs[self.output_names[0]]
        boxes, scores, class_ids = [], [], []

        for class_id, dets in enumerate(result):
            dets = np.asarray(dets, dtype=np.float32).reshape(-1, 5)
            for ymin, xmin, ymax, xmax, score in dets:
                if score <= self.conf_threshold:
                    continue
                # normalized -> input px -> original px (letterbox undo)
                x1 = (xmin * self.input_width - self.pad_x) / self.scale
                y1 = (ymin * self.input_height - self.pad_y) / self.scale
                x2 = (xmax * self.input_width - self.pad_x) / self.scale
                y2 = (ymax * self.input_height - self.pad_y) / self.scale
                boxes.append([(x1 + x2) / 2, (y1 + y2) / 2, x2 - x1, y2 - y1])
                scores.append(float(score))
                class_ids.append(class_id)

        if not boxes:
            empty = np.array([])
            return empty.reshape(0, 4), empty, empty
        return (np.array(boxes, dtype=np.float32),
                np.array(scores, dtype=np.float32),
                np.array(class_ids, dtype=int))

    def _infer_sync(self, tensor):
        """Like HailoOBB._infer_sync, but without forcing the buffers to
        float32 ndarrays: the on-chip NMS output is a per-class python
        structure, which must be passed through as-is."""
        import threading
        done = threading.Event()
        results = {}

        def callback(completion_info, bindings_list=None):
            try:
                if completion_info.exception:
                    results["__error__"] = completion_info.exception
                else:
                    binding = bindings_list[0]
                    for name in self.output_names:
                        results[name] = binding.output(name).get_buffer()
            finally:
                done.set()

        self.infer.run([tensor], callback)
        if not done.wait(self.timeout_ms / 1000.0):
            raise RuntimeError("Hailo inference timed out")
        if "__error__" in results:
            raise RuntimeError(f"Hailo inference failed: {results['__error__']}")
        return results

    # ------------------------------------------------------------------
    # Flavor 2: raw bbox + cls end nodes (6 outputs)
    # ------------------------------------------------------------------

    def _resolve_det_head_map(self, outputs):
        """{stride: {"bbox": name, "cls": name}} by channel count
        (64 = bbox-DFL, anything else = classes)."""
        by_scale = {}
        for name in sorted(self.output_names, key=_name_key):
            arr = self._strip_batch(np.asarray(outputs[name]))
            if arr.ndim != 3:
                raise RuntimeError(f"Unexpected output shape for '{name}'")
            by_scale.setdefault(arr.shape[0], []).append((name, arr.shape[2]))

        head_map = {}
        for h, entries in by_scale.items():
            stride = self.input_height // h
            bbox = [n for n, c in entries if c == 4 * self.REG_MAX]
            rest = [n for n, c in entries if c != 4 * self.REG_MAX]
            if len(bbox) != 1 or len(rest) != 1:
                raise RuntimeError(
                    f"Cannot identify detect heads at stride {stride}: "
                    f"{entries}. Expected one 64ch bbox and one cls output")
            head_map[stride] = {"bbox": bbox[0], "cls": rest[0]}

        if sorted(head_map.keys()) != [8, 16, 32]:
            raise RuntimeError(f"Unexpected strides {sorted(head_map.keys())}")
        return head_map

    def _decode_raw_heads(self, outputs):
        """DFL + dist2bbox (the OBB decode with angle = 0)."""
        if self._head_map is None:
            self._head_map = self._resolve_det_head_map(outputs)

        proj = np.arange(self.REG_MAX, dtype=np.float32)
        all_boxes, all_cls = [], []

        for stride in (8, 16, 32):
            names = self._head_map[stride]
            bbox = self._strip_batch(np.asarray(outputs[names["bbox"]]))
            cls = self._strip_batch(np.asarray(outputs[names["cls"]]))
            h, w = bbox.shape[0], bbox.shape[1]
            n = h * w

            dist = _softmax(bbox.reshape(n, 4, self.REG_MAX), axis=-1) @ proj
            lt, rb = dist[:, :2], dist[:, 2:]

            gx = np.tile(np.arange(w, dtype=np.float32) + 0.5, h)
            gy = np.repeat(np.arange(h, dtype=np.float32) + 0.5, w)

            boxes = np.stack([((rb[:, 0] - lt[:, 0]) / 2 + gx) * stride,
                              ((rb[:, 1] - lt[:, 1]) / 2 + gy) * stride,
                              (lt[:, 0] + rb[:, 0]) * stride,
                              (lt[:, 1] + rb[:, 1]) * stride], axis=1)

            nc = 1 if cls.ndim == 2 else cls.shape[2]
            all_boxes.append(boxes)
            all_cls.append(_sigmoid(cls.reshape(n, nc)))

        return np.concatenate(all_boxes), np.concatenate(all_cls)

    def _filter_and_nms_axis(self, boxes, cls_scores):
        scores = np.max(cls_scores, axis=1)
        mask = scores > self.conf_threshold
        if not np.any(mask):
            empty = np.array([])
            return empty.reshape(0, 4), empty, empty

        boxes = self.rescale_boxes(boxes[mask])
        scores = scores[mask]
        class_ids = np.argmax(cls_scores[mask], axis=1)

        tlwh = boxes.copy()
        tlwh[:, 0] -= tlwh[:, 2] / 2
        tlwh[:, 1] -= tlwh[:, 3] / 2
        indices = cv2.dnn.NMSBoxes(tlwh.tolist(),
                                   scores.astype(np.float32).tolist(),
                                   float(self.conf_threshold),
                                   float(self.iou_threshold))
        indices = np.array(indices).reshape(-1).astype(int)
        return boxes[indices], scores[indices], class_ids[indices]

    # ------------------------------------------------------------------

    def draw_detections(self, image, color=(0, 255, 0), thickness=2,
                        class_names=None):
        out = image.copy()
        for box, score, class_id in zip(self.boxes, self.scores,
                                        self.class_ids):
            cx, cy, w, h = box
            x1, y1 = int(cx - w / 2), int(cy - h / 2)
            x2, y2 = int(cx + w / 2), int(cy + h / 2)
            cv2.rectangle(out, (x1, y1), (x2, y2), color, thickness)
            name = class_names[class_id] if class_names else f"ID:{class_id}"
            cv2.putText(out, f"{name} {score:.2f}", (x1, y1 - 5),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 2)
        return out
