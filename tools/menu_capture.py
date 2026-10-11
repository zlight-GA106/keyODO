# -*- coding: utf-8 -*-
"""
menu_capture.py -- opens each dropdown and captures it.

The menus are self-drawn, so the only meaningful check is a picture. This drives
the main window with posted mouse messages (no cursor needed) and captures the
popup window with PrintWindow, which renders the window itself rather than
reading the screen -- so it also works when the session is not displaying
anything (a disconnected RDP desktop reads back pure black).

    python tools/menu_capture.py
"""
import ctypes
import ctypes.wintypes as wt
import os
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXE = os.path.join(ROOT, "build", "keyodo.exe")
OUT = os.path.join(ROOT, "build")

u32 = ctypes.WinDLL("user32", use_last_error=True)
g32 = ctypes.WinDLL("gdi32")
u32.SetProcessDPIAware()
u32.FindWindowA.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
u32.FindWindowA.restype = wt.HWND
u32.GetWindowRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
u32.PostMessageA.argtypes = [wt.HWND, wt.UINT, wt.WPARAM, wt.LPARAM]
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

WM_LBUTTONDOWN = 0x0201
WM_LBUTTONUP = 0x0202
WM_KEYDOWN = 0x0100
WM_KEYUP = 0x0101
MK_LBUTTON = 0x0001
VK_ESCAPE = 0x1B


class BIH(ctypes.Structure):
    _fields_ = [("biSize", wt.DWORD), ("biWidth", ctypes.c_long),
                ("biHeight", ctypes.c_long), ("biPlanes", wt.WORD),
                ("biBitCount", wt.WORD), ("biCompression", wt.DWORD),
                ("biSizeImage", wt.DWORD), ("biXPelsPerMeter", ctypes.c_long),
                ("biYPelsPerMeter", ctypes.c_long), ("biClrUsed", wt.DWORD),
                ("biClrImportant", wt.DWORD)]


class BI(ctypes.Structure):
    _fields_ = [("bmiHeader", BIH), ("bmiColors", wt.DWORD * 3)]


def print_window(hwnd):
    from PIL import Image
    r = wt.RECT()
    u32.GetWindowRect(hwnd, ctypes.byref(r))
    w, h = r.right - r.left, r.bottom - r.top
    if w <= 0 or h <= 0:
        return None
    src = u32.GetDC(None)
    mdc = g32.CreateCompatibleDC(src)
    bmp = g32.CreateCompatibleBitmap(src, w, h)
    g32.SelectObject(mdc, bmp)
    u32.PrintWindow(hwnd, mdc, 0)
    bi = BI()
    bi.bmiHeader.biSize = ctypes.sizeof(BIH)
    bi.bmiHeader.biWidth = w
    bi.bmiHeader.biHeight = -h
    bi.bmiHeader.biPlanes = 1
    bi.bmiHeader.biBitCount = 32
    buf = ctypes.create_string_buffer(w * h * 4)
    g32.GetDIBits(mdc, bmp, 0, h, buf, ctypes.byref(bi), 0)
    return Image.frombytes("RGBA", (w, h), buf.raw, "raw", "BGRA").convert("RGB")


def main():
    p = subprocess.Popen([EXE], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    h = None
    try:
        for _ in range(80):
            h = u32.FindWindowA(b"KeyODO_Main", None)
            if h:
                break
            time.sleep(0.1)
        if not h:
            print("window not found")
            return 1
        time.sleep(1.0)

        # the app's client coordinates: frame 6 + caption 36 = 42 is where the
        # menu bar starts at 200% scaling, so aim well inside it
        unit = 2
        hit_y = 60
        xs = [int((6 + i * 66) * unit) for i in range(5)]
        names = ["file", "edit", "view", "stats", "help"]

        for i, name in enumerate(names):
            lp = (hit_y << 16) | (xs[i] & 0xFFFF)
            u32.PostMessageA(h, WM_LBUTTONDOWN, MK_LBUTTON, lp)
            time.sleep(0.7)
            menu = u32.FindWindowA(b"KeyODO_Menu", None)
            if not menu:
                print("  %-6s -> no menu window" % name)
                continue
            img = print_window(menu)
            if img is None:
                print("  %-6s -> capture failed" % name)
            else:
                path = os.path.join(OUT, "menu_%s.png" % name)
                img.save(path)
                nz = len(set(img.getdata()))
                print("  %-6s -> %-16s %s  distinct colours: %d"
                      % (name, os.path.basename(path), img.size, nz))
            u32.PostMessageA(h, WM_KEYDOWN, VK_ESCAPE, 0)
            u32.PostMessageA(h, WM_KEYUP, VK_ESCAPE, 0)
            time.sleep(0.5)
    finally:
        if h:
            u32.PostMessageA(h, 0x0010, 0, 0)
        time.sleep(0.8)
        p.kill()
    return 0


if __name__ == "__main__":
    sys.exit(main())
