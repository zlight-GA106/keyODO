/* ===========================================================================
 * widgets.c -- hand-drawn Windows 98 controls.
 *
 * The reference renderer centred text on two different things:
 *   horizontal : the ADVANCE width  (so the glyph box is centred, not the ink)
 *   vertical   : the INK box        (so digits sit optically centred)
 * Reproducing that needs the ink box, which GetTextExtentPoint32A cannot give
 * (it returns the font cell height, not the ink). So the ink box is measured
 * once per (font, string) by drawing into a scratch 32bpp DIB and scanning it,
 * then cached -- the set of distinct strings is tiny.
 * ======================================================================== */

#include "widgets.h"
#include "theme.h"

#include <string.h>

/* ======================================================================
 * ink metrics with cache
 * ==================================================================== */

#define SCRATCH_W   320
#define SCRATCH_H   64
#define INK_CACHE   96
#define INK_MAXLEN  32

typedef struct InkEntry {
    HFONT font;
    char  text[INK_MAXLEN];
    int   top;      /* ink top, relative to the text cell top */
    int   height;
} InkEntry;

static InkEntry s_cache[INK_CACHE];
static int      s_cacheCount = 0;
static int      s_cacheNext  = 0;

static HDC       s_dc     = NULL;
static HBITMAP   s_bmp    = NULL;
static HGDIOBJ   s_oldBmp = NULL;
static unsigned int *s_bits = NULL;

typedef struct Bmi {
    BITMAPINFOHEADER hdr;
    DWORD            colors[3];
} Bmi;

static int scratch_ready(void)
{
    Bmi bmi;

    if (s_dc != NULL)
        return 1;

    s_dc = CreateCompatibleDC(NULL);
    if (s_dc == NULL)
        return 0;

    ZeroMemory(&bmi, sizeof(bmi));
    bmi.hdr.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.hdr.biWidth       = SCRATCH_W;
    bmi.hdr.biHeight      = -SCRATCH_H;   /* top-down */
    bmi.hdr.biPlanes      = 1;
    bmi.hdr.biBitCount    = 32;
    bmi.hdr.biCompression = BI_RGB;

    s_bmp = CreateDIBSection(s_dc, (BITMAPINFO *)&bmi, DIB_RGB_COLORS,
                             (void **)&s_bits, NULL, 0);
    if (s_bmp == NULL) {
        DeleteDC(s_dc);
        s_dc = NULL;
        return 0;
    }
    s_oldBmp = SelectObject(s_dc, s_bmp);
    return 1;
}

static void ink_measure(HFONT f, const char *text, int *top, int *height)
{
    HGDIOBJ oldFont;
    RECT all;
    HBRUSH white;
    int len, x, y, minY = SCRATCH_H, maxY = -1;

    *top = 0;
    *height = 0;

    len = lstrlenA(text);
    if (len <= 0 || len >= INK_MAXLEN)
        len = (len >= INK_MAXLEN) ? INK_MAXLEN - 1 : 0;
    if (len == 0)
        return;

    if (!scratch_ready())
        return;

    /* clear to white */
    all.left = 0; all.top = 0; all.right = SCRATCH_W; all.bottom = SCRATCH_H;
    white = (HBRUSH)GetStockObject(WHITE_BRUSH);
    FillRect(s_dc, &all, white);

    oldFont = SelectObject(s_dc, f);
    SetBkMode(s_dc, TRANSPARENT);
    SetTextColor(s_dc, RGB(0, 0, 0));
    TextOutA(s_dc, 0, 0, text, len);
    SelectObject(s_dc, oldFont);
    GdiFlush();

    for (y = 0; y < SCRATCH_H; y++) {
        const unsigned int *row = s_bits + (size_t)y * SCRATCH_W;
        for (x = 0; x < SCRATCH_W; x++) {
            if ((row[x] & 0x00FFFFFFu) != 0x00FFFFFFu) {
                if (y < minY) minY = y;
                if (y > maxY) maxY = y;
            }
        }
    }
    if (maxY < 0)
        return;
    *top = minY;
    *height = maxY - minY + 1;
}

