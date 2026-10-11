/* ===========================================================================
 * theme.c -- pinned Windows 98 SE palette, measured 3D edge recipes.
 *
 * The edge recipes below are not guesswork. tools/probe_gdi_edge.py rendered
 * the real user32!DrawEdge / DrawFrameControl primitives into a DIB and read
 * every pixel back; reports/gdi_edge_probe.txt records, for example:
 *
 *   DrawEdge EDGE_RAISED
 *       top    : y=0 COLOR_3DLIGHT   y=1 COLOR_WINDOW
 *       bottom : y=21 COLOR_3DDKSHADOW  y=20 COLOR_BTNSHADOW
 *
 * so EDGE_RAISED is light/dkshadow on the outside and hilight/shadow inside.
 *
 * Fonts live in metrics.c because they scale with the monitor DPI.
 * ======================================================================== */

#include "theme.h"

/* ------------------------------------------------------------------ inks */

#define INK_FACE       RGB(192, 192, 192)
#define INK_SHADOW     RGB(128, 128, 128)
#define INK_HILIGHT    RGB(255, 255, 255)
#define INK_LIGHT      RGB(223, 223, 223)
#define INK_DKSHADOW   RGB(128, 128, 128)
#define INK_WINDOW     RGB(255, 255, 255)
#define INK_WINDOWTEXT RGB(0, 0, 0)
#define INK_GRAYTEXT   RGB(128, 128, 128)
#define INK_MENU       RGB(192, 192, 192)
#define INK_FRAME      RGB(0, 0, 0)
#define INK_CAPTION    RGB(0, 0, 128)
#define INK_GRADCAP    RGB(166, 202, 240)
#define INK_CAPTEXT    RGB(255, 255, 255)
#define INK_CAPTION_OFF     RGB(128, 128, 128)
#define INK_CAPTEXT_OFF     RGB(192, 192, 192)
#define INK_HIGHLIGHT  RGB(0, 0, 128)
#define INK_HILITETEXT RGB(255, 255, 255)
#define INK_RED        RGB(255, 0, 0)

Theme g_theme;

/* --------------------------------------------------------------- glyphs */
/* Extracted from user32!DrawFrameControl(DFC_CAPTION, ...) at 16x14.
 * 1 = ink (COLOR_BTNTEXT), MSB is the leftmost column. */

const unsigned short kGlyphClose[14] = {
    0x0000, 0x0000, 0x0000, 0x0C30, 0x0660, 0x03C0, 0x0180,
    0x03C0, 0x0660, 0x0C30, 0x0000, 0x0000, 0x0000, 0x0000
};

const unsigned short kGlyphMin[14] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0FC0, 0x0FC0, 0x0000, 0x0000, 0x0000
};

void Theme_Init(void)
{
    Theme *t = &g_theme;

    t->face           = INK_FACE;
    t->shadow         = INK_SHADOW;
    t->hilight        = INK_HILIGHT;
    t->light          = INK_LIGHT;
    t->dkshadow       = INK_DKSHADOW;
    t->window         = INK_WINDOW;
    t->windowtext     = INK_WINDOWTEXT;
    t->btntext        = INK_WINDOWTEXT;
    t->graytext       = INK_GRAYTEXT;
    t->menu           = INK_MENU;
    t->menutext       = INK_WINDOWTEXT;
    t->frame          = INK_FRAME;
    t->caption        = INK_CAPTION;
    t->grad_caption   = INK_GRADCAP;
    t->caption_text   = INK_CAPTEXT;
    t->caption_inactive      = INK_CAPTION_OFF;
    t->caption_inactive_text = INK_CAPTEXT_OFF;
    t->highlight      = INK_HIGHLIGHT;
    t->highlight_text = INK_HILITETEXT;
    t->red            = INK_RED;
}

/* --------------------------------------------------------- primitives */

void Theme_Fill(HDC hdc, const RECT *rc, COLORREF c)
{
    RECT r = *rc;
    HBRUSH b;
    if (r.right <= r.left || r.bottom <= r.top)
        return;
    b = CreateSolidBrush(c);
    FillRect(hdc, &r, b);
    DeleteObject(b);
}

void Theme_HLine(HDC hdc, int x0, int x1, int y, COLORREF c)
{
    RECT r;
    if (x1 <= x0) return;
    r.left = x0; r.top = y; r.right = x1; r.bottom = y + 1;
    Theme_Fill(hdc, &r, c);
}

void Theme_VLine(HDC hdc, int x, int y0, int y1, COLORREF c)
{
    RECT r;
    if (y1 <= y0) return;
    r.left = x; r.top = y0; r.right = x + 1; r.bottom = y1;
    Theme_Fill(hdc, &r, c);
}

void Theme_Rect1(HDC hdc, const RECT *rc, COLORREF c)
{
    Theme_HLine(hdc, rc->left, rc->right, rc->top, c);
    Theme_HLine(hdc, rc->left, rc->right, rc->bottom - 1, c);
    Theme_VLine(hdc, rc->left, rc->top, rc->bottom, c);
    Theme_VLine(hdc, rc->right - 1, rc->top, rc->bottom, c);
}

void Theme_HLineT(HDC hdc, int x0, int x1, int y, int t, COLORREF c)
{
    RECT r;
    if (t < 1 || x1 <= x0) return;
    r.left = x0; r.top = y; r.right = x1; r.bottom = y + t;
    Theme_Fill(hdc, &r, c);
}

