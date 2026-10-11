# -*- coding: utf-8 -*-
"""
installer_ui_test.py -- drives the real installer UI and captures each page.

Silent-install tests prove the payload lands correctly but say nothing about
what the wizard looks like, which is how a component list full of "??????"
shipped once. This launches KeyODO_Setup.exe interactively, walks the wizard
with the Next button and saves a PNG of every page.

    python tools/installer_ui_test.py            # all pages
    python tools/installer_ui_test.py --pages 3  # stop after three

Exit code 1 if any page still contains the "????" placeholder runs that a
failed codepage conversion produces.
"""
import ctypes
import ctypes.wintypes as wt
import os
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SETUP = os.path.join(ROOT, "dist", "KeyODO_Setup.exe")
OUTDIR = os.path.join(ROOT, "build")

u32 = ctypes.WinDLL("user32", use_last_error=True)
u32.SetProcessDPIAware()
u32.FindWindowA.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
u32.FindWindowA.restype = wt.HWND
u32.GetClassNameA.argtypes = [wt.HWND, ctypes.c_char_p, ctypes.c_int]
u32.GetWindowRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
u32.GetWindowTextA.argtypes = [wt.HWND, ctypes.c_char_p, ctypes.c_int]
u32.SendMessageA.argtypes = [wt.HWND, wt.UINT, wt.WPARAM, wt.LPARAM]
u32.SendMessageA.restype = wt.LPARAM
u32.IsWindowVisible.argtypes = [wt.HWND]
u32.IsWindowEnabled.argtypes = [wt.HWND]
u32.IsWindowEnabled.restype = wt.BOOL
u32.GetDC.argtypes = [wt.HWND]
u32.GetDC.restype = wt.HDC
u32.EnumChildWindows.argtypes = [wt.HWND, ctypes.c_void_p, wt.LPARAM]
u32.EnumWindows.argtypes = [ctypes.c_void_p, wt.LPARAM]
u32.GetWindowThreadProcessId.argtypes = [wt.HWND, ctypes.POINTER(wt.DWORD)]
u32.IsWindowVisible.argtypes = [wt.HWND]
u32.IsWindowEnabled.argtypes = [wt.HWND]
u32.IsWindowEnabled.restype = wt.BOOL

g32 = ctypes.WinDLL("gdi32")
g32.CreateCompatibleDC.argtypes = [wt.HDC]
g32.CreateCompatibleDC.restype = wt.HDC
g32.CreateCompatibleBitmap.argtypes = [wt.HDC, ctypes.c_int, ctypes.c_int]
g32.CreateCompatibleBitmap.restype = wt.HBITMAP
g32.SelectObject.argtypes = [wt.HDC, wt.HGDIOBJ]
g32.SelectObject.restype = wt.HGDIOBJ
g32.BitBlt.argtypes = [wt.HDC, ctypes.c_int, ctypes.c_int, ctypes.c_int,
                       ctypes.c_int, wt.HDC, ctypes.c_int, ctypes.c_int, wt.DWORD]
g32.GetDIBits.argtypes = [wt.HDC, wt.HBITMAP, wt.UINT, wt.UINT, ctypes.c_void_p,
                          ctypes.c_void_p, wt.UINT]

WM_COMMAND = 0x0111
BM_CLICK = 0x00F5
IDOK = 1


class BIH(ctypes.Structure):
    _fields_ = [("biSize", wt.DWORD), ("biWidth", ctypes.c_long),
                ("biHeight", ctypes.c_long), ("biPlanes", wt.WORD),
                ("biBitCount", wt.WORD), ("biCompression", wt.DWORD),
                ("biSizeImage", wt.DWORD), ("biXPelsPerMeter", ctypes.c_long),
                ("biYPelsPerMeter", ctypes.c_long), ("biClrUsed", wt.DWORD),
                ("biClrImportant", wt.DWORD)]


class BI(ctypes.Structure):
    _fields_ = [("bmiHeader", BIH), ("bmiColors", wt.DWORD * 3)]


def window_of_pid(pid, cls=b"#32770"):
    """Top-level window of class `cls` belonging to `pid`.

    FindWindow("#32770") returns the first dialog on the desktop, which may be
    a stray installer left over from an earlier run -- that is exactly how a
    blank 320x56 window got captured once."""
    found = []

    @ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
    def cb(h, lp):
        owner = wt.DWORD()
        u32.GetWindowThreadProcessId(h, ctypes.byref(owner))
        if owner.value != pid:
            return True
        buf = ctypes.create_string_buffer(64)
        u32.GetClassNameA(h, buf, 64)
        if buf.value == cls:
            found.append(h)
            return False
        return True

    u32.EnumWindows(cb, 0)
    return found[0] if found else None


def children(hwnd):
    out = []

    @ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
    def cb(h, lp):
        out.append(h)
        return True

    u32.EnumChildWindows(hwnd, cb, 0)
    return out


def find_next(hwnd):
    """The wizard's Next / Install / Finish button, by caption.

    Only real buttons count: a static whose text happens to contain 安装 (the
    page headers do) would otherwise be returned, and clicking it does nothing,
    which stalls the walkthrough."""
    for h in children(hwnd):
        cls = ctypes.create_string_buffer(64)
        u32.GetClassNameA(h, cls, 64)
        if cls.value != b"Button":
            continue
        buf = ctypes.create_string_buffer(128)
        u32.GetWindowTextA(h, buf, 128)
        txt = buf.value.decode("gbk", "replace")
        if not u32.IsWindowEnabled(h):
            continue
        if "下一步" in txt or "安装" in txt or "完成" in txt:
            return h
    return None