static const InkEntry *ink_get(HFONT f, const char *text)
{
    int i;
    InkEntry *e;

    for (i = 0; i < s_cacheCount; i++) {
        if (s_cache[i].font == f && lstrcmpA(s_cache[i].text, text) == 0)
            return &s_cache[i];
    }
    if (s_cacheCount < INK_CACHE) {
        e = &s_cache[s_cacheCount++];
    } else {
        e = &s_cache[s_cacheNext];
        s_cacheNext = (s_cacheNext + 1) % INK_CACHE;
    }
    e->font = f;
    lstrcpynA(e->text, text, INK_MAXLEN);
    ink_measure(f, text, &e->top, &e->height);
    return e;
}

/* ======================================================================
 * text
 * ==================================================================== */

int Widget_TextWidth(HDC hdc, HFONT f, const char *s)
{
    HGDIOBJ old = SelectObject(hdc, f);
    SIZE sz;
    sz.cx = 0; sz.cy = 0;
    GetTextExtentPoint32A(hdc, s, lstrlenA(s), &sz);
    SelectObject(hdc, old);
    return sz.cx;
}

void Widget_TextAt(HDC hdc, int x, int y, const char *s,
                   HFONT f, COLORREF fg, COLORREF bg)
{
    HGDIOBJ old = SelectObject(hdc, f);
    SetBkMode(hdc, OPAQUE);
    SetBkColor(hdc, bg);
    SetTextColor(hdc, fg);
    TextOutA(hdc, x, y, s, lstrlenA(s));
    SelectObject(hdc, old);
}

void Widget_TextAtClear(HDC hdc, int x, int y, const char *s,
                        HFONT f, COLORREF fg)
{
    HGDIOBJ old = SelectObject(hdc, f);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, fg);
    TextOutA(hdc, x, y, s, lstrlenA(s));
    SelectObject(hdc, old);
}

void Widget_TextLeft(HDC hdc, int x, const RECT *box, const char *s,
                     HFONT f, COLORREF fg, COLORREF bg)
{
    const InkEntry *e = ink_get(f, s);
    int y = box->top + ((box->bottom - box->top) - e->height) / 2 - e->top;
    Widget_TextAt(hdc, x, y, s, f, fg, bg);
}

void Widget_TextCentered(HDC hdc, const RECT *box, const char *s,
                         HFONT f, COLORREF fg, COLORREF bg)
{
    const InkEntry *e = ink_get(f, s);
    int w = Widget_TextWidth(hdc, f, s);
    int x = box->left + ((box->right - box->left) - w) / 2;
    int y = box->top + ((box->bottom - box->top) - e->height) / 2 - e->top;
    Widget_TextAt(hdc, x, y, s, f, fg, bg);
}

/* ======================================================================
 * chrome
 * ==================================================================== */

void Widget_GroupBox(HDC hdc, const RECT *box, const char *caption)
{
    const Theme *t = &g_theme;
    int l = box->left, tp = box->top;
    int r = box->right - 1, b = box->bottom - 1;
    int tw = Widget_TextWidth(hdc, t->f_ui, caption);
    int gap0 = l + 9;
    int gap1 = gap0 + tw + 4;

    /* top edge, split by the caption gap */
    Theme_HLine(hdc, l + 1, gap0, tp, t->shadow);
    Theme_HLine(hdc, gap1, r, tp, t->shadow);
    /* left / bottom / right */
    Theme_VLine(hdc, l, tp + 1, b + 1, t->shadow);
    Theme_HLine(hdc, l + 1, r + 1, b, t->window);
    Theme_VLine(hdc, r, tp + 1, b + 1, t->window);
    /* corners */
    SetPixelV(hdc, l, tp, t->shadow);
    SetPixelV(hdc, r, tp, t->shadow);
    SetPixelV(hdc, l, b, t->window);
    SetPixelV(hdc, r, b, t->window);

    Widget_TextAt(hdc, gap0 + 2, tp - 5, caption, t->f_ui, t->btntext, t->face);
}

