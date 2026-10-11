# -*- coding: utf-8 -*-
"""
menu_test.py -- drives the self-drawn menus end to end.

Everything is done with posted messages, so no cursor or visible desktop is
needed (a disconnected RDP session reads back black on a screen capture).

  * opens 查看, walks down with the arrow keys, activates 换挡显示 with Enter
    and checks the setting really flipped by reading the ini after exit
  * checks Escape dismisses without changing anything
  * captures the menu with an item highlighted

    python tools/menu_test.py
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

WM_LBUTTONDOWN, WM_LBUTTONUP = 0x0201, 0x0202
WM_KEYDOWN, WM_KEYUP = 0x0100, 0x0101
WM_CLOSE = 0x0010
VK_DOWN, VK_RETURN, VK_ESCAPE = 0x28, 0x0D, 0x1B

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


def set_gate(value):
    txt = open(INI, encoding="gbk").read() if os.path.exists(INI) else ""
    if "[View]" in txt:
        txt = re.sub(r"ShowGate=\d", "ShowGate=%d" % value, txt)
    else:
        txt = txt.rstrip() + "\n\n[View]\nShowGate=%d\n" % value
    os.makedirs(os.path.dirname(INI), exist_ok=True)
    open(INI, "w", encoding="gbk").write(txt)


def launch():
    p = subprocess.Popen([EXE], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    h = None
    for _ in range(80):
        h = u32.FindWindowA(b"KeyODO_Main", None)
        if h:
            break
        time.sleep(0.1)
    time.sleep(0.9)
    return p, h


def key(h, vk, times=1):
    for _ in range(times):
        u32.PostMessageA(h, WM_KEYDOWN, vk, 0)
        u32.PostMessageA(h, WM_KEYUP, vk, 0)
        time.sleep(0.25)


def open_view_menu(h, click_x, click_y):
    u32.PostMessageA(h, WM_LBUTTONDOWN, 1, (click_y << 16) | (click_x & 0xFFFF))
    time.sleep(0.7)
    return u32.FindWindowA(b"KeyODO_Menu", None)


def print_window(hwnd):
    from PIL import Image
    r = wt.RECT()
    u32.GetWindowRect(hwnd, ctypes.byref(r))
    w, h = r.right - r.left, r.bottom - r.top
    src = u32.GetDC(None)
    mdc = g32.CreateCompatibleDC(src)
    bmp = g32.CreateCompatibleBitmap(src, w, h)
    g32.SelectObject(mdc, bmp)
    u32.PrintWindow(hwnd, mdc, 0)
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
    bi.bmiHeader.biHeight = -h
    bi.bmiHeader.biPlanes = 1
    bi.bmiHeader.biBitCount = 32
    buf = ctypes.create_string_buffer(w * h * 4)
    g32.GetDIBits(mdc, bmp, 0, h, buf, ctypes.byref(bi), 0)
    return Image.frombytes("RGBA", (w, h), buf.raw, "raw", "BGRA").convert("RGB")


def menu_bar_click_x(i, unit):
    return int((6 + i * 66) * unit)


def main():
    os.makedirs(OUT, exist_ok=True)
    unit = 2
    view_x, hit_y = menu_bar_click_x(2, unit), 60

    print("=== menu: keyboard activation ===")
    set_gate(1)
    p, h = launch()
    menu = open_view_menu(h, view_x, hit_y)
    check("查看 opens a self-drawn popup", bool(menu),
          "class KeyODO_Menu" if menu else "not found")
    if menu:
        # 查看's selectable items are: 桌面仪表, 显示 KPM, 换挡显示, 自动挡, 手动挡, 总在最前
        key(h, VK_DOWN, 3)                       # land on 换挡显示
        img = print_window(menu)
        img.save(os.path.join(OUT, "menu_view_hot.png"))
        key(h, VK_RETURN)
        time.sleep(0.5)
        check("popup closes after Enter",
              not u32.FindWindowA(b"KeyODO_Menu", None))
    u32.PostMessageA(h, WM_CLOSE, 0, 0)
    time.sleep(1.0)
    p.wait(timeout=10)
    ini = read_ini()
    check("换挡显示 was toggled off", ini.get("View.ShowGate") == "0",
          "ShowGate=%r" % ini.get("View.ShowGate"))

    print("\n=== menu: Escape changes nothing ===")
    set_gate(1)
    p, h = launch()
    menu = open_view_menu(h, view_x, hit_y)
    if menu:
        key(h, VK_DOWN, 3)
        key(h, VK_ESCAPE)
        time.sleep(0.4)
        check("popup closes after Escape",
              not u32.FindWindowA(b"KeyODO_Menu", None))
    u32.PostMessageA(h, WM_CLOSE, 0, 0)
    time.sleep(1.0)
    p.wait(timeout=10)
    ini = read_ini()
    check("setting untouched", ini.get("View.ShowGate") == "1",
          "ShowGate=%r" % ini.get("View.ShowGate"))

    print("\n=== menu: clicking outside dismisses ===")
    p, h = launch()
    menu = open_view_menu(h, view_x, hit_y)
    if menu:
        u32.PostMessageA(h, WM_LBUTTONDOWN, 1, (500 << 16) | 900)
        time.sleep(0.5)
        check("popup closes on an outside click",
              not u32.FindWindowA(b"KeyODO_Menu", None))
    u32.PostMessageA(h, WM_CLOSE, 0, 0)
    time.sleep(1.0)
    p.wait(timeout=10)

    print()
    if failures:
        print("FAIL: %s" % ", ".join(failures))
        return 1
    print("PASS: menus verified")
    return 0


if __name__ == "__main__":
    sys.exit(main())
