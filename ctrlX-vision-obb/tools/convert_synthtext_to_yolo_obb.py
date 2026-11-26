#!/usr/bin/env python3
"""
Convert SynthText dataset (gt.mat) into YOLO OBB format - and optionally
into the rotation-bins heading-classifier format used by the ctrlX AI
Detector pipeline (see tools/rolabelimg_to_obb360.py in that repo).

SynthText official page: https://www.robots.ox.ac.uk/~vgg/data/scenetext/
Download:  https://thor.robots.ox.ac.uk/~vgg/data/scenetext/SynthText.zip  (~41GB)

After unzipping you should have something like:
    SYNTHTEXT_ROOT/
        SynthText/
            gt.mat
            1/  ballet_106_0.jpg  ...
            ... (200 numbered subfolders)

charBB gives 4 corner points per character in READING ORIENTATION order
(TL, TR, BR, BL of the glyph) - so besides the OBB polygon, the true
glyph heading is recoverable, which this script can turn into
rotation-bin classifier crops compatible with the AI Detector's heading
model (front = glyph top, bin_000 = top facing right).

Typical usage for the ctrlX AI Detector letter models (8 tile classes,
uppercase only, both detector labels and 24-bin heading crops):

    python convert_synthtext_to_yolo_obb.py \\
        --gt-mat /path/to/SynthText/gt.mat \\
        --images-root /path/to/SynthText \\
        --out /path/to/synth_out \\
        --classes CFLORSTX --uppercase-only \\
        --heading-bins 24 --max-images 20000

Generic 36-class conversion (0-9, A-Z, case-folded) when run without
--classes / --uppercase-only.

Requires: scipy numpy pillow, plus opencv-python when --heading-bins > 0.
"""

import argparse
import math
import random
import re
import shutil
from pathlib import Path

import numpy as np
from PIL import Image

DEFAULT_CLASS_NAMES = [str(d) for d in range(10)] + \
    [chr(c) for c in range(ord("A"), ord("Z") + 1)]


def load_gt(gt_mat_path):
    from scipy.io import loadmat
    print(f"Loading {gt_mat_path} (can take a while / a lot of RAM)...")
    data = loadmat(gt_mat_path)
    return data["imnames"][0], data["charBB"][0], data["txt"][0]


def to_str(val):
    arr = np.atleast_1d(val)
    while arr.dtype == object or arr.ndim > 0:
        if arr.size == 1 and arr.dtype != object:
            break
        if arr.dtype == object:
            arr = np.atleast_1d(arr[0])
        else:
            arr = np.atleast_1d(arr[0] if arr.ndim > 0 else arr)
        if isinstance(arr, np.ndarray) and arr.dtype.kind in ("U", "S") \
                and arr.ndim == 0:
            break
    s = arr.item() if hasattr(arr, "item") else arr
    if isinstance(s, bytes):
        s = s.decode("utf-8")
    return str(s)


def flatten_chars(txt_field):
    """Ordered character sequence from SynthText's txt field."""
    entries = [to_str(t) for t in np.atleast_1d(txt_field)]
    words = []
    for entry in entries:
        for w in re.split(r"\s+", entry.strip()):
            if w:
                words.append(w)
    return "".join(words)


def get_char_boxes(charBB_field, n_chars):
    """charBB -> list of 4-point polygons (TL, TR, BR, BL), or None on
    char/box count mismatch (skip instead of mis-aligning labels)."""
    arr = np.array(charBB_field)
    if arr.ndim == 2:
        arr = arr[:, :, np.newaxis]
    assert arr.shape[0] == 2 and arr.shape[1] == 4, \
        f"unexpected charBB shape {arr.shape}"
    if arr.shape[2] != n_chars:
        return None
    return [
        [(float(arr[0, k, j]), float(arr[1, k, j])) for k in range(4)]
        for j in range(arr.shape[2])
    ]


def polygon_to_yolo_obb(polygon, img_w, img_h):
    coords = []
    for x, y in polygon:
        coords.append(min(1.0, max(0.0, x / img_w)))
        coords.append(min(1.0, max(0.0, y / img_h)))
    return coords


