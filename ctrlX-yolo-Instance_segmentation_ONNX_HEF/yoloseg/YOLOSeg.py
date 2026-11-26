import math
import time
import cv2
import numpy as np
import onnxruntime

from yoloseg.utils import xywh2xyxy, nms, draw_detections, sigmoid


class YOLOSeg:

    def __init__(self, path, conf_thres=0.7, iou_thres=0.5, num_masks=32):
        self.conf_threshold = conf_thres
        self.iou_threshold = iou_thres
        self.num_masks = num_masks

        # Initialize model
        self.initialize_model(path)

    def __call__(self, image):
        return self.segment_objects(image)

    def initialize_model(self, path):
        self.intra_op_num_threads = 8
        self.inter_op_num_threads = 8
        self.session = onnxruntime.InferenceSession(path,
                                                    providers=['CUDAExecutionProvider',
                                                                'CPUExecutionProvider'])
        # Get model info
        self.get_input_details()
        self.get_output_details()

    def segment_objects(self, image):
        input_tensor = self.prepare_input(image)

        # Perform inference on the image
        outputs = self.inference(input_tensor)

        self.boxes, self.scores, self.class_ids, mask_pred = self.process_box_output(outputs[0])
        self.mask_maps = self.process_mask_output(mask_pred, outputs[1])

        return self.boxes, self.scores, self.class_ids, self.mask_maps

    def prepare_input(self, image):
        self.img_height, self.img_width = image.shape[:2]

        input_img = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)

        # Resize input image
        input_img = cv2.resize(input_img, (self.input_width, self.input_height))

        # Scale input pixel values to 0 to 1
        input_img = input_img / 255.0
        input_img = input_img.transpose(2, 0, 1)
        input_tensor = input_img[np.newaxis, :, :, :].astype(np.float32)

        return input_tensor

    def inference(self, input_tensor):
        start = time.perf_counter()
        outputs = self.session.run(self.output_names, {self.input_names[0]: input_tensor})

        # print(f"Inference time: {(time.perf_counter() - start)*1000:.2f} ms")
        return outputs

    def process_box_output(self, box_output):

        predictions = np.squeeze(box_output).T
        num_classes = box_output.shape[1] - self.num_masks - 4

        # Filter out object confidence scores below threshold
        scores = np.max(predictions[:, 4:4+num_classes], axis=1)
        predictions = predictions[scores > self.conf_threshold, :]
        scores = scores[scores > self.conf_threshold]

        if len(scores) == 0:
            return [], [], [], np.array([])

        box_predictions = predictions[..., :num_classes+4]
        mask_predictions = predictions[..., num_classes+4:]

        # Get the class with the highest confidence
        class_ids = np.argmax(box_predictions[:, 4:], axis=1)

        # Get bounding boxes for each object
        boxes = self.extract_boxes(box_predictions)

        # Apply non-maxima suppression to suppress weak, overlapping bounding boxes
        indices = nms(boxes, scores, self.iou_threshold)

        return boxes[indices], scores[indices], class_ids[indices], mask_predictions[indices]

    def process_mask_output(self, mask_predictions, mask_output):

        if mask_predictions.shape[0] == 0:
            return []

        mask_output = np.squeeze(mask_output)

        # Calculate the mask maps for each box
        num_mask, mask_height, mask_width = mask_output.shape  # CHW
        masks = sigmoid(mask_predictions @ mask_output.reshape((num_mask, -1)))
        masks = masks.reshape((-1, mask_height, mask_width))

        # Downscale the boxes to match the mask size
        scale_boxes = self.rescale_boxes(self.boxes,
                                   (self.img_height, self.img_width),
                                   (mask_height, mask_width))

        # For every box/mask pair, get the mask map
        mask_maps = np.zeros((len(scale_boxes), self.img_height, self.img_width))
        blur_size = (int(self.img_width / mask_width), int(self.img_height / mask_height))
        for i in range(len(scale_boxes)):

            scale_x1 = int(math.floor(scale_boxes[i][0]))
            scale_y1 = int(math.floor(scale_boxes[i][1]))
            scale_x2 = int(math.ceil(scale_boxes[i][2]))
            scale_y2 = int(math.ceil(scale_boxes[i][3]))

            x1 = int(math.floor(self.boxes[i][0]))
            y1 = int(math.floor(self.boxes[i][1]))
            x2 = int(math.ceil(self.boxes[i][2]))
            y2 = int(math.ceil(self.boxes[i][3]))

            scale_crop_mask = masks[i][scale_y1:scale_y2, scale_x1:scale_x2]
            crop_mask = cv2.resize(scale_crop_mask,
                              (x2 - x1, y2 - y1),
                              interpolation=cv2.INTER_CUBIC)

            crop_mask = cv2.blur(crop_mask, blur_size)

            crop_mask = (crop_mask > 0.5).astype(np.uint8)
            mask_maps[i, y1:y2, x1:x2] = crop_mask

        return mask_maps

    def extract_boxes(self, box_predictions):
        # Extract boxes from predictions
        boxes = box_predictions[:, :4]

        # Scale boxes to original image dimensions
        boxes = self.rescale_boxes(boxes,
                                   (self.input_height, self.input_width),
                                   (self.img_height, self.img_width))

        # Convert boxes to xyxy format
        boxes = xywh2xyxy(boxes)

        # Check the boxes are within the image
        boxes[:, 0] = np.clip(boxes[:, 0], 0, self.img_width)
        boxes[:, 1] = np.clip(boxes[:, 1], 0, self.img_height)
        boxes[:, 2] = np.clip(boxes[:, 2], 0, self.img_width)
        boxes[:, 3] = np.clip(boxes[:, 3], 0, self.img_height)

        return boxes

    def draw_detections(self, image, draw_scores=True, mask_alpha=0.4):
        return draw_detections(image, self.boxes, self.scores,
                               self.class_ids, mask_alpha)

    def draw_masks(self, image, draw_scores=True, mask_alpha=0.5):
        return draw_detections(image, self.boxes, self.scores,
                               self.class_ids, mask_alpha, mask_maps=self.mask_maps)

