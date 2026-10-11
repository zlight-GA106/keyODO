/* ===========================================================================
 * keyodo_layout.h -- KeyODO layout constants
 *
 * Windows 98 SE / Win32 + GDI / ANSI build (do NOT define UNICODE)
 * Base size 480 x 320 @ 96 DPI  ->  client area 474 x 296
 *
 * Every number here is the same number used by mockup/keyodo_mockup.py, so the
 * design image and the C code cannot drift apart.
 *
 * ASCII ONLY. This header is compiled by an ANSI toolchain whose code page is
 * 936; a UTF-8 string literal here would be misread. All user-visible Chinese
 * text lives in res/keyodo.rc and is loaded with LoadStringA.
 *
 * COORDINATES
 *   RECT_* macros are CLIENT coordinates (origin = client top-left), i.e. the
 *   area inside the 3 px frame and below the 18 px caption.
 *   Right/bottom edges are EXCLUSIVE, exactly like Win32 RECT.
 *
 *   The window is 480 x 320 and draws its own chrome, so window coordinates
 *   are client coordinates offset by (KW_FRAME, KW_FRAME + KW_CAPTION_H).
 * ======================================================================== */

#ifndef KEYODO_LAYOUT_H
#define KEYODO_LAYOUT_H

/* -------------------------------------------------------- window metrics */
#define KW_FRAME            3        /* frame thickness, all four sides      */
#define KW_CAPTION_H        18       /* caption bar height                   */
#define KW_CLIENT_W         474
#define KW_CLIENT_H         296
#define KW_WIN_W            (KW_CLIENT_W + 2 * KW_FRAME)                /* 480 */
#define KW_WIN_H            (KW_CLIENT_H + 2 * KW_FRAME + KW_CAPTION_H) /* 320 */
#define KW_MENUBAR_H        20
#define KW_STATUSBAR_H      20
#define KW_CONTENT_Y        KW_MENUBAR_H                     /* 20  */
#define KW_CONTENT_H        256                              /* 296-20-20 */

/* caption buttons: 16x14, sitting 2 px inside the frame */
#define KW_CAPBTN_W         16
#define KW_CAPBTN_H         14
#define KW_CAPBTN_Y         (KW_FRAME + 2)                   /* 5   */
#define KW_CAPBTN_CLOSE_X   (KW_WIN_W - KW_FRAME - 2 - KW_CAPBTN_W)  /* 459 */
#define KW_CAPBTN_MIN_X     (KW_CAPBTN_CLOSE_X - 2 - KW_CAPBTN_W)    /* 441 */
#define KW_ICON_X           (KW_FRAME + 3)                   /* 6   */
#define KW_ICON_Y           (KW_FRAME + 1)                   /* 4   */
#define KW_TITLE_X          (KW_FRAME + 22)                  /* 25  */
#define KW_TITLE_Y          (KW_FRAME + 4)                   /* 7   */

/* ------------------------------------------------------------- groupboxes */
/* A group box caption is drawn at (left+9, top-5). The boxes therefore start
 * at least 7 px below the menu bar or the captions collide with it. */
#define KW_BOX_TOP          (KW_CONTENT_Y + 7)               /* 27  */
#define KW_BOX_BOT          (KW_CONTENT_Y + 251)             /* 271 */

#define RECT_GB_SPEED       {   4, KW_BOX_TOP, 256, KW_BOX_BOT }        /* 252x244 */
#define RECT_GB_ODO         { 260, KW_BOX_TOP, 470, KW_BOX_TOP + 114 }  /* 210x114 */
#define RECT_GB_RATE        { 260, KW_BOX_TOP + 124, 470, KW_BOX_BOT }  /* 210x120 */

#define GB_CAPTION_DX       9        /* caption inset from the left edge      */
#define GB_CAPTION_DY       5        /* caption sits this far ABOVE the frame */

/* ------------------------------------------------------------ speedometer */
#define RECT_DIAL_PANEL     {  38, KW_CONTENT_Y + 20, 222, KW_CONTENT_Y + 204 }
#define DIAL_CX             (130)                /* 184x184 panel centre      */
#define DIAL_CY             (KW_CONTENT_Y + 112) /* = 132                     */

