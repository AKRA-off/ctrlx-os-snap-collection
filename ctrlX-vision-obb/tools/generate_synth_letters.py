#!/usr/bin/env python3
r"""
Generate a synthetic OBB + heading-bin dataset of rotated letters,
domain-matched to the ctrlX letter-tile setup: bold Arial-Black glyphs on
a light background, dark ink, at the two apparent scales of the two
cameras, at all 0-360 rotations.

Perfect labels for free: since we PLACE each letter at a known rotation,
the OBB box and the heading are exact - no annotation, no flip audit.

Output is identical in layout to tools/rolabelimg_to_obb360.py, so it is
a drop-in dataset and can be merged with the real one (same class order):

    out/
      data.yaml   classes.txt
      images/train|val/            labels/train|val/   (YOLO-OBB)
      heading_crops/train|val/bin_000.. (rotation-bin classifier crops)

Usage (Windows, Arial Black is the default font):
    python generate_synth_letters.py --out D:/AI/YOLO/synth_letters \
        --num-images 10000 --classes CFLORSTX --heading-bins 24

IMPORTANT: keep --classes in the SAME ORDER as your real classes.txt
(C F L O R S T X) so ids match and the datasets/models stay compatible.

Requires: numpy, opencv-python, pillow.
"""

import argparse
import math
import os
import random
import sys
from pathlib import Path

import cv2
import numpy as np
from PIL import Image, ImageDraw, ImageFont

WIN_FONTS = r"C:\Windows\Fonts"
DEFAULT_FONT = os.path.join(WIN_FONTS, "ariblk.ttf")  # Arial Black


# --- geometry helpers (identical convention to rolabelimg_to_obb360.py) ---

def poly_geometry(poly):
    """poly = [TL, TR, BR, BL] image points -> center, w, h, heading.
    Heading = direction the letter TOP points (arrow-at-top convention);
    a bin_000 crop then shows the letter with its top facing right."""
    tl, tr, br, bl = [np.array(p, np.float64) for p in poly]
    center = (tl + tr + br + bl) / 4.0
    w = (np.linalg.norm(tr - tl) + np.linalg.norm(br - bl)) / 2.0
    h = (np.linalg.norm(bl - tl) + np.linalg.norm(br - tr)) / 2.0
    up = ((tl - bl) + (tr - br)) / 2.0            # bottom edge -> top edge
    return center, w, h, math.atan2(up[1], up[0])


def aligned_crop(img, center, w, h, crop_angle_rad, margin):
    """Rotate so crop_angle maps to +x, crop margin*size around center -
    identical to the converter, so train/inference crops match."""
    M = cv2.getRotationMatrix2D((float(center[0]), float(center[1])),
                                math.degrees(crop_angle_rad), 1.0)
    rot = cv2.warpAffine(img, M, (img.shape[1], img.shape[0]),
                         flags=cv2.INTER_LINEAR,
                         borderMode=cv2.BORDER_REPLICATE)
    cw, ch = w * margin, h * margin
    x0 = max(int(round(center[0] - cw / 2)), 0)
    y0 = max(int(round(center[1] - ch / 2)), 0)
    x1 = min(int(round(center[0] + cw / 2)), rot.shape[1])
    y1 = min(int(round(center[1] + ch / 2)), rot.shape[0])
    if x1 - x0 < 8 or y1 - y0 < 8:
        return None
    return rot[y0:y1, x0:x1]


# --- letter rendering ---

def render_glyph(char, font_path, target_h, fg_gray, base_font=200):
    """Render one glyph tight to its ink box at ~target_h pixels tall.
    Returns (bgr uint8, alpha float 0..1), both HxW."""
    font = ImageFont.truetype(font_path, base_font)
    l, t, r, b = font.getbbox(char)
    gh = max(b - t, 1)
    size = max(int(base_font * target_h / gh), 6)
    font = ImageFont.truetype(font_path, size)
    l, t, r, b = font.getbbox(char)
    gw, gh = max(r - l, 1), max(b - t, 1)
    pad = 4
    canvas = Image.new("L", (gw + 2 * pad, gh + 2 * pad), 0)
    ImageDraw.Draw(canvas).text((pad - l, pad - t), char, fill=255, font=font)
    alpha = np.asarray(canvas, np.float32) / 255.0
    bgr = np.full((*alpha.shape, 3), fg_gray, np.uint8)
    return bgr, alpha


