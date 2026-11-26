"""
Hailo-accelerated YOLO11-OBB inference for the ctrlX AI Detector.

The HEF must be compiled with the 9 raw head end nodes (per scale 8/16/32:
cv2 bbox-DFL 64ch, cv3 class scores, cv4 angle 1ch) as described in
360-OBB-Research-Findings.md section 1.3. All decoding (DFL, sigmoid,
angle, rotated NMS) runs here on the CPU, ported from the official Hailo
example: Hailo-Application-Code-Examples/runtime/python/
oriented_object_detection/oriented_object_detection_post_process.py

Output tensors are identified automatically per scale by channel count
(64 = bbox, 1 = angle, anything else = classes). For single-class models
both the angle and class head have 1 channel; the tie is broken by the
graph order of the compiled layer names (bbox -> angle -> cls, matching
the Ultralytics OBB head topology, e.g. conv53/conv54/conv57).

The returned angles follow the same convention as yoloobb.YOLOOBB
([-pi/4, 3pi/4) from the +x axis); the heading classifier in
yoloobb/heading.py applies on top unchanged.
"""

import math
import re
import threading

import cv2
import numpy as np

from common.hailo_inference import HailoInfer
from yoloobb.YOLOOBB import YOLOOBB


def _sigmoid(x):
    return 1.0 / (1.0 + np.exp(-x))


def _softmax(x, axis=-1):
    x_max = np.max(x, axis=axis, keepdims=True)
    e = np.exp(x - x_max)
    return e / np.sum(e, axis=axis, keepdims=True)


def _name_key(name):
    """Numeric sort key for hailo layer names like '.../conv53'."""
    nums = re.findall(r"\d+", name)
    return int(nums[-1]) if nums else 0


