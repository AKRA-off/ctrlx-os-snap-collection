"""
Chessboard calibration for the ctrlX AI Detector: maps image pixels to
real-world millimeters on the work plane.

How it works:
1. detect_board(): finds the inner chessboard corners in a captured
   frame (handles camera tilt/perspective via the corner grid).
2. The four EXTREME inner corners are labeled TL/TR/BL/BR in the
   preview, ordered by their position in the image.
3. build_calibration(): the user supplies world coordinates (mm) for
   those four corners (prefilled with board-frame defaults). A
   homography px->board is fitted on ALL inner corners, then a
   similarity transform board->world on the four anchor pairs. The
   composed 3x3 matrix maps pixels directly to world mm.
4. Calibration.px_to_world()/angle_to_world() convert positions and
   headings. Angles are converted by mapping the heading VECTOR through
   the homography per detection (under perspective the angular
   correction is location-dependent - never a constant offset).

Storage: calib<camid>.yaml next to the models (media folder).
"""

import math
from datetime import datetime

import cv2
import numpy as np
import yaml


def _grid_board_mm(pattern_w, pattern_h, square_mm):
    """World coords (board frame) of all inner corners, row-major."""
    pts = np.zeros((pattern_h * pattern_w, 2), np.float64)
    for r in range(pattern_h):
        for c in range(pattern_w):
            pts[r * pattern_w + c] = (c * square_mm, r * square_mm)
    return pts


def detect_board(img, pattern_w, pattern_h):
    """
    Find inner chessboard corners.

    Returns dict with:
      corners_px: (N, 2) float64 subpixel corner positions (row-major grid)
      extremes:   {"TL"|"TR"|"BL"|"BR": (grid_index, (x_px, y_px))}
                  labeled by their VISUAL position in the image
    Raises ValueError when the board is not found.
    """
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)

    # Prefer the modern sector-based detector (OpenCV >= 4): much more
    # robust to glare/bright lighting, and returns subpixel corners
    # directly. Fall back to the classic detector + subpixel refinement.
    found = False
    corners = None
    try:
        found, corners = cv2.findChessboardCornersSB(
            gray, (pattern_w, pattern_h),
            cv2.CALIB_CB_EXHAUSTIVE + cv2.CALIB_CB_ACCURACY)
    except cv2.error:
        pass
    if not found:
        flags = (cv2.CALIB_CB_ADAPTIVE_THRESH + cv2.CALIB_CB_NORMALIZE_IMAGE)
        found, corners = cv2.findChessboardCorners(
            gray, (pattern_w, pattern_h), flags)
        if found:
            corners = cv2.cornerSubPix(
                gray, corners, (11, 11), (-1, -1),
                (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 30,
                 0.01))
    if not found:
        raise ValueError(
            f"Chessboard {pattern_w}x{pattern_h} not found - check the "
            f"pattern size (INNER corners), lighting and that the whole "
            f"board is visible")

    corners_px = corners.reshape(-1, 2).astype(np.float64)

    # The four extreme inner corners of the grid ...
    idx = {
        "g00": 0,
        "gW0": pattern_w - 1,
        "g0H": (pattern_h - 1) * pattern_w,
        "gWH": pattern_h * pattern_w - 1,
    }
    # ... labeled by where they SIT IN THE IMAGE, so the user's TL/TR/
    # BL/BR table matches what the preview shows regardless of how the
    # detector ordered the grid.
    items = [(k, corners_px[i]) for k, i in idx.items()]
    by_sum = sorted(items, key=lambda kv: kv[1][0] + kv[1][1])
    tl, br = by_sum[0], by_sum[-1]
    by_diff = sorted(items, key=lambda kv: kv[1][0] - kv[1][1])
    bl, tr = by_diff[0], by_diff[-1]
    extremes = {"TL": (idx[tl[0]], tuple(tl[1])),
                "TR": (idx[tr[0]], tuple(tr[1])),
                "BL": (idx[bl[0]], tuple(bl[1])),
                "BR": (idx[br[0]], tuple(br[1]))}
    return {"corners_px": corners_px, "extremes": extremes}


def draw_preview(img, detection, pattern_w, pattern_h):
    """Overlay: all corners + big labeled TL/TR/BL/BR markers."""
    out = img.copy()
    pts = detection["corners_px"].astype(np.float32).reshape(-1, 1, 2)
    cv2.drawChessboardCorners(out, (pattern_w, pattern_h), pts, True)
    for label, (_, (x, y)) in detection["extremes"].items():
        p = (int(x), int(y))
        cv2.circle(out, p, 14, (0, 105, 255), 3)
        cv2.putText(out, label, (p[0] + 18, p[1] - 10),
                    cv2.FONT_HERSHEY_SIMPLEX, 1.1, (0, 105, 255), 3)
    return out


def default_world_table(detection, pattern_w, pattern_h, square_mm):
    """Board-frame default world coords for the four labeled corners
    (user adjusts these to shift/rotate the world frame)."""
    board = _grid_board_mm(pattern_w, pattern_h, square_mm)
    return {label: (round(float(board[i][0]), 2), round(float(board[i][1]), 2))
            for label, (i, _) in detection["extremes"].items()}


