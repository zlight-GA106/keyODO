# -*- coding: utf-8 -*-
"""
probe_gdi_edge.py -- Read the EXACT pixel recipe of the classic Win32 3D art
primitives (DrawEdge / DrawFrameControl / DrawStatusText) off the live GDI.

Why: KeyODO is a Windows 98 SE application drawn with Win32 + GDI. Its entire
"98 look" is produced by these three calls, not by bitmaps. Rather than trusting
memory, this probe renders each primitive into a 32bpp DIB section, reads the
pixels back, and classifies every pixel against the live system colour table.

The *structure* (which ring is which system colour index) is version-independent:
on Win98 SE the same structure resolves to the Windows Standard scheme
(ButtonFace #C0C0C0, ButtonShadow #808080, ButtonHilight #FFFFFF, 3DLight #DFDFDF,
3DDkShadow #808080). On this Win11 host the live scheme is the modern one
(ButtonFace #F0F0F0 ...), which is exactly why the probe prints the colour NAMES.

Output: reports/gdi_edge_probe.txt + reports/gdi_edge_probe.png
"""
import ctypes
import ctypes.wintypes as wt
import os

from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
OUT = os.path.join(ROOT, "reports")
os.makedirs(OUT, exist_ok=True)

u32 = ctypes.WinDLL("user32", use_last_error=True)
g32 = ctypes.WinDLL("gdi32", use_last_error=True)


class RECT(ctypes.Structure):
    _fields_ = [("left", wt.LONG), ("top", wt.LONG),
                ("right", wt.LONG), ("bottom", wt.LONG)]


class BITMAPINFOHEADER(ctypes.Structure):
    _fields_ = [("biSize", wt.DWORD), ("biWidth", wt.LONG), ("biHeight", wt.LONG),
                ("biPlanes", wt.WORD), ("biBitCount", wt.WORD),
                ("biCompression", wt.DWORD), ("biSizeImage", wt.DWORD),
                ("biXPelsPerMeter", wt.LONG), ("biYPelsPerMeter", wt.LONG),
                ("biClrUsed", wt.DWORD), ("biClrImportant", wt.DWORD)]


class BITMAPINFO(ctypes.Structure):
    _fields_ = [("bmiHeader", BITMAPINFOHEADER), ("bmiColors", wt.DWORD * 4)]


u32.DrawEdge.argtypes = [wt.HDC, ctypes.POINTER(RECT), wt.UINT, wt.UINT]
u32.DrawEdge.restype = wt.BOOL
u32.DrawFrameControl.argtypes = [wt.HDC, ctypes.POINTER(RECT), wt.UINT, wt.UINT]
u32.DrawFrameControl.restype = wt.BOOL
# DrawStatusText lives in comctl32, not user32. It may be absent from the v6
# build, so resolve it defensively.
c32 = ctypes.WinDLL("comctl32", use_last_error=True)
try:
    DrawStatusTextA = c32.DrawStatusTextA
    DrawStatusTextA.argtypes = [wt.HDC, ctypes.POINTER(RECT), ctypes.c_char_p, wt.UINT]
    DrawStatusTextA.restype = None
except AttributeError:
    DrawStatusTextA = None
u32.FillRect.argtypes = [wt.HDC, ctypes.POINTER(RECT), wt.HANDLE]
u32.GetSysColor.argtypes = [ctypes.c_int]
u32.GetSysColor.restype = wt.DWORD

# ---- the system colour table (wingdi.h indices) --------------------------
SYSCOLORS = {
    0: "COLOR_SCROLLBAR", 1: "COLOR_BACKGROUND/DESKTOP", 2: "COLOR_ACTIVECAPTION",
    3: "COLOR_INACTIVECAPTION", 4: "COLOR_MENU", 5: "COLOR_WINDOW",
    6: "COLOR_WINDOWFRAME", 7: "COLOR_MENUTEXT", 8: "COLOR_WINDOWTEXT",
    9: "COLOR_CAPTIONTEXT", 10: "COLOR_ACTIVEBORDER", 11: "COLOR_INACTIVEBORDER",
    12: "COLOR_APPWORKSPACE", 13: "COLOR_HIGHLIGHT", 14: "COLOR_HIGHLIGHTTEXT",
    15: "COLOR_BTNFACE/3DFACE", 16: "COLOR_BTNSHADOW/3DSHADOW",
    17: "COLOR_GRAYTEXT", 18: "COLOR_BTNTEXT", 19: "COLOR_INACTIVECAPTIONTEXT",
    20: "COLOR_BTNHIGHLIGHT/3DHILIGHT", 21: "COLOR_3DDKSHADOW",
    22: "COLOR_3DLIGHT", 23: "COLOR_INFOTEXT", 24: "COLOR_INFOBK",
    26: "COLOR_HOTLIGHT", 27: "COLOR_GRADIENTACTIVECAPTION",
    28: "COLOR_GRADIENTINACTIVECAPTION", 29: "COLOR_MENUHILIGHT",
    30: "COLOR_MENUBAR",
}

