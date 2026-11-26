import cv2
import numpy as np

from yoloobb.YOLOOBB import YOLOOBB


class YOLODet(YOLOOBB):
    """
    Standard YOLO11/YOLOv8 object detection with ONNX Runtime.

    Reuses the session management, letterbox preprocessing and
    letterbox-undo from YOLOOBB; only output decoding and drawing differ.

    Export format (stock Ultralytics): output (1, 4 + nc, N) where
    [0:4] = cx, cy, w, h in input pixels and [4:] = sigmoided class scores.
    """

    def __init__(self, path, conf_thres=0.7, iou_thres=0.5):
        super().__init__(path, conf_thres=conf_thres, iou_thres=iou_thres,
                         angle_mode="180")

    def detect_objects(self, image):
        """
        Returns:
            boxes:     (n, 4) [cx, cy, w, h] in original image pixels
            scores:    (n,) confidences
            class_ids: (n,) class indices
        """
        input_tensor = self.prepare_input(image)
        outputs = self.inference(input_tensor)
        self.boxes, self.scores, self.class_ids = self.process_output(outputs[0])
        return self.boxes, self.scores, self.class_ids

    def process_output(self, output):
        predictions = np.squeeze(output).T  # (N, 4 + nc)
        num_classes = predictions.shape[1] - 4

        class_scores = predictions[:, 4:4 + num_classes]
        scores = np.max(class_scores, axis=1)

        mask = scores > self.conf_threshold
        if not np.any(mask):
            empty = np.array([])
            return empty.reshape(0, 4), empty, empty

        predictions = predictions[mask]
        scores = scores[mask]
        class_ids = np.argmax(predictions[:, 4:4 + num_classes], axis=1)
        boxes = self.rescale_boxes(predictions[:, :4])

        indices = self.axis_aligned_nms(boxes, scores)
        return boxes[indices], scores[indices], class_ids[indices]

    def axis_aligned_nms(self, boxes, scores):
        """cv2.dnn.NMSBoxes on [x, y, w, h] (top-left) boxes."""
        if len(boxes) == 0:
            return np.array([], dtype=int)
        tlwh = boxes.copy()
        tlwh[:, 0] -= tlwh[:, 2] / 2
        tlwh[:, 1] -= tlwh[:, 3] / 2
        indices = cv2.dnn.NMSBoxes(tlwh.tolist(),
                                   scores.astype(np.float32).tolist(),
                                   float(self.conf_threshold),
                                   float(self.iou_threshold))
        return np.array(indices).reshape(-1).astype(int)

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