def build_calibration(detection, pattern_w, pattern_h, square_mm,
                      world_table):
    """
    Compose the final px->world-mm homography.

    world_table: {"TL": (x_mm, y_mm), ...} user-entered coordinates of
    the four labeled extreme corners.

    Returns (H_px2world 3x3, rms_board_mm, rms_anchor_mm)
    """
    corners_px = detection["corners_px"]
    board = _grid_board_mm(pattern_w, pattern_h, square_mm)

    # px -> board frame, fitted on all inner corners
    H_board, _ = cv2.findHomography(corners_px, board)
    if H_board is None:
        raise ValueError("Homography fit failed")
    proj = cv2.perspectiveTransform(
        corners_px.reshape(-1, 1, 2), H_board).reshape(-1, 2)
    rms_board = float(np.sqrt(np.mean(np.sum((proj - board) ** 2, axis=1))))

    # board frame -> user world frame (similarity: rotation+translation
    # from the four labeled anchors. estimateAffine2D (full affine) is
    # used, NOT estimateAffinePartial2D (similarity): the world frame may
    # be a REFLECTION of the image (e.g. robot Y opposite to image Y), and
    # only a full affine can represent a flip. For a rectangle->rectangle
    # fit this stays rigid (no spurious shear).
    src = np.array([board[i] for _, (i, _) in
                    sorted(detection["extremes"].items())], np.float64)
    dst = np.array([world_table[label] for label in
                    sorted(detection["extremes"].keys())], np.float64)
    T, _ = cv2.estimateAffine2D(
        src.reshape(-1, 1, 2), dst.reshape(-1, 1, 2), method=cv2.LMEDS)
    if T is None:
        raise ValueError("Anchor transform fit failed - check the four "
                         "world coordinates")
    mapped = (T[:, :2] @ src.T).T + T[:, 2]
    rms_anchor = float(np.sqrt(np.mean(np.sum((mapped - dst) ** 2, axis=1))))

    T3 = np.vstack([T, [0.0, 0.0, 1.0]])
    H = T3 @ H_board
    return H, rms_board, rms_anchor


class Calibration:
    """Loaded calibration: converts pixels/angles to world mm/degrees."""

    def __init__(self, H, meta=None):
        self.H = np.asarray(H, np.float64)
        self.meta = meta or {}

    def px_to_world(self, xs, ys):
        if len(xs) == 0:
            return [], []
        pts = np.array(list(zip(xs, ys)), np.float64).reshape(-1, 1, 2)
        w = cv2.perspectiveTransform(pts, self.H).reshape(-1, 2)
        return [float(p[0]) for p in w], [float(p[1]) for p in w]

    def angle_to_world(self, cx, cy, angle_deg, probe_px=50.0):
        """Map a heading at image position (cx, cy) to the world frame by
        transforming the heading vector (perspective-correct)."""
        a = math.radians(angle_deg)
        pts = np.array([[cx, cy],
                        [cx + probe_px * math.cos(a),
                         cy + probe_px * math.sin(a)]],
                       np.float64).reshape(-1, 1, 2)
        w = cv2.perspectiveTransform(pts, self.H).reshape(-1, 2)
        return math.degrees(math.atan2(w[1][1] - w[0][1],
                                       w[1][0] - w[0][0])) % 360.0

    def save(self, path):
        data = dict(self.meta)
        data["H"] = [[float(v) for v in row] for row in self.H]
        data["saved"] = datetime.now().isoformat(timespec="seconds")
        with open(path, "w", encoding="utf8") as f:
            yaml.safe_dump(data, f, sort_keys=False)

    @staticmethod
    def load(path):
        with open(path, encoding="utf8") as f:
            data = yaml.safe_load(f)
        H = data.pop("H")
        return Calibration(H, meta=data)


def adjust_world_table(cal, new_table):
    """Re-anchor an existing calibration to new world coordinates for the
    four labeled corners WITHOUT re-detecting the board: fits the
    similarity transform between the stored and the new table values and
    composes it onto the saved homography.

    Returns (new Calibration, rms_delta_mm). A large rms means the new
    values do not describe the same rigid rectangle as the old ones.
    """
    old = cal.meta.get("world_table")
    if not old:
        raise ValueError("calibration has no stored world table")
    labels = sorted(old.keys())
    src = np.array([old[lbl] for lbl in labels], np.float64)
    dst = np.array([new_table[lbl] for lbl in labels], np.float64)
    # full affine (allows a reflection/flip; see build_calibration)
    T, _ = cv2.estimateAffine2D(
        src.reshape(-1, 1, 2), dst.reshape(-1, 1, 2), method=cv2.LMEDS)
    if T is None:
        raise ValueError("adjustment fit failed - check the coordinates")
    mapped = (T[:, :2] @ src.T).T + T[:, 2]
    rms = float(np.sqrt(np.mean(np.sum((mapped - dst) ** 2, axis=1))))

    H_new = np.vstack([T, [0.0, 0.0, 1.0]]) @ cal.H
    meta = dict(cal.meta)
    meta["world_table"] = {k: [float(v[0]), float(v[1])]
                           for k, v in new_table.items()}
    meta["rms_anchor_mm"] = round(rms, 3)
    return Calibration(H_new, meta=meta), rms