#define R_PANEL             92       /* square instrument panel, half-side    */
#define R_BEZEL             86       /* raised circular bezel                 */
#define R_FACE              78       /* white dial face                       */
#define R_RED_IN            74       /* redline band, inner radius            */
#define R_RED_OUT           78       /* redline band, outer radius            */
#define R_TICK_OUT          74
#define R_MAJ_IN            64       /* major tick, every 20 WPM, 2 px wide   */
#define R_MED_IN            67       /* medium tick, every 10 WPM, 2 px       */
#define R_MIN_IN            70       /* minor tick, every 5 WPM, 1 px         */
#define R_LABEL             52       /* numeral ring                          */
#define R_NEEDLE            62       /* needle tip                            */
#define R_NEEDLE_TAIL      (-14)     /* counterweight behind the hub          */
#define R_HUB               7        /* hub disc                              */
#define R_TELLTALE_IN       62       /* peak-hold marker                      */
#define R_TELLTALE_OUT      76
#define R_UNIT_LABEL        27       /* printed unit, straight down (270 deg) */

#define WPM_MAX             120
#define WPM_REDLINE_FROM    100
#define WPM_LABEL_STEP      20
#define WPM_SWEEP_START     210.0    /* 0 WPM sits at 8 o'clock               */
#define WPM_SWEEP_TOTAL     240.0    /* clockwise to 4 o'clock at 120 WPM     */

/* ang(w) = WPM_SWEEP_START - WPM_SWEEP_TOTAL * (w / WPM_MAX)
 * 0 = 3 o'clock, counter-clockwise positive.
 *   x = cx + (int)(r * cos(rad));   y = cy - (int)(r * sin(rad));           */

/* Read-out embedded in the dial face, directly under the printed unit label.
 * It sits below the needle's lowest sweep: the 240 deg arc runs 8 o'clock
 * clockwise to 4 o'clock, so the needle never points straight down. */
#define RECT_DIAL_EMBED     {  88, KW_CONTENT_Y + 151, 172, KW_CONTENT_Y + 173 }
#define DIAL_EMBED_W        84
#define DIAL_EMBED_H        22

/* Below-dial read-out. ONE panel holding the gear then the value, so it reads
 * "D 62" the way a car cluster does. Both parts are left-anchored at fixed
 * offsets, so the display does not jitter when the digit count changes. */
#define RECT_GEAR_VALUE     {  72, KW_CONTENT_Y + 208, 188, KW_CONTENT_Y + 244 }
#define GV_W                116
#define GV_H                36
#define GV_GEAR_X           18       /* gear anchor, from the panel left edge   */
#define GV_VALUE_X          50       /* value anchor, from the panel left edge  */

/* --------------------------------------------------------------- odometer */
#define ODO_CAP_Y           (KW_CONTENT_Y + 19)              /* 39  */
#define RECT_ODO_DRUM       { 270, KW_CONTENT_Y + 33, 460, KW_CONTENT_Y + 63 }
#define TRIP_CAP_Y          (KW_CONTENT_Y + 69)              /* 89  */
#define RECT_TRIP_DRUM      { 348, KW_CONTENT_Y + 83, 460, KW_CONTENT_Y + 113 }
#define RECT_TRIP_RESET     { 270, KW_CONTENT_Y + 85, 340, KW_CONTENT_Y + 111 }

#define ODO_DIGITS          7        /* total keystrokes, leading zeros       */
#define TRIP_DIGITS         4        /* trip keystrokes, leading zeros        */
#define DRUM_CELL_W         26       /* digit cell pitch                      */
#define DRUM_PAD            2        /* inner padding inside the sunken frame */

