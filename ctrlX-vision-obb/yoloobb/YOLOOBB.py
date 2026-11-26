import math
import time

import cv2
import numpy as np
import onnxruntime

from yoloobb.utils import draw_obb_detections


class YOLOOBB:
    """
    YOLO11/YOLOv8 Oriented Bounding Box (OBB) detection with ONNX Runtime.

    Structured after the official Ultralytics YOLOv8-ONNXRuntime example,
    adapted for the OBB task per the documented export format:

      output: (1, 4 + nc + 1, N)
        [0:4]   cx, cy, w, h        decoded, in letterboxed input pixels
        [4:4+nc] class scores       sigmoid already applied in-graph
        [-1]    angle               radians in [-pi/4, 3pi/4), decoded
                                    in-graph -- do NOT sigmoid again

    Angle convention: measured from the +x axis, clockwise in image
    coordinates (y down). Standard OBB is 180-degree ambiguous; use
    yoloobb.heading.HeadingClassifier to resolve the full 360 heading.

    Preprocessing uses letterbox (aspect-preserving resize + gray pad),
    matching Ultralytics inference. A plain stretch-resize must NOT be
    used for OBB: anisotropic scaling corrupts rotated-box angles.
    """

    def __init__(self, path, conf_thres=0.7, iou_thres=0.5, angle_mode="180"):
        """
        Args:
            path: Path to the exported ONNX model
            conf_thres: Confidence threshold for filtering detections
            iou_thres: IoU threshold for rotated NMS
            angle_mode: "180" for a stock Ultralytics OBB model (default).
                        "360" only for a custom model whose in-graph angle
                        decode maps to [0, 2pi) -- angles then get
                        normalized into that range.
        """
        if angle_mode not in ("180", "360"):
            raise ValueError(f"angle_mode must be '180' or '360', got '{angle_mode}'")
        self.conf_threshold = conf_thres
        self.iou_threshold = iou_thres
        self.angle_mode = angle_mode
        self.initialize_model(path)

    def __call__(self, image):
        return self.detect_objects(image)

    def initialize_model(self, path):
        self.session = onnxruntime.InferenceSession(
            path, providers=['CUDAExecutionProvider', 'CPUExecutionProvider'])
        self.get_input_details()
        self.get_output_details()

    def detect_objects(self, image):
        """
        Run OBB detection.

        Returns:
            boxes:     (n, 4) array [cx, cy, w, h] in original image pixels
            scores:    (n,) confidence scores
            class_ids: (n,) class indices
            angles:    (n,) rotation angles in radians
        """
        input_tensor = self.prepare_input(image)
        outputs = self.inference(input_tensor)
        self.boxes, self.scores, self.class_ids, self.angles = \
            self.process_output(outputs[0])
        return self.boxes, self.scores, self.class_ids, self.angles

    def prepare_input(self, image):
        """Letterbox to model input size (Ultralytics-style), RGB, CHW, [0,1]."""
        self.img_height, self.img_width = image.shape[:2]

        # Aspect-preserving scale, then pad to input size with gray
        self.scale = min(self.input_width / self.img_width,
                         self.input_height / self.img_height)
        new_w = round(self.img_width * self.scale)
        new_h = round(self.img_height * self.scale)
        self.pad_x = (self.input_width - new_w) / 2
        self.pad_y = (self.input_height - new_h) / 2

        resized = cv2.resize(image, (new_w, new_h), interpolation=cv2.INTER_LINEAR)
        padded = np.full((self.input_height, self.input_width, 3), 114,
                         dtype=np.uint8)
        top, left = int(round(self.pad_y - 0.1)), int(round(self.pad_x - 0.1))
        padded[top:top + new_h, left:left + new_w] = resized

        input_img = cv2.cvtColor(padded, cv2.COLOR_BGR2RGB).astype(np.float32) / 255.0
        return input_img.transpose(2, 0, 1)[np.newaxis]

    def inference(self, input_tensor):
        start = time.perf_counter()
        outputs = self.session.run(self.output_names,
                                   {self.input_names[0]: input_tensor})
        self.inference_time_ms = (time.perf_counter() - start) * 1000
        return outputs

    def process_output(self, output):
        predictions = np.squeeze(output).T  # (N, 4 + nc + 1)
        num_classes = predictions.shape[1] - 5

        class_scores = predictions[:, 4:4 + num_classes]
        scores = np.max(class_scores, axis=1)

        mask = scores > self.conf_threshold
        if not np.any(mask):
            empty = np.array([])
            return empty.reshape(0, 4), empty, empty, empty

        predictions = predictions[mask]
        scores = scores[mask]
        class_ids = np.argmax(predictions[:, 4:4 + num_classes], axis=1)
        boxes = self.rescale_boxes(predictions[:, :4])
        angles = self.decode_angles(predictions[:, -1])

        indices = self.rotated_nms(boxes, angles, scores)
        return boxes[indices], scores[indices], class_ids[indices], angles[indices]

    def rescale_boxes(self, boxes):
        """Undo the letterbox: subtract pad, divide by scale. Exact for
        rotated boxes because letterboxing is a uniform scale + shift."""
        boxes = boxes.copy()
        boxes[:, 0] = (boxes[:, 0] - self.pad_x) / self.scale
        boxes[:, 1] = (boxes[:, 1] - self.pad_y) / self.scale
        boxes[:, 2:4] /= self.scale
        return boxes

    def decode_angles(self, angles_raw):
        """The ONNX graph already decodes the angle to radians
        ([-pi/4, 3pi/4) for stock models); never sigmoid here."""
        if self.angle_mode == "360":
            return np.mod(angles_raw, 2 * np.pi)
        return angles_raw

    @staticmethod
    def to_cv_rects(boxes, angles):
        """[cx, cy, w, h] + radians -> OpenCV ((cx, cy), (w, h), deg) rects."""
        return [((float(b[0]), float(b[1])), (float(b[2]), float(b[3])),
                 float(np.degrees(a))) for b, a in zip(boxes, angles)]

    def rotated_nms(self, boxes, angles, scores):
        """Rotated NMS. Prefers OpenCV's cv2.dnn.NMSBoxesRotated; falls
        back to a pure-Python implementation if unavailable."""
        if len(boxes) == 0:
            return np.array([], dtype=int)

        rects = self.to_cv_rects(boxes, angles)
        try:
            indices = cv2.dnn.NMSBoxesRotated(
                rects, scores.astype(np.float32).tolist(),
                float(self.conf_threshold), float(self.iou_threshold))
            return np.array(indices).reshape(-1).astype(int)
        except (cv2.error, AttributeError):
            return self._python_rotated_nms(rects, scores)

    def _python_rotated_nms(self, rects, scores):
        idxs = np.argsort(scores)[::-1].tolist()
        keep = []
        while idxs:
            i = idxs.pop(0)
            keep.append(i)
            idxs = [j for j in idxs
                    if self.rotated_iou(rects[i], rects[j]) <= self.iou_threshold]
        return np.array(keep, dtype=int)

    @staticmethod
    def rotated_iou(rect1, rect2):
        try:
            inter_type, inter_points = cv2.rotatedRectangleIntersection(rect1, rect2)
            if inter_points is None or inter_type == cv2.INTERSECT_NONE:
                return 0.0
            inter_area = abs(cv2.contourArea(inter_points))
            union = rect1[1][0] * rect1[1][1] + rect2[1][0] * rect2[1][1] - inter_area
            return inter_area / (union + 1e-9) if union > 0 else 0.0
        except cv2.error:
            return 0.0

    def draw_detections(self, image, draw_scores=True):
        return draw_obb_detections(image, self.boxes, self.scores,
                                   self.class_ids, self.angles, draw_scores)

    def draw_obb(self, image, color=(0, 255, 0), thickness=2,
                 draw_angle_arrow=True, angles=None, class_names=None):
        """
        Draw oriented boxes with heading arrows.

        Args:
            angles: optional override, e.g. 360-resolved headings from the
                    HeadingClassifier; defaults to the detected angles.
            class_names: optional list mapping class id -> display name.
        """
        out = image.copy()
        if angles is None:
            angles = self.angles

        for box, score, class_id, angle in zip(self.boxes, self.scores,
                                               self.class_ids, angles):
            cx, cy, w, h = box
            rect = ((float(cx), float(cy)), (float(w), float(h)),
                    float(np.degrees(angle)))
            box_points = cv2.boxPoints(rect).astype(np.int32)
            cv2.polylines(out, [box_points], True, color, thickness)

            if draw_angle_arrow:
                # Angle measured from +x axis, clockwise in image coords
                arrow_len = max(w, h) / 2
                end_x = cx + arrow_len * math.cos(float(angle))
                end_y = cy + arrow_len * math.sin(float(angle))
                cv2.arrowedLine(out, (int(cx), int(cy)),
                                (int(end_x), int(end_y)),
                                (255, 0, 0), thickness, tipLength=0.3)

            name = class_names[class_id] if class_names else f"ID:{class_id}"
            label = f"{name} {score:.2f} {math.degrees(float(angle)) % 360:.0f}deg"
            tl = box_points.min(axis=0)
            cv2.putText(out, label, (int(tl[0]), int(tl[1]) - 5),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 2)
        return out

    def get_input_details(self):
        model_inputs = self.session.get_inputs()
        self.input_names = [inp.name for inp in model_inputs]
        self.input_shape = model_inputs[0].shape
        self.input_height = self.input_shape[2]
        self.input_width = self.input_shape[3]

    def get_output_details(self):
        self.output_names = [out.name for out in self.session.get_outputs()]


if __name__ == '__main__':
    # Example usage with a stock exported model:
    #   yolo export model=yolo11n-obb.pt format=onnx opset=12 imgsz=640
    detector = YOLOOBB("../models/yolo11n-obb.onnx", conf_thres=0.3, iou_thres=0.5)
    img = cv2.imread("test_image.jpg")
    boxes, scores, class_ids, angles = detector(img)
    print(f"Detected {len(boxes)} objects "
          f"(inference {detector.inference_time_ms:.1f} ms)")
    for i, (box, score, cid, ang) in enumerate(zip(boxes, scores, class_ids, angles)):
        print(f"  {i}: class {cid}, score {score:.2f}, "
              f"angle {math.degrees(float(ang)):.1f} deg")
    cv2.imwrite("obb_result.jpg", detector.draw_obb(img))
