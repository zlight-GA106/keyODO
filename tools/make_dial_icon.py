# -*- coding: utf-8 -*-
"""
make_dial_icon.py -- builds res/keyodo.ico from the speedometer the program
itself draws.

The icon is not redrawn by hand: the desktop gauge is captured at a high DPI,
so the icon is literally the same instrument the app puts on screen, down to
the tick spacing and the red line.

Sizes 16, 32 and 48 are written as 4bpp BMP entries -- the Windows 98 SE
form -- and 64/128 as 32bpp. No PNG-compressed entries: those are a Vista
feature and older shells choke on them.

    python tools/make_dial_icon.py --source build/dial_hi.png --out res/keyodo.ico
"""
import argparse
import os
import struct
import sys

from PIL import Image, ImageDraw

# 4bpp is plenty: the dial is flat grey, white, black and red.
PLANAR_SIZES = [16, 32, 48]
DIRECT_SIZES = [64, 128]


def round_source(img):
    """Square-crop to the dial and knock out everything outside the circle."""
    w, h = img.size
    side = min(w, h)
    img = img.crop(((w - side) // 2, (h - side) // 2,
                    (w - side) // 2 + side, (h - side) // 2 + side)).convert("RGBA")
    mask = Image.new("L", (side, side), 0)
    ImageDraw.Draw(mask).ellipse((0, 0, side - 1, side - 1), fill=255)
    img.putalpha(mask)
    return img


def quantize(img, colors=16):
    """Downscale and reduce to a fixed palette, keeping the alpha."""
    alpha = img.split()[3]
    rgb = img.convert("RGB")
    p = rgb.convert("P", palette=Image.ADAPTIVE, colors=colors)
    pal = p.getpalette()[:colors * 3]
    out = p.convert("RGB")
    out.putalpha(alpha)
    return out, pal


def bmp_entry(img, pal, size, bpp):
    """One ICO image: BITMAPINFOHEADER + palette + XOR data + AND mask."""
    w = h = size
    xor_stride = ((w * bpp + 31) // 32) * 4
    and_stride = ((w + 31) // 32) * 4
    xor = bytearray(xor_stride * h)
    andb = bytearray(and_stride * h)

    px = img.load()
    for y in range(h):
        row = h - 1 - y                      # BMP rows are bottom-up
        for x in range(w):
            r, g, b, a = px[x, y]
            if a < 128:
                andb[row * and_stride + (x >> 3)] |= (0x80 >> (x & 7))
            if a < 128:
                r = g = b = 0                # transparent pixels show the AND mask
            if bpp == 4:
                idx = nearest(pal, r, g, b)
                off = row * xor_stride + (x >> 1)
                if x & 1:
                    xor[off] |= idx & 0x0F
                else:
                    xor[off] |= (idx & 0x0F) << 4
            else:
                off = row * xor_stride + x * 4
                xor[off] = b
                xor[off + 1] = g
                xor[off + 2] = r
                xor[off + 3] = 0

    hdr = struct.pack("<IiiHHIIiiII",
                      40, w, h * 2, 1, bpp, 0,
                      len(xor) + len(andb), 0, 0,
                      (1 << bpp) if bpp <= 8 else 0, 0)
    paldata = b""
    if bpp <= 8:
        n = 1 << bpp
        entries = [tuple(pal[i * 3:i * 3 + 3]) for i in range(len(pal) // 3)]
        while len(entries) < n:
            entries.append((0, 0, 0))
        for r, g, b in entries[:n]:
            paldata += struct.pack("<BBBB", b, g, r, 0)
    return hdr + paldata + bytes(xor) + bytes(andb)


def nearest(pal, r, g, b):
    best, bi = 1 << 30, 0
    for i in range(len(pal) // 3):
        pr, pg, pb = pal[i * 3], pal[i * 3 + 1], pal[i * 3 + 2]
        d = (pr - r) ** 2 + (pg - g) ** 2 + (pb - b) ** 2
        if d < best:
            best, bi = d, i
    return bi


def outlined(img, size):
    """Add a thin dark rim.

    The dial's own bezel is light grey, which disappears against the white
    header of the installer wizard and against a white Explorer background.
    A 1 px rim at icon scale makes the icon read on both light and dark
    backgrounds without altering the instrument itself.
    """
    w = 1 if size <= 48 else 2
    d = ImageDraw.Draw(img)
    d.ellipse((0, 0, size - 1, size - 1), outline=(0x40, 0x40, 0x40, 255), width=w)
    return img


def build(src, out):
    src = round_source(src)
    images = []
    for s in PLANAR_SIZES:
        small = outlined(src.resize((s, s), Image.LANCZOS), s)
        q, pal = quantize(small, 16)
        images.append((s, 4, bmp_entry(q, pal, s, 4)))
    for s in DIRECT_SIZES:
        small = outlined(src.resize((s, s), Image.LANCZOS), s)
        images.append((s, 32, bmp_entry(small, [], s, 32)))

    n = len(images)
    dir_entries = b""
    offset = 6 + 16 * n
    blob = b""
    for s, bpp, data in images:
        dir_entries += struct.pack("<BBBBHHII",
                                   s if s < 256 else 0,
                                   s if s < 256 else 0,
                                   1 << bpp if bpp <= 8 else 0, 0,
                                   1, bpp, len(data), offset)
        blob += data
        offset += len(data)

    with open(out, "wb") as f:
        f.write(struct.pack("<HHH", 0, 1, n) + dir_entries + blob)
    return n, 6 + 16 * n + len(blob)


def main(argv):
    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.dirname(here)
    ap = argparse.ArgumentParser()
    ap.add_argument("--source", default=os.path.join(root, "build", "dial_hi.png"))
    ap.add_argument("--out", default=os.path.join(root, "res", "keyodo.ico"))
    args = ap.parse_args(argv[1:])

    if not os.path.exists(args.source):
        print("missing source image: %s" % args.source)
        return 1
    src = Image.open(args.source).convert("RGBA")
    n, size = build(src, args.out)
    print("wrote %s: %d images (%s) %d bytes"
          % (args.out, n,
             ", ".join("%d@%dbpp" % (s, b) for s, b, _ in
                       [(s, b, None) for s in PLANAR_SIZES for b in (4,)] +
                       [(s, 32, None) for s in DIRECT_SIZES]),
             size))

    # verification strip so the result can be eyeballed
    sizes = PLANAR_SIZES + DIRECT_SIZES
    sheet = Image.new("RGB", (sum(sizes) + 6 * (n - 1), 132), (255, 255, 255))
    x = 0
    for s in sizes:
        tile = outlined(src.resize((s, s), Image.LANCZOS), s)
        flat = Image.new("RGB", (s, s), (255, 255, 255))
        flat.paste(tile, (0, 0), tile)      # composite over white via alpha
        sheet.paste(flat, (x, (132 - s) // 2))
        x += s + 6
    prev = os.path.join(root, "build", "icon_preview.png")
    sheet.save(prev)
    print("preview: %s" % prev)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