class HailoOBB:
    """YOLO11-OBB on Hailo. Same call interface as yoloobb.YOLOOBB."""

    REG_MAX = 16  # fixed for the YOLO11 OBB head

    # Borrow the post-detection logic from the ONNX implementation so the
    # letterbox undo, rotated NMS and drawing exist exactly once.
    rescale_boxes = YOLOOBB.rescale_boxes
    decode_angles = YOLOOBB.decode_angles
    rotated_nms = YOLOOBB.rotated_nms
    _python_rotated_nms = YOLOOBB._python_rotated_nms
    to_cv_rects = staticmethod(YOLOOBB.to_cv_rects)
    rotated_iou = staticmethod(YOLOOBB.rotated_iou)
    draw_obb = YOLOOBB.draw_obb

    def __init__(self, hef_path, conf_thres=0.3, iou_thres=0.5,
                 timeout_ms=10000):
        self.conf_threshold = conf_thres
        self.iou_threshold = iou_thres
        self.angle_mode = "180"
        self.timeout_ms = timeout_ms

        self.infer = HailoInfer(hef_path, batch_size=1,
                                input_type="UINT8", output_type="FLOAT32")
        shape = self.infer.get_input_shape()  # (H, W, C)
        self.input_height, self.input_width = int(shape[0]), int(shape[1])
        self.output_names = list(self.infer.output_type.keys())
        self._head_map = None  # resolved on first frame from output shapes

    def __call__(self, image):
        return self.detect_objects(image)

    def close(self):
        self.infer.close()

    def detect_objects(self, image):
        """
        Returns:
            boxes:     (n, 4) [cx, cy, w, h] in original image pixels
            scores:    (n,) confidences
            class_ids: (n,) class indices
            angles:    (n,) radians, [-pi/4, 3pi/4)
        """
        tensor = self.prepare_input(image)
        outputs = self._infer_sync(tensor)
        raw = self._decode_heads(outputs)  # boxes/cls/angles in input px
        self.boxes, self.scores, self.class_ids, self.angles = \
            self._filter_and_nms(*raw)
        return self.boxes, self.scores, self.class_ids, self.angles

    def prepare_input(self, image):
        """Letterbox to input size; RGB uint8 HWC (Hailo quantizes on-chip).
        Same geometry as yoloobb.YOLOOBB.prepare_input."""
        self.img_height, self.img_width = image.shape[:2]
        self.scale = min(self.input_width / self.img_width,
                         self.input_height / self.img_height)
        new_w = round(self.img_width * self.scale)
        new_h = round(self.img_height * self.scale)
        self.pad_x = (self.input_width - new_w) / 2
        self.pad_y = (self.input_height - new_h) / 2

        resized = cv2.resize(image, (new_w, new_h),
                             interpolation=cv2.INTER_LINEAR)
        padded = np.full((self.input_height, self.input_width, 3), 114,
                         dtype=np.uint8)
        top, left = int(round(self.pad_y - 0.1)), int(round(self.pad_x - 0.1))
        padded[top:top + new_h, left:left + new_w] = resized
        return cv2.cvtColor(padded, cv2.COLOR_BGR2RGB)

    def _infer_sync(self, tensor):
        """Run one frame through the async API and wait for completion."""
        done = threading.Event()
        results = {}

        def callback(completion_info, bindings_list=None):
            try:
                if completion_info.exception:
                    results["__error__"] = completion_info.exception
                else:
                    binding = bindings_list[0]
                    for name in self.output_names:
                        results[name] = np.asarray(
                            binding.output(name).get_buffer(), dtype=np.float32)
            finally:
                done.set()

        self.infer.run([tensor], callback)
        if not done.wait(self.timeout_ms / 1000.0):
            raise RuntimeError("Hailo inference timed out")
        if "__error__" in results:
            raise RuntimeError(f"Hailo inference failed: {results['__error__']}")
        return results

    @staticmethod
    def _strip_batch(arr):
        """Drop a leading batch dim of 1 (buffers may be HWC or NHWC),
        keeping 1-channel tensors 3-dimensional."""
        return arr[0] if arr.ndim == 4 and arr.shape[0] == 1 else arr

    def _resolve_head_map(self, outputs):
        """Group output tensors by scale and role.

        Returns {stride: {"bbox": name, "cls": name, "angle": name}}.
        """
        by_scale = {}
        for name in sorted(self.output_names, key=_name_key):
            arr = self._strip_batch(outputs[name])
            if arr.ndim != 3:
                raise RuntimeError(
                    f"Unexpected output shape {outputs[name].shape} for "
                    f"'{name}' - HEF must expose the 9 raw conv end nodes")
            h = arr.shape[0]
            by_scale.setdefault(h, []).append((name, arr.shape[2]))

        head_map = {}
        for h, entries in by_scale.items():
            stride = self.input_height // h
            bbox = [n for n, c in entries if c == 4 * self.REG_MAX]
            ones = [n for n, c in entries if c == 1]
            rest = [n for n, c in entries
                    if c != 4 * self.REG_MAX and c != 1]
            if len(bbox) != 1 or len(entries) != 3:
                raise RuntimeError(
                    f"Cannot identify OBB heads at stride {stride}: "
                    f"{entries}. Expected 3 outputs (64ch bbox, cls, 1ch angle)")
            if rest:  # multi-class model: the 1-channel one is the angle
                cls_name, angle_name = rest[0], ones[0]
            else:     # single-class: graph order is bbox -> angle -> cls
                angle_name, cls_name = ones[0], ones[1]
                print("HailoOBB: single-class model, assuming layer order "
                      f"angle='{angle_name}' cls='{cls_name}'", flush=True)
            head_map[stride] = {"bbox": bbox[0], "cls": cls_name,
                                "angle": angle_name}

        if sorted(head_map.keys()) != [8, 16, 32]:
            raise RuntimeError(f"Unexpected strides {sorted(head_map.keys())}, "
                               f"expected [8, 16, 32]")
        return head_map

    def _decode_heads(self, outputs):
        """DFL + dist2rbox decode (numpy port of the Hailo example /
        Ultralytics OBB head). Returns boxes (N,4) in input pixels,
        class scores (N,nc), angles (N,) radians."""
        if self._head_map is None:
            self._head_map = self._resolve_head_map(outputs)

        proj = np.arange(self.REG_MAX, dtype=np.float32)
        all_boxes, all_cls, all_angles = [], [], []

        for stride in (8, 16, 32):
            names = self._head_map[stride]
            bbox = self._strip_batch(outputs[names["bbox"]])   # (H, W, 64)
            cls = self._strip_batch(outputs[names["cls"]])     # (H, W, nc)
            angle = self._strip_batch(outputs[names["angle"]]) # (H, W, 1)
            h, w = bbox.shape[0], bbox.shape[1]
            n = h * w

            # DFL: 64 channels -> 4 distances (l, t, r, b) in grid units
            dist = _softmax(bbox.reshape(n, 4, self.REG_MAX), axis=-1) @ proj
            lt, rb = dist[:, :2], dist[:, 2:]

            ang = (_sigmoid(angle.reshape(n)) - 0.25) * np.pi

            # dist2rbox: rotate the center offset by the angle
            off = (rb - lt) / 2.0
            cos_a, sin_a = np.cos(ang), np.sin(ang)
            off_x = off[:, 0] * cos_a - off[:, 1] * sin_a
            off_y = off[:, 0] * sin_a + off[:, 1] * cos_a

            gx = np.tile(np.arange(w, dtype=np.float32) + 0.5, h)
            gy = np.repeat(np.arange(h, dtype=np.float32) + 0.5, w)

            boxes = np.stack([(off_x + gx) * stride,
                              (off_y + gy) * stride,
                              (lt[:, 0] + rb[:, 0]) * stride,
                              (lt[:, 1] + rb[:, 1]) * stride], axis=1)

            nc = 1 if cls.ndim == 2 else cls.shape[2]
            all_boxes.append(boxes)
            all_cls.append(_sigmoid(cls.reshape(n, nc)))
            all_angles.append(ang)

        return (np.concatenate(all_boxes), np.concatenate(all_cls),
                np.concatenate(all_angles))

    def _filter_and_nms(self, boxes, cls_scores, angles):
        scores = np.max(cls_scores, axis=1)
        mask = scores > self.conf_threshold
        if not np.any(mask):
            empty = np.array([])
            return empty.reshape(0, 4), empty, empty, empty

        boxes = self.rescale_boxes(boxes[mask])
        scores = scores[mask]
        class_ids = np.argmax(cls_scores[mask], axis=1)
        angles = self.decode_angles(angles[mask])

        indices = self.rotated_nms(boxes, angles, scores)
        return (boxes[indices], scores[indices],
                class_ids[indices], angles[indices])