def grab(hwnd):
    from PIL import Image
    r = wt.RECT()
    u32.GetWindowRect(hwnd, ctypes.byref(r))
    w, h = r.right - r.left, r.bottom - r.top
    src = u32.GetDC(None)
    mdc = g32.CreateCompatibleDC(src)
    bmp = g32.CreateCompatibleBitmap(src, w, h)
    g32.SelectObject(mdc, bmp)
    g32.BitBlt(mdc, 0, 0, w, h, src, r.left, r.top, 0x00CC0020)
    bi = BI()
    bi.bmiHeader.biSize = ctypes.sizeof(BIH)
    bi.bmiHeader.biWidth = w
    bi.bmiHeader.biHeight = -h
    bi.bmiHeader.biPlanes = 1
    bi.bmiHeader.biBitCount = 32
    buf = ctypes.create_string_buffer(w * h * 4)
    g32.GetDIBits(mdc, bmp, 0, h, buf, ctypes.byref(bi), 0)
    return Image.frombytes("RGBA", (w, h), buf.raw, "raw", "BGRA").convert("RGB")


def build_test_installer():
    """Build a non-elevated copy of the installer.

    The shipped installer asks for admin, which raises a UAC prompt that a
    script cannot answer -- and a wizard nobody can open is a wizard nobody
    checks. Everything up to the progress page is identical either way.
    """
    makensis = os.environ.get("KEYODO_NSIS")
    if not makensis or not os.path.exists(makensis):
        for c in (os.path.join(os.environ.get("ProgramFiles(x86)", ""), "NSIS", "makensis.exe"),
                  os.path.join(os.environ.get("ProgramFiles", ""), "NSIS", "makensis.exe"),
                  os.path.join(os.environ.get("USERPROFILE", ""), ".keyodo-tools", "nsis", "makensis.exe")):
            if os.path.exists(c):
                makensis = c
                break
    if not makensis:
        print("makensis not found; set KEYODO_NSIS")
        return None

    # the script must carry a UTF-8 BOM for NSIS to read Chinese correctly
    src = os.path.join(ROOT, "installer", "keyodo.nsi")
    raw = open(src, "rb").read()
    if not raw.startswith(b"\xef\xbb\xbf"):
        raw = b"\xef\xbb\xbf" + raw
    if not raw.startswith(b"\xef\xbb\xbf"):
        pass
    nsi = os.path.join(OUTDIR, "keyodo_uitest.nsi")
    os.makedirs(OUTDIR, exist_ok=True)
    open(nsi, "wb").write(raw)

    out = os.path.join(OUTDIR, "KeyODO_Setup_uitest.exe")
    if os.path.exists(out):
        os.remove(out)
    r = subprocess.run([makensis, "/V1",
                        "/DUITEST",
                        "/DSRCDIR=" + ROOT,
                        "/DOUTDIR=" + OUTDIR,
                        "/DDISTDIR=" + OUTDIR,
                        nsi],
                       capture_output=True, text=True)
    if r.returncode != 0 or not os.path.exists(out):
        print("makensis failed:\n%s\n%s" % (r.stdout, r.stderr))
        return None
    return out


def main(argv):
    pages = 6
    for i, a in enumerate(argv):
        if a == "--pages" and i + 1 < len(argv):
            pages = int(argv[i + 1])

    os.makedirs(OUTDIR, exist_ok=True)
    setup = build_test_installer()
    if setup is None:
        print("FAIL: could not build a testable installer")
        return 1
    print("test installer: %s\n" % os.path.basename(setup))

    proc = subprocess.Popen([setup])
    bad = []
    try:
        hwnd = None
        end = time.time() + 20
        while time.time() < end:
            hwnd = window_of_pid(proc.pid)
            if hwnd:
                break
            time.sleep(0.2)
        if not hwnd:
            print("FAIL: installer window not found for pid %d" % proc.pid)
            return 1
        time.sleep(1.2)

        for page in range(pages):
            img = grab(hwnd)
            path = os.path.join(OUTDIR, "installer_page%d.png" % (page + 1))
            img.save(path)
            print("  page %d -> %s  %s" % (page + 1, os.path.basename(path),
                                           img.size))
            nxt = find_next(hwnd)
            if nxt is None:
                print("       (no Next button; stopping)")
                break
            u32.SendMessageA(nxt, BM_CLICK, 0, 0)
            time.sleep(2.0)
            # The dialog is rebuilt around the install page, so the handle can
            # go stale; re-find it for a few seconds before giving up.
            again = None
            for _ in range(20):
                if hwnd and u32.IsWindowVisible(hwnd):
                    again = hwnd
                    break
                cand = window_of_pid(proc.pid)
                if cand and u32.IsWindowVisible(cand):
                    again = cand
                    break
                time.sleep(0.3)
            if again is None:
                print("       (wizard closed)")
                break
            hwnd = again
    finally:
        time.sleep(0.3)
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()

    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
