#!/usr/bin/env python3
"""
Golden-set evaluation: run the full OBB + heading pipeline (the exact
runtime classes the snap uses) on annotated images and measure detection
completeness and circular heading error against the roLabelImg XML
ground truth, per class.

Run this after EVERY retraining, on a held-out set of annotated images
that never enters training - it turns "is the new model better?" into
numbers and catches regressions before deployment.

Usage:
    python eval_heading_accuracy.py \\
        --images  path/to/golden/images \\
        --xml-dir path/to/golden/xmls   (default: same as --images) \\
        --obb-model models/obb.onnx --heading-model models/heading.onnx \\
        --class-yaml data0.yaml \\
        --symmetric O,S,X \\
        --save-overlays out/
"""

import argparse
import glob
import math
import os
import sys
from collections import defaultdict
from xml.etree import ElementTree

import cv2
import numpy as np
import yaml

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from yoloobb.YOLOOBB import YOLOOBB          # noqa: E402
from yoloobb.heading import HeadingClassifier  # noqa: E402


def load_gt(xml_path):
    root = ElementTree.parse(xml_path).getroot()
    gts = []
    for obj in root.findall("object"):
        rb = obj.find("robndbox")
        if rb is None:
            continue
        gts.append({
            "label": obj.find("name").text,
            "cx": float(rb.find("cx").text), "cy": float(rb.find("cy").text),
            "w": float(rb.find("w").text), "h": float(rb.find("h").text),
            "heading": float(rb.find("angle").text),
        })
    return gts


def circ_err_deg(a_rad, b_rad):
    d = abs(math.degrees(a_rad) - math.degrees(b_rad)) % 360
    return min(d, 360 - d)


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--images", required=True)
    ap.add_argument("--xml-dir", default=None)
    ap.add_argument("--obb-model", required=True)
    ap.add_argument("--heading-model", required=True)
    ap.add_argument("--class-yaml", required=True)
    ap.add_argument("--conf", type=float, default=0.3)
    ap.add_argument("--iou", type=float, default=0.5)
    ap.add_argument("--symmetric", default="O,S,X",
                    help="comma-separated classes whose heading is "
                         "physically undefined (excluded from the summary)")
    ap.add_argument("--save-overlays", default=None)
    ap.add_argument("--clahe", action="store_true",
                    help="apply the app's CLAHE preprocessing to each frame "
                         "before inference (use when testing a CLAHE-trained "
                         "model, so eval input matches training)")
    args = ap.parse_args()

    pre = None
    if args.clahe:
        from yoloobb.preprocess import preprocess_frame as pre

    xml_dir = args.xml_dir or args.images
    symmetric = {s.strip() for s in args.symmetric.split(",") if s.strip()}

    with open(args.class_yaml, encoding="utf8") as f:
        names = list(yaml.safe_load(f)["names"].values())

    detector = YOLOOBB(args.obb_model, conf_thres=args.conf,
                       iou_thres=args.iou)
    heading = HeadingClassifier(args.heading_model)
    print(f"heading model: mode={heading.mode}, "
          f"classes={heading.num_classes}")

    errors = defaultdict(list)
    n_gt = n_matched = n_extra = 0
    images = sorted(glob.glob(os.path.join(args.images, "*.jpg")) +
                    glob.glob(os.path.join(args.images, "*.png")) +
                    glob.glob(os.path.join(args.images, "*.bmp")))
    if not images:
        sys.exit(f"no images in {args.images}")

    for img_path in images:
        stem = os.path.splitext(os.path.basename(img_path))[0]
        xml_path = os.path.join(xml_dir, stem + ".xml")
        if not os.path.isfile(xml_path):
            print(f"  (skipping {stem}: no xml)")
            continue
        gts = load_gt(xml_path)
        img = cv2.imread(img_path)
        if pre is not None:
            img = pre(img)

        boxes, scores, class_ids, angles = detector(img)
        headings, _ = heading.resolve(img, boxes, angles)
        used = set()
        n_gt += len(gts)

        for gt in gts:
            best_i, best_d = None, 1e9
            for i, b in enumerate(boxes):
                if i in used:
                    continue
                d = math.hypot(b[0] - gt["cx"], b[1] - gt["cy"])
                if d < best_d:
                    best_i, best_d = i, d
            if best_i is None or best_d > 0.6 * max(gt["w"], gt["h"]):
                errors[gt["label"]].append(None)  # miss
                continue
            used.add(best_i)
            n_matched += 1
            wrong_cls = names[class_ids[best_i]] != gt["label"] \
                if class_ids[best_i] < len(names) else True
            errors[gt["label"]].append(
                ("WRONGCLS", names[class_ids[best_i]]) if wrong_cls
                else circ_err_deg(headings[best_i], gt["heading"]))
        n_extra += len(boxes) - len(used)

        if args.save_overlays:
            os.makedirs(args.save_overlays, exist_ok=True)
            out = detector.draw_obb(img, angles=headings,
                                    class_names=names, thickness=3)
            cv2.imwrite(os.path.join(args.save_overlays,
                                     f"eval_{stem}.jpg"), out)

    print(f"\nGT objects: {n_gt}, matched: {n_matched}, "
          f"missed: {n_gt - n_matched}, extra detections: {n_extra}\n")
    print(f"{'class':<6}{'n':>4}{'miss':>6}{'wrongcls':>9}"
          f"{'median err':>12}{'mean err':>10}{'max':>7}")
    print("-" * 56)

    all_errs = []
    for label in sorted(errors):
        entries = errors[label]
        vals = [e for e in entries if isinstance(e, float)]
        miss = sum(1 for e in entries if e is None)
        wrong = sum(1 for e in entries if isinstance(e, tuple))
        note = "  (symmetric)" if label in symmetric else ""
        if vals:
            print(f"{label:<6}{len(vals):>4}{miss:>6}{wrong:>9}"
                  f"{np.median(vals):>11.1f} {np.mean(vals):>9.1f}"
                  f"{max(vals):>7.1f}{note}")
        else:
            print(f"{label:<6}{0:>4}{miss:>6}{wrong:>9}{'-':>11} {'-':>9}"
                  f"{'-':>7}{note}")
        if label not in symmetric:
            all_errs.extend(vals)

    print("-" * 56)
    if all_errs:
        arr = np.array(all_errs)
        print(f"Asymmetric classes: median {np.median(arr):.1f} deg, "
              f"mean {arr.mean():.1f} deg, within 15 deg: "
              f"{100 * (arr < 15).mean():.0f}%")

    # Misclassification pairs (which letter each class gets confused with)
    from collections import Counter
    confus = {lbl: Counter(e[1] for e in errors[lbl] if isinstance(e, tuple))
              for lbl in sorted(errors)}
    if any(confus.values()):
        print("\nMisclassifications (true -> predicted x N):")
        for lbl in sorted(confus):
            if confus[lbl]:
                pairs = ", ".join(f"{p} x{n}"
                                  for p, n in confus[lbl].most_common())
                print(f"  {lbl} -> {pairs}")
    else:
        print("\nNo misclassifications - every matched box got the right class.")


if __name__ == "__main__":
    main()
