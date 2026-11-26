import math

import cv2
import numpy as np
import onnxruntime


class HeadingClassifier:
    """
    Resolves OBB detections to a full 0..2pi heading using a classifier
    trained on crops from tools/rolabelimg_to_obb360.py (yolo11n-cls ONNX).

    Two model flavors, auto-detected from the ONNX output size:

    * 2 outputs  - binary front/back (folders head_fwd / head_rev):
      0 = front faces +x after derotation, 1 = faces -x (add 180 deg).
      Right choice for ELONGATED objects, where the OBB angle is reliable
      and only the 180-degree ambiguity remains.

    * N > 2 outputs - rotation bins (folders bin_000, bin_015, ...):
      class k means the front points at k*(360/N) degrees within the
      derotated crop; heading = box angle + that rotation. Softmax-weighted
      circular interpolation gives sub-bin precision. Right choice for
      SQUARE-ish objects, where the OBB angle is degenerate (a square fits
      at any angle, so the detector's angle carries no information).

    The crop transform MUST stay identical to aligned_crop() in
    tools/rolabelimg_to_obb360.py: rotate the frame about the box center
    by the detected angle (border replicate), crop margin*w x margin*h.
    """

    def __init__(self, path, margin=1.15):
        self.margin = margin
        self.session = onnxruntime.InferenceSession(
            path, providers=['CUDAExecutionProvider', 'CPUExecutionProvider'])
        inp = self.session.get_inputs()[0]
        self.input_name = inp.name
        self.input_height = inp.shape[2]
        self.input_width = inp.shape[3]
        self.output_names = [o.name for o in self.session.get_outputs()]

        out_shape = self.session.get_outputs()[0].shape
        self.num_classes = int(out_shape[-1])
        self.mode = "binary" if self.num_classes == 2 else "bins"
        if self.mode == "bins":
            step = 2 * math.pi / self.num_classes
            self._bin_sin = np.sin(np.arange(self.num_classes) * step)
            self._bin_cos = np.cos(np.arange(self.num_classes) * step)

    def resolve(self, image, boxes, angles):
        """
        Args:
            image:  original BGR frame the boxes were detected in
            boxes:  (n, 4) [cx, cy, w, h] in image pixels
            angles: (n,) OBB angles in radians

        Returns:
            headings: (n,) resolved angles in [0, 2pi)
            flipped:  (n,) bool, True where the correction exceeded 90 deg
        """
        headings = np.zeros(len(boxes), dtype=np.float64)
        flipped = np.zeros(len(boxes), dtype=bool)

        for i, (box, angle) in enumerate(zip(boxes, angles)):
            angle = float(angle)
            crop = self._aligned_crop(image, box, angle)
            if crop is None:
                headings[i] = angle % (2 * math.pi)
                continue
            probs = self._probs(crop)

            if self.mode == "binary":
                rot = math.pi if int(np.argmax(probs)) == 1 else 0.0
            else:
                # Argmax bin refined by its immediate neighbors. A global
                # circular mean is NOT robust here: near-symmetric objects
                # produce multimodal bin distributions whose mean lands
                # between the modes instead of on the best one.
                k = int(np.argmax(probs))
                idx = [(k - 1) % self.num_classes, k,
                       (k + 1) % self.num_classes]
                w = probs[idx]
                rot = math.atan2(float(w @ self._bin_sin[idx]),
                                 float(w @ self._bin_cos[idx]))
            flipped[i] = math.cos(rot) < 0
            headings[i] = (angle + rot) % (2 * math.pi)

        return headings, flipped

    def _aligned_crop(self, img, box, angle):
        cx, cy, w, h = (float(v) for v in box)
        M = cv2.getRotationMatrix2D((cx, cy), math.degrees(angle), 1.0)
        rotated = cv2.warpAffine(img, M, (img.shape[1], img.shape[0]),
                                 flags=cv2.INTER_LINEAR,
                                 borderMode=cv2.BORDER_REPLICATE)
        cw, ch = w * self.margin, h * self.margin
        x0 = max(int(round(cx - cw / 2)), 0)
        y0 = max(int(round(cy - ch / 2)), 0)
        x1 = min(int(round(cx + cw / 2)), rotated.shape[1])
        y1 = min(int(round(cy + ch / 2)), rotated.shape[0])
        if x1 - x0 < 4 or y1 - y0 < 4:
            return None
        return rotated[y0:y1, x0:x1]

    def _probs(self, crop):
        """Class probabilities for one crop."""
        resized = cv2.resize(crop, (self.input_width, self.input_height),
                             interpolation=cv2.INTER_LINEAR)
        rgb = cv2.cvtColor(resized, cv2.COLOR_BGR2RGB).astype(np.float32) / 255.0
        tensor = rgb.transpose(2, 0, 1)[np.newaxis]
        out = np.squeeze(self.session.run(self.output_names,
                                          {self.input_name: tensor})[0])
        # Softmax unless the output is already a probability vector
        if out.min() < 0 or abs(out.sum() - 1.0) > 1e-3:
            e = np.exp(out - out.max())
            out = e / e.sum()
        return out
