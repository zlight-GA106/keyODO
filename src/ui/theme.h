/* ===========================================================================
 * theme.h -- the Windows 98 SE "Windows Standard" look, pinned.
 *
 * WHY PINNED AND NOT GetSysColor()
 *   On a real Windows 98 SE the classic scheme IS the system scheme, so both
 *   approaches are identical. On a modern Windows the system scheme is the
 *   flat light theme, and calling DrawEdge/DrawFrameControl there would paint
 *   Windows 11 grey inside a Windows 98 layout. The reference screenshots are
 *   the visual specification, so KeyODO pins the classic palette and draws
 *   every 3D edge itself using the recipes measured from real GDI
 *   (reports/gdi_edge_probe.txt).
 *
 * Fonts are NOT here: they depend on the monitor DPI, so they live in
 * metrics.h alongside the rest of the device-dependent values.
 * ======================================================================== */

#ifndef KEYODO_THEME_H
#define KEYODO_THEME_H

#include <windows.h>

/* Edge kinds. The first four reproduce EDGE_*, the last four the single-ring
 * BDR_* forms. See reports/gdi_edge_probe.txt for the measured basis. */
enum {
    TK_RAISED = 0,
    TK_SUNKEN,
    TK_ETCHED,
    TK_BUMP,
    TK_RAISED_OUTER,
    TK_RAISED_INNER,
    TK_SUNKEN_OUTER,
    TK_SUNKEN_INNER
};

typedef struct Theme {
    COLORREF face;          /* COLOR_3DFACE      #C0C0C0 */
    COLORREF shadow;        /* COLOR_3DSHADOW    #808080 */
    COLORREF hilight;       /* COLOR_3DHILIGHT   #FFFFFF */
    COLORREF light;         /* COLOR_3DLIGHT     #DFDFDF */
    COLORREF dkshadow;      /* COLOR_3DDKSHADOW  #808080 */
    COLORREF window;        /* COLOR_WINDOW      #FFFFFF */
    COLORREF windowtext;
    COLORREF btntext;
    COLORREF graytext;
    COLORREF menu;
    COLORREF menutext;
    COLORREF frame;         /* COLOR_WINDOWFRAME #000000 */
    COLORREF caption;       /* COLOR_ACTIVECAPTION        #000080 */
    COLORREF grad_caption;  /* COLOR_GRADIENTACTIVECAPTION #A6CAF0 */
    COLORREF caption_text;
    COLORREF caption_inactive;       /* COLOR_INACTIVECAPTION      #808080 */
    COLORREF caption_inactive_text;  /* COLOR_INACTIVECAPTIONTEXT  #C0C0C0 */
    COLORREF highlight;     /* COLOR_HIGHLIGHT   #000080 */
    COLORREF highlight_text;
    COLORREF red;
} Theme;

extern Theme g_theme;

void Theme_Init(void);

/* exact single-pixel primitives */
void Theme_HLine(HDC hdc, int x0, int x1, int y, COLORREF c);
void Theme_VLine(HDC hdc, int x, int y0, int y1, COLORREF c);
/* thickness variants, for the 1 px frame of a group box at scaled DPI */
void Theme_HLineT(HDC hdc, int x0, int x1, int y, int t, COLORREF c);
void Theme_VLineT(HDC hdc, int x, int y0, int y1, int t, COLORREF c);
void Theme_Fill(HDC hdc, const RECT *rc, COLORREF c);
void Theme_Rect1(HDC hdc, const RECT *rc, COLORREF c);

/* measured DrawEdge replacement; `thickness` is Metrics.bevel at the caller */
void Theme_DrawEdge(HDC hdc, const RECT *rc, int kind, int thickness);
void Theme_Panel(HDC hdc, const RECT *rc, COLORREF fill, int kind, int thickness);

/* Two-ring bevel with explicit inks, used by the push and caption buttons.
 * At bevel 2 this is exactly the 96-DPI shape: 1 px outer ring, 1 px inner. */
void Theme_Bevel(HDC hdc, const RECT *rc,
                 COLORREF tlOuter, COLORREF tlInner,
                 COLORREF brInner, COLORREF brOuter, int bevel);

/* caption-button glyph masks, 16 wide x 14 tall, MSB = leftmost pixel */
extern const unsigned short kGlyphClose[14];
extern const unsigned short kGlyphMin[14];

/* `px` is the size of one mask cell in device pixels (Metrics.bevel at 100%). */
void Theme_Glyph(HDC hdc, int x, int y, const unsigned short *mask,
                 COLORREF ink, int px);

#endif /* KEYODO_THEME_H */