void Theme_VLineT(HDC hdc, int x, int y0, int y1, int t, COLORREF c)
{
    RECT r;
    if (t < 1 || y1 <= y0) return;
    r.left = x; r.top = y0; r.right = x + t; r.bottom = y1;
    Theme_Fill(hdc, &r, c);
}

/* ------------------------------------------------------- edge recipes */

typedef struct EdgeInk {
    COLORREF tl0, tl1, br1, br0;
} EdgeInk;

static void edge_ink(int kind, const Theme *t, EdgeInk *e)
{
    switch (kind) {
    case TK_RAISED:
        e->tl0 = t->light;    e->tl1 = t->hilight;
        e->br1 = t->shadow;   e->br0 = t->dkshadow;   break;
    case TK_SUNKEN:
        e->tl0 = t->shadow;   e->tl1 = t->dkshadow;
        e->br1 = t->light;    e->br0 = t->hilight;    break;
    case TK_ETCHED:
        e->tl0 = t->shadow;   e->tl1 = t->hilight;
        e->br1 = t->shadow;   e->br0 = t->hilight;    break;
    case TK_BUMP:
        e->tl0 = t->light;    e->tl1 = t->dkshadow;
        e->br1 = t->light;    e->br0 = t->dkshadow;   break;
    case TK_RAISED_OUTER:
        e->tl0 = t->light;    e->tl1 = t->light;
        e->br1 = t->dkshadow; e->br0 = t->dkshadow;   break;
    case TK_RAISED_INNER:
        e->tl0 = t->hilight;  e->tl1 = t->hilight;
        e->br1 = t->shadow;   e->br0 = t->shadow;     break;
    case TK_SUNKEN_OUTER:
        e->tl0 = t->shadow;   e->tl1 = t->shadow;
        e->br1 = t->hilight;  e->br0 = t->hilight;    break;
    default: /* TK_SUNKEN_INNER */
        e->tl0 = t->dkshadow; e->tl1 = t->dkshadow;
        e->br1 = t->light;    e->br0 = t->light;      break;
    }
}

/* One band of `t` pixels hugging the given rectangle.
 *
 *   outer = 1 : the band sits on the rectangle edge, drawn with the top-left
 *               inks on the top/left sides and the bottom-right inks below
 *   outer = 0 : the band sits `off` pixels inside, with the inner ring inks
 *
 * At bevel 2 the two calls reproduce the measured 96-DPI shape exactly:
 * one 1 px outer ring and one 1 px inner ring.
 */
static void bevel_band(HDC hdc, const RECT *rc, int t, int off,
                       COLORREF tl, COLORREF br)
{
    RECT r;
    int l = rc->left + off, tp = rc->top + off;
    int rr = rc->right - off, b = rc->bottom - off;

    if (t < 1 || rr <= l || b <= tp)
        return;

    r.left = l;       r.top = tp;      r.right = rr;      r.bottom = tp + t;
    Theme_Fill(hdc, &r, tl);
    r.left = l;       r.top = tp;      r.right = l + t;   r.bottom = b;
    Theme_Fill(hdc, &r, tl);
    r.left = l;       r.top = b - t;   r.right = rr;      r.bottom = b;
    Theme_Fill(hdc, &r, br);
    r.left = rr - t;  r.top = tp;      r.right = rr;      r.bottom = b;
    Theme_Fill(hdc, &r, br);
}

void Theme_Bevel(HDC hdc, const RECT *rc,
                 COLORREF tlOuter, COLORREF tlInner,
                 COLORREF brInner, COLORREF brOuter, int bevel)
{
    int tOuter, tInner;

    if (rc->right <= rc->left || rc->bottom <= rc->top)
        return;
    if (bevel < 2) {
        bevel_band(hdc, rc, 1, 0, tlOuter, brOuter);
        return;
    }
    tOuter = bevel / 2;
    tInner = bevel - tOuter;
    bevel_band(hdc, rc, tOuter, 0, tlOuter, brOuter);
    bevel_band(hdc, rc, tInner, tOuter, tlInner, brInner);
}

void Theme_DrawEdge(HDC hdc, const RECT *rc, int kind, int thickness)
{
    EdgeInk e;

    if (rc->right <= rc->left || rc->bottom <= rc->top)
        return;
    edge_ink(kind, &g_theme, &e);

    if (thickness <= 1) {
        bevel_band(hdc, rc, 1, 0, e.tl0, e.br0);
        return;
    }
    Theme_Bevel(hdc, rc, e.tl0, e.tl1, e.br1, e.br0, thickness);
}

void Theme_Panel(HDC hdc, const RECT *rc, COLORREF fill, int kind, int thickness)
{
    RECT in = *rc;
    int t = (thickness < 1) ? 1 : thickness;
    Theme_DrawEdge(hdc, rc, kind, thickness);
    in.left += t;
    in.top += t;
    in.right -= t;
    in.bottom -= t;
    Theme_Fill(hdc, &in, fill);
}

/* ------------------------------------------------------------ glyphs */

void Theme_Glyph(HDC hdc, int x, int y, const unsigned short *mask,
                 COLORREF ink, int px)
{
    int row, col;
    if (px < 1)
        px = 1;
    for (row = 0; row < 14; row++) {
        unsigned int bits = mask[row];
        if (bits == 0)
            continue;
        for (col = 0; col < 16; col++) {
            if (bits & (0x8000u >> col)) {
                RECT r;
                r.left = x + col * px;
                r.top = y + row * px;
                r.right = r.left + px;
                r.bottom = r.top + px;
                Theme_Fill(hdc, &r, ink);
            }
        }
    }
}
