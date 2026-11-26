"""
Lighting normalization shared by the app (runtime) and the dataset
batch preprocessor (tools/clahe_dataset.py).

CRITICAL: training and inference MUST use the identical function and
parameters or the model sees different pixels than it was trained on.
Keep the constants here as the single source of truth - never duplicate
them elsewhere.

grayscale + CLAHE (local adaptive contrast) makes a bright/washed-out
frame and a dim frame look nearly identical to the model, which is the
fix for uncontrolled/variable lighting (e.g. a camera near a window).
Color is dropped on purpose - the letters are black/white and it also
removes warm/cool colour cast from changing daylight.
"""

import cv2

CLAHE_CLIP_LIMIT = 3.0
CLAHE_TILE_GRID = (8, 8)

_clahe = None


def _get_clahe():
    global _clahe
    if _clahe is None:
        _clahe = cv2.createCLAHE(clipLimit=CLAHE_CLIP_LIMIT,
                                 tileGridSize=CLAHE_TILE_GRID)
    return _clahe


def preprocess_frame(img):
    """BGR frame -> grayscale + CLAHE, returned as 3-channel BGR (so the
    downstream letterbox / model input is unchanged in shape)."""
    if img is None:
        return None
    if img.ndim == 3:
        gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    else:
        gray = img
    eq = _get_clahe().apply(gray)
    return cv2.cvtColor(eq, cv2.COLOR_GRAY2BGR)
