# -*- coding: utf-8 -*-
"""
probe_rawinput.py -- does Raw Input see SendInput-generated keystrokes here?

This exists to separate two possibilities when KeyODO's count stays at zero:
  (a) the program's Raw Input code is wrong, or
  (b) this session cannot deliver injected keystrokes to Raw Input at all.

It builds its own window, registers Raw Input with RIDEV_INPUTSINK exactly the
way KeyODO does, injects one keystroke, pumps messages, and reports how many
WM_INPUT messages arrived.
"""
import ctypes
import ctypes.wintypes as wt
import sys
import time

u32 = ctypes.WinDLL("user32", use_last_error=True)
k32 = ctypes.WinDLL("kernel32", use_last_error=True)

try:
    u32.SetProcessDPIAware()
except Exception:
    pass

WM_INPUT = 0x00FF
WM_DESTROY = 0x0002
RID_INPUT = 0x10000003
RIDEV_INPUTSINK = 0x00000100

LRESULT = ctypes.c_ssize_t


class WNDCLASSA(ctypes.Structure):
    _fields_ = [("style", wt.UINT), ("lpfnWndProc", ctypes.c_void_p),
                ("cbClsExtra", ctypes.c_int), ("cbWndExtra", ctypes.c_int),
                ("hInstance", wt.HINSTANCE), ("hIcon", wt.HANDLE),
                ("hCursor", wt.HANDLE), ("hbrBackground", wt.HANDLE),
                ("lpszMenuName", wt.LPCSTR), ("lpszClassName", wt.LPCSTR)]


class RAWINPUTDEVICE(ctypes.Structure):
    _fields_ = [("usUsagePage", wt.USHORT), ("usUsage", wt.USHORT),
                ("dwFlags", wt.DWORD), ("hwndTarget", wt.HWND)]


class RAWINPUTHEADER(ctypes.Structure):
    _fields_ = [("dwType", wt.DWORD), ("dwSize", wt.DWORD),
                ("hDevice", wt.HANDLE), ("wParam", wt.WPARAM)]


class KEYBDINPUT(ctypes.Structure):
    _fields_ = [("wVk", wt.WORD), ("wScan", wt.WORD), ("dwFlags", wt.DWORD),
                ("time", wt.DWORD), ("dwExtraInfo", ctypes.POINTER(wt.ULONG))]


class _U(ctypes.Union):
    _fields_ = [("ki", KEYBDINPUT), ("padding", ctypes.c_byte * 32)]


class INPUT(ctypes.Structure):
    _fields_ = [("type", wt.DWORD), ("u", _U)]


WNDPROC = ctypes.WINFUNCTYPE(LRESULT, wt.HWND, wt.UINT, wt.WPARAM, wt.LPARAM)

counts = {"input": 0, "keys": 0}


def wndproc(hwnd, msg, wp, lp):
    if msg == WM_INPUT:
        counts["input"] += 1
        size = wt.UINT(0)
        u32.GetRawInputData(wt.HANDLE(lp), RID_INPUT, None, ctypes.byref(size),
                            ctypes.sizeof(RAWINPUTHEADER))
        buf = ctypes.create_string_buffer(size.value if size.value else 64)
        got = u32.GetRawInputData(wt.HANDLE(lp), RID_INPUT, buf, ctypes.byref(size),
                                  ctypes.sizeof(RAWINPUTHEADER))
        if got == size.value and size.value >= ctypes.sizeof(RAWINPUTHEADER):
            kb = ctypes.cast(ctypes.byref(buf, ctypes.sizeof(RAWINPUTHEADER)),
                             ctypes.POINTER(KEYBDINPUT))
            counts["keys"] += 1
            print("   WM_INPUT: vk=0x%02X flags=0x%04X" % (kb.contents.wVk,
                                                          kb.contents.dwFlags))
        return 0
    if msg == WM_DESTROY:
        return 0
    return u32.DefWindowProcA(hwnd, msg, wp, lp)


def main():
    hinst = k32.GetModuleHandleA(None)
    proc = WNDPROC(wndproc)

    wc = WNDCLASSA()
    wc.lpfnWndProc = ctypes.cast(proc, ctypes.c_void_p)
    wc.hInstance = hinst
    wc.lpszClassName = b"KeyODO_RawProbe"
    if not u32.RegisterClassA(ctypes.byref(wc)):
        print("RegisterClass failed", ctypes.get_last_error())
        return 1

    hwnd = u32.CreateWindowExA(0, b"KeyODO_RawProbe", b"probe", 0,
                               0, 0, 100, 100, None, None, hinst, None)
    if not hwnd:
        print("CreateWindowEx failed", ctypes.get_last_error())
        return 1
    print("probe window =", hwnd)

    rid = RAWINPUTDEVICE()
    rid.usUsagePage = 0x01
    rid.usUsage = 0x06
    rid.dwFlags = RIDEV_INPUTSINK
    rid.hwndTarget = hwnd
    ok = u32.RegisterRawInputDevices(ctypes.byref(rid), 1, ctypes.sizeof(rid))
    print("RegisterRawInputDevices ->", ok, "err", ctypes.get_last_error())
    if not ok:
        return 1

    u32.SendInput.argtypes = [wt.UINT, ctypes.POINTER(INPUT), ctypes.c_int]
    seq = (INPUT * 2)()
    for i, fl in ((0, 0), (1, 2)):
        seq[i].type = 1
        seq[i].u.ki.wVk = 0x7C          # VK_F13
        seq[i].u.ki.dwFlags = fl
    sent = u32.SendInput(2, seq, ctypes.sizeof(INPUT))
    print("SendInput ->", sent, "sizeof(INPUT)", ctypes.sizeof(INPUT))

    msg = wt.MSG()
    end = time.time() + 2.0
    while time.time() < end:
        while u32.PeekMessageA(ctypes.byref(msg), None, 0, 0, 1):
            u32.TranslateMessage(ctypes.byref(msg))
            u32.DispatchMessageA(ctypes.byref(msg))
        time.sleep(0.02)

    print("WM_INPUT received = %d, keyboard events = %d"
          % (counts["input"], counts["keys"]))
    u32.DestroyWindow(hwnd)
    return 0 if counts["keys"] > 0 else 1


if __name__ == "__main__":
    sys.exit(main())
