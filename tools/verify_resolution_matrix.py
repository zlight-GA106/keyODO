# -*- coding: utf-8 -*-
"""
verify_resolution_matrix.py -- checks KeyODO fits the displays it claims to
support.

The window and the desktop gauge are sized from the monitor DPI, so the sizes
are MEASURED here by launching the real program with each scaling forced, not
re-derived from the formula. Those measured sizes are then placed on every
resolution/aspect-ratio combination and checked against a work area that leaves
room for a taskbar.

    python tools/verify_resolution_matrix.py
"""
import ctypes
import ctypes.wintypes as wt
import os
import subprocess
import sys
import time

u32 = ctypes.WinDLL("user32", use_last_error=True)
try:
    u32.SetProcessDPIAware()
except AttributeError:
    pass
u32.FindWindowA.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
u32.FindWindowA.restype = wt.HWND
u32.GetWindowRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXE = os.path.join(ROOT, "build", "keyodo.exe")

SCALES = [96, 120, 144, 192]

# name, width, height, aspect label, DPI. The scalings are the ones these
# panels normally ship with.
DISPLAYS = [
    ("1920x1080", 1920, 1080, "16:9", 96),
    ("1920x1080", 1920, 1080, "16:9", 120),
    ("2560x1440", 2560, 1440, "16:9", 120),
    ("2560x1440", 2560, 1440, "16:9", 144),
    ("3840x2160", 3840, 2160, "16:9", 144),
    ("3840x2160", 3840, 2160, "16:9", 192),
    ("1920x1200", 1920, 1200, "16:10", 96),
    ("1920x1200", 1920, 1200, "16:10", 120),
    ("2560x1600", 2560, 1600, "16:10", 144),
    ("3840x2400", 3840, 2400, "16:10", 192),
]

TASKBAR = 48          # a generous allowance for the taskbar / dock
failures = []


def measure(dpi):
    """Launch KeyODO at a forced DPI and measure both windows."""
    p = subprocess.Popen([EXE, "--dpi=%d" % dpi],
                         stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    try:
        main = gauge = None
        end = time.time() + 8
        while time.time() < end and not (main and gauge):
            main = main or u32.FindWindowA(b"KeyODO_Main", None)
            gauge = gauge or u32.FindWindowA(b"KeyODO_Gauge", None)
            time.sleep(0.1)
        time.sleep(0.4)
        out = []
        for h in (main, gauge):
            r = wt.RECT()
            u32.GetWindowRect(h, ctypes.byref(r))
            out.append((r.right - r.left, r.bottom - r.top))
        return out[0], out[1]
    finally:
        if main:
            u32.PostMessageA(main, 0x0010, 0, 0)
        try:
            p.wait(timeout=8)
        except subprocess.TimeoutExpired:
            p.kill()


def main():
    print("=== measured window sizes (real program, forced DPI) ===")
    sizes = {}
    for dpi in SCALES:
        w, g = measure(dpi)
        sizes[dpi] = (w, g)
        print("  %3d dpi (%3d%%)   main %-9s gauge %s"
              % (dpi, round(dpi * 100 / 96), "%dx%d" % w, "%dx%d" % g))

    print("\n=== fit on each display ===")
    print("  %-11s %-6s %-5s %-10s %-10s %-9s %s"
          % ("display", "aspect", "dpi", "main", "gauge", "work area", "verdict"))
    for name, sw, sh, aspect, dpi in DISPLAYS:
        (mw, mh), (gw, gh) = sizes[dpi]
        area = (sw, sh - TASKBAR)
        fits_main = mw <= area[0] and mh <= area[1]
        fits_gauge = gw <= area[0] and gh <= area[1]
        # the window must also leave a sensible margin, not fill the screen
        roomy = mw <= area[0] * 0.9 and mh <= area[1] * 0.9
        ok = fits_main and fits_gauge and roomy
        if not ok:
            failures.append("%s@%d%%" % (name, round(dpi * 100 / 96)))
        print("  %-11s %-6s %-5s %-10s %-10s %-9s %s"
              % (name, aspect, "%d%%" % round(dpi * 100 / 96),
                 "%dx%d" % (mw, mh), "%dx%d" % (gw, gh),
                 "%dx%d" % area, "OK" if ok else "DOES NOT FIT"))

    print()
    if failures:
        print("FAIL: %s" % ", ".join(failures))
        return 1
    print("PASS: fits every supported resolution, scaling and aspect ratio")
    return 0


if __name__ == "__main__":
    sys.exit(main())