# ---- the Windows Standard (Win98 SE default) scheme values ---------------
WIN98_STANDARD = {
    "COLOR_SCROLLBAR": (192, 192, 192),
    "COLOR_BACKGROUND/DESKTOP": (0, 128, 128),
    "COLOR_ACTIVECAPTION": (0, 0, 128),
    "COLOR_INACTIVECAPTION": (128, 128, 128),
    "COLOR_MENU": (192, 192, 192),
    "COLOR_WINDOW": (255, 255, 255),
    "COLOR_WINDOWFRAME": (0, 0, 0),
    "COLOR_MENUTEXT": (0, 0, 0),
    "COLOR_WINDOWTEXT": (0, 0, 0),
    "COLOR_CAPTIONTEXT": (255, 255, 255),
    "COLOR_ACTIVEBORDER": (192, 192, 192),
    "COLOR_INACTIVEBORDER": (192, 192, 192),
    "COLOR_APPWORKSPACE": (128, 128, 128),
    "COLOR_HIGHLIGHT": (0, 0, 128),
    "COLOR_HIGHLIGHTTEXT": (255, 255, 255),
    "COLOR_BTNFACE/3DFACE": (192, 192, 192),
    "COLOR_BTNSHADOW/3DSHADOW": (128, 128, 128),
    "COLOR_GRAYTEXT": (128, 128, 128),
    "COLOR_BTNTEXT": (0, 0, 0),
    "COLOR_INACTIVECAPTIONTEXT": (192, 192, 192),
    "COLOR_BTNHIGHLIGHT/3DHILIGHT": (255, 255, 255),
    "COLOR_3DDKSHADOW": (128, 128, 128),
    "COLOR_3DLIGHT": (223, 223, 223),
    "COLOR_INFOTEXT": (0, 0, 0),
    "COLOR_INFOBK": (255, 255, 225),
    "COLOR_GRADIENTACTIVECAPTION": (166, 202, 240),
    "COLOR_GRADIENTINACTIVECAPTION": (192, 192, 192),
}


def bgr(rgb):
    r, g, b = rgb
    return r | (g << 8) | (b << 16)


def live_table():
    t = {}
    for idx, name in SYSCOLORS.items():
        v = u32.GetSysColor(idx)
        t[name] = ((v >> 0) & 0xFF, (v >> 8) & 0xFF, (v >> 16) & 0xFF)
    t["(background probe fill)"] = (255, 0, 255)
    return t


def make_dc(w, h):
    hdc = g32.CreateCompatibleDC(None)
    bmi = BITMAPINFO()
    bmi.bmiHeader.biSize = ctypes.sizeof(BITMAPINFOHEADER)
    bmi.bmiHeader.biWidth = w
    bmi.bmiHeader.biHeight = -h          # top-down
    bmi.bmiHeader.biPlanes = 1
    bmi.bmiHeader.biBitCount = 32
    bmi.bmiHeader.biCompression = 0      # BI_RGB
    ppv = ctypes.c_void_p()
    hbm = g32.CreateDIBSection(hdc, ctypes.byref(bmi), 0, ctypes.byref(ppv), None, 0)
    old = g32.SelectObject(hdc, hbm)
    return hdc, hbm, ppv, old


def free_dc(hdc, hbm, old):
    g32.SelectObject(hdc, old)
    g32.DeleteObject(hbm)
    g32.DeleteDC(hdc)


def read_pixels(ppv, w, h):
    g32.GdiFlush()
    buf = ctypes.string_at(ppv, w * h * 4)
    img = Image.frombuffer("RGBA", (w, h), buf, "raw", "BGRA", 0, 1)
    return img.convert("RGB")


