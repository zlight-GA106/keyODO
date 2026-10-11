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
 *
 * Every size goes through Metrics, so the same code paints a correctly scaled
 * control at 100%, 125%, 150% and 200%.
 * ======================================================================== */

#include "widgets.h"
#include "theme.h"
#include "metrics.h"

#include <string.h>

/* ======================================================================
 * ink metrics with cache
 * ==================================================================== */

/* Big enough for the widest string at 200% scaling. */
#define SCRATCH_W   768
#define SCRATCH_H   192
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
    if (len <= 0)
        return;
    if (len >= INK_MAXLEN)
        len = INK_MAXLEN - 1;

    if (!scratch_ready())
        return;

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

/* Copies `text` without its mnemonic markers and reports where the mnemonic
 * landed, so measuring and drawing always agree. */
static void strip_mnemonic(const char *text, char *out, int cch, int *mnemonic)
{
    int i = 0;
    const char *p;
    *mnemonic = -1;
    for (p = text; *p != 0 && i < cch - 1; p++) {
        if (*p == '&') {
            if (p[1] == '&') {
                out[i++] = '&';
                p++;
                continue;
            }
            *mnemonic = i;
            continue;
        }
        out[i++] = *p;
    }
    out[i] = 0;
}

int Widget_TextWidthMnemonic(HDC hdc, HFONT f, const char *s)
{
    char clean[128];
    int mnemonic;
    strip_mnemonic(s, clean, sizeof(clean), &mnemonic);
    return Widget_TextWidth(hdc, f, clean);
}

/* Draws `text` with the mnemonic underlined. `&` marks the mnemonic and is not
 * shown; `&&` is a literal ampersand. Returns the width drawn.
 *
 * Windows writes captions two ways -- "&File" in English, and the CJK
 * convention that puts the letter in brackets -- and both mean the same thing,
 * so both go through here and come out as the caption plus an underline. */
int Widget_TextMnemonic(HDC hdc, int x, int y, const char *text,
                        HFONT f, COLORREF fg, COLORREF bg)
{
    char clean[128];
    int mnemonic = -1;
    int i, hasMnemonic;
    HGDIOBJ old;
    SIZE sz, pre, one;
    char mc[2];

    strip_mnemonic(text, clean, sizeof(clean), &mnemonic);
    i = lstrlenA(clean);
    mc[0] = 0;
    mc[1] = 0;
    if (mnemonic >= 0 && mnemonic < i)
        mc[0] = clean[mnemonic];

    old = SelectObject(hdc, f);
    SetBkMode(hdc, OPAQUE);
    SetBkColor(hdc, bg);
    SetTextColor(hdc, fg);
    TextOutA(hdc, x, y, clean, i);
    sz.cx = 0;
    sz.cy = 0;
    GetTextExtentPoint32A(hdc, clean, i, &sz);

    /* Measure the mnemonic while OUR font is still selected. Measuring after
     * the SelectObject below would use the DC's previous font and put the rule
     * in the wrong place. */
    pre.cx = 0; pre.cy = 0;
    one.cx = 0; one.cy = 0;
    hasMnemonic = (mnemonic >= 0 && mnemonic < i);
    if (hasMnemonic) {
        GetTextExtentPoint32A(hdc, clean, mnemonic, &pre);
        GetTextExtentPoint32A(hdc, mc, 1, &one);
    }
    SelectObject(hdc, old);

    if (hasMnemonic)
        Theme_HLine(hdc, x + pre.cx, x + pre.cx + one.cx, y + sz.cy - 1, fg);
    return sz.cx;
}

/* ======================================================================
 * chrome
 * ==================================================================== */