void Widget_PushButton(HDC hdc, const RECT *box, const char *caption, int pressed)
{
    const Theme *t = &g_theme;
    int l = box->left, tp = box->top;
    int r = box->right - 1, b = box->bottom - 1;
    COLORREF o1, i1, i2, o2;

    if (pressed) {
        o1 = t->dkshadow; i1 = t->shadow; i2 = t->light; o2 = t->hilight;
    } else {
        o1 = t->hilight;  i1 = t->light;  i2 = t->shadow; o2 = t->dkshadow;
    }

    /* face */
    Theme_Fill(hdc, box, t->face);
    /* two-pixel bevel: outer then inner, measured from DFCS_BUTTONPUSH */
    Theme_HLine(hdc, l, r + 1, tp, o1);
    Theme_HLine(hdc, l, r + 1, tp + 1, i1);
    Theme_VLine(hdc, l, tp, b + 1, o1);
    Theme_VLine(hdc, l + 1, tp, b + 1, i1);
    Theme_HLine(hdc, l, r + 1, b, o2);
    Theme_HLine(hdc, l, r + 1, b - 1, i2);
    Theme_VLine(hdc, r, tp, b + 1, o2);
    Theme_VLine(hdc, r - 1, tp, b + 1, i2);

    if (caption != NULL && caption[0] != 0) {
        RECT txt = *box;
        if (pressed) {
            txt.left += 1;
            txt.top += 1;
        }
        Widget_TextCentered(hdc, &txt, caption, t->f_ui, t->btntext, t->face);
    }
}

void Widget_CaptionButton(HDC hdc, const RECT *box,
                          const unsigned short *glyph, int pressed)
{
    const Theme *t = &g_theme;
    int l = box->left, tp = box->top;
    int r = box->right - 1, b = box->bottom - 1;
    COLORREF o1, i1, i2, o2;

    if (pressed) {
        o1 = t->dkshadow; i1 = t->shadow; i2 = t->light; o2 = t->hilight;
    } else {
        o1 = t->hilight;  i1 = t->light;  i2 = t->shadow; o2 = t->dkshadow;
    }

    Theme_Fill(hdc, box, t->face);
    Theme_HLine(hdc, l, r + 1, tp, o1);
    Theme_HLine(hdc, l, r + 1, tp + 1, i1);
    Theme_VLine(hdc, l, tp, b + 1, o1);
    Theme_VLine(hdc, l + 1, tp, b + 1, i1);
    Theme_HLine(hdc, l, r + 1, b, o2);
    Theme_HLine(hdc, l, r + 1, b - 1, i2);
    Theme_VLine(hdc, r, tp, b + 1, o2);
    Theme_VLine(hdc, r - 1, tp, b + 1, i2);

    Theme_Glyph(hdc, l, tp, glyph, t->btntext);
}

/* ======================================================================
 * read-outs
 * ==================================================================== */

void Widget_Readout(HDC hdc, const RECT *box, const char *text, HFONT f)
{
    const Theme *t = &g_theme;
    Theme_Panel(hdc, box, t->window, TK_SUNKEN, 2);
    Widget_TextCentered(hdc, box, text, f, t->windowtext, t->window);
}

void Widget_Drum(HDC hdc, const RECT *box, const char *digits, int cellW)
{
    const Theme *t = &g_theme;
    int ix, iy, ih, i, n;
    RECT cell;

    Theme_Panel(hdc, box, t->window, TK_SUNKEN, 2);

    ix = box->left + 4;
    iy = box->top + 4;
    ih = (box->bottom - box->top) - 8;
    n = lstrlenA(digits);

    for (i = 0; i < n; i++) {
        int cxx = ix + i * cellW;
        char one[2];
        if (i > 0) {
            Theme_VLine(hdc, cxx, iy, iy + ih, t->light);
            Theme_VLine(hdc, cxx + 1, iy, iy + ih, t->window);
        }
        one[0] = digits[i];
        one[1] = 0;
        cell.left = cxx + 1;
        cell.top = iy;
        cell.right = cxx + cellW;
        cell.bottom = iy + ih;
        Widget_TextCentered(hdc, &cell, one, t->f_drum, t->windowtext, t->window);
    }
}

void Widget_Shutdown(void)
{
    if (s_dc != NULL) {
        if (s_oldBmp != NULL)
            SelectObject(s_dc, s_oldBmp);
        if (s_bmp != NULL)
            DeleteObject(s_bmp);
        DeleteDC(s_dc);
        s_dc = NULL;
        s_bmp = NULL;
        s_bits = NULL;
    }
    s_cacheCount = 0;
    s_cacheNext = 0;
}