/* ------------------------------------------------------------------- rate */
#define KPM_CAP_Y           (KW_CONTENT_Y + 146)             /* 166 */
#define RECT_KPM_PANEL      { 368, KW_CONTENT_Y + 140, 460, KW_CONTENT_Y + 164 }
#define MAX_CAP_Y           (KW_CONTENT_Y + 174)             /* 194 */
#define RECT_MAX_PANEL      { 368, KW_CONTENT_Y + 168, 460, KW_CONTENT_Y + 192 }
#define RECT_GEAR_GATE      { 270, KW_CONTENT_Y + 196, 460, KW_CONTENT_Y + 218 }
#define RECT_PEAK_RESET     { 270, KW_CONTENT_Y + 221, 382, KW_CONTENT_Y + 247 }

/* --------------------------------------------------------------- gearbox */
/* The selector gate shows ONLY the ratios belonging to the active
 * transmission, and the gear name is a plain string so the two sets never
 * have to share an index.
 *
 * Gear is DISPLAY ONLY: the UI never decides a gear, it renders what the
 * statistics module reports. */
#define KTM_AUTO            0        /* automatic: park / reverse / neutral / drive */
#define KTM_MANUAL          1        /* manual: reverse / neutral / three ratios    */

#define KTM_GEARS_AUTO      { "P", "R", "N", "D" }
#define KTM_GEARS_MANUAL    { "R", "N", "M1", "M2", "M3" }
#define KTM_COUNT_AUTO      4
#define KTM_COUNT_MANUAL    5

/* Ratio semantics:
 *   P  idle, no typing activity        R  Backspace
 *   N  Space                           D  ordinary character keys
 *   M1 M2 M3  manual ratios, selected from the current rate                */
#define KG_RED_ONLY         'R'      /* reverse prints in red, others black     */

/* ---------------------------------------------------------- read-out mode */
/* Drives the printed in-dial unit, the embedded read-out and the combined
 * read-out below the dial. The needle always shows speed: when KPM is
 * selected the needle is driven by KPM / 5. */
#define KDM_WPM             0
#define KDM_KPM             1
#define KDM_UNIT_WPM        "WPM"
#define KDM_UNIT_KPM        "KPM"

/* -------------------------------------------------------------- statusbar */
#define SB_PANE3_W          118
#define SB_PANE2_W          116
#define SB_PANE3_X          (KW_CLIENT_W - 2 - SB_PANE3_W)             /* 354 */
#define SB_PANE2_X          (SB_PANE3_X - 2 - SB_PANE2_W)              /* 236 */
#define SB_TEXT_DX          3
#define SB_TEXT_DY          4
#define SB_DIVIDER_W        2        /* 1 px shadow + 1 px highlight           */

/* C89 cannot assign a braced initialiser to an already-declared struct, so the
 * RECT_* macros above (which are braced, so they can also be used directly in
 * a declaration) are applied to an existing RECT through this helper. Requires
 * <windows.h> to be included first, which every translation unit does. */
#define RECT_SET(dst, init) \
    do { static const RECT kodo_rect_ = init; (dst) = kodo_rect_; } while (0)

/* ------------------------------------------------------ Windows Standard */
/* KeyODO pins the classic palette so the picture is identical on Windows 98
 * SE and on a modern Windows. See DESIGN.md section 4.3. */
#define W98_BTNFACE         RGB(192,192,192)
#define W98_BTNSHADOW       RGB(128,128,128)
#define W98_BTNHIGHLIGHT    RGB(255,255,255)
#define W98_3DLIGHT         RGB(223,223,223)
#define W98_3DDKSHADOW      RGB(128,128,128)
#define W98_WINDOW          RGB(255,255,255)
#define W98_WINDOWTEXT      RGB(0,0,0)
#define W98_GRAYTEXT        RGB(128,128,128)
#define W98_ACTIVECAPTION   RGB(0,0,128)
#define W98_GRADACTIVE      RGB(166,202,240)   /* Win98 caption gradient end   */
#define W98_HIGHLIGHT       RGB(0,0,128)

/* Instrument inks. These are fixed and never follow the colour scheme. */
#define KI_RED              RGB(255,0,0)
#define KI_DARKRED          RGB(128,0,0)

#endif /* KEYODO_LAYOUT_H */
