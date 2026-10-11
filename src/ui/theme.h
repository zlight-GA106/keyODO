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

    HFONT f_ui;             /* SimSun 9pt  (12 px) -- menu, labels, buttons */
    HFONT f_ui_bold;        /* SimSun 9pt bold -- caption text              */
    HFONT f_dial;           /* Arial Bold 8pt  (11 px) -- tick numerals     */
    HFONT f_drum;           /* Arial Bold 15pt (20 px) -- odometer drum     */
    HFONT f_readout;        /* Arial Bold 17 px -- KPM / MAX / embedded     */
    HFONT f_huge;           /* Arial Bold 22pt (29 px) -- main read-out     */
    HFONT f_gear_sm;        /* Arial Bold 20 px -- "M1".."M3"               */
    HFONT f_gate;           /* Arial Bold 14 px -- selector gate            */
} Theme;

extern Theme g_theme;

void Theme_Init(void);
void Theme_Done(void);

/* exact single-pixel primitives */
void Theme_HLine(HDC hdc, int x0, int x1, int y, COLORREF c);
void Theme_VLine(HDC hdc, int x, int y0, int y1, COLORREF c);
void Theme_Fill(HDC hdc, const RECT *rc, COLORREF c);
void Theme_Rect1(HDC hdc, const RECT *rc, COLORREF c);

/* measured DrawEdge replacement */
void Theme_DrawEdge(HDC hdc, const RECT *rc, int kind, int thickness);
void Theme_Panel(HDC hdc, const RECT *rc, COLORREF fill, int kind, int thickness);

/* caption-button glyph masks, 16 wide x 14 tall, MSB = leftmost pixel */
extern const unsigned short kGlyphClose[14];
extern const unsigned short kGlyphMin[14];

void Theme_Glyph(HDC hdc, int x, int y, const unsigned short *mask, COLORREF ink);

#endif /* KEYODO_THEME_H */
