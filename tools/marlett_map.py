# -*- coding: utf-8 -*-
"""
marlett_map.py -- Dump MARLETT.TTF into a labelled contact sheet + text table.

MARLETT.TTF is the genuine Windows 95/98 UI glyph font (still shipped in
C:\\Windows\\Fonts on Windows 11). It is not a text font: it is a symbol font in
which individual letters map to pixel-exact pieces of the classic chrome --
scrollbar arrows, the combo-box drop arrow, menu checkmarks/bullets, the
minimise/maximise/close/restore caption glyphs, the size grip, and so on.

A Win98 application draws that chrome by `SelectObject`-ing MARLETT and
`TextOut`-ing the right letter. This script renders every slot and reports the
ink bounding box of each, so the glyph -> letter mapping can be read off
evidence instead of trusted to memory.

Outputs: reports/marlett_sheet.png, reports/marlett_map.txt
"""
import os
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
OUT = os.path.join(ROOT, "reports")
os.makedirs(OUT, exist_ok=True)

FONT_PATH = r"C:\Windows\Fonts\marlett.ttf"

PT = 16            # nominal pixel size the glyph font is designed for
CANVAS = 20        # square canvas the glyph is drawn into
Z = 3              # nearest-neighbour zoom for the sheet
CELL = CANVAS * Z + 6
LABEL_H = 12
COLS = 12

codes = list(range(0x20, 0x100))


def ink_stats(img):
    """Return (count, bbox) of non-white pixels."""
    px = img.load()
    w, h = img.size
    n, x0, y0, x1, y1 = 0, w, h, -1, -1
    for y in range(h):
        for x in range(w):
            if px[x, y] != (255, 255, 255):
                n += 1
                x0, y0 = min(x0, x), min(y0, y)
                x1, y1 = max(x1, x), max(y1, y)
    return n, (x0, y0, x1, y1) if n else None


def main():
    if not os.path.exists(FONT_PATH):
        raise SystemExit("MARLETT not found at %s" % FONT_PATH)

    f = ImageFont.truetype(FONT_PATH, PT)

    rows = (len(codes) + COLS - 1) // COLS
    sheet = Image.new("RGB", (COLS * CELL, rows * (CELL + LABEL_H) + 26), (192, 192, 192))
    d = ImageDraw.Draw(sheet)
    d.text((4, 4), "MARLETT.TTF glyph map  %s   glyphs rendered at %dpx, zoom x%d"
           % (FONT_PATH, PT, Z), fill=(0, 0, 0))

    table, empty = [], []
    for i, c in enumerate(codes):
        cell = Image.new("RGB", (CANVAS, CANVAS), (255, 255, 255))
        dc = ImageDraw.Draw(cell)
        # anchor "la" = left / ascender, so glyphs sit on a common baseline
        try:
            dc.text((2, 2), chr(c), font=f, fill=(0, 0, 0), anchor="la")
        except Exception:
            dc.text((2, 2), chr(c), font=f, fill=(0, 0, 0))
        n, bbox = ink_stats(cell)

        cx = (i % COLS) * CELL
        cy = (i // COLS) * (CELL + LABEL_H) + 26 + LABEL_H
        big = cell.resize((CANVAS * Z, CANVAS * Z), Image.NEAREST)
        d.rectangle([cx, cy, cx + big.size[0] - 1, cy + big.size[1] - 1], outline=(128, 128, 128))
        sheet.paste(big, (cx, cy))
        label = "%02X %s" % (c, chr(c) if 0x21 <= c <= 0x7E else ".")
        d.text((cx + 1, cy - LABEL_H + 1), label, fill=(0, 0, 0))

        if n:
            bw, bh = bbox[2] - bbox[0] + 1, bbox[3] - bbox[1] + 1
            table.append("  0x%02X  %-3s  ink=%-4d bbox=(%2d,%2d)-(%2d,%2d)  %dx%d"
                         % (c, repr(chr(c))[1:-1], n, bbox[0], bbox[1], bbox[2], bbox[3], bw, bh))
        else:
            empty.append("0x%02X" % c)

    sheet.save(os.path.join(OUT, "marlett_sheet.png"))

    lines = []
    lines.append("MARLETT.TTF glyph inventory -- %s" % FONT_PATH)
    lines.append("rendered at %dpx into a %dx%d canvas, anchor=left/ascender" % (PT, CANVAS, CANVAS))
    lines.append("")
    lines.append("SLOTS WITH INK (%d of %d):" % (len(table), len(codes)))
    lines.extend(table)
    lines.append("")
    lines.append("EMPTY SLOTS (%d): %s" % (len(empty), " ".join(empty)))
    with open(os.path.join(OUT, "marlett_map.txt"), "w", encoding="utf-8") as fh:
        fh.write("\n".join(lines))

    print("\n".join(lines[:60]))
    print("...")
    print("slots with ink: %d / %d" % (len(table), len(codes)))
    print("wrote reports/marlett_sheet.png  %s" % (sheet.size,))


if __name__ == "__main__":
    main()
