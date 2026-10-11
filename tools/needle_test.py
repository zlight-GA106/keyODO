# -*- coding: utf-8 -*-
"""
needle_test.py -- measures the needle's real motion.

Reads the needle ANGLE out of a screenshot rather than trusting an internal
value: the black needle is the only near-black ink inside the dial at radius
20..45 px (the numerals sit at radius 52, the ticks at 64+, the printed unit is
grey). The angle is converted back to WPM through the dial's own sweep, so this
checks what the user actually sees.

It then confirms the coast-down: after a burst of typing the needle must fall
back smoothly and settle, not sit still for the whole statistics window and
then snap to zero.

    python tools/needle_test.py
"""
import ctypes
import ctypes.wintypes as wt
import math
import os
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import capture_window                      # noqa: E402
from smoke_test import tap                 # noqa: E402

u32 = ctypes.WinDLL("user32", use_last_error=True)
u32.FindWindowA.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
u32.FindWindowA.restype = wt.HWND
u32.PostMessageA.argtypes = [wt.HWND, wt.UINT, wt.WPARAM, wt.LPARAM]

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXE = os.path.join(ROOT, "build", "keyodo.exe")

# 96 DPI layout: frame 3, caption 18, panel (38,40)-(222,224), centre = +92,+92
CX = 3 + 38 + 92
CY = 3 + 18 + 40 + 92
SWEEP_START = 210.0
SWEEP_TOTAL = 240.0
WPM_MAX = 120.0


def needle_wpm(img):
    """Angle of the needle, read off the screen.

    Averaging the black pixels does not work: the needle is a wedge with a tip
    AND a short tail either side of the hub, so the two halves cancel out. What
    is distinctive is that the needle is a solid radial run, far longer than
    anything else on the face, so the angle with the longest black run wins.
    """
    px = img.load()
    w, h = img.size
    best_angle, best_run = None, 0

    for deg in range(0, 360):
        a = math.radians(deg)
        ca, sa = math.cos(a), math.sin(a)
        run = 0
        started = False
        for r in range(10, 62):
            x = int(round(CX + r * ca))
            y = int(round(CY - r * sa))
            if not (0 <= x < w and 0 <= y < h):
                break
            p = px[x, y]
            black = p[0] < 60 and p[1] < 60 and p[2] < 60
            if black:
                run += 1
                started = True
            elif started:
                break               # run ended
        if run > best_run:
            best_run, best_angle = run, deg

    if best_angle is None or best_run < 12:
        return None
    return max(0.0, min(WPM_MAX,
                        (SWEEP_START - best_angle) * WPM_MAX / SWEEP_TOTAL))


def main():
    p = subprocess.Popen([EXE, "--dpi=96"], stdout=subprocess.DEVNULL,
                         stderr=subprocess.DEVNULL)
    try:
        hwnd = None
        end = time.time() + 8
        while time.time() < end:
            hwnd = u32.FindWindowA(b"KeyODO_Main", None)
            if hwnd:
                break
            time.sleep(0.1)
        if not hwnd:
            print("FAIL: window not found")
            return 1
        time.sleep(0.8)

        rest = needle_wpm(capture_window.grab(hwnd))
        print("needle at rest              : %s WPM" % fmt(rest))

        # burst of typing
        for _ in range(36):
            tap(0x41)                        # 'A'
        print("injected 36 keystrokes")

        samples = []
        t0 = time.time()
        while time.time() - t0 < 6.0:
            v = needle_wpm(capture_window.grab(hwnd))
            samples.append((round(time.time() - t0, 2), v))
            time.sleep(0.12)

        vals = [(t, v) for t, v in samples if v is not None]
        peak_t, peak = max(vals, key=lambda tv: tv[1])
        print("\n  t(s)   needle")
        for t, v in vals[::3]:
            print("  %4.1f   %5.1f  %s" % (t, v, "#" * int(v / 2)))

        # only look AFTER the peak, otherwise "half the peak" is crossed during
        # the rise before any typing has registered
        settled = next((t for t, v in vals if t > peak_t + 0.3 and v < 3.0), None)
        half = next((t for t, v in vals if t > peak_t and v < peak * 0.5), None)
        fall = (settled - peak_t) if settled else None

        print()
        ok = True

        def check(name, cond, detail=""):
            nonlocal ok
            print("  %-4s %s%s" % ("OK" if cond else "FAIL", name,
                                   ("  -- " + detail) if detail else ""))
            if not cond:
                ok = False

        check("needle rises while typing", peak > 12.0,
              "peak %.1f WPM at %.1fs" % (peak, peak_t))
        check("needle falls back to rest", settled is not None,
              "reached <3 WPM %.1fs after the peak" % fall if fall else "never")
        check("the fall is gradual, not a snap",
              fall is not None and 0.5 <= fall <= 6.0,
              "coast-down took %.1fs" % fall if fall else "n/a")
        check("half the speed is shed in a visible time",
              half is not None and (half - peak_t) > 0.25,
              "half-way %.2fs after the peak" % (half - peak_t) if half else "n/a")
        check("needle settles back on the rest position",
              abs((rest or 0) - vals[-1][1]) < 4.0,
              "rest %s -> final %.1f" % (fmt(rest), vals[-1][1]))

        print()
        print("PASS: needle coast-down verified" if ok else "FAIL")
        return 0 if ok else 1
    finally:
        if hwnd:
            u32.PostMessageA(hwnd, 0x0010, 0, 0)
        try:
            p.wait(timeout=8)
        except subprocess.TimeoutExpired:
            p.kill()


def fmt(v):
    return "n/a" if v is None else "%.1f" % v


if __name__ == "__main__":
    sys.exit(main())
