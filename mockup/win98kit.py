# -*- coding: utf-8 -*-
"""
win98kit.py -- a Windows 98 SE "Windows Standard" rendering kit.

KEY IDEA
--------
On Windows 9x the chrome is not made of bitmaps. It is *generated* by three GDI
calls: DrawEdge, DrawFrameControl and DrawStatusText. So the most faithful way
to draw Win98 chrome is to let GDI draw it -- and simply pin the colour scheme.

This kit therefore works from two evidence-backed sources:

  1. REAL GDI ART.  Push buttons, caption buttons, check boxes, arrows, size
     grips, scroll arrows and menu glyphs are rendered by the genuine
     user32/comctl32 code at the exact size needed, captured from a DIB section,
     and then RE-COLOURED: every pixel's live system colour is looked up and
     rewritten to the Windows 98 SE "Windows Standard" equivalent. That is what
     turns today's #F0F0F0 chrome into 1999's #C0C0C0 chrome -- the same GDI
     code path Win98 SE itself would run, with the scheme forced.

  2. MEASURED PIXEL RECIPES.  For the shapes GDI will not draw for us
     (instrument bezels, annular sectors, drum cells) the recipes below come
     from reports/gdi_edge_probe.txt, which was produced by rendering the real
     primitives and reading the pixels back -- not from memory.

Everything is drawn on a PIL surface because PIL's primitives are exact-pixel
and non-antialiased, which is what GDI gives us too. Text, however, is rendered
by real GDI so that the actual MS Sans Serif / SimSun / Arial bitmaps and
hinting are used. The mockup is therefore a picture of what the C code will
produce, not an artist's impression of it.
"""
import ctypes
import ctypes.wintypes as wt
import math

from PIL import Image, ImageDraw

# ---------------------------------------------------------------------------
# 1. Windows 98 SE "Windows Standard" colour scheme (pinned)
# ---------------------------------------------------------------------------
PAL = {
    "SCROLLBAR": (192, 192, 192),
    "DESKTOP": (0, 128, 128),
    "ACTIVECAPTION": (0, 0, 128),
    "GRADIENTACTIVECAPTION": (166, 202, 240),   # Win98's signature caption fade
    "GRADIENTINACTIVECAPTION": (192, 192, 192),
    "INACTIVECAPTION": (128, 128, 128),
    "INACTIVECAPTIONTEXT": (192, 192, 192),
    "CAPTIONTEXT": (255, 255, 255),
    "MENU": (192, 192, 192),
    "MENUTEXT": (0, 0, 0),
    "WINDOW": (255, 255, 255),
    "WINDOWTEXT": (0, 0, 0),
    "WINDOWFRAME": (0, 0, 0),
    "BTNFACE": (192, 192, 192),
    "BTNSHADOW": (128, 128, 128),
    "BTNHIGHLIGHT": (255, 255, 255),
    "BTNTEXT": (0, 0, 0),
    "GRAYTEXT": (128, 128, 128),
    "3DLIGHT": (223, 223, 223),     # see docs/COLOR.md for why not #C0C0C0
    "3DDKSHADOW": (128, 128, 128),
    "HIGHLIGHT": (0, 0, 128),
    "HIGHLIGHTTEXT": (255, 255, 255),
    "INFOBK": (255, 255, 225),
    # extra inks used only by the instrument, all inside the Win98 16-colour set
    "RED": (255, 0, 0),
    "DARKRED": (128, 0, 0),
    "GREEN": (0, 128, 0),
}

C = PAL  # short alias

# ---------------------------------------------------------------------------
# 2. GDI plumbing
# ---------------------------------------------------------------------------
u32 = ctypes.WinDLL("user32", use_last_error=True)
g32 = ctypes.WinDLL("gdi32", use_last_error=True)


class RECT(ctypes.Structure):
    _fields_ = [("left", wt.LONG), ("top", wt.LONG),
                ("right", wt.LONG), ("bottom", wt.LONG)]


class SIZE(ctypes.Structure):
    _fields_ = [("cx", wt.LONG), ("cy", wt.LONG)]


class BITMAPINFOHEADER(ctypes.Structure):
    _fields_ = [("biSize", wt.DWORD), ("biWidth", wt.LONG), ("biHeight", wt.LONG),
                ("biPlanes", wt.WORD), ("biBitCount", wt.WORD),
                ("biCompression", wt.DWORD), ("biSizeImage", wt.DWORD),
                ("biXPelsPerMeter", wt.LONG), ("biYPelsPerMeter", wt.LONG),
                ("biClrUsed", wt.DWORD), ("biClrImportant", wt.DWORD)]