void Widget_GroupBox(HDC hdc, const RECT *box, const char *caption)
{
    const Theme *t = &g_theme;
    int l = box->left, tp = box->top;
    int r = box->right - 1, b = box->bottom - 1;
    int u = M.unit;
    int tw = Widget_TextWidth(hdc, M.fUi, caption);
    int gap0 = l + Metrics_S(9);
    int gap1 = gap0 + tw + Metrics_S(4);

    /* top edge, split by the caption gap */
    Theme_HLineT(hdc, l + u, gap0, tp, u, t->shadow);
    Theme_HLineT(hdc, gap1, r, tp, u, t->shadow);
    /* left / bottom / right */
    Theme_VLineT(hdc, l, tp + u, b + 1, u, t->shadow);
    Theme_HLineT(hdc, l + u, r + 1, b, u, t->window);
    Theme_VLineT(hdc, r, tp + u, b + 1, u, t->window);
    /* corners */
    {
        RECT c1, c2, c3, c4;
        c1.left = l; c1.top = tp; c1.right = l + u; c1.bottom = tp + u;
        c2.left = r; c2.top = tp; c2.right = r + u; c2.bottom = tp + u;
        c3.left = l; c3.top = b;  c3.right = l + u; c3.bottom = b + u;
        c4.left = r; c4.top = b;  c4.right = r + u; c4.bottom = b + u;
        Theme_Fill(hdc, &c1, t->shadow);
        Theme_Fill(hdc, &c2, t->shadow);
        Theme_Fill(hdc, &c3, t->window);
        Theme_Fill(hdc, &c4, t->window);
    }

    Widget_TextAt(hdc, gap0 + Metrics_S(2), tp - Metrics_S(5), caption,
                  M.fUi, t->btntext, t->face);
}

void Widget_PushButton(HDC hdc, const RECT *box, const char *caption, int pressed)
{
    const Theme *t = &g_theme;
    COLORREF o1, i1, i2, o2;

    if (pressed) {
        o1 = t->dkshadow; i1 = t->shadow; i2 = t->light; o2 = t->hilight;
    } else {
        o1 = t->hilight;  i1 = t->light;  i2 = t->shadow; o2 = t->dkshadow;
    }

    Theme_Fill(hdc, box, t->face);
    Theme_Bevel(hdc, box, o1, i1, i2, o2, M.bevel);

    if (caption != NULL && caption[0] != 0) {
        RECT txt = *box;
        if (pressed) {
            txt.left += M.unit;
            txt.top += M.unit;
        }
        Widget_TextCentered(hdc, &txt, caption, M.fUi, t->btntext, t->face);
    }
}

void Widget_CaptionButton(HDC hdc, const RECT *box,
                          const unsigned short *glyph, int pressed)
{
    const Theme *t = &g_theme;
    COLORREF o1, i1, i2, o2;

    if (pressed) {
        o1 = t->dkshadow; i1 = t->shadow; i2 = t->light; o2 = t->hilight;
    } else {
        o1 = t->hilight;  i1 = t->light;  i2 = t->shadow; o2 = t->dkshadow;
    }

    Theme_Fill(hdc, box, t->face);
    Theme_Bevel(hdc, box, o1, i1, i2, o2, M.bevel);

    Theme_Glyph(hdc, box->left, box->top, glyph, t->btntext, M.unit);
}

/* ======================================================================
 * read-outs
 * ==================================================================== */

void Widget_Readout(HDC hdc, const RECT *box, const char *text, HFONT f)
{
    const Theme *t = &g_theme;
    Theme_Panel(hdc, box, t->window, TK_SUNKEN, M.bevel);
    Widget_TextCentered(hdc, box, text, f, t->windowtext, t->window);
}

void Widget_Drum(HDC hdc, const RECT *box, const char *digits, int cellW)
{
    const Theme *t = &g_theme;
    int ix, iy, ih, i, n;
    RECT cell;
    int u = M.unit;
    int pad = Metrics_S(4);

    Theme_Panel(hdc, box, t->window, TK_SUNKEN, M.bevel);

    ix = box->left + pad;
    iy = box->top + pad;
    ih = (box->bottom - box->top) - 2 * pad;
    n = lstrlenA(digits);

    for (i = 0; i < n; i++) {
        int cxx = ix + i * cellW;
        char one[2];
        if (i > 0) {
            Theme_VLineT(hdc, cxx, iy, iy + ih, u, t->light);
            Theme_VLineT(hdc, cxx + u, iy, iy + ih, u, t->window);
        }
        one[0] = digits[i];
        one[1] = 0;
        cell.left = cxx + u;
        cell.top = iy;
        cell.right = cxx + cellW;
        cell.bottom = iy + ih;
        Widget_TextCentered(hdc, &cell, one, M.fDrum, t->windowtext, t->window);
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
