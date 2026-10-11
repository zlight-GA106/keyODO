/* ===========================================================================
 * theme.c -- pinned Windows 98 SE palette, measured 3D edge recipes, fonts.
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
 * ======================================================================== */

#include "theme.h"

#include <string.h>

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

/* ---------------------------------------------------------------- fonts */

/* SimSun ("Song Ti") in code page 936. Written as escapes so this source file
 * stays pure ASCII and cannot be corrupted by an editor re-encoding it. */
static const char kFaceUI[] = "\xcb\xce\xcc\xe5";

static HFONT make_font(const char *face, int px, int weight, BYTE quality)
{
    return CreateFontA(-px, 0, 0, 0, weight, FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       quality, DEFAULT_PITCH, face);
}

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

    /* Sizes mirror mockup/win98kit.py exactly: the design render and the
     * program must not drift apart. */
    t->f_ui       = make_font(kFaceUI, 12, FW_NORMAL, DEFAULT_QUALITY);
    t->f_ui_bold  = make_font(kFaceUI, 12, FW_BOLD,   DEFAULT_QUALITY);
    t->f_dial     = make_font("Arial", 11, FW_BOLD, NONANTIALIASED_QUALITY);
    t->f_drum     = make_font("Arial", 20, FW_BOLD, NONANTIALIASED_QUALITY);
    t->f_readout  = make_font("Arial", 17, FW_BOLD, NONANTIALIASED_QUALITY);
    t->f_huge     = make_font("Arial", 29, FW_BOLD, ANTIALIASED_QUALITY);
    t->f_gear_sm  = make_font("Arial", 20, FW_BOLD, NONANTIALIASED_QUALITY);
    t->f_gate     = make_font("Arial", 14, FW_BOLD, NONANTIALIASED_QUALITY);
}

void Theme_Done(void)
{
    HFONT *fonts = &g_theme.f_ui;
    int i;
    for (i = 0; i < 8; i++) {
        if (fonts[i] != NULL) {
            DeleteObject(fonts[i]);
            fonts[i] = NULL;
        }
    }
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

void Theme_DrawEdge(HDC hdc, const RECT *rc, int kind, int thickness)
{
    EdgeInk e;
    int l = rc->left, t = rc->top;
    int r = rc->right - 1, b = rc->bottom - 1;

    if (rc->right <= rc->left || rc->bottom <= rc->top)
        return;
    edge_ink(kind, &g_theme, &e);

    if (thickness <= 1) {
        Theme_HLine(hdc, l, r + 1, t, e.tl0);
        Theme_VLine(hdc, l, t, b + 1, e.tl0);
        Theme_HLine(hdc, l, r + 1, b, e.br0);
        Theme_VLine(hdc, r, t, b + 1, e.br0);
        return;
    }

    Theme_HLine(hdc, l, r + 1, t, e.tl0);
    Theme_HLine(hdc, l, r + 1, t + 1, e.tl1);
    Theme_VLine(hdc, l, t, b + 1, e.tl0);
    Theme_VLine(hdc, l + 1, t, b + 1, e.tl1);

    Theme_HLine(hdc, l, r + 1, b, e.br0);
    Theme_HLine(hdc, l, r + 1, b - 1, e.br1);
    Theme_VLine(hdc, r, t, b + 1, e.br0);
    Theme_VLine(hdc, r - 1, t, b + 1, e.br1);
}

void Theme_Panel(HDC hdc, const RECT *rc, COLORREF fill, int kind, int thickness)
{
    RECT in = *rc;
    Theme_DrawEdge(hdc, rc, kind, thickness);
    in.left += thickness;
    in.top += thickness;
    in.right -= thickness;
    in.bottom -= thickness;
    Theme_Fill(hdc, &in, fill);
}

/* ------------------------------------------------------------ glyphs */

void Theme_Glyph(HDC hdc, int x, int y, const unsigned short *mask, COLORREF ink)
{
    int row, col;
    for (row = 0; row < 14; row++) {
        unsigned int bits = mask[row];
        if (bits == 0)
            continue;
        for (col = 0; col < 16; col++) {
            if (bits & (0x8000u >> col))
                SetPixelV(hdc, x + col, y + row, ink);
        }
    }
}