def render(w, h, draw_fn):
    hdc, hbm, ppv, old = make_dc(w, h)
    brush = g32.CreateSolidBrush(bgr((255, 0, 255)))
    r = RECT(0, 0, w, h)
    u32.FillRect(hdc, ctypes.byref(r), brush)
    g32.DeleteObject(brush)
    draw_fn(hdc)
    img = read_pixels(ppv, w, h)
    free_dc(hdc, hbm, old)
    return img


# ---------------- primitive flag constants --------------------------------
BDR_RAISEDOUTER, BDR_SUNKENOUTER = 0x0001, 0x0002
BDR_RAISEDINNER, BDR_SUNKENINNER = 0x0004, 0x0008
EDGE_RAISED = BDR_RAISEDOUTER | BDR_RAISEDINNER
EDGE_SUNKEN = BDR_SUNKENOUTER | BDR_SUNKENINNER
EDGE_ETCHED = BDR_SUNKENOUTER | BDR_RAISEDINNER
EDGE_BUMP = BDR_RAISEDOUTER | BDR_SUNKENINNER
BF_RECT = 0x000F

DFC_CAPTION, DFC_MENU, DFC_SCROLL, DFC_BUTTON = 1, 2, 3, 4
DFCS_BUTTONCHECK, DFCS_BUTTONRADIO = 0x0000, 0x0004
DFCS_BUTTONPUSH = 0x0010
DFCS_INACTIVE, DFCS_PUSHED, DFCS_CHECKED = 0x0100, 0x0200, 0x0400
DFCS_FLAT = 0x4000
DFCS_CAPTIONCLOSE, DFCS_CAPTIONMIN, DFCS_CAPTIONMAX = 0x0000, 0x0001, 0x0002
DFCS_MENUCHECK = 0x0001
DFCS_SCROLLUP, DFCS_SCROLLDOWN, DFCS_SCROLLLEFT = 0x0000, 0x0001, 0x0002
DFCS_SCROLLRIGHT, DFCS_SCROLLCOMBOBOX, DFCS_SCROLLSIZEGRIP = 0x0003, 0x0005, 0x0008
SBT_NOBORDERS, SBT_POPOUT = 0x0100, 0x0200


def edge_flags(flags, w=22, h=22):
    def fn(hdc):
        rc = RECT(0, 0, w, h)
        u32.DrawEdge(hdc, ctypes.byref(rc), flags, BF_RECT)
    return fn


def dfc(kind, state, w, h):
    def fn(hdc):
        rc = RECT(0, 0, w, h)
        u32.DrawFrameControl(hdc, ctypes.byref(rc), kind, state)
    return fn


def status_text(text=b"", flags=0, w=60, h=22):
    def fn(hdc):
        rc = RECT(0, 0, w, h)
        if DrawStatusTextA is None:
            return
        DrawStatusTextA(hdc, ctypes.byref(rc), text, flags)
    return fn


