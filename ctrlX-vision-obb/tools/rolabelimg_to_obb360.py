#!/usr/bin/env python3
"""
Convert roLabelImg XML annotations (with full 0..2pi heading, see the
patched roLabelImg in ../roLabelImg) into the two training sets needed
for the 360-degree OBB pipeline:

1. labels/<stem>.txt   -- Ultralytics YOLO-OBB format:
                          "cls x1 y1 x2 y2 x3 y3 x4 y4" (normalized).
                          Direction is NOT encoded here (Ultralytics
                          discards it anyway); trains the 180-deg OBB.

2. heading_crops/head_fwd/  -- object crops rotated so the annotated
   heading_crops/head_rev/     heading points +x ("fwd"); each crop's
                               180-degree rotation goes to "rev".
                               Trains the binary heading classifier
                               that resolves OBB angle vs angle+180.

Angle convention (matches roLabelImg XML and Ultralytics ONNX output):
heading unit vector = (cos a, sin a) in image coordinates (y down),
i.e. a is measured from the +x axis, clockwise on screen.

Usage:
  python rolabelimg_to_obb360.py --xml-dir ann/ --img-dir imgs/ --out-dir dataset/
"""

import argparse
import math
import random
import shutil
import sys
from pathlib import Path
from xml.etree import ElementTree

try:
    import cv2
except ImportError:
    cv2 = None  # labels can still be generated; heading crops need OpenCV

IMG_EXTS = [".jpg", ".jpeg", ".png", ".bmp"]


def parse_xml(xml_path):
    """Return (img_w, img_h, objects); each object is
    dict(label, cx, cy, w, h, angle). Plain bndbox objects get angle 0."""
    root = ElementTree.parse(str(xml_path)).getroot()
    size = root.find("size")
    img_w = int(size.find("width").text)
    img_h = int(size.find("height").text)

    objects = []
    for obj in root.findall("object"):
        label = obj.find("name").text
        type_el = obj.find("type")
        obj_type = type_el.text if type_el is not None else "bndbox"

        if obj_type == "robndbox":
            rb = obj.find("robndbox")
            objects.append({
                "label": label,
                "cx": float(rb.find("cx").text),
                "cy": float(rb.find("cy").text),
                "w": float(rb.find("w").text),
                "h": float(rb.find("h").text),
                "angle": float(rb.find("angle").text),
            })
        else:
            bb = obj.find("bndbox")
            xmin, ymin = float(bb.find("xmin").text), float(bb.find("ymin").text)
            xmax, ymax = float(bb.find("xmax").text), float(bb.find("ymax").text)
            objects.append({
                "label": label,
                "cx": (xmin + xmax) / 2, "cy": (ymin + ymax) / 2,
                "w": xmax - xmin, "h": ymax - ymin,
                "angle": 0.0,
            })
    return img_w, img_h, objects


def obb_corners(cx, cy, w, h, angle):
    """4 corners of the rotated box, same math as roLabelImg's reader:
    R(angle) applied to the axis-aligned corners about the center."""
    cos_a, sin_a = math.cos(angle), math.sin(angle)
    corners = []
    for dx, dy in ((-w / 2, -h / 2), (w / 2, -h / 2),
                   (w / 2, h / 2), (-w / 2, h / 2)):
        corners.append((cx + cos_a * dx - sin_a * dy,
                        cy + sin_a * dx + cos_a * dy))
    return corners


def find_image(xml_path, img_dir):
    for ext in IMG_EXTS:
        cand = img_dir / (xml_path.stem + ext)
        if cand.exists():
            return cand
    return None


