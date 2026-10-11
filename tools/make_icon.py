# -*- coding: utf-8 -*-
"""
make_icon.py -- build res/keyodo.ico for KeyODO.

The icon is the one resource that cannot come from GDI, so it is drawn here as
explicit pixel art in the Windows 98 palette. The .ICO container is written by
hand rather than through Pillow's ICO plugin, for two reasons:

  1. Windows 98 SE wants a 4-bpp (16-colour) BMP image with an AND mask. A
     32-bpp or PNG-compressed icon shows up as a black block.
  2. The 16x16 and 32x32 images need different art (the 32x32 is drawn at
     double detail, not upscaled), so both are supplied explicitly.

ICO layout produced here:
    ICONDIR (6 bytes)
    ICONDIRENTRY * N (16 bytes each)
    for each image:
        BITMAPINFOHEADER, biHeight = 2 * height, biBitCount = 4, biClrUsed = 16
        16-entry BGRA palette
        XOR bitmap, 4 bpp, bottom-up, rows padded to 4 bytes
        AND mask, 1 bpp, bottom-up, rows padded to 4 bytes

Run:  python tools/make_icon.py
Out:  res/keyodo.ico
"""
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
OUT = os.path.join(ROOT, "res", "keyodo.ico")

# 16-colour palette actually used
PALETTE = [
    (0, 0, 0),        # 0 black
    (192, 192, 192),  # 1 COLOR_BTNFACE
    (128, 128, 128),  # 2 COLOR_BTNSHADOW
    (255, 255, 255),  # 3 COLOR_BTNHIGHLIGHT
]
PALETTE += [(0, 0, 0)] * (16 - len(PALETTE))

IDX = {"#": 0, "f": 1, "k": 2, "w": 3}

ART16 = [
    "................",
    "................",
    ".##############.",
    ".#wwwwwwwwwwww#.",
    ".#wfkkffkkffkk#.",
    ".#ffffffffffff#.",
    ".#fkkffkkffkkf#.",
    ".#ffffffffffff#.",
    ".#fkkffkkffkkf#.",
    ".#ffffffffffff#.",
    ".#fffkkkkkkfff#.",
    ".#ffffffffffff#.",
    ".#ffffffffffff#.",
    ".##############.",
    "................",
    "................",
]

ART32 = [
    "................................",
    "................................",
    "................................",
    "................................",
    "..############################..",
    "..############################..",
    "..#wwwwwwwwwwwwwwwwwwwwwwwwww#..",
    "..#wwwwwwwwwwwwwwwwwwwwwwwwww#..",
    "..#ffkkffkkffkkffkkffkkffkkff#..",
    "..#ffkkffkkffkkffkkffkkffkkff#..",
    "..#ffffffffffffffffffffffffff#..",
    "..#ffffffffffffffffffffffffff#..",
    "..#ffkkffkkffkkffkkffkkffkkff#..",
    "..#ffkkffkkffkkffkkffkkffkkff#..",
    "..#ffffffffffffffffffffffffff#..",
    "..#ffffffffffffffffffffffffff#..",
    "..#ffkkffkkffkkffkkffkkffkkff#..",
    "..#ffkkffkkffkkffkkffkkffkkff#..",
    "..#ffffffffffffffffffffffffff#..",
    "..#ffffffffffffffffffffffffff#..",
    "..#ffkkffkkffkkffkkffkkffkkff#..",
    "..#ffkkffkkffkkffkkffkkffkkff#..",
    "..#ffffffffffffffffffffffffff#..",
    "..#ffffffffffffffffffffffffff#..",
    "..#fffffkkkkkkkkkkkkkkfffffff#..",
    "..#fffffkkkkkkkkkkkkkkfffffff#..",
    "..#ffffffffffffffffffffffffff#..",
    "..#ffffffffffffffffffffffffff#..",
    "..############################..",
    "..############################..",
    "................................",
    "................................",
]


def encode_image(art):
    """Returns the BITMAPINFOHEADER + palette + XOR + AND block for one size."""
    h = len(art)
    w = len(art[0])
    assert w == h and w in (16, 32), "square 16 or 32 only"

    xor_stride = ((w * 4 + 31) // 32) * 4      # 4 bpp rows, padded to 32 bits
    and_stride = ((w + 31) // 32) * 4          # 1 bpp rows, padded to 32 bits

    xor = bytearray(xor_stride * h)
    andm = bytearray(and_stride * h)

    for y in range(h):
        src_row = art[y]
        # DIB rows are bottom-up
        row = h - 1 - y
        for x in range(w):
            ch = src_row[x]
            if ch == ".":
                # transparent: AND bit set
                andm[row * and_stride + (x // 8)] |= 0x80 >> (x % 8)
                continue
            idx = IDX[ch]
            byte = row * xor_stride + (x // 2)
            if x % 2 == 0:
                xor[byte] |= (idx & 0x0F) << 4
            else:
                xor[byte] |= (idx & 0x0F)

    header = struct.pack(
        "<IiiHHIIiiII",
        40,              # biSize
        w,               # biWidth
        h * 2,           # biHeight: XOR + AND
        1,               # biPlanes
        4,               # biBitCount
        0,               # biCompression = BI_RGB
        len(xor) + len(andm),
        0, 0,            # pixels per metre
        16,              # biClrUsed
        0,               # biClrImportant
    )

    pal = bytearray()
    for r, g, b in PALETTE:
        pal += bytes((b, g, r, 0))

    return bytes(header) + bytes(pal) + bytes(xor) + bytes(andm)


def main():
    images = [encode_image(ART16), encode_image(ART32)]

    out = bytearray()
    out += struct.pack("<HHH", 0, 1, len(images))     # reserved, type=icon, count

    offset = 6 + 16 * len(images)
    for art, blob in zip((ART16, ART32), images):
        n = len(art)
        out += struct.pack(
            "<BBBBHHII",
            n if n < 256 else 0,     # width
            n if n < 256 else 0,     # height
            16,                      # colour count (16-colour palette)
            0,                       # reserved
            1,                       # planes
            4,                       # bit count
            len(blob),
            offset,
        )
        offset += len(blob)

    for blob in images:
        out += blob

    with open(OUT, "wb") as f:
        f.write(out)

    # verify by parsing back
    data = open(OUT, "rb").read()
    _res, typ, cnt = struct.unpack("<HHH", data[:6])
    info = []
    for i in range(cnt):
        w, h, cc, _r, pl, bc, size, off = struct.unpack(
            "<BBBBHHII", data[6 + 16 * i:6 + 16 * (i + 1)])
        bi = struct.unpack("<IiiHHIIiiII", data[off:off + 40])
        info.append("%dx%d %dbpp clrUsed=%d bytes=%d" % (w, h, bi[4], bi[9], size))
    print("wrote %s (%d bytes)" % (OUT, len(data)))
    print("  type=%d entries=%d" % (typ, cnt))
    for s in info:
        print("  " + s)
    return 0


if __name__ == "__main__":
    sys.exit(main())