PROBES = [
    ("DrawEdge EDGE_RAISED (BDR_RAISEDOUTER|BDR_RAISEDINNER) BF_RECT", 22, 22, edge_flags(EDGE_RAISED)),
    ("DrawEdge EDGE_SUNKEN (BDR_SUNKENOUTER|BDR_SUNKENINNER) BF_RECT", 22, 22, edge_flags(EDGE_SUNKEN)),
    ("DrawEdge EDGE_ETCHED (BDR_SUNKENOUTER|BDR_RAISEDINNER) BF_RECT", 22, 22, edge_flags(EDGE_ETCHED)),
    ("DrawEdge EDGE_BUMP (BDR_RAISEDOUTER|BDR_SUNKENINNER) BF_RECT", 22, 22, edge_flags(EDGE_BUMP)),
    ("DrawEdge BDR_SUNKENOUTER only", 22, 22, edge_flags(BDR_SUNKENOUTER)),
    ("DrawEdge BDR_SUNKENINNER only", 22, 22, edge_flags(BDR_SUNKENINNER)),
    ("DrawEdge BDR_RAISEDOUTER only", 22, 22, edge_flags(BDR_RAISEDOUTER)),
    ("DrawEdge BDR_RAISEDINNER only", 22, 22, edge_flags(BDR_RAISEDINNER)),
    ("DrawFrameControl DFC_BUTTON/DFCS_BUTTONPUSH (40x22)", 40, 22, dfc(DFC_BUTTON, DFCS_BUTTONPUSH, 40, 22)),
    ("DrawFrameControl DFC_BUTTON/DFCS_BUTTONPUSH|DFCS_PUSHED (40x22)", 40, 22, dfc(DFC_BUTTON, DFCS_BUTTONPUSH | DFCS_PUSHED, 40, 22)),
    ("DrawFrameControl DFC_BUTTON/DFCS_BUTTONCHECK (16x16)", 16, 16, dfc(DFC_BUTTON, DFCS_BUTTONCHECK, 16, 16)),
    ("DrawFrameControl DFC_BUTTON/DFCS_BUTTONCHECK|DFCS_CHECKED (16x16)", 16, 16, dfc(DFC_BUTTON, DFCS_BUTTONCHECK | DFCS_CHECKED, 16, 16)),
    ("DrawFrameControl DFC_BUTTON/DFCS_BUTTONRADIO (16x16)", 16, 16, dfc(DFC_BUTTON, DFCS_BUTTONRADIO, 16, 16)),
    ("DrawFrameControl DFC_BUTTON/DFCS_BUTTONRADIO|DFCS_CHECKED (16x16)", 16, 16, dfc(DFC_BUTTON, DFCS_BUTTONRADIO | DFCS_CHECKED, 16, 16)),
    ("DrawFrameControl DFC_BUTTON/DFCS_BUTTONPUSH|DFCS_FLAT|DFCS_CHECKED (toolbar toggle, 22x22)", 22, 22, dfc(DFC_BUTTON, DFCS_BUTTONPUSH | DFCS_FLAT | DFCS_CHECKED, 22, 22)),
    ("DrawFrameControl DFC_BUTTON/DFCS_BUTTONPUSH|DFCS_FLAT (toolbar hot, 22x22)", 22, 22, dfc(DFC_BUTTON, DFCS_BUTTONPUSH | DFCS_FLAT, 22, 22)),
    ("DrawFrameControl DFC_CAPTION/DFCS_CAPTIONCLOSE (16x14)", 16, 14, dfc(DFC_CAPTION, DFCS_CAPTIONCLOSE, 16, 14)),
    ("DrawFrameControl DFC_CAPTION/DFCS_CAPTIONMIN (16x14)", 16, 14, dfc(DFC_CAPTION, DFCS_CAPTIONMIN, 16, 14)),
    ("DrawFrameControl DFC_CAPTION/DFCS_CAPTIONMAX (16x14)", 16, 14, dfc(DFC_CAPTION, DFCS_CAPTIONMAX, 16, 14)),
    ("DrawFrameControl DFC_MENU/DFCS_MENUCHECK (16x16)", 16, 16, dfc(DFC_MENU, DFCS_MENUCHECK, 16, 16)),
    ("DrawFrameControl DFC_MENU/DFCS_MENUARROW (16x16)", 16, 16, dfc(DFC_MENU, 0x0000, 16, 16)),
    ("DrawFrameControl DFC_SCROLL/DFCS_SCROLLUP (16x16)", 16, 16, dfc(DFC_SCROLL, DFCS_SCROLLUP, 16, 16)),
    ("DrawFrameControl DFC_SCROLL/DFCS_SCROLLDOWN (16x16)", 16, 16, dfc(DFC_SCROLL, DFCS_SCROLLDOWN, 16, 16)),
    ("DrawFrameControl DFC_SCROLL/DFCS_SCROLLCOMBOBOX (16x16)", 16, 16, dfc(DFC_SCROLL, DFCS_SCROLLCOMBOBOX, 16, 16)),
    ("DrawFrameControl DFC_SCROLL/DFCS_SCROLLSIZEGRIP (16x16)", 16, 16, dfc(DFC_SCROLL, DFCS_SCROLLSIZEGRIP, 16, 16)),
    ("DrawStatusTextA(\"\",0) (60x22)", 60, 22, status_text(b"", 0, 60, 22)),
    ("DrawStatusTextA(\"\",SBT_NOBORDERS) (60x22)", 60, 22, status_text(b"", SBT_NOBORDERS, 60, 22)),
]