class BITMAPINFO(ctypes.Structure):
    _fields_ = [("bmiHeader", BITMAPINFOHEADER), ("bmiColors", wt.DWORD * 4)]


g32.CreateCompatibleDC.restype = wt.HDC
g32.CreateCompatibleDC.argtypes = [wt.HDC]
g32.CreateDIBSection.restype = wt.HBITMAP
g32.CreateDIBSection.argtypes = [wt.HDC, ctypes.POINTER(BITMAPINFO), wt.UINT,
                                 ctypes.POINTER(ctypes.c_void_p), wt.HANDLE, wt.DWORD]
g32.CreateSolidBrush.restype = wt.HBRUSH
g32.CreateSolidBrush.argtypes = [wt.DWORD]
g32.CreatePen.restype = wt.HPEN
g32.CreatePen.argtypes = [ctypes.c_int, ctypes.c_int, wt.DWORD]
g32.CreateFontA.restype = wt.HFONT
g32.CreateFontA.argtypes = ([ctypes.c_int] * 5 + [wt.DWORD] * 8 + [ctypes.c_char_p])
g32.SelectObject.restype = wt.HGDIOBJ
g32.SelectObject.argtypes = [wt.HDC, wt.HGDIOBJ]
g32.DeleteObject.argtypes = [wt.HGDIOBJ]
g32.GetStockObject.restype = wt.HGDIOBJ
g32.GetStockObject.argtypes = [ctypes.c_int]
g32.DeleteDC.argtypes = [wt.HDC]
g32.DeleteDC.restype = wt.BOOL
u32.FillRect.argtypes = [wt.HDC, ctypes.POINTER(RECT), wt.HBRUSH]
u32.DrawFrameControl.argtypes = [wt.HDC, ctypes.POINTER(RECT), wt.UINT, wt.UINT]
u32.GetSysColor.argtypes = [ctypes.c_int]
u32.GetSysColor.restype = wt.DWORD
g32.TextOutA.argtypes = [wt.HDC, ctypes.c_int, ctypes.c_int, ctypes.c_char_p, ctypes.c_int]
g32.GetTextExtentPoint32A.argtypes = [wt.HDC, ctypes.c_char_p, ctypes.c_int,
                                      ctypes.POINTER(SIZE)]
g32.SetTextColor.argtypes = [wt.HDC, wt.DWORD]
g32.SetBkMode.argtypes = [wt.HDC, ctypes.c_int]

NULL_BRUSH = 5
NULL_PEN = 8
TRANSPARENT = 1
DEFAULT_CHARSET = 1

DFC_CAPTION, DFC_MENU, DFC_SCROLL, DFC_BUTTON = 1, 2, 3, 4
DFCS_BUTTONCHECK, DFCS_BUTTONRADIO, DFCS_BUTTONPUSH = 0x0000, 0x0004, 0x0010
DFCS_INACTIVE, DFCS_PUSHED, DFCS_CHECKED, DFCS_FLAT = 0x0100, 0x0200, 0x0400, 0x4000
DFCS_CAPTIONCLOSE, DFCS_CAPTIONMIN, DFCS_CAPTIONMAX = 0x0000, 0x0001, 0x0002
DFCS_SCROLLUP, DFCS_SCROLLDOWN, DFCS_SCROLLCOMBOBOX, DFCS_SCROLLSIZEGRIP = 0, 1, 5, 8

QUALITY_DEFAULT, QUALITY_NONANTIALIASED = 0, 3
QUALITY_ANTIALIASED, QUALITY_CLEARTYPE = 4, 5


def _bgr(rgb):
    r, g, b = rgb
    return r | (g << 8) | (b << 16)


