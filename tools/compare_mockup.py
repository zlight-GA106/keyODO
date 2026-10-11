# -*- coding: utf-8 -*-
"""
compare_mockup.py -- compare a captured window against the reference design.

The reference render in mockup/out/ carries sample values (62 WPM, ODO
0012345...), while a freshly started KeyODO carries the real ones (0). A whole
image diff would therefore always fail. Instead this compares the regions that
do not depend on the data -- frame, caption, menu bar, group-box frames, button
bodies, status-bar chrome, the dial face -- and reports the rest separately.

    python tools/compare_mockup.py --shot build/capture.png

Exit code 0 when the static chrome matches within tolerance.
"""
import argparse
import os
import sys

from PIL import Image, ImageChops

# KeyODO draws its own chrome, so the window IS the client area shifted by the
# frame and caption. Regions below are written in CLIENT coordinates -- the same
# space as include/keyodo_layout.h -- and converted here, which is far less
# error-prone than hand-adding (3, 21) to every box.
FRAME = 3
CAPTION = 18


def W(l, t, r, b):
    return (l + FRAME, t + FRAME + CAPTION, r + FRAME, b + FRAME + CAPTION)


# region -> (box, must_match)
# must_match regions contain no live data, so they have to match the reference
# render nearly exactly. The rest carry the running values and are reported for
# information only.
REGIONS = [
    ("outer frame",        (0, 0, 480, 3),          True),
    ("caption strip",      (3, 3, 477, 21),         True),
    ("menu bar",           (0, 0, 474, 20),         True),
    ("speed group frame",  (4, 27, 256, 34),        True),
    ("odo group frame",    (260, 27, 470, 34),      True),
    ("rate group frame",   (260, 151, 470, 158),    True),
    ("dial panel top",     (38, 40, 222, 47),       True),
    ("status bar chrome",  (0, 276, 474, 281),      True),
    ("button 清零",         (270, 105, 340, 131),    True),
    ("button 复位",         (270, 241, 382, 267),    True),
    ("dial face",          (50, 60, 210, 210),      False),
    ("odo drum digits",    (270, 53, 460, 83),      False),
    ("kpm/max readouts",   (368, 160, 460, 212),    False),
    ("gear gate",          (270, 216, 460, 238),    False),
    ("status bar text",    (0, 278, 474, 296),      False),
]

REGIONS = [(name, W(*box) if len(box) == 4 and name not in
            ("outer frame", "caption strip") else box, static)
           for name, box, static in REGIONS]


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.dirname(here)

    ap = argparse.ArgumentParser()
    ap.add_argument("--shot", default=os.path.join(root, "build", "capture.png"))
    ap.add_argument("--ref", default=os.path.join(root, "mockup", "out",
                                                  "keyodo_480x320.png"))
    ap.add_argument("--tolerance", type=float, default=0.5,
                    help="percent of differing pixels allowed in static regions")
    args = ap.parse_args()

    if not os.path.exists(args.shot):
        print("missing capture: %s (run tools/capture_window.py first)" % args.shot)
        return 1
    if not os.path.exists(args.ref):
        print("missing reference: %s" % args.ref)
        return 1

    shot = Image.open(args.shot).convert("RGB")
    ref = Image.open(args.ref).convert("RGB")
    if shot.size != ref.size:
        print("size mismatch: capture %s vs reference %s" % (shot.size, ref.size))
        return 1

    worst = 0.0
    failures = []
    print("%-20s %8s %10s %8s" % ("region", "pixels", "differing", "ratio"))
    print("-" * 50)
    for name, box, static in REGIONS:
        a = shot.crop(box)
        b = ref.crop(box)
        diff = ImageChops.difference(a, b)
        hist = diff.convert("L").histogram()
        total = a.size[0] * a.size[1]
        differing = total - hist[0]
        pct = 100.0 * differing / total
        flag = ""
        if static:
            worst = max(worst, pct)
            if pct > args.tolerance:
                flag = "  <-- FAIL"
                failures.append(name)
        else:
            flag = "  (data dependent)"
        print("%-20s %8d %10d %7.2f%%%s" % (name, total, differing, pct, flag))

    print("-" * 50)
    print("worst static-region difference: %.2f%% (tolerance %.2f%%)"
          % (worst, args.tolerance))
    if failures:
        print("FAIL: %s" % ", ".join(failures))
        return 1
    print("PASS: chrome matches the reference design")
    return 0


if __name__ == "__main__":
    sys.exit(main())