###cr
    def get_mask_polygons(self, min_area: float = 10.0, approx_frac: float = 0.01):
        """
        Return list of polygons per instance.
        Each item is a list of polygons (numpy arrays of shape [N, 2]) for that instance.
        - min_area filters tiny contours.
        - approx_frac controls polygon simplification (fraction of contour perimeter).
        """
        if getattr(self, 'mask_maps', None) is None or len(self.mask_maps) == 0:
            return []

        all_instance_polygons = []
        for mask in self.mask_maps:
            bin_mask = (mask > 0).astype(np.uint8)
            contours, _ = cv2.findContours(bin_mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

            instance_polys = []
            for cnt in contours:
                if cv2.contourArea(cnt) < min_area:
                    continue
                eps = approx_frac * cv2.arcLength(cnt, True)
                cnt = cv2.approxPolyDP(cnt, eps, True)
                instance_polys.append(cnt[:, 0, :])
            all_instance_polygons.append(instance_polys)

        return all_instance_polygons

    def get_mask_bounding_boxes(self, rotated: bool = False, min_area: float = 10.0):
        """
        Returns one bounding box per instance mask, derived from the largest contour.
        - If rotated=False: returns (x, y, w, h) tuples (axis-aligned).
        - If rotated=True: returns 4-point boxes as int ndarray shape [4,2].
        """
        if getattr(self, 'mask_maps', None) is None or len(self.mask_maps) == 0:
            return []

        results = []
        for mask in self.mask_maps:
            bin_mask = (mask > 0).astype(np.uint8)
            contours, _ = cv2.findContours(bin_mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
            if not contours:
                results.append(None)
                continue
            cnt = max(contours, key=cv2.contourArea)
            if cv2.contourArea(cnt) < min_area:
                results.append(None)
                continue

            if rotated:
                rect = cv2.minAreaRect(cnt)
                box = cv2.boxPoints(rect).astype(int)
                results.append(box)
            else:
                x, y, w, h = cv2.boundingRect(cnt)
                results.append((x, y, w, h))
        return results

    def get_mask_axis_length(self, use_all_mask_pixels: bool = False, min_area: float = 10.0):
        """
        Measures the object's longest extent along its principal axis (through the centroid).

        Returns a list per instance with dict fields:
        - length: float length in pixels along the principal axis
        - center: (cx, cy)
        - axis: (ux, uy) unit direction vector of principal axis
        - endpoints: ndarray [[x_min, y_min], [x_max, y_max]] along the axis

        If use_all_mask_pixels is True, uses all mask pixels for PCA (robust but slower).
        Otherwise uses dense contour points (CHAIN_APPROX_NONE) of the largest contour.
        """
        if getattr(self, 'mask_maps', None) is None or len(self.mask_maps) == 0:
            return []

        results = []
        for mask in self.mask_maps:
            bin_mask = (mask > 0).astype(np.uint8)

            # Get contours to filter out tiny/noisy instances first
            contours, _ = cv2.findContours(bin_mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
            if not contours:
                results.append(None)
                continue
            cnt = max(contours, key=cv2.contourArea)
            if cv2.contourArea(cnt) < min_area:
                results.append(None)
                continue

            if use_all_mask_pixels:
                ys, xs = np.where(bin_mask > 0)
                points = np.column_stack((xs, ys)).astype(np.float32)
            else:
                # Dense boundary points
                contours_dense, _ = cv2.findContours(bin_mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_NONE)
                cnt_dense = max(contours_dense, key=cv2.contourArea)
                points = cnt_dense.reshape(-1, 2).astype(np.float32)

            if points.shape[0] < 2:
                results.append(None)
                continue

            centroid = points.mean(axis=0)
            centered = points - centroid
            cov = np.cov(centered.T)
            # Eigenvectors of covariance matrix; principal axis is eigenvector with largest eigenvalue
            eigvals, eigvecs = np.linalg.eigh(cov)
            principal = eigvecs[:, int(np.argmax(eigvals))]
            norm = float(np.linalg.norm(principal))
            if norm == 0.0:
                results.append(None)
                continue
            principal /= norm

            projections = centered @ principal  # scalar per point
            t_min = float(projections.min())
            t_max = float(projections.max())
            length = t_max - t_min

            p1 = centroid + principal * t_min
            p2 = centroid + principal * t_max
            endpoints = np.vstack([p1, p2]).astype(np.float32)

            results.append({
                'length': float(length),
                'center': (float(centroid[0]), float(centroid[1])),
                'axis': (float(principal[0]), float(principal[1])),
                'endpoints': endpoints
            })

        return results

    def draw_axes(self, image, color=(255, 255, 0), thickness: int = 2,
                   use_all_mask_pixels: bool = False, min_area: float = 10.0):
        """
        Draws the principal axis line for each instance on the given image.
        Returns the modified image.
        """
        if image is None:
            return image

        axis_info_list = self.get_mask_axis_length(use_all_mask_pixels=use_all_mask_pixels, min_area=min_area)
        if not axis_info_list:
            return image

        out = image
        for info in axis_info_list:
            if info is None:
                continue
            pts = info['endpoints'].astype(np.int32)
            c = (int(info['center'][0]), int(info['center'][1]))
            cv2.line(out, tuple(pts[0]), tuple(pts[1]), color, thickness, lineType=cv2.LINE_AA)
            cv2.circle(out, c, radius=3, color=color, thickness=-1, lineType=cv2.LINE_AA)
        return out

###/cr

    def get_input_details(self):
        model_inputs = self.session.get_inputs()
        self.input_names = [model_inputs[i].name for i in range(len(model_inputs))]

        self.input_shape = model_inputs[0].shape
        self.input_height = self.input_shape[2]
        self.input_width = self.input_shape[3]

    def get_output_details(self):
        model_outputs = self.session.get_outputs()
        self.output_names = [model_outputs[i].name for i in range(len(model_outputs))]

    @staticmethod
    def rescale_boxes(boxes, input_shape, image_shape):
        # Rescale boxes to original image dimensions
        input_shape = np.array([input_shape[1], input_shape[0], input_shape[1], input_shape[0]])
        boxes = np.divide(boxes, input_shape, dtype=np.float32)
        boxes *= np.array([image_shape[1], image_shape[0], image_shape[1], image_shape[0]])

        return boxes


if __name__ == '__main__':
    from imread_from_url import imread_from_url

    model_path = "../models/yolov8m-seg.onnx"

    # Initialize YOLOv8 Instance Segmentator
    yoloseg = YOLOSeg(model_path, conf_thres=0.3, iou_thres=0.5)

    img_url = "https://live.staticflickr.com/13/19041780_d6fd803de0_3k.jpg"
    img = imread_from_url(img_url)

    # Detect Objects
    yoloseg(img)

    # Draw detections
    combined_img = yoloseg.draw_masks(img)
    cv2.namedWindow("Output", cv2.WINDOW_NORMAL)
    cv2.imshow("Output", combined_img)
    cv2.waitKey(0)