# ---------------------------------------------------------------------------
# 3. live system colours -> Win98 colours  (the re-colouring table)
# ---------------------------------------------------------------------------
# wingdi.h index -> palette role.  Two roles may share a value in a given scheme;
# that is fine as long as they resolve to the SAME Win98 colour.  Indices 1
# (DESKTOP) and 12 (APPWORKSPACE) are deliberately excluded: a chrome primitive
# never paints them, and on this host DESKTOP is #000000 which would collide
# with the black used for check marks, arrows and caption glyphs.
_SYSROLE = {
    0: "SCROLLBAR", 2: "ACTIVECAPTION", 3: "INACTIVECAPTION",
    4: "MENU", 5: "WINDOW", 6: "WINDOWFRAME", 7: "MENUTEXT", 8: "WINDOWTEXT",
    9: "CAPTIONTEXT", 10: "BTNFACE", 11: "BTNFACE",
    13: "HIGHLIGHT", 14: "HIGHLIGHTTEXT", 15: "BTNFACE", 16: "BTNSHADOW",
    17: "GRAYTEXT", 18: "BTNTEXT", 19: "INACTIVECAPTIONTEXT",
    20: "BTNHIGHLIGHT", 21: "3DDKSHADOW", 22: "3DLIGHT", 23: "WINDOWTEXT",
    24: "INFOBK", 26: "HIGHLIGHT", 27: "GRADIENTACTIVECAPTION",
    28: "GRADIENTINACTIVECAPTION", 30: "MENU",
}

# Which role owns a colour when two roles collide on a host.
_ROLE_PRIORITY = ["BTNFACE", "BTNSHADOW", "BTNHIGHLIGHT", "3DLIGHT", "3DDKSHADOW",
                  "WINDOW", "MENU", "WINDOWTEXT", "BTNTEXT", "GRAYTEXT",
                  "CAPTIONTEXT", "HIGHLIGHT", "WINDOWFRAME", "SCROLLBAR"]

CONFLICTS = []


def _build_recolor():
    m, owner = {}, {}
    items = sorted(_SYSROLE.items(),
                   key=lambda kv: _ROLE_PRIORITY.index(kv[1])
                   if kv[1] in _ROLE_PRIORITY else 99)
    for idx, role in items:
        v = u32.GetSysColor(idx)
        live = ((v >> 0) & 0xFF, (v >> 8) & 0xFF, (v >> 16) & 0xFF)
        target = PAL[role]
        if live in m:
            if m[live] != target:
                CONFLICTS.append((live, owner[live], m[live], role, target))
            continue
        m[live] = target
        owner[live] = role
    return m


RECOLOR = _build_recolor()


def recolor(img):
    """Rewrite every pixel from the live scheme to the Win98 SE scheme."""
    px = img.load()
    w, h = img.size
    for y in range(h):
        for x in range(w):
            p = px[x, y]
            t = RECOLOR.get(p)
            if t is not None:
                px[x, y] = t
    return img


# ---------------------------------------------------------------------------
# 4. off-screen GDI surfaces
# ---------------------------------------------------------------------------
class Surface:
    """A 32bpp top-down DIB section with a memory DC."""

    def __init__(self, w, h, fill=(255, 0, 255)):
        self.w, self.h = w, h
        self.hdc = g32.CreateCompatibleDC(None)
        bmi = BITMAPINFO()
        bmi.bmiHeader.biSize = ctypes.sizeof(BITMAPINFOHEADER)
        bmi.bmiHeader.biWidth = w
        bmi.bmiHeader.biHeight = -h
        bmi.bmiHeader.biPlanes = 1
        bmi.bmiHeader.biBitCount = 32
        bmi.bmiHeader.biCompression = 0
        self.ppv = ctypes.c_void_p()
        self.hbm = g32.CreateDIBSection(self.hdc, ctypes.byref(bmi), 0,
                                        ctypes.byref(self.ppv), None, 0)
        self._old = g32.SelectObject(self.hdc, self.hbm)
        self.clear(fill)

    def clear(self, rgb):
        b = g32.CreateSolidBrush(_bgr(rgb))
        r = RECT(0, 0, self.w, self.h)
        u32.FillRect(self.hdc, ctypes.byref(r), b)
        g32.DeleteObject(b)

    def rect(self, l, t, r, b, rgb):
        br = g32.CreateSolidBrush(_bgr(rgb))
        rc = RECT(l, t, r, b)
        u32.FillRect(self.hdc, ctypes.byref(rc), br)
        g32.DeleteObject(br)

    def dfc(self, l, t, r, b, kind, state):
        rc = RECT(l, t, r, b)
        u32.DrawFrameControl(self.hdc, ctypes.byref(rc), kind, state)

    def image(self):
        g32.GdiFlush()
        buf = ctypes.string_at(self.ppv, self.w * self.h * 4)
        return Image.frombuffer("RGBA", (self.w, self.h), buf, "raw", "BGRA", 0, 1).convert("RGB")

    def release(self):
        g32.SelectObject(self.hdc, self._old)
        g32.DeleteObject(self.hbm)
        g32.DeleteDC(self.hdc)


