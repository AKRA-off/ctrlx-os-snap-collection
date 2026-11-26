"""
Generate a modern flat icon set for the ctrlX OBB Annotator into
icons_modern/, rendered from the Windows system icon font
(Segoe Fluent Icons on Win11, Segoe MDL2 Assets fallback) in the
Bosch Rexroth palette. Run once after changing the mapping:

    python generate_modern_icons.py
"""

import os
import sys

from PyQt5.QtGui import (QColor, QFont, QFontDatabase, QGuiApplication,
                         QImage, QPainter)
from PyQt5.QtCore import Qt, QRectF

from libs.rexroth_style import PRIMARY_BLUE, SUCCESS_GREEN, ACCENT_ORANGE

SIZE = 64

# icon name -> (glyph codepoint, color)
# Codepoints are from the Segoe MDL2 Assets / Fluent Icons table.
ICONS = {
    "app":        (0xE7AD, PRIMARY_BLUE),   # Rotate - the app's essence
    "open":       (0xE8E5, PRIMARY_BLUE),   # OpenFile
    "openAnnotation": (0xE7C3, PRIMARY_BLUE),  # Page
    "opendir":    (0xE838, PRIMARY_BLUE),   # FolderOpen
    "save":       (0xE74E, PRIMARY_BLUE),   # Save
    "save-as":    (0xE792, PRIMARY_BLUE),   # SaveAs
    "close":      (0xE711, PRIMARY_BLUE),   # Cancel
    "quit":       (0xE7E8, ACCENT_ORANGE),  # PowerButton
    "new":        (0xE710, PRIMARY_BLUE),   # Add (new box)
    "newRo":      (0xE7AD, PRIMARY_BLUE),   # Rotate (new rotated box)
    "copy":       (0xE8C8, PRIMARY_BLUE),   # Copy
    "delete":     (0xE74D, ACCENT_ORANGE),  # Delete
    "edit":       (0xE70F, PRIMARY_BLUE),   # Edit
    "color":      (0xE790, PRIMARY_BLUE),   # Color
    "color_line": (0xE891, PRIMARY_BLUE),   # Highlight
    "hide":       (0xE7B3, PRIMARY_BLUE),   # RedEye
    "eye":        (0xE7B3, PRIMARY_BLUE),   # RedEye
    "done":       (0xE8FB, SUCCESS_GREEN),  # Accept (verify)
    "next":       (0xE72A, PRIMARY_BLUE),   # Forward
    "prev":       (0xE72B, PRIMARY_BLUE),   # Back
    "help":       (0xE897, PRIMARY_BLUE),   # Help
    "labels":     (0xE8EC, PRIMARY_BLUE),   # Tag
    "expert":     (0xE713, PRIMARY_BLUE),   # Settings
    "expert1":    (0xE713, PRIMARY_BLUE),   # Settings
    "expert2":    (0xE90F, PRIMARY_BLUE),   # Repair
    "zoom":       (0xE71E, PRIMARY_BLUE),   # Zoom
    "zoom-in":    (0xE8A3, PRIMARY_BLUE),   # ZoomIn
    "zoom-out":   (0xE71F, PRIMARY_BLUE),   # ZoomOut
    "fit":        (0xE9A6, PRIMARY_BLUE),   # FitPage
    "fit-window": (0xE9A6, PRIMARY_BLUE),   # FitPage
    "fit-width":  (0xE8E3, PRIMARY_BLUE),   # AlignCenter
    "file":       (0xE7C3, PRIMARY_BLUE),   # Page
    "cancel":     (0xE711, ACCENT_ORANGE),  # Cancel
    "undo":       (0xE7A7, PRIMARY_BLUE),   # Undo
    "undo-cross": (0xE894, ACCENT_ORANGE),  # Clear
    "resetall":   (0xE894, ACCENT_ORANGE),  # Clear
    "verify":     (0xE8FB, SUCCESS_GREEN),  # Accept
    "export":     (0xEDE1, SUCCESS_GREEN),  # Export (training dataset)
}


def pick_font():
    families = set(QFontDatabase().families())
    for name in ("Segoe Fluent Icons", "Segoe MDL2 Assets"):
        if name in families:
            return name
    sys.exit("No Segoe icon font found - run on Windows 10/11")


def main():
    app = QGuiApplication(sys.argv)  # needed for font rendering
    out_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                           "icons_modern")
    os.makedirs(out_dir, exist_ok=True)

    family = pick_font()
    font = QFont(family)
    font.setPixelSize(int(SIZE * 0.72))

    for name, (codepoint, color) in ICONS.items():
        img = QImage(SIZE, SIZE, QImage.Format_ARGB32)
        img.fill(Qt.transparent)
        p = QPainter(img)
        p.setRenderHint(QPainter.Antialiasing)
        p.setRenderHint(QPainter.TextAntialiasing)
        p.setFont(font)
        p.setPen(QColor(color))
        p.drawText(QRectF(0, 0, SIZE, SIZE), Qt.AlignCenter, chr(codepoint))
        p.end()
        img.save(os.path.join(out_dir, name + ".png"))

    print(f"Generated {len(ICONS)} icons ({family}) -> {out_dir}")


if __name__ == "__main__":
    main()
