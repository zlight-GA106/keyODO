# -*- coding: utf-8 -*-
"""
capture_window.py -- launch KeyODO, grab its window, save a PNG, close it.

Used to verify that the running program matches the reference render in
mockup/out/. The window paints itself with GDI in WM_PAINT, so the capture is
taken from the window DC rather than through PrintWindow (which would need a
WM_PRINTCLIENT handler).

    python tools/capture_window.py [--exe build/keyodo.exe] [--out build/capture.png]
                                   [--keep] [--seconds 1.5]
"""
import ctypes
import ctypes.wintypes as wt
import os
import subprocess
import sys
import time

from PIL import Image

u32 = ctypes.WinDLL("user32", use_last_error=True)
g32 = ctypes.WinDLL("gdi32", use_last_error=True)

# Without this the capturing process is DPI-virtualised on a scaled display and
# GetWindowRect reports half the real size (a 480x320 window reads as 240x160
# at 200% scaling). KeyODO itself is DPI aware, so the capture must be too.
try:
    u32.SetProcessDPIAware()
except Exception:
    pass

u32.FindWindowA.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
u32.FindWindowA.restype = wt.HWND
u32.GetWindowRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
u32.GetWindowDC.argtypes = [wt.HWND]
u32.GetWindowDC.restype = wt.HDC
u32.ReleaseDC.argtypes = [wt.HWND, wt.HDC]
g32.CreateCompatibleDC.argtypes = [wt.HDC]
g32.CreateCompatibleDC.restype = wt.HDC
g32.CreateCompatibleBitmap.argtypes = [wt.HDC, ctypes.c_int, ctypes.c_int]
g32.CreateCompatibleBitmap.restype = wt.HBITMAP
g32.SelectObject.argtypes = [wt.HDC, wt.HGDIOBJ]
g32.SelectObject.restype = wt.HGDIOBJ
g32.DeleteObject.argtypes = [wt.HGDIOBJ]
g32.DeleteDC.argtypes = [wt.HDC]


class BITMAPINFOHEADER(ctypes.Structure):
    _fields_ = [("biSize", wt.DWORD), ("biWidth", wt.LONG), ("biHeight", wt.LONG),
                ("biPlanes", wt.WORD), ("biBitCount", wt.WORD),
                ("biCompression", wt.DWORD), ("biSizeImage", wt.DWORD),
                ("biXPelsPerMeter", wt.LONG), ("biYPelsPerMeter", wt.LONG),
                ("biClrUsed", wt.DWORD), ("biClrImportant", wt.DWORD)]


class BITMAPINFO(ctypes.Structure):
    _fields_ = [("bmiHeader", BITMAPINFOHEADER), ("bmiColors", wt.DWORD * 3)]


g32.GetDIBits.argtypes = [wt.HDC, wt.HBITMAP, wt.UINT, wt.UINT,
                          ctypes.c_void_p, ctypes.POINTER(BITMAPINFO), wt.UINT]

# Every handle-taking call needs explicit argtypes: without them ctypes assumes
# C int and overflows on 64-bit handles.
g32.BitBlt.argtypes = [wt.HDC, ctypes.c_int, ctypes.c_int, ctypes.c_int,
                       ctypes.c_int, wt.HDC, ctypes.c_int, ctypes.c_int, wt.DWORD]
g32.BitBlt.restype = wt.BOOL
u32.GetDC.argtypes = [wt.HWND]
u32.GetDC.restype = wt.HDC
u32.ReleaseDC.argtypes = [wt.HWND, wt.HDC]
u32.ReleaseDC.restype = ctypes.c_int
u32.SetForegroundWindow.argtypes = [wt.HWND]
u32.BringWindowToTop.argtypes = [wt.HWND]
u32.UpdateWindow.argtypes = [wt.HWND]
u32.GetClientRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]


def grab(hwnd):
    rc = wt.RECT()
    u32.GetWindowRect(hwnd, ctypes.byref(rc))
    w = rc.right - rc.left
    h = rc.bottom - rc.top

    wdc = u32.GetWindowDC(hwnd)
    mdc = g32.CreateCompatibleDC(wdc)
    bmp = g32.CreateCompatibleBitmap(wdc, w, h)
    old = g32.SelectObject(mdc, bmp)

    g32.BitBlt(mdc, 0, 0, w, h, wdc, 0, 0, 0x00CC0020)   # SRCCOPY

    bmi = BITMAPINFO()
    bmi.bmiHeader.biSize = ctypes.sizeof(BITMAPINFOHEADER)
    bmi.bmiHeader.biWidth = w
    bmi.bmiHeader.biHeight = -h
    bmi.bmiHeader.biPlanes = 1
    bmi.bmiHeader.biBitCount = 32
    bmi.bmiHeader.biCompression = 0
    buf = ctypes.create_string_buffer(w * h * 4)
    g32.GetDIBits(mdc, bmp, 0, h, buf, ctypes.byref(bmi), 0)

    img = Image.frombuffer("RGBA", (w, h), buf, "raw", "BGRA", 0, 1).convert("RGB")

    g32.SelectObject(mdc, old)
    g32.DeleteObject(bmp)
    g32.DeleteDC(mdc)
    u32.ReleaseDC(hwnd, wdc)
    return img


def main(argv):
    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.dirname(here)

    exe = os.path.join(root, "build", "keyodo.exe")
    out = os.path.join(root, "build", "capture.png")
    keep = "--keep" in argv
    seconds = 1.5
    exe_args = []
    wnd_class = "KeyODO_Main"
    for i, a in enumerate(argv):
        if a == "--exe" and i + 1 < len(argv):
            exe = argv[i + 1]
        if a == "--out" and i + 1 < len(argv):
            out = argv[i + 1]
        if a == "--seconds" and i + 1 < len(argv):
            seconds = float(argv[i + 1])
        if a == "--arg" and i + 1 < len(argv):
            exe_args.append(argv[i + 1])
        if a == "--widget":
            wnd_class = "KeyODO_Gauge"
        if a == "--class" and i + 1 < len(argv):
            wnd_class = argv[i + 1]
        if a.startswith("--dpi="):
            exe_args.append(a)

    if not os.path.exists(exe):
        print("missing executable: %s" % exe)
        return 1

    proc = subprocess.Popen([exe] + exe_args, stdout=subprocess.DEVNULL,
                            stderr=subprocess.DEVNULL)
    try:
        hwnd = None
        deadline = time.time() + 10
        while time.time() < deadline:
            hwnd = u32.FindWindowA(wnd_class.encode(), None)
            if hwnd:
                break
            time.sleep(0.1)
        if not hwnd:
            print("window not found")
            return 1

        u32.SetForegroundWindow(hwnd)
        u32.BringWindowToTop(hwnd)
        time.sleep(seconds)
        u32.UpdateWindow(hwnd)
        time.sleep(0.3)

        img = grab(hwnd)
        img.save(out)
        print("captured %s  %s" % (out, img.size))
    finally:
        if not keep and proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=5)
            except Exception:
                proc.kill()
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
