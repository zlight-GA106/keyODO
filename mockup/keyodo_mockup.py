# -*- coding: utf-8 -*-
"""
keyodo_mockup.py -- renders the KeyODO main window at exactly 480x320 @ 96 DPI.

This file IS the layout specification in executable form. Every rectangle below
is expressed the way it will appear in the Win32 source, so the C code can be
written straight from it:

    window (480x320)  =  3 px frame  +  18 px caption  +  client 474x296  +  3 px frame
    client (474x296)  =  20 px menu bar  +  256 px content  +  20 px status bar

Outputs: mockup/out/*.png
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from PIL import Image, ImageDraw

from win98kit import (C, PAL, TEXT, F_DIALOG, F_DIALOG_B, F_CAPTION, F_SMALL,
                      F_DIAL_TICK, F_DIAL_UNIT, F_DIGIT_HUGE, F_DRUM, F_READOUT,
                      F_EMBED, F_GEAR_BIG, F_GEAR_SM, F_GATE,
                      edge, panel, group_box, hl, vl, pt, arc_points,
                      annulus_sector, radial_bar, circle_outline, as_image,
                      push_button, caption_button, paste_keyed)

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "out")
os.makedirs(OUT, exist_ok=True)

# ===========================================================================
# WINDOW GEOMETRY  (outer coordinates; 96 DPI, Win98 SE window metrics)
# ===========================================================================
WIN_W, WIN_H = 480, 320
FRAME = 3                 # SM_CXDLGFRAME / SM_CYDLGFRAME on Win98 SE
CAPTION_H = 18            # SM_CYCAPTION, Windows Standard scheme

CLIENT_X, CLIENT_Y = FRAME, FRAME + CAPTION_H
CLIENT_W, CLIENT_H = WIN_W - FRAME * 2, WIN_H - (FRAME + CAPTION_H) - FRAME
MENUBAR_H = 20
STATUSBAR_H = 20

# content band, client coordinates
CONTENT_Y = MENUBAR_H
CONTENT_H = CLIENT_H - MENUBAR_H - STATUSBAR_H      # = 256

# --- the three group boxes -------------------------------------------------
# The group-box caption sits 5 px above the frame, so the boxes must start at
# least 7 px below the menu bar or the captions collide with it.
BOX_TOP = CONTENT_Y + 7
BOX_BOT = CONTENT_Y + 251
GB_SPEED = (4, BOX_TOP, 256, BOX_BOT)                            # 252 x 244
GB_ODO = (260, BOX_TOP, 470, BOX_TOP + 114)                      # 210 x 114
GB_RATE = (260, BOX_TOP + 124, 470, BOX_BOT)                     # 210 x 120

# --- speedometer ----------------------------------------------------------
DIAL_PANEL = (38, CONTENT_Y + 20, 222, CONTENT_Y + 20 + 184)     # 184 x 184
DIAL_CX = (DIAL_PANEL[0] + DIAL_PANEL[2]) // 2                   # 130
DIAL_CY = (DIAL_PANEL[1] + DIAL_PANEL[3]) // 2                   # 132
R_PANEL = 92
R_BEZEL = 86
R_FACE = 78
R_RED_IN, R_RED_OUT = 74, 78
R_TICK_OUT = 74
R_MAJ_IN, R_MED_IN, R_MIN_IN = 64, 67, 70
R_LABEL = 52
R_NEEDLE = 62
R_HUB = 7
R_TELLTALE_IN, R_TELLTALE_OUT = 62, 76

WPM_MAX = 120
REDLINE_FROM = 100
LABEL_STEP = 20
SWEEP_START = 210.0       # 0 WPM sits at 8 o'clock
SWEEP_TOTAL = 240.0       # clockwise to 4 o'clock at 120 WPM


def ang(wpm):
    """WPM -> dial angle in degrees (0 = 3 o'clock, counter-clockwise positive)."""
    return SWEEP_START - SWEEP_TOTAL * (wpm / float(WPM_MAX))


# in-dial embedded read-out, directly under the printed unit label
DIAL_EMBED = (88, CONTENT_Y + 151, 172, CONTENT_Y + 151 + 22)    #  84 x 22
# below-dial read-out: ONE panel holding the gear character then the value,
# so it reads "D 62" the way a car cluster does.
GEAR_VALUE = (72, CONTENT_Y + 208, 188, CONTENT_Y + 208 + 36)    # 116 x 36
GV_GEAR_X = 18            # gear anchor, px from the panel's left edge
GV_VALUE_X = 50           # value anchor, px from the panel's left edge

# --- odometer -------------------------------------------------------------
ODO_CAP_Y = CONTENT_Y + 19
ODO_DRUM = (270, CONTENT_Y + 33, 460, CONTENT_Y + 63)            # 190 x 30, 7 cells
TRIP_CAP_Y = CONTENT_Y + 69
TRIP_DRUM = (348, CONTENT_Y + 83, 460, CONTENT_Y + 113)          # 112 x 30, 4 cells
TRIP_RESET = (270, CONTENT_Y + 85, 340, CONTENT_Y + 111)         # 70 x 26

# --- rate -----------------------------------------------------------------
KPM_CAP_Y = CONTENT_Y + 146
KPM_PANEL = (368, CONTENT_Y + 140, 460, CONTENT_Y + 164)         # 92 x 24
MAX_CAP_Y = CONTENT_Y + 174
MAX_PANEL = (368, CONTENT_Y + 168, 460, CONTENT_Y + 192)         # 92 x 24
GEAR_GATE = (270, CONTENT_Y + 196, 460, CONTENT_Y + 218)         # 190 x 22
PEAK_RESET = (270, CONTENT_Y + 221, 382, CONTENT_Y + 247)        # 112 x 26

# --- transmission ---------------------------------------------------------
# The gate shows only the ratios that belong to the selected transmission.
# AUTO   : P R N D       -- park / reverse / neutral / drive
# MANUAL : R N M1 M2 M3  -- reverse / neutral / the three manual ratios
TM_AUTO, TM_MANUAL = "AUTO", "MANUAL"
GEARS_BY_TM = {
    TM_AUTO:   ["P", "R", "N", "D"],
    TM_MANUAL: ["R", "N", "M1", "M2", "M3"],
}

MODE_WPM, MODE_KPM = "WPM", "KPM"

# --- status bar panes -----------------------------------------------------
PANE3_W, PANE2_W = 118, 116
PANE3_X = CLIENT_W - 2 - PANE3_W
PANE2_X = PANE3_X - 2 - PANE2_W

# sample values for the mockup ---------------------------------------------
VAL_WPM, VAL_MAX = 62, 88
VAL_ODO, VAL_TRIP = 1234567, 4812
VAL_KPM = 310
VAL_UPTIME = "00:12:34"

# ---------------------------------------------------------------------------
# icon: 16x16 keyboard, hand-drawn in the Win98 16-colour palette
# ---------------------------------------------------------------------------
ICON16 = [
    "................",
    "................",
    ".##############.",
    ".#wwwwwwwwwwww#.",
    ".#wfkkffkkffkk#.",
    ".#ffffffffffff#.",
    ".#fkkffkkffkkf#.",
    ".#ffffffffffff#.",
    ".#fkkffkkffkkf#.",
    ".#ffffffffffff#.",
    ".#fffkkkkkkfff#.",
    ".#ffffffffffff#.",
    ".#ffffffffffff#.",
    ".##############.",
    "................",
    "................",
]
ICON_INK = {"#": (0, 0, 0), "f": C["BTNFACE"], "k": C["BTNSHADOW"], "w": C["BTNHIGHLIGHT"]}


def make_icon():
    im = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
    px = im.load()
    for y, row in enumerate(ICON16):
        for x, ch in enumerate(row):
            if ch in ICON_INK:
                r, g, b = ICON_INK[ch]
                px[x, y] = (r, g, b, 255)
    return im


# ---------------------------------------------------------------------------
# window chrome
# ---------------------------------------------------------------------------
def draw_title_bar(canvas, d):
    l, t, r, b = FRAME, FRAME, WIN_W - FRAME, FRAME + CAPTION_H
    # Win98 signature caption fade: COLOR_ACTIVECAPTION -> COLOR_GRADIENTACTIVECAPTION
    a, z = C["ACTIVECAPTION"], C["GRADIENTACTIVECAPTION"]
    for x in range(l, r):
        f = (x - l) / float(max(r - l - 1, 1))
        d.line([(x, t), (x, b - 1)],
               fill=(int(a[0] + (z[0] - a[0]) * f),
                     int(a[1] + (z[1] - a[1]) * f),
                     int(a[2] + (z[2] - a[2]) * f)))
    icon = make_icon()
    canvas.paste(icon, (l + 3, t + 1), icon)
    TEXT.draw(d, (l + 22, t + 4), "KeyODO", F_CAPTION, C["CAPTIONTEXT"],
              bg=C["ACTIVECAPTION"])
    # system buttons: only minimise + close (the window is a fixed-size tool)
    cb = caption_button(16, 14, "close")
    mb = caption_button(16, 14, "min")
    paste_keyed(canvas, (r - 2 - 16, t + 2), cb)
    paste_keyed(canvas, (r - 2 - 16 - 2 - 16, t + 2), mb)


def draw_frame(d):
    # Win98 outer frame: 1 px black outline, then the grey sizing border
    d.rectangle([0, 0, WIN_W - 1, WIN_H - 1], outline=C["WINDOWFRAME"])
    for i in range(1, FRAME):
        d.rectangle([i, i, WIN_W - 1 - i, WIN_H - 1 - i], outline=C["BTNFACE"])


MENU_ITEMS = [("文件(F)", "F"), ("编辑(E)", "E"), ("查看(V)", "V"),
              ("统计(S)", "S"), ("帮助(H)", "H")]


def draw_menu_bar(d):
    # NOTE: the menu bar and the status bar are drawn into the CLIENT surface,
    # so these are client-local coordinates.
    l, t, r, b = 0, 0, CLIENT_W, MENUBAR_H
    d.rectangle([l, t, r - 1, b - 1], fill=C["MENU"])
    x = l + 2
    for label, accel in MENU_ITEMS:
        TEXT.draw(d, (x + 6, t + 4), label, F_DIALOG, C["MENUTEXT"], bg=C["MENU"])
        # accelerator underline, positioned from the real font's ink box
        img, (ox, oy) = TEXT.render(label, F_DIALOG, C["MENUTEXT"], C["MENU"])
        bb = TEXT.ink_bbox(img, C["MENU"])
        prefix = label.split("(")[0] + "("
        px = x + 6 + TEXT.measure(prefix, F_DIALOG)[0]
        aw = TEXT.measure(accel, F_DIALOG)[0]
        uy = min(t + 4 + bb[3] + 2, t + MENUBAR_H - 3)
        hl(d, px, px + aw, uy, C["MENUTEXT"])
        x += TEXT.measure(label, F_DIALOG)[0] + 12


def draw_status_bar(d, mode, tmode):
    l, t, r, b = 0, CLIENT_H - STATUSBAR_H, CLIENT_W, CLIENT_H
    d.rectangle([l, t, r - 1, b - 1], fill=C["BTNFACE"])
    edge(d, (l, t, r, b), "sunken", 1)          # DrawStatusText's 1 px frame
    for bx in (PANE2_X, PANE3_X):
        vl(d, bx - 2, t + 1, b - 1, C["BTNSHADOW"])
        vl(d, bx - 1, t + 1, b - 1, C["BTNHIGHLIGHT"])
    value = VAL_KPM if mode == MODE_KPM else VAL_WPM
    TEXT.draw(d, (l + 5, t + 4), "就绪", F_DIALOG, C["BTNTEXT"], bg=C["BTNFACE"])
    TEXT.draw(d, (PANE2_X + 3, t + 4), VAL_UPTIME, F_DIALOG,
              C["BTNTEXT"], bg=C["BTNFACE"])
    TEXT.draw(d, (PANE3_X + 3, t + 4), "%d %s" % (value, mode), F_DIALOG,
              C["BTNTEXT"], bg=C["BTNFACE"])


# ---------------------------------------------------------------------------
# the instrument
# ---------------------------------------------------------------------------
def _paste(d, x_adv, y_ink, text, spec, color, bg):
    """Paste text so its advance box starts at x_adv and its ink top is y_ink."""
    img_obj = as_image(d)
    img, (ox, oy) = TEXT.render(text, spec, color, bg)
    bb = TEXT.ink_bbox(img, bg)
    if bb is None:
        return 0
    mask = Image.new("L", img.size, 0)
    ip, mp = img.load(), mask.load()
    for yy in range(img.size[1]):
        for xx in range(img.size[0]):
            if ip[xx, yy] != bg:
                mp[xx, yy] = 255
    img_obj.paste(img, (x_adv - ox, y_ink - bb[1]), mask)
    return TEXT.measure(text, spec)[0]


def draw_text_centered(d, box, text, spec, color, bg):
    img, (ox, oy) = TEXT.render(text, spec, color, bg)
    bb = TEXT.ink_bbox(img, bg)
    if bb is None:
        return
    adv = TEXT.measure(text, spec)[0]
    ink_h = bb[3] - bb[1] + 1
    _paste(d, box[0] + (box[2] - box[0] - adv) // 2,
           box[1] + (box[3] - box[1] - ink_h) // 2, text, spec, color, bg)


def draw_text_left(d, x, box, text, spec, color, bg):
    """Left-aligned at x, optically centred vertically inside box."""
    img, (ox, oy) = TEXT.render(text, spec, color, bg)
    bb = TEXT.ink_bbox(img, bg)
    if bb is None:
        return 0
    ink_h = bb[3] - bb[1] + 1
    return _paste(d, x, box[1] + (box[3] - box[1] - ink_h) // 2,
                  text, spec, color, bg)


def math_cos_sin(angle_deg):
    import math
    a = math.radians(angle_deg)
    return math.cos(a), math.sin(a)


def value_to_wpm(value, mode):
    """The needle always shows a speed, so map KPM back onto the WPM scale
    (KPM = WPM x 5 for the usual five-character word)."""
    return value / 5.0 if mode == MODE_KPM else value


def draw_dial(surf, mode, value, peak):
    """mode  = MODE_WPM | MODE_KPM -- drives the printed unit and both read-outs
    value = the switchable reading
    peak  = the peak-hold telltale position, always in WPM"""
    d = ImageDraw.Draw(surf)
    cx, cy = DIAL_CX, DIAL_CY

    # 1. recessed instrument panel (square, 2 px sunken)
    panel(d, DIAL_PANEL, C["BTNFACE"], "sunken", 2)

    # 2. raised circular bezel: bevels split along the upper-left / lower-right axis
    d.ellipse([cx - R_BEZEL, cy - R_BEZEL, cx + R_BEZEL - 1, cy + R_BEZEL - 1],
              fill=C["BTNFACE"])
    d.line(arc_points(cx, cy, R_BEZEL - 1, 45, 225), fill=C["BTNHIGHLIGHT"], width=1)
    d.line(arc_points(cx, cy, R_BEZEL - 1, 225, 405), fill=C["BTNSHADOW"], width=1)
    d.line(arc_points(cx, cy, R_BEZEL - 2, 45, 225), fill=C["3DLIGHT"], width=1)
    d.line(arc_points(cx, cy, R_BEZEL - 2, 225, 405), fill=C["3DDKSHADOW"], width=1)

    # 3. white dial face
    d.ellipse([cx - R_FACE, cy - R_FACE, cx + R_FACE - 1, cy + R_FACE - 1],
              fill=C["WINDOW"], outline=C["BTNSHADOW"])

    # 4. redline band, printed just inside the face edge
    annulus_sector(d, cx, cy, R_RED_OUT - 1, R_RED_IN, ang(REDLINE_FROM), ang(WPM_MAX),
                   C["RED"])

    # 5. ticks -- the scale always runs to WPM_MAX; `peak` only moves the telltale
    for w in range(0, WPM_MAX + 1, 5):
        a = ang(w)
        if w % LABEL_STEP == 0:
            radial_bar(d, cx, cy, R_MAJ_IN, R_TICK_OUT, a, 2.0, C["WINDOWTEXT"])
        elif w % 10 == 0:
            radial_bar(d, cx, cy, R_MED_IN, R_TICK_OUT, a, 2.0, C["WINDOWTEXT"])
        else:
            radial_bar(d, cx, cy, R_MIN_IN, R_TICK_OUT, a, 1.0, C["GRAYTEXT"])

    # 6. numerals
    for w in range(0, WPM_MAX + 1, LABEL_STEP):
        a = ang(w)
        tx, ty = pt(cx, cy, R_LABEL, a)
        draw_text_centered(d, (int(tx) - 16, int(ty) - 7, int(tx) + 16, int(ty) + 7),
                           str(w), F_DIAL_TICK, C["WINDOWTEXT"], C["WINDOW"])

    # 7. unit printed on the face, like km/h on a real speedometer; it follows
    #    the selected mode and labels the embedded read-out directly below it
    ux, uy = pt(cx, cy, 27, 270)
    draw_text_centered(d, (int(ux) - 20, int(uy) - 6, int(ux) + 20, int(uy) + 6),
                       mode, F_DIAL_UNIT, C["GRAYTEXT"], C["WINDOW"])

    # 8. peak-hold telltale (the dial's own record of the best WPM)
    radial_bar(d, cx, cy, R_TELLTALE_IN, R_TELLTALE_OUT, ang(peak), 3.0, C["RED"])

    # 9. needle
    a = ang(value_to_wpm(value, mode))
    ux_, uy_ = math_cos_sin(a)

    def np_(r, off):
        return (cx + r * ux_ + off * uy_, cy - r * uy_ + off * ux_)

    half_hub, half_tail = 3.2, 2.4
    d.polygon([np_(R_NEEDLE, 0),
               np_(0, half_hub), np_(-14, half_tail), np_(-14, -half_tail),
               np_(0, -half_hub)], fill=C["WINDOWTEXT"])
    d.ellipse([cx - R_HUB, cy - R_HUB, cx + R_HUB - 1, cy + R_HUB - 1],
              fill=C["BTNFACE"], outline=C["WINDOWTEXT"])
    d.ellipse([cx - 2, cy - 2, cx + 1, cy + 1], fill=C["WINDOWTEXT"])

    # 10. read-out embedded in the dial face. It sits below the needle's lowest
    #     sweep -- the 240 deg arc runs 8 o'clock clockwise to 4 o'clock, so the
    #     needle never points straight down -- and is therefore never occluded.
    panel(d, DIAL_EMBED, C["WINDOW"], "sunken", 2)
    draw_text_centered(d, DIAL_EMBED, str(value), F_EMBED, C["WINDOWTEXT"], C["WINDOW"])


def draw_drum(surf, box, value, ndigits, cell_w=26):
    """A mechanical counter window: sunken panel, one cell per digit."""
    d = ImageDraw.Draw(surf)
    panel(d, box, C["WINDOW"], "sunken", 2)
    ix, iy = box[0] + 4, box[1] + 4
    ih = (box[3] - box[1]) - 8
    s = "%0*d" % (ndigits, value)
    for i, ch in enumerate(s):
        cxx = ix + i * cell_w
        if i:
            vl(d, cxx, iy, iy + ih, C["3DLIGHT"])
            vl(d, cxx + 1, iy, iy + ih, C["WINDOW"])
        draw_text_centered(d, (cxx + 1, iy, cxx + cell_w, iy + ih), ch,
                           F_DRUM, C["WINDOWTEXT"], C["WINDOW"])


def gear_color(gear):
    return C["RED"] if gear == "R" else C["WINDOWTEXT"]


def steel_gear_font(gear):
    return F_GEAR_BIG if len(gear) == 1 else F_GEAR_SM


def draw_gear_value(surf, box, gear, value):
    """One panel reading "<gear> <value>", e.g. "D 62".

    Both parts are left-anchored at fixed offsets so the display does not jitter
    as the digit count changes -- the field simply holds its ground.
    """
    d = ImageDraw.Draw(surf)
    panel(d, box, C["WINDOW"], "sunken", 2)
    draw_text_left(d, box[0] + GV_GEAR_X, box, gear, steel_gear_font(gear),
                   gear_color(gear), C["WINDOW"])
    draw_text_left(d, box[0] + GV_VALUE_X, box, str(value), F_DIGIT_HUGE,
                   C["WINDOWTEXT"], C["WINDOW"])


def draw_gear_gate(surf, box, tmode, gear):
    """Selector gate for the active transmission.

    AUTO shows P R N D only; MANUAL shows R N M1 M2 M3 only. The engaged ratio
    is reversed out.
    """
    d = ImageDraw.Draw(surf)
    panel(d, box, C["WINDOW"], "sunken", 2)
    names = GEARS_BY_TM[tmode]
    n = len(names)
    iw = (box[2] - box[0]) - 4
    slot = min(34, iw // n)
    x0 = box[0] + 2 + (iw - slot * n) // 2
    for i, name in enumerate(names):
        sx = x0 + i * slot
        if name == gear:
            d.rectangle([sx, box[1] + 2, sx + slot - 1, box[3] - 3], fill=C["HIGHLIGHT"])
            fg = C["HIGHLIGHTTEXT"]
        else:
            fg = C["GRAYTEXT"]
        draw_text_centered(d, (sx, box[1] + 2, sx + slot, box[3] - 2), name,
                           F_GATE, fg, C["HIGHLIGHT"] if name == gear else C["WINDOW"])


def draw_content(surf, mode, tmode, gear):
    d = ImageDraw.Draw(surf)
    d.rectangle([0, CONTENT_Y, CLIENT_W - 1, CONTENT_Y + CONTENT_H - 1], fill=C["BTNFACE"])

    value = VAL_KPM if mode == MODE_KPM else VAL_WPM

    # ---- speedometer ------------------------------------------------------
    group_box(d, GB_SPEED, "速度表", F_DIALOG)
    draw_dial(surf, mode, value, VAL_MAX)
    draw_gear_value(surf, GEAR_VALUE, gear, value)

    # ---- odometer ---------------------------------------------------------
    group_box(d, GB_ODO, "里程表", F_DIALOG)
    TEXT.draw(d, (270, ODO_CAP_Y), "ODO:", F_DIALOG, C["BTNTEXT"], bg=C["BTNFACE"])
    draw_drum(surf, ODO_DRUM, VAL_ODO, 7)
    TEXT.draw(d, (270, TRIP_CAP_Y), "TRIP:", F_DIALOG, C["BTNTEXT"], bg=C["BTNFACE"])
    draw_drum(surf, TRIP_DRUM, VAL_TRIP, 4)
    b = push_button(TRIP_RESET[2] - TRIP_RESET[0], TRIP_RESET[3] - TRIP_RESET[1])
    paste_keyed(surf, (TRIP_RESET[0], TRIP_RESET[1]), b)
    draw_text_centered(d, TRIP_RESET, "清零(C)", F_DIALOG, C["BTNTEXT"], C["BTNFACE"])

    # ---- rate -------------------------------------------------------------
    group_box(d, GB_RATE, "速率", F_DIALOG)
    TEXT.draw(d, (270, KPM_CAP_Y), "KPM:", F_DIALOG, C["BTNTEXT"], bg=C["BTNFACE"])
    panel(d, KPM_PANEL, C["WINDOW"], "sunken", 2)
    draw_text_centered(d, KPM_PANEL, str(VAL_KPM), F_READOUT, C["WINDOWTEXT"], C["WINDOW"])
    TEXT.draw(d, (270, MAX_CAP_Y), "MAX:", F_DIALOG, C["BTNTEXT"], bg=C["BTNFACE"])
    panel(d, MAX_PANEL, C["WINDOW"], "sunken", 2)
    draw_text_centered(d, MAX_PANEL, str(VAL_MAX), F_READOUT, C["WINDOWTEXT"], C["WINDOW"])
    draw_gear_gate(surf, GEAR_GATE, tmode, gear)
    b = push_button(PEAK_RESET[2] - PEAK_RESET[0], PEAK_RESET[3] - PEAK_RESET[1])
    paste_keyed(surf, (PEAK_RESET[0], PEAK_RESET[1]), b)
    draw_text_centered(d, PEAK_RESET, "复位(R)", F_DIALOG, C["BTNTEXT"], C["BTNFACE"])


# ---------------------------------------------------------------------------
def render_window(mode=MODE_WPM, tmode=TM_AUTO, gear="D"):
    client = Image.new("RGB", (CLIENT_W, CLIENT_H), C["BTNFACE"])
    dc = ImageDraw.Draw(client)
    draw_menu_bar(dc)
    draw_content(client, mode, tmode, gear)
    draw_status_bar(dc, mode, tmode)

    win = Image.new("RGB", (WIN_W, WIN_H), C["BTNFACE"])
    dw = ImageDraw.Draw(win)
    draw_frame(dw)
    draw_title_bar(win, dw)
    win.paste(client, (CLIENT_X, CLIENT_Y))
    return win


def render_states():
    """Sheet: the combined read-out, then every gear in both transmissions."""
    cell_w, cell_h, cols = 262, 84, 3
    items = ([(TM_AUTO, g) for g in GEARS_BY_TM[TM_AUTO]] +
             [(TM_MANUAL, g) for g in GEARS_BY_TM[TM_MANUAL]])
    rows = (len(items) + cols - 1) // cols
    sheet = Image.new("RGB", (cols * cell_w, rows * cell_h + 96), (192, 192, 192))
    d = ImageDraw.Draw(sheet)

    # top strip: the merged gear+value read-out
    for i, (g, v, m) in enumerate([("D", 62, MODE_WPM), ("R", 62, MODE_WPM),
                                   ("P", 0, MODE_WPM), ("M2", 310, MODE_KPM)]):
        cell = Image.new("RGB", (116, 36), C["BTNFACE"])
        draw_gear_value(cell, (0, 0, 116, 36), g, v)
        sheet.paste(cell, (16 + i * 186, 22))
        d.text((16 + i * 186, 62), "%s %d  (%s)" % (g, v, m), fill=(0, 0, 0))

    for i, (tm, g) in enumerate(items):
        cx = (i % cols) * cell_w
        cy = (i // cols) * cell_h + 98
        rv = Image.new("RGB", (116, 36), C["BTNFACE"])
        draw_gear_value(rv, (0, 0, 116, 36), g, 62)
        sheet.paste(rv, (cx + 8, cy))
        gate = Image.new("RGB", (190, 22), C["BTNFACE"])
        draw_gear_gate(gate, (0, 0, 190, 22), tm, g)
        sheet.paste(gate, (cx + 8, cy + 42))
        d.text((cx + 130, cy + 12), "%s mode" % tm, fill=(0, 0, 0))
    return sheet


def main():
    win = render_window(MODE_WPM, TM_AUTO, "D")
    win.save(os.path.join(OUT, "keyodo_480x320.png"))
    win.resize((WIN_W * 2, WIN_H * 2), Image.NEAREST).save(os.path.join(OUT, "keyodo_2x.png"))
    win.crop((CLIENT_X, CLIENT_Y, CLIENT_X + CLIENT_W, CLIENT_Y + CLIENT_H)) \
       .resize((CLIENT_W * 2, CLIENT_H * 2), Image.NEAREST) \
       .save(os.path.join(OUT, "keyodo_client_2x.png"))

    render_window(MODE_KPM, TM_AUTO, "R").save(os.path.join(OUT, "keyodo_kpm_r.png"))
    render_window(MODE_WPM, TM_AUTO, "P").save(os.path.join(OUT, "keyodo_park_p.png"))
    render_window(MODE_WPM, TM_MANUAL, "M2").save(os.path.join(OUT, "keyodo_manual_m2.png"))
    render_states().save(os.path.join(OUT, "keyodo_states.png"))

    print("window %dx%d  client %dx%d" % (WIN_W, WIN_H, CLIENT_W, CLIENT_H))
    for tm, gs in GEARS_BY_TM.items():
        print("  %-6s gate: %s" % (tm, " ".join(gs)))
    print("wrote", OUT)


if __name__ == "__main__":
    main()