_scratch = None


def _dc():
    global _scratch
    if _scratch is None:
        _scratch = g32.CreateCompatibleDC(None)
    return _scratch


# ---------------------------------------------------------------------------
# 5. authentic widgets: real GDI art, re-coloured to the Win98 scheme
# ---------------------------------------------------------------------------
def gdi_widget(w, h, draw_fn, key=(255, 0, 255)):
    """Render a widget with real GDI, recolour it, key out the background."""
    s = Surface(w, h, fill=key)
    draw_fn(s)
    img = recolor(s.image())
    s.release()
    return img


def push_button(w, h, state="up"):
    st = DFCS_BUTTONPUSH
    if state == "down":
        st |= DFCS_PUSHED
    elif state == "disabled":
        st |= DFCS_INACTIVE
    elif state == "default":
        pass
    return gdi_widget(w, h, lambda s: s.dfc(0, 0, w, h, DFC_BUTTON, st))


def caption_button(w, h, glyph="close", state=0):
    g = {"close": DFCS_CAPTIONCLOSE, "min": DFCS_CAPTIONMIN,
         "max": DFCS_CAPTIONMAX}[glyph]
    return gdi_widget(w, h, lambda s: s.dfc(0, 0, w, h, DFC_CAPTION, g | state))


def scroll_glyph(size, kind="down", state=0):
    g = {"up": DFCS_SCROLLUP, "down": DFCS_SCROLLDOWN,
         "combo": DFCS_SCROLLCOMBOBOX, "grip": DFCS_SCROLLSIZEGRIP}[kind]
    return gdi_widget(size, size, lambda s: s.dfc(0, 0, size, size, DFC_SCROLL, g | state))


def check_box(state=0):
    return gdi_widget(16, 16, lambda s: s.dfc(0, 0, 16, 16, DFC_BUTTON, DFCS_BUTTONCHECK | state))


def as_image(obj):
    """Accept either a PIL Image or an ImageDraw and return the Image."""
    if hasattr(obj, "_image"):
        return obj._image
    return obj


def paste_keyed(dst, xy, src, key=(255, 0, 255)):
    """Paste a widget, treating `key` as transparent."""
    mask = Image.new("L", src.size, 0)
    sp, mp = src.load(), mask.load()
    for y in range(src.size[1]):
        for x in range(src.size[0]):
            if sp[x, y] != key:
                mp[x, y] = 255
    dst.paste(src, xy, mask)


# ---------------------------------------------------------------------------
# 6. real GDI text
# ---------------------------------------------------------------------------
class GdiText:
    """Text rendered by the same GDI the application will call.

    Font specs are (facename, height_px, weight, quality).  Height is given the
    way the application will pass it to CreateFont: as a positive em height,
    negated internally so GDI matches the classic pixel sizes (12 px = 9 pt at
    96 dpi, which is the Win98 SE Simplified-Chinese UI size).
    """

    def __init__(self):
        self._fonts = {}

    def _font(self, spec):
        face, height, weight, quality = spec
        key = (face, height, weight, quality)
        if key not in self._fonts:
            self._fonts[key] = g32.CreateFontA(
                -height, 0, 0, 0, weight, 0, 0, 0, DEFAULT_CHARSET,
                0, 0, quality, 0, face.encode("gbk"))
        return self._fonts[key]

    def measure(self, text, spec):
        old = g32.SelectObject(_dc(), self._font(spec))
        sz = SIZE()
        b = text.encode("gbk")
        g32.GetTextExtentPoint32A(_dc(), b, len(b), ctypes.byref(sz))
        g32.SelectObject(_dc(), old)
        return sz.cx, sz.cy

    def render(self, text, spec, color, bg):
        """Return an RGB image of `text` drawn on an opaque `bg`."""
        w, h = self.measure(text, spec)
        pad = 3
        s = Surface(max(w + pad * 2, 1), max(h + pad * 2, 1), fill=bg)
        old = g32.SelectObject(s.hdc, self._font(spec))
        g32.SetBkMode(s.hdc, TRANSPARENT)
        g32.SetTextColor(s.hdc, _bgr(color))
        b = text.encode("gbk")
        g32.TextOutA(s.hdc, pad, pad, b, len(b))
        g32.SelectObject(s.hdc, old)
        img = s.image()
        s.release()
        return img, (pad, pad)

    def ink_bbox(self, img, bg):
        """Bounding box of the inked pixels (relative to the rendered image)."""
        ip = img.load()
        w, h = img.size
        xs, ys = [], []
        for y in range(h):
            for x in range(w):
                if ip[x, y] != bg:
                    xs.append(x)
                    ys.append(y)
        if not xs:
            return None
        return min(xs), min(ys), max(xs), max(ys)

    def draw(self, dst, xy, text, spec, color, bg=None):
        """Draw text onto a PIL image (or ImageDraw); returns the advance width."""
        img_obj = as_image(dst)
        if bg is None:
            bg = img_obj.getpixel((max(xy[0], 0), max(xy[1], 0)))
        img, (ox, oy) = self.render(text, spec, color, bg)
        # paste with the background keyed out, so glyph interiors stay clean
        mask = Image.new("L", img.size, 0)
        ip, mp = img.load(), mask.load()
        for y in range(img.size[1]):
            for x in range(img.size[0]):
                if ip[x, y] != bg:
                    mp[x, y] = 255
        img_obj.paste(img, (xy[0] - ox, xy[1] - oy), mask)
        return self.measure(text, spec)[0]