def rotate_rgba(bgr, alpha, theta_rad):
    """Rotate glyph+alpha by theta (our R convention), expanded canvas.
    Returns rotated bgr, alpha, and the 2x2 linear part used."""
    h, w = alpha.shape
    c, s = math.cos(theta_rad), math.sin(theta_rad)
    # expanded canvas that fits the rotated glyph
    nw = int(abs(w * c) + abs(h * s)) + 2
    nh = int(abs(w * s) + abs(h * c)) + 2
    M = np.array([[c, -s, 0], [s, c, 0]], np.float64)
    M[0, 2] = nw / 2 - (M[0, 0] * w / 2 + M[0, 1] * h / 2)
    M[1, 2] = nh / 2 - (M[1, 0] * w / 2 + M[1, 1] * h / 2)
    rb = cv2.warpAffine(bgr, M, (nw, nh), flags=cv2.INTER_LINEAR)
    ra = cv2.warpAffine(alpha, M, (nw, nh), flags=cv2.INTER_LINEAR)
    return rb, ra


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[1])
    ap.add_argument("--out", required=True, type=Path)
    ap.add_argument("--classes", default="CFLORSTX",
                    help="letters; ID = position. KEEP the order matching "
                         "your real classes.txt (default CFLORSTX).")
    ap.add_argument("--num-images", type=int, default=10000)
    ap.add_argument("--img-size", type=int, default=640)
    ap.add_argument("--font", default=DEFAULT_FONT)
    ap.add_argument("--scales", default="0.22,0.12",
                    help="two box-side fractions of the image (the two "
                         "camera apparent scales); each image picks one")
    ap.add_argument("--min-objs", type=int, default=3)
    ap.add_argument("--max-objs", type=int, default=9)
    ap.add_argument("--box-pad", type=float, default=1.15,
                    help="square OBB box side = letter size * this (tile-like)")
    ap.add_argument("--heading-bins", type=int, default=24,
                    help="0 = skip classifier crops; else N dividing 360")
    ap.add_argument("--bin-jitter", type=float, default=-1.0,
                    help="degrees; -1 = half bin width (full coverage)")
    ap.add_argument("--crop-margin", type=float, default=1.15)
    ap.add_argument("--max-crops-per-class", type=int, default=4000,
                    help="cap heading-crop OBJECTS per class")
    ap.add_argument("--val-split", type=float, default=0.02)
    ap.add_argument("--seed", type=int, default=0)
    # domain randomization (mild by default; reduces sim2real gap)
    ap.add_argument("--bg-gray", default="238,255")
    ap.add_argument("--fg-gray", default="0,45")
    ap.add_argument("--blur-max", type=float, default=1.2)
    ap.add_argument("--noise-max", type=float, default=6.0)
    ap.add_argument("--preview", action="store_true",
                    help="also write preview_labeled.jpg + preview_bins.jpg")
    args = ap.parse_args()

    if not os.path.isfile(args.font):
        sys.exit(f"Font not found: {args.font} (pass --font <path to .ttf>)")
    if args.heading_bins and 360 % args.heading_bins:
        sys.exit("--heading-bins must divide 360")
    if args.heading_bins and args.bin_jitter < 0:
        args.bin_jitter = 360 / args.heading_bins / 2

    classes = list(args.classes.upper())
    scales = [float(x) for x in args.scales.split(",")]
    bg_lo, bg_hi = [int(x) for x in args.bg_gray.split(",")]
    fg_lo, fg_hi = [int(x) for x in args.fg_gray.split(",")]
    S = args.img_size
    rng = random.Random(args.seed)
    nprng = np.random.default_rng(args.seed)

    splits = ("train", "val")
    for s in splits:
        (args.out / "images" / s).mkdir(parents=True, exist_ok=True)
        (args.out / "labels" / s).mkdir(parents=True, exist_ok=True)
    if args.heading_bins:
        step = 360 // args.heading_bins
        for s in splits:
            for b in range(args.heading_bins):
                (args.out / "heading_crops" / s /
                 f"bin_{b * step:03d}").mkdir(parents=True, exist_ok=True)

    crops_per_class = {c: 0 for c in classes}
    n_boxes = n_crops = 0
    prev_label = prev_bins = None

    for idx in range(args.num_images):
        split = "val" if rng.random() < args.val_split else "train"
        bg = rng.randint(bg_lo, bg_hi)
        img = np.full((S, S, 3), bg, np.uint8)
        box_side = int(rng.choice(scales) * S)
        placed = []            # (cx, cy, radius) for overlap rejection
        objects = []           # (cls, corners(4x2), heading)

        for _ in range(rng.randint(args.min_objs, args.max_objs)):
            ch = rng.choice(classes)
            side = int(box_side * rng.uniform(0.85, 1.15))
            theta = rng.uniform(0, 2 * math.pi)
            fg = rng.randint(fg_lo, fg_hi)
            glyph, alpha = render_glyph(ch, args.font, int(side * 0.72), fg)
            rglyph, ralpha = rotate_rgba(glyph, alpha, theta)
            rh, rw = ralpha.shape
            half = side * args.box_pad / 2 * 1.4142  # keep rotated box inside
            margin = int(half) + 2
            if 2 * margin >= S:
                continue
            cx = rng.randint(margin, S - margin)
            cy = rng.randint(margin, S - margin)
            r_excl = side * args.box_pad * 0.62
            if any((cx - px) ** 2 + (cy - py) ** 2 < (r_excl + pr) ** 2
                   for px, py, pr in placed):
                continue
            # composite glyph
            x0, y0 = cx - rw // 2, cy - rh // 2
            xa0, ya0 = max(x0, 0), max(y0, 0)
            xa1, ya1 = min(x0 + rw, S), min(y0 + rh, S)
            gx0, gy0 = xa0 - x0, ya0 - y0
            a = ralpha[gy0:gy0 + (ya1 - ya0), gx0:gx0 + (xa1 - xa0)][..., None]
            roi = img[ya0:ya1, xa0:xa1].astype(np.float32)
            g = rglyph[gy0:gy0 + (ya1 - ya0), gx0:gx0 + (xa1 - xa0)]
            img[ya0:ya1, xa0:xa1] = (roi * (1 - a) + g * a).astype(np.uint8)
            placed.append((cx, cy, r_excl))

            # square OBB box, rotated by theta -> corners TL,TR,BR,BL
            s2 = side * args.box_pad / 2
            c, sn = math.cos(theta), math.sin(theta)
            R = np.array([[c, -sn], [sn, c]])
            local = np.array([[-s2, -s2], [s2, -s2], [s2, s2], [-s2, s2]])
            corners = (R @ local.T).T + [cx, cy]
            _, _, _, heading = poly_geometry(corners)
            objects.append((ch, corners, heading))

        # domain randomization: blur + noise
        if args.blur_max > 0:
            k = rng.uniform(0, args.blur_max)
            if k > 0.2:
                img = cv2.GaussianBlur(img, (0, 0), k)
        if args.noise_max > 0:
            img = np.clip(img.astype(np.float32) +
                          nprng.normal(0, rng.uniform(0, args.noise_max),
                                       img.shape), 0, 255).astype(np.uint8)

        stem = f"synth_{idx:06d}"
        cv2.imwrite(str(args.out / "images" / split / f"{stem}.jpg"), img)
        lines = []
        for ch, corners, heading in objects:
            cid = classes.index(ch)
            coords = " ".join(f"{min(max(x / S, 0), 1):.6f} "
                              f"{min(max(y / S, 0), 1):.6f}"
                              for x, y in corners)
            lines.append(f"{cid} {coords}")
            n_boxes += 1
        (args.out / "labels" / split / f"{stem}.txt").write_text(
            "\n".join(lines) + "\n")

        if args.heading_bins:
            for k, (ch, corners, heading) in enumerate(objects):
                if crops_per_class[ch] >= args.max_crops_per_class:
                    continue
                center = corners.mean(0)
                w = h = box_side * args.box_pad
                for b in range(args.heading_bins):
                    phi = b * step
                    jit = rng.uniform(-args.bin_jitter, args.bin_jitter)
                    crop = aligned_crop(img, center, w, h,
                                        heading - math.radians(phi + jit),
                                        args.crop_margin)
                    if crop is not None:
                        cv2.imwrite(str(
                            args.out / "heading_crops" / split /
                            f"bin_{phi:03d}" / f"{stem}_{k}_{ch}.jpg"), crop)
                        n_crops += 1
                crops_per_class[ch] += 1

        if args.preview and prev_label is None and len(objects) >= 3:
            vis = img.copy()
            for ch, corners, heading in objects:
                cv2.polylines(vis, [corners.astype(np.int32)], True,
                              (0, 180, 0), 2)
                cen = corners.mean(0)
                tip = cen + 40 * np.array([math.cos(heading),
                                           math.sin(heading)])
                cv2.arrowedLine(vis, tuple(cen.astype(int)),
                                tuple(tip.astype(int)), (0, 0, 255), 2,
                                tipLength=0.3)
                cv2.putText(vis, ch, tuple((cen + [6, -8]).astype(int)),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 0, 0), 2)
            prev_label = args.out / "preview_labeled.jpg"
            cv2.imwrite(str(prev_label), vis)

    (args.out / "classes.txt").write_text("\n".join(classes) + "\n")
    names = "\n".join(f"  {i}: {c}" for i, c in enumerate(classes))
    (args.out / "data.yaml").write_text(
        f"path: {args.out.resolve().as_posix()}\n"
        f"train: images/train\nval: images/val\nnc: {len(classes)}\n"
        f"names:\n{names}\n")

    # bin_000 verification sheet: every letter's TOP must face RIGHT
    if args.preview and args.heading_bins:
        import glob
        fs = sorted(glob.glob(str(args.out / "heading_crops" / "train" /
                                  "bin_000" / "*.jpg")))[:24]
        cells = []
        for f in fs:
            im = cv2.resize(cv2.imread(f), (90, 90))
            cv2.rectangle(im, (0, 0), (89, 89), (200, 200, 200), 1)
            cells.append(im)
        if cells:
            while len(cells) % 6:
                cells.append(np.full((90, 90, 3), 255, np.uint8))
            rows = [np.hstack(cells[i:i + 6]) for i in range(0, len(cells), 6)]
            cv2.imwrite(str(args.out / "preview_bins.jpg"), np.vstack(rows))

    print(f"Done: {args.num_images} images, {n_boxes} boxes"
          + (f", {n_crops} heading crops" if args.heading_bins else ""))
    print(f"classes {classes}; crops/class "
          f"{ {k: v for k, v in crops_per_class.items() if v} }")
    print(f"Output: {args.out}")
    print(f"Train:  yolo obb train data={args.out / 'data.yaml'} "
          f"model=yolo11n-obb.pt epochs=100 imgsz={S}")


if __name__ == "__main__":
    main()
