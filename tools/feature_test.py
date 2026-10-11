# -*- coding: utf-8 -*-
"""
feature_test.py -- verifies the 0.2 desktop-integration features.

  * language switch really re-renders the window (Chinese vs English differ in
    the chrome, and the English render uses the Latin face)
  * "run at startup" writes and removes the HKCU Run entry, driven through the
    actual menu and keyboard, not by calling the API directly
  * minimising with the tray icon enabled hides the window, and the tray
    callback brings it back
  * the fit-to-screen logic shrinks the layout on a small work area and leaves
    it alone when there is room

Everything uses posted window messages, so no cursor or visible desktop is
needed.

    python tools/feature_test.py
"""
import ctypes
import ctypes.wintypes as wt
import os
import re
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXE = os.path.join(ROOT, "build", "keyodo.exe")
OUT = os.path.join(ROOT, "build")
INI = os.path.join(os.environ["APPDATA"], "KeyODO", "keyodo.ini")
RUN_KEY = r"Software\Microsoft\Windows\CurrentVersion\Run"

u32 = ctypes.WinDLL("user32", use_last_error=True)
g32 = ctypes.WinDLL("gdi32")
u32.SetProcessDPIAware()
u32.FindWindowA.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
u32.FindWindowA.restype = wt.HWND
u32.GetWindowRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
u32.PostMessageA.argtypes = [wt.HWND, wt.UINT, wt.WPARAM, wt.LPARAM]
u32.IsWindowVisible.argtypes = [wt.HWND]
u32.PrintWindow.argtypes = [wt.HWND, wt.HDC, wt.UINT]
u32.PrintWindow.restype = wt.BOOL
u32.GetDC.argtypes = [wt.HWND]
u32.GetDC.restype = wt.HDC
g32.CreateCompatibleDC.argtypes = [wt.HDC]
g32.CreateCompatibleDC.restype = wt.HDC
g32.CreateCompatibleBitmap.argtypes = [wt.HDC, ctypes.c_int, ctypes.c_int]
g32.CreateCompatibleBitmap.restype = wt.HBITMAP
g32.SelectObject.argtypes = [wt.HDC, wt.HGDIOBJ]
g32.SelectObject.restype = wt.HGDIOBJ
g32.GetDIBits.argtypes = [wt.HDC, wt.HBITMAP, wt.UINT, wt.UINT, ctypes.c_void_p,
                          ctypes.c_void_p, wt.UINT]

WM_LBUTTONDOWN, WM_LBUTTONUP = 0x0201, 0x0202
WM_KEYDOWN, WM_KEYUP = 0x0100, 0x0101
WM_CLOSE = 0x0010
UIWM_TRAY = 0x8000 + 2
VK_DOWN, VK_RETURN = 0x28, 0x0D
MK_LBUTTON = 0x0001

failures = []


def check(name, cond, detail=""):
    print("  %-4s %s%s" % ("OK" if cond else "FAIL", name,
                           ("  -- " + detail) if detail else ""))
    if not cond:
        failures.append(name)


def write_ini(settings):
    txt = open(INI, encoding="gbk").read() if os.path.exists(INI) else ""
    for sec, kv in settings.items():
        body = "[%s]\n" % sec + "".join("%s=%s\n" % (k, v) for k, v in kv.items())
        if "[%s]" % sec in txt:
            txt = re.sub(r"\[%s\][^\[]*" % sec, body, txt)
        else:
            txt = txt.rstrip() + "\n\n" + body
    os.makedirs(os.path.dirname(INI), exist_ok=True)
    open(INI, "w", encoding="gbk").write(txt)