TEXT = GdiText()

# font specs used by KeyODO -------------------------------------------------
# Win98 SE Simplified Chinese uses SimSun 9 pt (12 px) as the dialog/shell font.
F_DIALOG = ("\u5b8b\u4f53", 12, 400, QUALITY_DEFAULT)        # 宋体 9pt
F_DIALOG_B = ("\u5b8b\u4f53", 12, 700, QUALITY_DEFAULT)
F_CAPTION = ("\u5b8b\u4f53", 12, 700, QUALITY_DEFAULT)
F_SMALL = ("\u5b8b\u4f53", 11, 400, QUALITY_DEFAULT)
F_DIAL_TICK = ("Arial", 11, 700, QUALITY_NONANTIALIASED)      # 8pt dial numerals
F_DIAL_UNIT = ("Arial", 11, 700, QUALITY_NONANTIALIASED)
F_DIGIT_HUGE = ("Arial", 29, 700, QUALITY_ANTIALIASED)        # big WPM readout
F_DRUM = ("Arial", 20, 700, QUALITY_NONANTIALIASED)           # odometer drum
F_READOUT = ("Arial", 17, 700, QUALITY_NONANTIALIASED)        # KPM / MAX
F_EMBED = ("Arial", 17, 700, QUALITY_NONANTIALIASED)          # in-dial value
F_GEAR_BIG = ("Arial", 29, 700, QUALITY_ANTIALIASED)          # one-letter gear
F_GEAR_SM = ("Arial", 20, 700, QUALITY_NONANTIALIASED)        # "M1".."M3"
F_GATE = ("Arial", 14, 700, QUALITY_NONANTIALIASED)           # gate strip

# ---------------------------------------------------------------------------
# 7. measured chrome recipes  (from reports/gdi_edge_probe.txt)
# ---------------------------------------------------------------------------
# kind -> (top/left outer, top/left inner, bottom/right inner, bottom/right outer)
EDGE_RECIPES = {
    "raised":      (C["3DLIGHT"],   C["BTNHIGHLIGHT"], C["BTNSHADOW"],  C["3DDKSHADOW"]),
    "sunken":      (C["BTNSHADOW"], C["3DDKSHADOW"],   C["3DLIGHT"],    C["BTNHIGHLIGHT"]),
    "etched":      (C["BTNSHADOW"], C["WINDOW"],       C["BTNSHADOW"],  C["WINDOW"]),
    "bump":        (C["3DLIGHT"],   C["3DDKSHADOW"],   C["3DLIGHT"],    C["3DDKSHADOW"]),
    "raisedouter": (C["3DLIGHT"],   None,              None,            C["3DDKSHADOW"]),
    "raisedinner": (C["WINDOW"],    None,              C["BTNSHADOW"],  None),
    "sunkenouter": (C["BTNSHADOW"], None,              None,            C["WINDOW"]),
    "sunkeninner": (C["3DDKSHADOW"], None,             C["3DLIGHT"],    None),
}


def hl(d, x0, x1, y, color):
    """1-pixel horizontal line (x1 exclusive)."""
    if x1 > x0:
        d.rectangle([x0, y, x1 - 1, y], fill=color)


def vl(d, x, y0, y1, color):
    """1-pixel vertical line (y1 exclusive)."""
    if y1 > y0:
        d.rectangle([x, y0, x, y1 - 1], fill=color)