def aligned_crop(img, cx, cy, w, h, angle, margin=1.15):
    """Rotate the image so the annotated heading points +x, then crop
    the (margin-padded) box. cv2's positive angle rotates content
    counter-clockwise on screen, which cancels our clockwise `angle`.

    (A patch-based fast path was tried but is NOT bit-identical to this
    full-image warp - max ~8/255 pixel diff that padding doesn't remove -
    so it's kept as-is to guarantee training crops match the deployed
    classifier's runtime crops. See yoloobb/heading.py._aligned_crop.)"""
    M = cv2.getRotationMatrix2D((cx, cy), math.degrees(angle), 1.0)
    rotated = cv2.warpAffine(img, M, (img.shape[1], img.shape[0]),
                             flags=cv2.INTER_LINEAR,
                             borderMode=cv2.BORDER_REPLICATE)
    cw, ch = w * margin, h * margin
    x0, y0 = int(round(cx - cw / 2)), int(round(cy - ch / 2))
    x1, y1 = int(round(cx + cw / 2)), int(round(cy + ch / 2))
    x0, y0 = max(x0, 0), max(y0, 0)
    x1 = min(x1, rotated.shape[1])
    y1 = min(y1, rotated.shape[0])
    if x1 - x0 < 4 or y1 - y0 < 4:
        return None
    return rotated[y0:y1, x0:x1]


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[1])
    ap.add_argument("--xml-dir", required=True, type=Path)
    ap.add_argument("--img-dir", type=Path, default=None,
                    help="defaults to --xml-dir")
    ap.add_argument("--out-dir", required=True, type=Path)
    ap.add_argument("--classes", type=Path, default=None,
                    help="optional classes.txt fixing class-id order")
    ap.add_argument("--margin", type=float, default=1.15,
                    help="crop margin factor around the box (default 1.15)")
    ap.add_argument("--no-copy-images", action="store_true",
                    help="do not copy images into out-dir/images")
    ap.add_argument("--heading-offset", type=int, choices=[0, 180], default=0,
                    help="180 if your annotation arrows point at the BACK of "
                         "the objects: headings are rotated 180 deg for the "
                         "classifier crops (OBB labels are unaffected)")
    ap.add_argument("--heading-bins", type=int, default=0,
                    help="N > 0 switches the classifier dataset from binary "
                         "front/back to N rotation bins (e.g. 24 = 15 deg "
                         "steps): each object is exported at every bin "
                         "rotation. Use for SQUARE-ish objects, where the "
                         "OBB angle is degenerate and the classifier must "
                         "recover the full rotation itself. N must divide 360.")
    ap.add_argument("--bin-jitter", type=float, default=-1.0,
                    help="bins mode: random rotation jitter in degrees "
                         "around each bin center, so the classifier sees "
                         "between-bin angles like at inference. Default -1 = "
                         "half the bin width (full coverage, no gaps between "
                         "bins; e.g. 7.5 for 24 bins). 0 disables.")
    ap.add_argument("--val-split", type=float, default=0.2,
                    help="fraction of images assigned to val (default 0.2; "
                         "0 disables the split and emits a flat layout). "
                         "Split is per source image, so an object's fwd/rev "
                         "crop pair never straddles train and val.")
    ap.add_argument("--seed", type=int, default=42,
                    help="random seed for the train/val split (default 42)")
    args = ap.parse_args()
    if not 0 <= args.val_split < 1:
        sys.exit("--val-split must be in [0, 1)")
    if args.heading_bins and 360 % args.heading_bins != 0:
        sys.exit("--heading-bins must divide 360 (e.g. 8, 12, 24, 36)")
    if args.heading_bins and args.bin_jitter < 0:
        args.bin_jitter = 360 / args.heading_bins / 2  # full bin coverage

    img_dir = args.img_dir or args.xml_dir

    xml_files = sorted(args.xml_dir.glob("*.xml"))
    if not xml_files:
        sys.exit(f"No XML files found in {args.xml_dir}")

    # Deterministic per-image split (shuffle, then cut) so reruns with the
    # same seed and file set reproduce the same train/val assignment.
    splits = ("train", "val") if args.val_split > 0 else ("",)
    shuffled = list(xml_files)
    random.Random(args.seed).shuffle(shuffled)
    n_val = round(len(shuffled) * args.val_split)
    if args.val_split > 0 and len(shuffled) > 1:
        n_val = min(max(n_val, 1), len(shuffled) - 1)
    val_stems = {x.stem for x in shuffled[:n_val]} if args.val_split > 0 else set()

    # heading_crops is purely generated content: always rebuild from
    # scratch so a mode switch (binary <-> bins) or stale Ultralytics
    # .cache files can never contaminate the classifier dataset
    crops_root = args.out_dir / "heading_crops"
    if crops_root.exists():
        shutil.rmtree(crops_root)

    labels_dir = {s: args.out_dir / "labels" / s for s in splits}
    images_dir = {s: args.out_dir / "images" / s for s in splits}
    crop_dirs = {}  # (split, class_folder) -> Path
    if args.heading_bins:
        bin_step = 360 // args.heading_bins
        bin_names = [f"bin_{k * bin_step:03d}" for k in range(args.heading_bins)]
    else:
        bin_names = ["head_fwd", "head_rev"]
    for s in splits:
        for b in bin_names:
            crop_dirs[(s, b)] = args.out_dir / "heading_crops" / s / b
    for d in list(labels_dir.values()) + list(crop_dirs.values()):
        d.mkdir(parents=True, exist_ok=True)
    if not args.no_copy_images:
        for d in images_dir.values():
            d.mkdir(parents=True, exist_ok=True)

    # Class-name -> id mapping
    if args.classes:
        names = [ln.strip() for ln in args.classes.read_text().splitlines()
                 if ln.strip()]
    else:
        names = sorted({o["label"] for x in xml_files
                        for o in parse_xml(x)[2]})
    cls_id = {n: i for i, n in enumerate(names)}

    n_boxes = n_crops = 0
    jitter_rng = random.Random(args.seed + 1)
    for xml_path in xml_files:
        split = ("val" if xml_path.stem in val_stems else "train") \
            if args.val_split > 0 else ""
        img_w, img_h, objects = parse_xml(xml_path)
        img_path = find_image(xml_path, img_dir)
        img = cv2.imread(str(img_path)) if (img_path and cv2) else None
        if img is None:
            reason = "OpenCV not installed" if cv2 is None else "no image found"
            print(f"WARNING: {reason} for {xml_path.name}, "
                  f"labels written but no crops", file=sys.stderr)

        lines = []
        for k, o in enumerate(objects):
            if o["label"] not in cls_id:
                print(f"WARNING: label '{o['label']}' not in classes, "
                      f"skipped ({xml_path.name})", file=sys.stderr)
                continue
            corners = obb_corners(o["cx"], o["cy"], o["w"], o["h"], o["angle"])
            coords = " ".join(f"{min(max(x / img_w, 0), 1):.6f} "
                              f"{min(max(y / img_h, 0), 1):.6f}"
                              for x, y in corners)
            lines.append(f"{cls_id[o['label']]} {coords}")
            n_boxes += 1

            if img is not None:
                heading = o["angle"] + math.radians(args.heading_offset)
                stem = f"{xml_path.stem}_{k}_{o['label']}"
                if args.heading_bins:
                    # One crop per rotation bin: the object's front ends up
                    # at bin angle phi (+- jitter) within the crop; the
                    # label stays the bin, teaching between-bin robustness
                    for b in range(args.heading_bins):
                        phi = b * (360 // args.heading_bins)
                        jit = jitter_rng.uniform(-args.bin_jitter,
                                                 args.bin_jitter)
                        crop = aligned_crop(img, o["cx"], o["cy"],
                                            o["w"], o["h"],
                                            heading - math.radians(phi + jit),
                                            args.margin)
                        if crop is not None:
                            cv2.imwrite(str(crop_dirs[(split, f"bin_{phi:03d}")]
                                            / f"{stem}.jpg"), crop)
                            n_crops += 1
                else:
                    crop = aligned_crop(img, o["cx"], o["cy"], o["w"], o["h"],
                                        heading, args.margin)
                    if crop is not None:
                        cv2.imwrite(str(crop_dirs[(split, "head_fwd")]
                                        / f"{stem}.jpg"), crop)
                        cv2.imwrite(str(crop_dirs[(split, "head_rev")]
                                        / f"{stem}.jpg"),
                                    cv2.rotate(crop, cv2.ROTATE_180))
                        n_crops += 2

        (labels_dir[split] / f"{xml_path.stem}.txt").write_text(
            "\n".join(lines) + "\n")
        if img_path and not args.no_copy_images:
            shutil.copy2(img_path, images_dir[split] / img_path.name)

    (args.out_dir / "classes.txt").write_text("\n".join(names) + "\n")

    # Ready-to-train Ultralytics OBB dataset config
    if args.val_split > 0:
        train_path, val_path = "images/train", "images/val"
    else:
        train_path = val_path = "images"
    names_yaml = "\n".join(f"  {i}: {n}" for i, n in enumerate(names))
    (args.out_dir / "data.yaml").write_text(
        f"path: {args.out_dir.resolve().as_posix()}\n"
        f"train: {train_path}\nval: {val_path}\nnames:\n{names_yaml}\n")

    split_info = (f"val split: {len(val_stems)}/{len(xml_files)} images "
                  f"(seed {args.seed})") if args.val_split > 0 else "no split"
    print(f"Done: {len(xml_files)} images, {n_boxes} OBB labels, "
          f"{n_crops} heading crops ({len(names)} classes: {names}); "
          f"{split_info}")
    print(f"Train OBB:        yolo obb train data={args.out_dir / 'data.yaml'} "
          f"model=yolo11n-obb.pt epochs=100 imgsz=640")
    print(f"Train classifier: yolo classify train "
          f"data={args.out_dir / 'heading_crops'} model=yolo11n-cls.pt "
          f"epochs=50 imgsz=64")


if __name__ == "__main__":
    main()