def poly_geometry(poly):
    """Center, width (reading direction), height, and heading of a glyph.

    Heading follows the AI Detector tile convention: the vector from the
    glyph center toward the glyph TOP (annotation arrow at letter top),
    so a bin_000 crop shows the letter with its top facing right -
    identical to the tile training crops.
    """
    tl, tr, br, bl = [np.array(p, dtype=np.float64) for p in poly]
    center = (tl + tr + br + bl) / 4.0
    w = (np.linalg.norm(tr - tl) + np.linalg.norm(br - bl)) / 2.0
    h = (np.linalg.norm(bl - tl) + np.linalg.norm(br - tr)) / 2.0
    up = ((tl - bl) + (tr - br)) / 2.0  # bottom edge -> top edge
    heading = math.atan2(up[1], up[0])
    return center, w, h, heading


def aligned_char_crop(img, center, w, h, crop_angle_rad, margin):
    """Rotate the image so `crop_angle_rad` maps to +x, crop margin*size
    around the center - same construction as the tile converter."""
    import cv2
    M = cv2.getRotationMatrix2D((float(center[0]), float(center[1])),
                                math.degrees(crop_angle_rad), 1.0)
    rotated = cv2.warpAffine(img, M, (img.shape[1], img.shape[0]),
                             flags=cv2.INTER_LINEAR,
                             borderMode=cv2.BORDER_REPLICATE)
    cw, ch = w * margin, h * margin
    x0 = max(int(round(center[0] - cw / 2)), 0)
    y0 = max(int(round(center[1] - ch / 2)), 0)
    x1 = min(int(round(center[0] + cw / 2)), rotated.shape[1])
    y1 = min(int(round(center[1] + ch / 2)), rotated.shape[0])
    if x1 - x0 < 8 or y1 - y0 < 8:
        return None
    return rotated[y0:y1, x0:x1]


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--gt-mat", required=True)
    ap.add_argument("--images-root", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--classes", default=None,
                    help="Restrict + remap to these characters, ids by "
                         "position (e.g. CFLORSTX -> C=0..X=7 to match the "
                         "AI Detector's classes.txt). Default: 36 classes "
                         "0-9 A-Z.")
    ap.add_argument("--uppercase-only", action="store_true",
                    help="Skip lowercase source characters instead of case-"
                         "folding them (lowercase r/f/l/t do NOT look like "
                         "their uppercase tiles - folding corrupts shapes). "
                         "Recommended together with --classes.")
    ap.add_argument("--min-side", type=float, default=12.0,
                    help="Skip characters whose box short side is below this "
                         "many pixels (default 12) - tiny background text "
                         "teaches nothing")
    ap.add_argument("--heading-bins", type=int, default=0,
                    help="N > 0: additionally generate heading-classifier "
                         "crops (heading_crops/<split>/bin_XXX/) in the AI "
                         "Detector rotation-bins format. N must divide 360. "
                         "Requires opencv-python.")
    ap.add_argument("--bin-jitter", type=float, default=-1.0,
                    help="bins mode rotation jitter in degrees; -1 = half "
                         "bin width (full coverage)")
    ap.add_argument("--crop-margin", type=float, default=1.15)
    ap.add_argument("--max-images", type=int, default=20000,
                    help="Cap on source images (full set is 858,750; "
                         "SynthText is heavily redundant per scene)")
    ap.add_argument("--max-crops-per-class", type=int, default=2000,
                    help="bins mode: stop generating crops for a character "
                         "class once it has this many objects (keeps the "
                         "crop set balanced and the disk sane)")
    ap.add_argument("--val-fraction", type=float, default=0.02)
    ap.add_argument("--seed", type=int, default=0)
    args = ap.parse_args()

    if args.heading_bins:
        if 360 % args.heading_bins != 0:
            raise SystemExit("--heading-bins must divide 360")
        if args.bin_jitter < 0:
            args.bin_jitter = 360 / args.heading_bins / 2
        import cv2  # fail early if missing

    if args.classes:
        class_names = list(args.classes.upper())
    else:
        class_names = DEFAULT_CLASS_NAMES
    class_to_id = {name: i for i, name in enumerate(class_names)}

    images_root = Path(args.images_root)
    out_root = Path(args.out)
    for split in ("train", "val"):
        (out_root / "images" / split).mkdir(parents=True, exist_ok=True)
        (out_root / "labels" / split).mkdir(parents=True, exist_ok=True)
        if args.heading_bins:
            step = 360 // args.heading_bins
            for b in range(args.heading_bins):
                (out_root / "heading_crops" / split /
                 f"bin_{b * step:03d}").mkdir(parents=True, exist_ok=True)

    imnames, charBBs, txts = load_gt(args.gt_mat)
    total = len(imnames)
    print(f"gt.mat: {total} images; converting up to {args.max_images}")
    print(f"classes ({len(class_names)}): {''.join(class_names)}")

    order = list(range(total))
    random.Random(args.seed).shuffle(order)
    order = order[: args.max_images]

    rng = random.Random(args.seed)
    jitter_rng = random.Random(args.seed + 1)
    n_written = n_boxes = n_crop_objects = 0
    n_skipped_mismatch = n_skipped_missing = 0
    crops_per_class = {name: 0 for name in class_names}

    for idx in order:
        imname = to_str(imnames[idx])
        src_path = images_root / imname
        if not src_path.exists():
            n_skipped_missing += 1
            continue

        chars = flatten_chars(txts[idx])
        boxes = get_char_boxes(charBBs[idx], len(chars))
        if boxes is None:
            n_skipped_mismatch += 1
            continue

        try:
            with Image.open(src_path) as im:
                img_w, img_h = im.size
        except Exception:
            n_skipped_missing += 1
            continue

        split = "val" if rng.random() < args.val_fraction else "train"

        label_lines = []
        crop_jobs = []  # (class_name, center, w, h, heading)
        for ch, poly in zip(chars, boxes):
            if args.uppercase_only and ch.isalpha() and not ch.isupper():
                continue
            ch_upper = ch.upper()
            if ch_upper not in class_to_id:
                continue
            center, w, h, heading = poly_geometry(poly)
            if min(w, h) < args.min_side:
                continue
            coords = polygon_to_yolo_obb(poly, img_w, img_h)
            label_lines.append(
                str(class_to_id[ch_upper]) + " " +
                " ".join(f"{c:.6f}" for c in coords))
            n_boxes += 1
            if args.heading_bins and \
                    crops_per_class[ch_upper] < args.max_crops_per_class:
                crop_jobs.append((ch_upper, center, w, h, heading))
                crops_per_class[ch_upper] += 1

        if not label_lines:
            continue

        dst_name = imname.replace("/", "_")
        shutil.copy(src_path, out_root / "images" / split / dst_name)
        (out_root / "labels" / split / (Path(dst_name).stem + ".txt")
         ).write_text("\n".join(label_lines), encoding="utf-8")
        n_written += 1

        if crop_jobs:
            import cv2
            img = cv2.imread(str(src_path))
            if img is not None:
                step = 360 // args.heading_bins
                for k, (cname, center, w, h, heading) in enumerate(crop_jobs):
                    for b in range(args.heading_bins):
                        phi = b * step
                        jit = jitter_rng.uniform(-args.bin_jitter,
                                                 args.bin_jitter)
                        crop = aligned_char_crop(
                            img, center, w, h,
                            heading - math.radians(phi + jit),
                            args.crop_margin)
                        if crop is not None:
                            cv2.imwrite(str(
                                out_root / "heading_crops" / split /
                                f"bin_{phi:03d}" /
                                f"{Path(dst_name).stem}_{k}_{cname}.jpg"),
                                crop)
                    n_crop_objects += 1

    (out_root / "classes.txt").write_text("\n".join(class_names),
                                          encoding="utf-8")
    yaml_lines = [
        f"path: {out_root.resolve()}",
        "train: images/train",
        "val: images/val",
        f"nc: {len(class_names)}",
        "names:",
    ] + [f"  {i}: '{name}'" for i, name in enumerate(class_names)]
    (out_root / "data.yaml").write_text("\n".join(yaml_lines),
                                        encoding="utf-8")

    print(f"\nDone. {n_written} images, {n_boxes} boxes kept"
          + (f", {n_crop_objects} objects -> "
             f"{n_crop_objects * args.heading_bins} heading crops"
             if args.heading_bins else ""))
    if args.heading_bins:
        print("crops per class:",
              {k: v for k, v in crops_per_class.items() if v})
    if n_skipped_mismatch:
        print(f"  {n_skipped_mismatch} images skipped (char/box mismatch)")
    if n_skipped_missing:
        print(f"  {n_skipped_missing} images skipped (missing/unreadable)")
    print(f"Output: {out_root}")
    print(f"\nPretrain detector:\n  yolo obb train data={out_root/'data.yaml'} "
          f"model=yolo11n-obb.pt epochs=50 imgsz=640")
    print("Then fine-tune on the real tile dataset starting from the "
          "pretrained best.pt.")


if __name__ == "__main__":
    main()