def launch(extra_args=()):
    p = subprocess.Popen([EXE] + list(extra_args),
                         stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    h = None
    for _ in range(80):
        h = u32.FindWindowA(b"KeyODO_Main", None)
        if h:
            break
        time.sleep(0.1)
    time.sleep(0.9)
    return p, h


def stop(p, h):
    u32.PostMessageA(h, WM_CLOSE, 0, 0)
    time.sleep(1.0)
    try:
        p.wait(timeout=10)
    except subprocess.TimeoutExpired:
        p.kill()


def shot(h):
    from PIL import Image
    r = wt.RECT()
    u32.GetWindowRect(h, ctypes.byref(r))
    w, hh = r.right - r.left, r.bottom - r.top
    src = u32.GetDC(None)
    mdc = g32.CreateCompatibleDC(src)
    bmp = g32.CreateCompatibleBitmap(src, w, hh)
    g32.SelectObject(mdc, bmp)
    u32.PrintWindow(h, mdc, 0)

    class BIH(ctypes.Structure):
        _fields_ = [("biSize", wt.DWORD), ("biWidth", ctypes.c_long),
                    ("biHeight", ctypes.c_long), ("biPlanes", wt.WORD),
                    ("biBitCount", wt.WORD), ("biCompression", wt.DWORD),
                    ("biSizeImage", wt.DWORD), ("biXPelsPerMeter", ctypes.c_long),
                    ("biYPelsPerMeter", ctypes.c_long), ("biClrUsed", wt.DWORD),
                    ("biClrImportant", wt.DWORD)]

    class BI(ctypes.Structure):
        _fields_ = [("bmiHeader", BIH), ("bmiColors", wt.DWORD * 3)]

    bi = BI()
    bi.bmiHeader.biSize = ctypes.sizeof(BIH)
    bi.bmiHeader.biWidth = w
    bi.bmiHeader.biHeight = -hh
    bi.bmiHeader.biPlanes = 1
    bi.bmiHeader.biBitCount = 32
    buf = ctypes.create_string_buffer(w * hh * 4)
    g32.GetDIBits(mdc, bmp, 0, hh, buf, ctypes.byref(bi), 0)
    return Image.frombytes("RGBA", (w, hh), buf.raw, "raw", "BGRA").convert("RGB")


def size_of(h):
    r = wt.RECT()
    u32.GetWindowRect(h, ctypes.byref(r))
    return (r.right - r.left, r.bottom - r.top)


def menu_click(h, index, unit=2):
    x = int((6 + index * 66) * unit)
    u32.PostMessageA(h, WM_LBUTTONDOWN, MK_LBUTTON, (60 << 16) | (x & 0xFFFF))
    time.sleep(0.6)


def key(h, vk, times=1):
    for _ in range(times):
        u32.PostMessageA(h, WM_KEYDOWN, vk, 0)
        u32.PostMessageA(h, WM_KEYUP, vk, 0)
        time.sleep(0.2)


def autostart_entry():
    import winreg
    try:
        k = winreg.OpenKey(winreg.HKEY_CURRENT_USER, RUN_KEY, 0, winreg.KEY_READ)
    except OSError:
        return None
    try:
        return winreg.QueryValueEx(k, "KeyODO")[0]
    except OSError:
        return None
    finally:
        winreg.CloseKey(k)


def main():
    os.makedirs(OUT, exist_ok=True)

    print("=== language ===")
    write_ini({"View": {"Language": 0}})
    p, h = launch()
    zh = shot(h)
    zh.save(os.path.join(OUT, "lang_zh.png"))
    zh_size = size_of(h)
    stop(p, h)

    write_ini({"View": {"Language": 1}})
    p, h = launch()
    en = shot(h)
    en.save(os.path.join(OUT, "lang_en.png"))
    en_size = size_of(h)
    stop(p, h)

    if zh.size == en.size:
        diff = sum(1 for a, b in zip(zh.getdata(), en.getdata()) if a != b)
    else:
        diff = -1
    total = zh.size[0] * zh.size[1]
    check("Chinese and English render differently", diff > total * 0.02,
          "%d of %d pixels differ" % (diff, total))
    check("both languages keep the same window size", zh_size == en_size,
          "%s vs %s" % (zh_size, en_size))

    print("\n=== run at startup (through the File menu) ===")
    have_before = autostart_entry()
    write_ini({"View": {"Language": 0}})
    p, h = launch()
    menu_click(h, 0)                     # File
    key(h, VK_DOWN, 1)                   # 开机自启
    key(h, VK_RETURN)
    time.sleep(0.5)
    after_on = autostart_entry()
    check("menu item writes the Run entry", after_on is not None,
          "%r" % (after_on,))
    check("entry points at this executable",
          bool(after_on) and "keyodo.exe" in after_on.lower(),
          "%r" % (after_on,))

    menu_click(h, 0)
    key(h, VK_DOWN, 1)
    key(h, VK_RETURN)
    time.sleep(0.5)
    after_off = autostart_entry()
    check("menu item removes it again", after_off is None, "%r" % (after_off,))
    stop(p, h)
    if have_before is None and autostart_entry() is not None:
        import winreg
        k = winreg.OpenKey(winreg.HKEY_CURRENT_USER, RUN_KEY, 0, winreg.KEY_SET_VALUE)
        winreg.DeleteValue(k, "KeyODO")
        winreg.CloseKey(k)

    print("\n=== tray icon and minimize ===")
    write_ini({"View": {"Language": 0, "TrayIcon": 1}})
    p, h = launch()
    check("window is visible to start with", bool(u32.IsWindowVisible(h)))

    # Caption buttons, derived from the layout rather than guessed: at 96 DPI
    # the close button ends 3 px + 2 px in from the right edge and is 16 px
    # wide, and minimise sits another 2 px + 16 px to its left.
    r = wt.RECT()
    u32.GetWindowRect(h, ctypes.byref(r))
    w = r.right - r.left
    scale = w / 480.0
    cap_w = 16.0 * scale
    close_x = w - (3.0 + 2.0) * scale - cap_w
    min_x = close_x - (2.0 + 16.0) * scale
    mx = int(min_x + cap_w / 2)
    my = int((5.0 + 8.0) * scale)
    u32.PostMessageA(h, WM_LBUTTONDOWN, MK_LBUTTON, (my << 16) | (mx & 0xFFFF))
    u32.PostMessageA(h, WM_LBUTTONUP, 0, (my << 16) | (mx & 0xFFFF))
    time.sleep(0.7)
    alive = u32.FindWindowA(b"KeyODO_Main", None)
    check("the app is still running (we hit minimise, not close)", bool(alive),
          "clicked at %d,%d of width %d" % (mx, my, w))
    check("minimising with the tray icon hides the window",
          bool(alive) and not u32.IsWindowVisible(h))
    u32.PostMessageA(h, UIWM_TRAY, 1, 0x0202)   # WM_LBUTTONUP from the icon
    time.sleep(0.6)
    check("the tray callback brings it back",
          bool(alive) and bool(u32.IsWindowVisible(h)))
    if alive:
        stop(p, h)
    else:
        p.kill()

    print("\n=== fit to screen ===")
    # 96 dpi on a big screen: no shrink
    write_ini({"View": {"Language": 0}, "Window": {"WorkWidth": 0, "WorkHeight": 0}})
    p, h = launch(["--dpi=96"])
    full = size_of(h)
    stop(p, h)

    # 800x600 at 200% would want 960x640; the layout must shrink to fit
    write_ini({"View": {"Language": 0},
               "Window": {"WorkWidth": 800, "WorkHeight": 600}})
    p, h = launch(["--dpi=192"])
    fitted = size_of(h)
    stop(p, h)

    check("without a constraint the window is full size", full == (480, 320),
          "%s" % (full,))
    check("a small work area shrinks the window",
          fitted[0] <= 800 and fitted[1] <= 600, "%s fits in 800x600" % (fitted,))
    check("the shrink is proportional",
          fitted[0] * 320 == fitted[1] * 480, "%s" % (fitted,))

    write_ini({"View": {"Language": 0},
               "Window": {"WorkWidth": 0, "WorkHeight": 0}})

    print()
    if failures:
        print("FAIL: %s" % ", ".join(failures))
        return 1
    print("PASS: language, tray, autostart and fit-to-screen verified")
    return 0


if __name__ == "__main__":
    sys.exit(main())
