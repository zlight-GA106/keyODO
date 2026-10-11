# -*- coding: utf-8 -*-
"""
verify_scaling.py -- checks that the UI really scales, rather than merely
becoming bigger.

For each supported scaling it launches KeyODO with a forced DPI, captures the
window and locates structural landmarks (the horizontal 3D bevel lines that the
group boxes, the dial panel and the status bar draw). Every landmark must sit at
scale x its 96-DPI position, within one pixel of rounding.

It also checks the window size is exactly proportional, and that text is
re-rasterised at the larger size instead of being bitmap-stretched -- a
stretched render would be identical to a nearest-neighbour upscale of the 100%
render, which is precisely what it must NOT be.

    python tools/verify_scaling.py
"""
import os
import subprocess
import sys

from PIL import Image
import numpy as np

SCALES = [96, 120, 144, 192]
SHADOW = 0x80


def capture(root, dpi, out):
    exe = os.path.join(root, "build", "keyodo.exe")
    tool = os.path.join(root, "tools", "capture_window.py")
    subprocess.run([sys.executable, tool, "--out", out, "--dpi=%d" % dpi,
                    "--seconds", "0.9"], check=True, stdout=subprocess.DEVNULL)
    return Image.open(out).convert("RGB")


def bevel_rows(img, min_run):
    """Rows containing a long horizontal run of COLOR_3DSHADOW, i.e. the top
    edge of a sunken or raised frame. `min_run` is given in device pixels so
    the same logical edges are found at every scaling."""
    a = np.asarray(img.convert("L"))
    rows = []
    for y in range(a.shape[0]):
        row = a[y] == SHADOW
        best = cur = 0
        for v in row:
            cur = cur + 1 if v else 0
            best = max(best, cur)
        if best >= min_run:
            rows.append(y)
    # collapse consecutive rows into one landmark
    out = []
    for y in rows:
        if out and y - out[-1] <= 2:
            continue
        out.append(y)
    return out


def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    tmpl = os.path.join(root, "build", "_scale_%d.png")

    images = {}
    print("=== window size ===")
    ok = True
    for dpi in SCALES:
        img = capture(root, dpi, tmpl % dpi)
        images[dpi] = img
        want = (round(480 * dpi / 96), round(320 * dpi / 96))
        good = img.size == want
        ok = ok and good
        print("  %3d dpi  %-12s expected %-12s %s"
              % (dpi, img.size, want, "OK" if good else "MISMATCH"))

    print("\n=== structural landmarks (row of a long 3D bevel line) ===")
    ref = None
    for dpi in SCALES:
        s = dpi / 96.0
        rows = bevel_rows(images[dpi], int(100 * s))
        if ref is None:
            ref = rows
            print("  %3d dpi  100%%  rows = %s   (reference)" % (dpi, rows))
            continue
        # Every 96-DPI landmark must reappear at scale x its position.
        worst = 0
        missing = []
        for y in ref:
            want = round(y * s)
            near = min((abs(r - want) for r in rows), default=10 ** 6)
            if near > 1:
                missing.append((y, want))
            worst = max(worst, near)
        extra = [r for r in rows
                 if min((abs(r - round(y * s)) for y in ref), default=10 ** 6) > 1]
        good = not missing
        ok = ok and good
        print("  %3d dpi  %4.0f%%  rows = %s" % (dpi, s * 100, rows))
        print("             landmarks matched, worst delta = %d px%s  %s"
              % (worst,
                 "" if not missing else ", missing=%s" % missing,
                 "OK" if good else "MISMATCH"))
        if extra:
            print("             (+%d extra edge(s) only long enough to register "
                  "above 100%%: %s)" % (len(extra), extra))

    print("\n=== text is re-rasterised, not bitmap-stretched ===")
    base = images[96]
    for dpi in SCALES[1:]:
        s = dpi / 96.0
        up = base.resize(images[dpi].size, Image.NEAREST)
        a = np.asarray(images[dpi].convert("L")).astype(int)
        b = np.asarray(up.convert("L")).astype(int)
        diff = np.abs(a - b)
        mean = diff.mean()
        # A genuine re-render differs from a blocky upscale at every glyph edge.
        good = mean > 0.5
        ok = ok and good
        print("  %3d dpi  mean|render - upscaled 100%%| = %6.2f  %s"
              % (dpi, mean, "OK (crisp re-render)" if good else "SUSPECT (stretched)"))

    for dpi in SCALES:
        try:
            os.remove(tmpl % dpi)
        except OSError:
            pass

    print("\n%s" % ("PASS: scaling verified" if ok else "FAIL"))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
