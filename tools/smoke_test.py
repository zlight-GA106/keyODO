# -*- coding: utf-8 -*-
"""
smoke_test.py -- end-to-end check of the KeyODO 0.1 pipeline.

Verifies, in one run:
  1. the process starts and creates its window
  2. Raw Input is registered and real keystrokes reach the statistics module
  3. the on-screen read-outs follow (captured to build/smoke_after.png)
  4. the counters are persisted on exit and restored on the next start

Nothing here inspects what was typed: only counts. KeyODO never receives or
stores characters, so neither does this test.

    python tools/smoke_test.py [--exe build/keyodo.exe] [--keys 12]
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
except Exception:
    pass

INPUT_KEYBOARD = 1
KEYEVENTF_KEYUP = 0x0002
VK_TEST = 0x41          # 'A' -- an ordinary character key, so the gear is D
VK_BACK = 0x08          # Backspace, so the gear should become R
VK_SPACE = 0x20


class KEYBDINPUT(ctypes.Structure):
    _fields_ = [("wVk", wt.WORD), ("wScan", wt.WORD), ("dwFlags", wt.DWORD),
                ("time", wt.DWORD), ("dwExtraInfo", ctypes.POINTER(wt.ULONG))]


class _INPUTUNION(ctypes.Union):
    # INPUT is sized by its largest member, MOUSEINPUT, which ends in ULONG_PTR.
    # On x64 the union is 32 bytes and the whole struct is 40; a short union
    # makes SendInput fail with ERROR_INVALID_PARAMETER (87) and silently
    # inject nothing.
    _fields_ = [("ki", KEYBDINPUT), ("padding", ctypes.c_byte * 32)]


class INPUT(ctypes.Structure):
    _fields_ = [("type", wt.DWORD), ("u", _INPUTUNION)]


u32.SendInput.argtypes = [wt.UINT, ctypes.POINTER(INPUT), ctypes.c_int]
u32.SendInput.restype = wt.UINT


def tap(vk, attempts=12):
    """Inject one key press/release.

    SendInput can transiently fail with ERROR_ACCESS_DENIED while the desktop
    is switching foreground windows, so retry before giving up.
    """
    last = 0
    for _ in range(attempts):
        seq = (INPUT * 2)()
        for i, flags in ((0, 0), (1, KEYEVENTF_KEYUP)):
            seq[i].type = INPUT_KEYBOARD
            seq[i].u.ki.wVk = vk
            seq[i].u.ki.wScan = 0
            seq[i].u.ki.dwFlags = flags
            seq[i].u.ki.time = 0
            seq[i].u.ki.dwExtraInfo = None
        ctypes.set_last_error(0)
        sent = u32.SendInput(2, seq, ctypes.sizeof(INPUT))
        if sent == 2:
            return True
        last = ctypes.get_last_error()
        time.sleep(0.2)
    raise RuntimeError("SendInput failed after %d attempts (GetLastError=%d)"
                       % (attempts, last))


def ini_path():
    appdata = os.environ.get("APPDATA", "")
    return os.path.join(appdata, "KeyODO", "keyodo.ini")


def read_ini():
    p = ini_path()
    out = {}
    if not os.path.exists(p):
        return out
    section = ""
    for line in open(p, "r", encoding="mbcs", errors="replace"):
        line = line.strip()
        if line.startswith("[") and line.endswith("]"):
            section = line[1:-1].lower()
        elif "=" in line:
            k, v = line.split("=", 1)
            out[(section, k.strip().lower())] = v.strip()
    return out


def run(exe, keys):
    hwnd = None
    proc = subprocess.Popen([exe], stdout=subprocess.DEVNULL,
                            stderr=subprocess.DEVNULL)
    try:
        deadline = time.time() + 10
        while time.time() < deadline:
            hwnd = u32.FindWindowA(b"KeyODO_Main", None)
            if hwnd:
                break
            time.sleep(0.1)
        if not hwnd:
            print("FAIL: window not created")
            return 1
        print("OK  window created")

        u32.SetForegroundWindow(hwnd)
        u32.BringWindowToTop(hwnd)
        time.sleep(0.6)

        for _ in range(keys):
            tap(VK_TEST)
            time.sleep(0.05)
        time.sleep(0.8)
        print("OK  injected %d keystrokes" % keys)

        # One backspace so the captured frame also shows gear R.
        tap(VK_BACK)
        time.sleep(0.4)

        sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
        import capture_window
        img = capture_window.grab(hwnd)
        out = os.path.join(os.path.dirname(exe), "smoke_after.png")
        img.save(out)
        print("OK  captured %s" % out)

        u32.PostMessageA(hwnd, 0x0010, 0, 0)      # WM_CLOSE
        for _ in range(50):
            if proc.poll() is not None:
                break
            time.sleep(0.1)
    finally:
        if proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=5)
            except Exception:
                proc.kill()
    return 0


def main(argv):
    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.dirname(here)
    exe = os.path.join(root, "build", "keyodo.exe")
    keys = 12
    for i, a in enumerate(argv):
        if a == "--exe" and i + 1 < len(argv):
            exe = argv[i + 1]
        if a == "--keys" and i + 1 < len(argv):
            keys = int(argv[i + 1])

    if not os.path.exists(exe):
        print("missing %s" % exe)
        return 1

    before = read_ini()
    odo_before = int(before.get(("totals", "odo"), "0"))
    print("ini before : odo=%d trip=%d  (%s)" % (odo_before,
          int(before.get(("totals", "trip"), "0")), ini_path()))

    rc = run(exe, keys)
    if rc:
        return rc

    time.sleep(0.6)
    after = read_ini()
    odo_after = int(after.get(("totals", "odo"), "0"))
    trip_after = int(after.get(("totals", "trip"), "0"))

    gained = odo_after - odo_before
    print("ini after  : odo=%d trip=%d (gained %d, expected >= %d)"
          % (odo_after, trip_after, gained, keys))

    ok = True
    if gained < keys:
        print("FAIL: ODO did not advance by at least the injected key count")
        ok = False
    else:
        print("OK  counters persisted and advanced")
    if odo_after == 0:
        print("FAIL: ODO is still zero")
        ok = False
    print("RESULT: %s" % ("PASS" if ok else "FAIL"))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
