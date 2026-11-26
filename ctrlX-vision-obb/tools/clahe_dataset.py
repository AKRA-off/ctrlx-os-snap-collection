#!/usr/bin/env python3
"""
Batch CLAHE a labelled image folder for training, using the SAME
preprocess_frame() the snap applies at inference (yoloobb/preprocess.py) -
so training and runtime pixels match exactly.

For each image it writes <stem>_CLAHE.<ext> and copies the matching
<stem>.xml to <stem>_CLAHE.xml (annotations are unchanged - CLAHE moves
no pixels, only contrast). The _CLAHE files can then be merged with the
original raw dataset so the model trains on both.

Usage:
    python clahe_dataset.py --dir "C:/path/to/folder"
      [--remove-originals]   also delete the raw <stem>.* in this folder
                             (safe when the folder is a disposable copy;
                             leaves a CLAHE-only folder ready to merge)
"""

import argparse
import os
import shutil
import sys
from pathlib import Path

import cv2

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from yoloobb.preprocess import preprocess_frame  # noqa: E402

IMG_EXTS = (".jpg", ".jpeg", ".png", ".bmp")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[1])
    ap.add_argument("--dir", required=True, type=Path)
    ap.add_argument("--remove-originals", action="store_true")
    args = ap.parse_args()

    imgs = [p for p in sorted(args.dir.iterdir())
            if p.suffix.lower() in IMG_EXTS and "_CLAHE" not in p.stem]
    if not imgs:
        sys.exit(f"No images found in {args.dir}")

    n_img = n_xml = 0
    for ip in imgs:
        img = cv2.imread(str(ip))
        if img is None:
            print(f"  skip unreadable: {ip.name}", file=sys.stderr)
            continue
        out_img = ip.with_name(ip.stem + "_CLAHE" + ip.suffix)
        cv2.imwrite(str(out_img), preprocess_frame(img))
        n_img += 1

        xml = ip.with_suffix(".xml")
        if xml.is_file():
            shutil.copy2(xml, ip.with_name(ip.stem + "_CLAHE.xml"))
            n_xml += 1

        if args.remove_originals:
            ip.unlink()
            if xml.is_file():
                xml.unlink()

    print(f"CLAHE done: {n_img} images -> *_CLAHE, {n_xml} xmls copied")
    if args.remove_originals:
        print("  originals removed (folder is now CLAHE-only)")
    else:
        print("  originals kept (folder has both raw and _CLAHE versions)")
    print(f"Folder: {args.dir}")


if __name__ == "__main__":
    main()
