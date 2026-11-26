#!/usr/bin/env python3
"""
Frame diagnostics: run the full pipeline on arbitrary (unannotated)
images and print, per detection, the heading classifier's internal bin
probabilities - the tool for separating "confident and correct" from
"flat smear" (out-of-distribution input) from "confidently wrong"
(180-degree twin confusion).

Reading the output:
  top-bin p ~0.9+, entropy < 0.5  -> in-distribution, trust the heading
  two bins splitting ~0.5/0.4     -> the class's symmetric-twin
                                     confusion; more training data from
                                     this condition needed
  flat (entropy > 1.2)            -> input regime unknown to the model

Usage:
    python diagnose_frames.py --images path/or/glob \\
        --obb-model obb.onnx --heading-model heading.onnx \\
        --class-yaml data0.yaml [--save-overlays out/]
"""

import argparse
import glob
import math
import os
import sys

import cv2
import numpy as np
import yaml

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from yoloobb.YOLOOBB import YOLOOBB          # noqa: E402
from yoloobb.heading import HeadingClassifier  # noqa: E402


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--images", required=True,
                    help="image file, directory, or glob")
    ap.add_argument("--obb-model", required=True)
    ap.add_argument("--heading-model", required=True)
    ap.add_argument("--class-yaml", required=True)
    ap.add_argument("--conf", type=float, default=0.3)
    ap.add_argument("--save-overlays", default=None)
    args = ap.parse_args()

    with open(args.class_yaml, encoding="utf8") as f:
        names = list(yaml.safe_load(f)["names"].values())

    if os.path.isdir(args.images):
        paths = sorted(sum((glob.glob(os.path.join(args.images, e))
                            for e in ("*.jpg", "*.png", "*.bmp")), []))
    else:
        paths = sorted(glob.glob(args.images))
    if not paths:
        sys.exit(f"no images match {args.images}")

    det = YOLOOBB(args.obb_model, conf_thres=args.conf, iou_thres=0.5)
    hc = HeadingClassifier(args.heading_model)
    step = 360 // hc.num_classes if hc.mode == "bins" else 180

    for img_path in paths:
        stem = os.path.splitext(os.path.basename(img_path))[0]
        img = cv2.imread(img_path)
        if img is None:
            print(f"unreadable: {img_path}")
            continue
        boxes, scores, class_ids, angles = det(img)
        headings, _ = hc.resolve(img, boxes, angles)

        print(f"\n=== {stem}: {len(boxes)} detections ===")
        print(f"{'cls':>4} {'conf':>5} {'boxang':>7} {'final':>6} "
              f"{'top p':>6} {'entropy':>8}  top-3 bins")
        for b, s, c, a, h in zip(boxes, scores, class_ids, angles, headings):
            crop = hc._aligned_crop(img, b, float(a))
            if crop is None:
                continue
            probs = hc._probs(crop)
            top = np.argsort(probs)[::-1][:3]
            ent = float(-(probs * np.log(probs + 1e-9)).sum())
            top_str = ", ".join(f"{k * step}deg:{probs[k]:.2f}" for k in top)
            name = names[c] if c < len(names) else str(c)
            print(f"{name:>4} {s:>5.2f} {math.degrees(float(a)):>6.1f} "
                  f"{math.degrees(float(h)):>6.1f} {probs.max():>6.2f} "
                  f"{ent:>8.2f}  {top_str}")

        if args.save_overlays:
            os.makedirs(args.save_overlays, exist_ok=True)
            out = det.draw_obb(img, angles=headings, class_names=names,
                               thickness=3)
            cv2.imwrite(os.path.join(args.save_overlays,
                                     f"diag_{stem}.jpg"), out)


if __name__ == "__main__":
    main()