def matrix_lines(img, live):
    """Render the WHOLE raster as a compact character matrix with a legend.

    Each distinct colour gets one legend symbol, so the complete pixel recipe of
    the primitive (all four border rings and corners included) is readable at a
    glance instead of a 500-line hex dump.
    """
    rev = {}
    for name, rgb in live.items():
        rev.setdefault(rgb, name)

    order, seen = [], {}
    symbols = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
    w, h = img.size
    for y in range(h):
        for x in range(w):
            rgb = img.getpixel((x, y))
            if rgb not in seen:
                seen[rgb] = symbols[len(order) % len(symbols)]
                order.append(rgb)

    lines = ["  legend:"]
    for rgb in order:
        lines.append("    %s = #%02X%02X%02X  %s" % (seen[rgb], rgb[0], rgb[1], rgb[2],
                                                     rev.get(rgb, "(other)")))
    lines.append("")
    for y in range(h):
        lines.append("    %2d |%s|" % (y, "".join(seen[img.getpixel((x, y))] for x in range(w))))

    def nm(rgb):
        return rev.get(rgb, "#%02X%02X%02X" % rgb)

    mx, my = w // 2, h // 2
    lines.append("")
    lines.append("  ring scan (outermost -> inward, at mid-side):")
    lines.append("    top    : " + "  ".join("y=%d %s" % (y, nm(img.getpixel((mx, y)))) for y in range(min(4, h))))
    lines.append("    bottom : " + "  ".join("y=%d %s" % (y, nm(img.getpixel((mx, y)))) for y in range(h - 1, max(h - 5, -1), -1)))
    lines.append("    left   : " + "  ".join("x=%d %s" % (x, nm(img.getpixel((x, my)))) for x in range(min(4, w))))
    lines.append("    right  : " + "  ".join("x=%d %s" % (x, nm(img.getpixel((x, my)))) for x in range(w - 1, max(w - 5, -1), -1)))
    return lines


def main():
    live = live_table()
    rev = {}
    for name, rgb in live.items():
        rev.setdefault(rgb, []).append(name)

    out = []
    out.append("=" * 78)
    out.append("GDI CLASSIC 3D-ART PROBE  --  rendered by the live user32/gdi32")
    out.append("=" * 78)
    out.append("")
    out.append("LIVE system colours on this host (Windows 11, modern scheme):")
    for idx in sorted(SYSCOLORS):
        name = SYSCOLORS[idx]
        rgb = live[name]
        out.append("  %-32s = #%02X%02X%02X" % (name, *rgb))
    out.append("")
    out.append("Win98 SE 'Windows Standard' scheme reference values:")
    for name in SYSCOLORS.values():
        rgb = WIN98_STANDARD.get(name)
        if rgb:
            out.append("  %-32s = #%02X%02X%02X" % (name, *rgb))
    out.append("")

    sheet_results = []
    for title, w, h, fn in PROBES:
        img = render(w, h, fn)
        sheet_results.append((title, img))
        out.append("-" * 78)
        out.append(title)
        out.append("-" * 78)
        out.append("  live system colours seen in this primitive: " + "  ".join(
            "%s=%s" % (n, "#%02X%02X%02X" % live[n]) for n in
            sorted(live, key=lambda k: live[k])))
        out.append("")
        for ln in matrix_lines(img, live):
            out.append(ln)
        out.append("")

    # contact sheet, 3x nearest-neighbour zoom with captions
    Z = 3
    pad, lab = 8, 14
    cols = 3
    cell_w = max(im.size[0] for _, im in sheet_results) * Z + pad * 2
    cell_h = max(im.size[1] for _, im in sheet_results) * Z + pad * 2 + lab
    rows = (len(sheet_results) + cols - 1) // cols
    sheet = Image.new("RGB", (cols * cell_w, rows * cell_h), (192, 192, 192))
    d = ImageDraw.Draw(sheet)
    for i, (title, im) in enumerate(sheet_results):
        cx, cy = (i % cols) * cell_w, (i // cols) * cell_h
        big = im.resize((im.size[0] * Z, im.size[1] * Z), Image.NEAREST)
        ox = cx + (cell_w - big.size[0]) // 2
        oy = cy + lab + pad
        d.rectangle([ox - 1, oy - 1, ox + big.size[0], oy + big.size[1]], outline=(0, 0, 0))
        sheet.paste(big, (ox, oy))
        d.text((cx + 4, cy + 3), "%02d %s" % (i, title[:78]), fill=(0, 0, 0))
    sheet.save(os.path.join(OUT, "gdi_edge_probe.png"))

    with open(os.path.join(OUT, "gdi_edge_probe.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(out))
    print("\n".join(out[:40]))
    print("...")
    print("wrote reports/gdi_edge_probe.txt  (%d probes)" % len(PROBES))
    print("wrote reports/gdi_edge_probe.png")


if __name__ == "__main__":
    main()
