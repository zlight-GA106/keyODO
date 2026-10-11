# -*- coding: utf-8 -*-
"""
widget_test.py -- end-to-end check of the detached desktop gauge.

Verifies that
  * a hidden gauge stays hidden,
  * a shown gauge appears at the saved position,
  * the gauge is exactly the dial panel (nothing else),
  * the gauge survives an exit and its position and visibility round-trip
    through %APPDATA%\\KeyODO\\keyodo.ini,
  * the main window and the gauge can both be present at once.

    python tools/widget_test.py
"""
import ctypes
import ctypes.wintypes as wt
import os
import re
import subprocess
import sys
import time

u32 = ctypes.WinDLL("user32", use_last_error=True)

# KeyODO is DPI aware, so it places windows in physical pixels. Without this the
# test process gets virtualised coordinates and every rectangle it reads or
# writes would be off by the scaling factor (2x on a 200% display).
try:
    ctypes.WinDLL("user32").SetProcessDPIAware()
except AttributeError:
    pass

u32.FindWindowA.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
u32.FindWindowA.restype = wt.HWND
u32.GetWindowRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
u32.IsWindowVisible.argtypes = [wt.HWND]
u32.PostMessageA.argtypes = [wt.HWND, wt.UINT, wt.WPARAM, wt.LPARAM]
u32.SetWindowPos.argtypes = [wt.HWND, wt.HWND, ctypes.c_int, ctypes.c_int,
                             ctypes.c_int, ctypes.c_int, wt.UINT]
u32.SetWindowPos.restype = wt.BOOL

INI = os.path.join(os.environ["APPDATA"], "KeyODO", "keyodo.ini")
EXE = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                   "build", "keyodo.exe")

failures = []


def check(name, cond, detail=""):
    print("  %-4s %s%s" % ("OK" if cond else "FAIL", name,
                           ("  -- " + detail) if detail else ""))
    if not cond:
        failures.append(name)


def read_ini():
    if not os.path.exists(INI):
        return {}
    txt = open(INI, encoding="gbk", errors="replace").read()
    out = {}
    for sec, body in re.findall(r"\[(\w+)\]\s*(.*?)(?=\n\[|\Z)", txt, re.S):
        for k, v in re.findall(r"(\w+)=(.*)", body):
            out["%s.%s" % (sec, k)] = v.strip()
    return out


def write_widget(shown, x=None, y=None):
    txt = open(INI, encoding="gbk").read() if os.path.exists(INI) else ""
    body = "[Widget]\nShown=%d\n" % shown
    if x is not None:
        body += "X=%d\nY=%d\n" % (x, y)
    if "[Widget]" in txt:
        txt = re.sub(r"\[Widget\][^\[]*", body, txt)
    else:
        txt = txt.rstrip() + "\n\n" + body
    os.makedirs(os.path.dirname(INI), exist_ok=True)
    open(INI, "w", encoding="gbk").write(txt)


def launch():
    return subprocess.Popen([EXE], stdout=subprocess.DEVNULL,
                            stderr=subprocess.DEVNULL)


def wait_window(cls, seconds=8):
    end = time.time() + seconds
    while time.time() < end:
        h = u32.FindWindowA(cls.encode(), None)
        if h:
            return h
        time.sleep(0.1)
    return None


def rect(h):
    r = wt.RECT()
    u32.GetWindowRect(h, ctypes.byref(r))
    return r.left, r.top, r.right - r.left, r.bottom - r.top


def close(h):
    u32.PostMessageA(h, 0x0010, 0, 0)   # WM_CLOSE
    time.sleep(0.8)


def system_dpi():
    g32 = ctypes.WinDLL("gdi32")
    g32.GetDeviceCaps.argtypes = [wt.HDC, ctypes.c_int]
    hdc = u32.GetDC(None)
    dpi = g32.GetDeviceCaps(hdc, 88)          # LOGPIXELSX
    u32.ReleaseDC(None, hdc)
    return dpi if 48 <= dpi <= 480 else 96


def main():
    dpi = system_dpi()
    dial = round(184 * dpi / 96)
    print("=== desktop gauge ===  (display %d dpi, dial panel %dx%d)"
          % (dpi, dial, dial))

    # 1. hidden by default
    write_widget(0)
    p = launch()
    main_w = wait_window("KeyODO_Main")
    check("main window appears", bool(main_w))
    time.sleep(0.6)
    g = u32.FindWindowA(b"KeyODO_Gauge", None)
    check("gauge window exists", bool(g))
    check("gauge hidden when Widget\\Shown=0",
          not u32.IsWindowVisible(g) if g else False)
    close(main_w)
    p.wait(timeout=10)

    # 2. shown at a saved position
    write_widget(1, 140, 160)
    p = launch()
    main_w = wait_window("KeyODO_Main")
    g = wait_window("KeyODO_Gauge")
    check("gauge window appears", bool(g))
    time.sleep(0.6)
    check("gauge visible when Widget\\Shown=1", u32.IsWindowVisible(g))
    x, y, w, h = rect(g)
    check("gauge is exactly the dial panel", (w, h) == (dial, dial),
          "got %dx%d" % (w, h))
    check("gauge restored to the saved position", (x, y) == (140, 160),
          "got (%d,%d)" % (x, y))
    check("main window and gauge coexist", u32.IsWindowVisible(main_w))

    # 3. move it, then confirm the position is written back on exit
    u32.SetWindowPos(g, None, 300, 220, 0, 0, 0x0001 | 0x0004 | 0x0010)
    time.sleep(0.5)
    close(main_w)
    p.wait(timeout=10)
    ini = read_ini()
    check("Widget\\Shown persisted", ini.get("Widget.Shown") == "1",
          "got %r" % ini.get("Widget.Shown"))
    check("Widget\\X persisted", ini.get("Widget.X") == "300",
          "got %r" % ini.get("Widget.X"))
    check("Widget\\Y persisted", ini.get("Widget.Y") == "220",
          "got %r" % ini.get("Widget.Y"))

    # 4. off-screen positions are pulled back onto the work area
    write_widget(1, -9000, -9000)
    p = launch()
    wait_window("KeyODO_Main")
    g = wait_window("KeyODO_Gauge")
    time.sleep(0.7)
    x, y, w, h = rect(g)
    check("gauge clamped into the work area", x >= 0 and y >= 0,
          "got (%d,%d)" % (x, y))
    print("       work-area clamp moved it to (%d,%d)" % (x, y))
    close(u32.FindWindowA(b"KeyODO_Main", None))
    p.wait(timeout=10)

    print()
    if failures:
        print("FAIL: %s" % ", ".join(failures))
        return 1
    print("PASS: desktop gauge verified")
    return 0


if __name__ == "__main__":
    sys.exit(main())