def edge(d, box, kind, thickness=2):
    """Draw a border using the measured DrawEdge recipe.

    thickness 2 reproduces EDGE_* (outer ring + inner ring); thickness 1
    reproduces a single BDR_* ring.
    """
    tl_o, tl_i, br_i, br_o = EDGE_RECIPES[kind]
    l, t, r, b = box
    r -= 1
    b -= 1
    if thickness == 1:
        tl, br = tl_o, br_o
        hl(d, l, r + 1, t, tl)
        vl(d, l, t, b + 1, tl)
        hl(d, l, r + 1, b, br)
        vl(d, r, t, b + 1, br)
        return
    hl(d, l, r + 1, t, tl_o)
    hl(d, l, r + 1, t + 1, tl_i)
    vl(d, l, t, b + 1, tl_o)
    vl(d, l + 1, t, b + 1, tl_i)
    hl(d, l, r + 1, b, br_o)
    hl(d, l, r + 1, b - 1, br_i)
    vl(d, r, t, b + 1, br_o)
    vl(d, r - 1, t, b + 1, br_i)


def panel(d, box, fill, kind="sunken", thickness=2):
    edge(d, box, kind, thickness)
    inset = thickness
    d.rectangle([box[0] + inset, box[1] + inset, box[2] - 1 - inset, box[3] - 1 - inset],
                fill=fill)


def group_box(d, box, caption, font=None):
    """Win98 group box: 1 px etched frame with a gap in the top edge for the label."""
    font = font or F_DIALOG
    l, t, r, b = box
    r -= 1
    b -= 1
    tw = TEXT.measure(caption, font)[0]
    gap0 = l + 9
    gap1 = gap0 + tw + 4
    # frame with the label gap
    hl(d, l + 1, gap0, t, C["BTNSHADOW"])
    hl(d, gap1, r, t, C["BTNSHADOW"])
    vl(d, l, t + 1, b + 1, C["BTNSHADOW"])
    hl(d, l + 1, r + 1, b, C["WINDOW"])
    vl(d, r, t + 1, b + 1, C["WINDOW"])
    # rounded-off corner pixels of the etched frame
    d.point((l, t), fill=C["BTNSHADOW"])
    d.point((r, t), fill=C["BTNSHADOW"])
    d.point((l, b), fill=C["WINDOW"])
    d.point((r, b), fill=C["WINDOW"])
    TEXT.draw(d, (gap0 + 2, t - 5), caption, font, C["BTNTEXT"], bg=C["BTNFACE"])


# ---------------------------------------------------------------------------
# 8. geometry helpers that map 1:1 onto the GDI calls in the C code
# ---------------------------------------------------------------------------
def pt(cx, cy, r, angle_deg):
    """Screen point at radius r, angle in degrees, 0 = 3 o'clock, CCW positive."""
    a = math.radians(angle_deg)
    return (cx + r * math.cos(a), cy - r * math.sin(a))


def arc_points(cx, cy, r, a0, a1, step=3.0):
    """Point list along a circular arc from a0 to a1 (degrees, CCW)."""
    n = max(2, int(abs(a1 - a0) / step) + 1)
    return [pt(cx, cy, r, a0 + (a1 - a0) * i / (n - 1)) for i in range(n)]


def annulus_sector(d, cx, cy, r_out, r_in, a0, a1, fill, step=2.0):
    """Filled annular sector -- the GDI equivalent is a single Polygon() call."""
    outer = arc_points(cx, cy, r_out, a0, a1, step)
    inner = arc_points(cx, cy, r_in, a1, a0, step)
    d.polygon(list(outer) + list(inner), fill=fill)


def radial_bar(d, cx, cy, r0, r1, angle, width, fill):
    """A tick mark: a thin quad along the radius (a single Polygon() in GDI)."""
    hw = width / 2.0
    ux, uy = math.cos(math.radians(angle)), math.sin(math.radians(angle))
    rad = (ux, -uy)          # radial direction in screen space
    per = (uy, ux)           # perpendicular in screen space

    def corner(r, off):
        return (cx + r * rad[0] + off * per[0], cy + r * rad[1] + off * per[1])

    d.polygon([corner(r0, -hw), corner(r0, hw), corner(r1, hw), corner(r1, -hw)],
              fill=fill)


def circle_outline(d, cx, cy, r, color):
    d.ellipse([cx - r, cy - r, cx + r - 1, cy + r - 1], outline=color)
