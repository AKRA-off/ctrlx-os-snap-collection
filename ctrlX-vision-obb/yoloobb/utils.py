import cv2
import numpy as np


def xywh2xyxy(boxes):
    """
    Convert bounding boxes from [cx, cy, w, h] to [x1, y1, x2, y2] format

    Args:
        boxes: Numpy array of boxes in [cx, cy, w, h] format

    Returns:
        boxes_xyxy: Numpy array of boxes in [x1, y1, x2, y2] format
    """
    boxes_xyxy = np.copy(boxes)
    boxes_xyxy[:, 0] = boxes[:, 0] - boxes[:, 2] / 2  # x1 = cx - w/2
    boxes_xyxy[:, 1] = boxes[:, 1] - boxes[:, 3] / 2  # y1 = cy - h/2
    boxes_xyxy[:, 2] = boxes[:, 0] + boxes[:, 2] / 2  # x2 = cx + w/2
    boxes_xyxy[:, 3] = boxes[:, 1] + boxes[:, 3] / 2  # y2 = cy + h/2

    return boxes_xyxy


def draw_obb_detections(image, boxes, scores, class_ids, angles, draw_scores=True,
                        color=(0, 255, 0), thickness=2, draw_angle_arrow=True):
    """
    Draw oriented bounding boxes on image with labels and optional angle arrows

    Args:
        image: Input image
        boxes: Numpy array of boxes in [cx, cy, w, h] format
        scores: Confidence scores
        class_ids: Class labels
        angles: Rotation angles in radians
        draw_scores: Whether to draw confidence scores
        color: Color for drawing (BGR format)
        thickness: Line thickness
        draw_angle_arrow: Whether to draw arrow showing orientation

    Returns:
        Image with OBB detections drawn
    """
    out = image.copy()

    for box, score, class_id, angle in zip(boxes, scores, class_ids, angles):
        cx, cy, w, h = box
        angle_deg = float(np.degrees(angle))

        # Create rotated rectangle
        rect = ((float(cx), float(cy)), (float(w), float(h)), angle_deg)
        box_points = cv2.boxPoints(rect)
        box_points = np.int0(box_points)

        # Draw rotated rectangle
        cv2.polylines(out, [box_points], True, color, thickness)

        # Draw angle indicator arrow (optional)
        if draw_angle_arrow:
            angle_rad = float(angle)
            arrow_len = min(w, h) / 3

            # Arrow points in the direction of 0° (upward by convention)
            end_x = cx + arrow_len * np.sin(angle_rad)
            end_y = cy - arrow_len * np.cos(angle_rad)

            cv2.arrowedLine(out, (int(cx), int(cy)),
                           (int(end_x), int(end_y)),
                           (255, 0, 0), thickness, tipLength=0.4)

        # Draw label with class and score
        if draw_scores:
            label = f"ID:{class_id} {score:.2f} {angle_deg:.0f}°"
        else:
            label = f"ID:{class_id} {angle_deg:.0f}°"

        # Get top-left corner for label placement
        tl = tuple(box_points.min(axis=0))

        # Draw label background for better readability
        (label_width, label_height), _ = cv2.getTextSize(
            label, cv2.FONT_HERSHEY_SIMPLEX, 0.5, 2
        )
        cv2.rectangle(out,
                     (int(tl[0]), int(tl[1]) - label_height - 10),
                     (int(tl[0]) + label_width, int(tl[1])),
                     color, -1)

        # Draw label text
        cv2.putText(out, label, (int(tl[0]), int(tl[1]) - 5),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 2)

    return out


def obb_iou(rect1, rect2):
    """
    Compute IoU (Intersection over Union) for two oriented bounding boxes

    Args:
        rect1, rect2: Rotated rectangles in format ((cx, cy), (w, h), angle_deg)

    Returns:
        iou: Intersection over union value [0, 1]
    """
    try:
        # Use OpenCV's rotated rectangle intersection
        inter_type, inter_points = cv2.rotatedRectangleIntersection(rect1, rect2)

        if inter_points is None or inter_type == cv2.INTERSECT_NONE:
            return 0.0

        # Calculate intersection area
        inter_area = abs(cv2.contourArea(inter_points))

        # Calculate areas of both rectangles
        area1 = rect1[1][0] * rect1[1][1]
        area2 = rect2[1][0] * rect2[1][1]

        # Calculate union area
        union_area = area1 + area2 - inter_area

        if union_area <= 0:
            return 0.0

        # Calculate IoU
        iou = inter_area / (union_area + 1e-9)

        return iou
    except:
        # If any error occurs, return 0 (no overlap)
        return 0.0
