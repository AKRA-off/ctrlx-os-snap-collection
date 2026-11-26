#!/usr/bin/env python3
"""
Heading-consistency audit for an exported classifier dataset.

Checks that all front-facing crops of each class share one orientation:
builds a robust per-class template and flags crops that correlate better
with the 180-degree-rotated template - the signature of a backwards
annotation (arrow at the wrong end).

Works on both export flavors:
  - binary:  <root>/<split>/head_fwd/
  - bins:    <root>/<split>/bin_000/   (front faces right by definition)

Symmetric classes (an O/S/X-like shape equals its own 180-degree
rotation) always produce small-margin "suspects" - only LARGE margins
(> ~0.2) on asymmetric classes indicate real annotation errors.

Usage:
    python audit_heading_crops.py <path-to-heading_crops>
"""

import glob
import os
import sys
from collections import defaultdict

import cv2
import numpy as np

S = 64


def load(f):
    img = cv2.imread(f, cv2.IMREAD_GRAYSCALE)
    if img is None:
        return None
    img = cv2.resize(img, (S, S)).astype(np.float32)
    img -= img.mean()
    n = np.linalg.norm(img)
    return img / n if n > 0 else img


def corr(a, b):
    return float((a * b).sum())


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    root = sys.argv[1]

    files = sorted(glob.glob(os.path.join(root, "*", "head_fwd", "*.jpg")))
    kind = "head_fwd"
    if not files:
        files = sorted(glob.glob(os.path.join(root, "*", "bin_000", "*.jpg")))
        kind = "bin_000"
    if not files:
        sys.exit(f"No head_fwd or bin_000 crops found under {root}")

    by_class = defaultdict(list)
    for f in files:
        label = os.path.splitext(os.path.basename(f))[0].rsplit("_", 1)[-1]
        im = load(f)
        if im is not None:
            by_class[label].append((f, im))

    print(f"{sum(len(v) for v in by_class.values())} front-facing crops "
          f"({kind}), {len(by_class)} classes\n")
    print(f"{'class':<6} {'n':>4} {'self-sym':>9}  suspects (corr180 > corr)")
    print("-" * 70)

    total_suspects = 0
    for label in sorted(by_class):
        items = by_class[label]
        mean = np.mean([im for _, im in items], axis=0)
        mean -= mean.mean()
        norm = np.linalg.norm(mean)
        if norm > 0:
            mean /= norm

        for _ in range(2):  # rebuild template from majority-consistent crops
            keep = [im for _, im in items
                    if corr(im, mean) >= corr(np.rot90(im, 2), mean)]
            if len(keep) >= 3:
                mean = np.mean(keep, axis=0)
                mean -= mean.mean()
                norm = np.linalg.norm(mean)
                if norm > 0:
                    mean /= norm

        self_sym = corr(np.rot90(mean, 2), mean)
        suspects = sorted(
            ((corr(np.rot90(im, 2), mean) - corr(im, mean),
              os.path.relpath(f, root))
             for f, im in items
             if corr(np.rot90(im, 2), mean) > corr(im, mean)),
            reverse=True)

        flag = "  (SYMMETRIC - ignore small margins)" if self_sym > 0.8 else ""
        print(f"{label:<6} {len(items):>4} {self_sym:>9.2f}  "
              f"{len(suspects)} suspect(s){flag}")
        for margin, name in suspects[:8]:
            marker = "  <-- CHECK" if margin > 0.2 and self_sym <= 0.8 else ""
            print(f"{'':>21}  margin {margin:.3f}  {name}{marker}")
        total_suspects += sum(1 for m, _ in suspects
                              if m > 0.2 and self_sym <= 0.8)

    print("-" * 70)
    if total_suspects:
        print(f"{total_suspects} high-margin suspect(s) on asymmetric "
              f"classes - fix those annotations (F key) and re-export "
              f"before training.")
        sys.exit(1)
    print("Clean: no high-margin suspects on asymmetric classes.")


if __name__ == "__main__":
    main()
