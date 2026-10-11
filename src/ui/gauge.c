/* ===========================================================================
 * gauge.c -- the dial, drawn with GDI primitives only.
 *
 * Direct port of draw_dial() in mockup/keyodo_mockup.py, with every radius and
 * length taken from Metrics so it renders correctly at any monitor DPI.
 *
 * The dial is drawn in its OWN coordinate space, (0,0) to
 * (M.dialPanelW, M.dialPanelH). The main window positions it with a viewport
 * origin; the detached desktop widget simply draws it at its own origin. That
 * is why the same routine serves both.
 *
 * GDI notes
 *   - Polyline draws the bezel arcs. Arc() has its own angle convention, so
 *     arcs are sampled and stroked instead.
 *   - Polygon fills the redline band, the ticks and the needle. Using one fill
 *     primitive everywhere keeps the Windows 98 SE compatibility surface small.
 *   - Ellipse's bounding box is right/bottom exclusive, which matches the
 *     reference renderer's inclusive box shifted by one.
 * ======================================================================== */

#include "gauge.h"
#include "theme.h"
#include "widgets.h"
#include "metrics.h"

#include "keyodo_layout.h"
#include "../common.h"

#include <math.h>

#define K_PI 3.14159265358979323846

static POINT polar(int cx, int cy, double r, double deg)
{
    double a = deg * K_PI / 180.0;
    POINT p;
    p.x = (LONG)floor(cx + r * cos(a) + 0.5);
    p.y = (LONG)floor(cy - r * sin(a) + 0.5);
    return p;
}

static int arc_points(POINT *out, int cap, int cx, int cy, double r,
                      double a0, double a1, double step)
{
    int n, i;
    double span = fabs(a1 - a0);
    n = (int)(span / step) + 1;
    if (n < 2) n = 2;
    if (n > cap) n = cap;
    for (i = 0; i < n; i++)
        out[i] = polar(cx, cy, r, a0 + (a1 - a0) * i / (double)(n - 1));
    return n;
}

static void polyline(HDC hdc, const POINT *pts, int n, COLORREF c, int width)
{
    HPEN pen, old;
    if (n < 2) return;
    if (width < 1) width = 1;
    pen = CreatePen(PS_SOLID, width, c);
    old = (HPEN)SelectObject(hdc, pen);
    Polyline(hdc, pts, n);
    SelectObject(hdc, old);
    DeleteObject(pen);
}

static void fill_poly(HDC hdc, const POINT *pts, int n, COLORREF c)
{
    HBRUSH br, ob;
    HGDIOBJ op;
    if (n < 3) return;
    br = CreateSolidBrush(c);
    ob = (HBRUSH)SelectObject(hdc, br);
    op = SelectObject(hdc, GetStockObject(NULL_PEN));
    Polygon(hdc, pts, n);
    SelectObject(hdc, ob);
    SelectObject(hdc, op);
    DeleteObject(br);
}

static void fill_ellipse(HDC hdc, int cx, int cy, int r, COLORREF fill)
{
    HBRUSH br, ob;
    HGDIOBJ op;
    br = CreateSolidBrush(fill);
    ob = (HBRUSH)SelectObject(hdc, br);
    op = SelectObject(hdc, GetStockObject(NULL_PEN));
    Ellipse(hdc, cx - r, cy - r, cx + r, cy + r);
    SelectObject(hdc, ob);
    SelectObject(hdc, op);
    DeleteObject(br);
}

static void ellipse_outline(HDC hdc, int cx, int cy, int r,
                            COLORREF fill, COLORREF ink, int width)
{
    HBRUSH br, ob;
    HPEN pen, op;
    if (width < 1) width = 1;
    br = CreateSolidBrush(fill);
    pen = CreatePen(PS_SOLID, width, ink);
    ob = (HBRUSH)SelectObject(hdc, br);
    op = (HPEN)SelectObject(hdc, pen);
    Ellipse(hdc, cx - r, cy - r, cx + r, cy + r);
    SelectObject(hdc, ob);
    SelectObject(hdc, op);
    DeleteObject(br);
    DeleteObject(pen);
}

/* Filled annular sector: the GDI equivalent is one Polygon() of two arcs. */
static void annulus_sector(HDC hdc, int cx, int cy, double rOut, double rIn,
                           double a0, double a1, COLORREF c)
{
    POINT pts[200];
    int n, half;
    half = arc_points(pts, 100, cx, cy, rOut, a0, a1, 2.0);
    n = half;
    half = arc_points(pts + n, 100, cx, cy, rIn, a1, a0, 2.0);
    n += half;
    fill_poly(hdc, pts, n, c);
}

/* A tick: a thin quad along the radius. */
static void radial_bar(HDC hdc, int cx, int cy, double r0, double r1,
                       double angle, double width, COLORREF c)
{
    POINT p[4];
    double hw = width / 2.0;
    double a = angle * K_PI / 180.0;
    double rx = cos(a), ry = -sin(a);   /* radial, screen space */
    double px = sin(a), py = cos(a);    /* perpendicular        */

    p[0].x = (LONG)floor(cx + r0 * rx - hw * px + 0.5);
    p[0].y = (LONG)floor(cy + r0 * ry - hw * py + 0.5);
    p[1].x = (LONG)floor(cx + r0 * rx + hw * px + 0.5);
    p[1].y = (LONG)floor(cy + r0 * ry + hw * py + 0.5);
    p[2].x = (LONG)floor(cx + r1 * rx + hw * px + 0.5);
    p[2].y = (LONG)floor(cy + r1 * ry + hw * py + 0.5);
    p[3].x = (LONG)floor(cx + r1 * rx - hw * px + 0.5);
    p[3].y = (LONG)floor(cy + r1 * ry - hw * py + 0.5);
    fill_poly(hdc, p, 4, c);
}

static double ang(double wpm)
{
    return WPM_SWEEP_START - WPM_SWEEP_TOTAL * (wpm / (double)WPM_MAX);
}

void Gauge_Size(int *w, int *h)
{
    if (w) *w = M.dialPanelW;
    if (h) *h = M.dialPanelH;
}

int Gauge_DialRadius(void)
{
    return M.rBezel;
}

void Gauge_DialSize(int *w, int *h)
{
    int d = 2 * M.rBezel;
    if (w) *w = d;
    if (h) *h = d;
}

void Gauge_DirtyRect(RECT *out)
{
    out->left = 0;
    out->top = 0;
    out->right = M.dialPanelW;
    out->bottom = M.dialPanelH;
}

void Gauge_Draw(HDC hdc, const GaugeSpec *g)
{
    const Theme *t = &g_theme;
    RECT rc, box;
    POINT pts[128], poly[8];
    POINT p;
    int n, cx, cy;
    int u = M.unit;
    double a, ux, uy;
    double scale = (double)M.dpi / 96.0;
    char buf[16];
    int w;
    int i;
    struct Corner { double r, off; } corner[5];

    /* Draw the dial alone, or the dial sitting in its recessed square panel.
     * The centre follows whichever box is in use so the same routine serves
     * the main window and the detached circular widget. */
    if (g->bare) {
        cx = cy = M.rBezel;
    } else {
        cx = M.dialCx;
        cy = M.dialCy;

        /* 1. recessed instrument panel */
        rc.left = 0; rc.top = 0;
        rc.right = M.dialPanelW; rc.bottom = M.dialPanelH;
        Theme_Panel(hdc, &rc, t->face, TK_SUNKEN, M.bevel);
    }

    /* 2. raised circular bezel: bevels split along the upper-left axis */
    fill_ellipse(hdc, cx, cy, M.rBezel, t->face);
    n = arc_points(pts, ARRAY_LEN(pts), cx, cy, M.rBezel - u, 45, 225, 3.0);
    polyline(hdc, pts, n, t->hilight, u);
    n = arc_points(pts, ARRAY_LEN(pts), cx, cy, M.rBezel - u, 225, 405, 3.0);
    polyline(hdc, pts, n, t->shadow, u);
    n = arc_points(pts, ARRAY_LEN(pts), cx, cy, M.rBezel - 2 * u, 45, 225, 3.0);
    polyline(hdc, pts, n, t->light, u);
    n = arc_points(pts, ARRAY_LEN(pts), cx, cy, M.rBezel - 2 * u, 225, 405, 3.0);
    polyline(hdc, pts, n, t->dkshadow, u);

    /* 3. white dial face */
    ellipse_outline(hdc, cx, cy, M.rFace, t->window, t->shadow, u);

    /* 4. redline band */
    annulus_sector(hdc, cx, cy, M.rRedOut - u, M.rRedIn,
                   ang(WPM_REDLINE_FROM), ang(WPM_MAX), t->red);

    /* 5. ticks */
    for (w = 0; w <= WPM_MAX; w += 5) {
        a = ang(w);
        if (w % WPM_LABEL_STEP == 0)
            radial_bar(hdc, cx, cy, M.rMajIn, M.rTickOut, a, 2.0 * scale, t->windowtext);
        else if (w % 10 == 0)
            radial_bar(hdc, cx, cy, M.rMedIn, M.rTickOut, a, 2.0 * scale, t->windowtext);
        else
            radial_bar(hdc, cx, cy, M.rMinIn, M.rTickOut, a, 1.0 * scale, t->graytext);
    }

    /* 6. numerals */
    for (w = 0; w <= WPM_MAX; w += WPM_LABEL_STEP) {
        wsprintfA(buf, "%d", w);
        p = polar(cx, cy, M.rLabel, ang(w));
        box.left = p.x - Metrics_S(16); box.top = p.y - Metrics_S(7);
        box.right = p.x + Metrics_S(16); box.bottom = p.y + Metrics_S(7);
        Widget_TextCentered(hdc, &box, buf, M.fDial, t->windowtext, t->window);
    }

    /* 7. printed unit, following the selected mode */
    p = polar(cx, cy, M.rUnitLabel, 270.0);
    box.left = p.x - Metrics_S(20); box.top = p.y - Metrics_S(6);
    box.right = p.x + Metrics_S(20); box.bottom = p.y + Metrics_S(6);
    Widget_TextCentered(hdc, &box, g->mode_kpm ? KDM_UNIT_KPM : KDM_UNIT_WPM,
                        M.fDial, t->graytext, t->window);

    /* 8. peak-hold telltale */
    radial_bar(hdc, cx, cy, M.rTelltaleIn, M.rTelltaleOut, ang(g->peak),
               3.0 * scale, t->red);

    /* 9. needle, driven by the smoothed speed */
    a = g->needle;
    if (a < 0) a = 0;
    if (a > WPM_MAX) a = WPM_MAX;
    a = ang(a);
    ux = cos(a * K_PI / 180.0);
    uy = sin(a * K_PI / 180.0);
    {
        double rx = ux, ry = -uy;
        double px = uy, py = ux;
        corner[0].r = M.rNeedle;              corner[0].off = 0.0;
        corner[1].r = 0.0;                    corner[1].off = M.needleHalfHub;
        corner[2].r = R_NEEDLE_TAIL * scale;  corner[2].off = M.needleHalfTail;
        corner[3].r = R_NEEDLE_TAIL * scale;  corner[3].off = -M.needleHalfTail;
        corner[4].r = 0.0;                    corner[4].off = -M.needleHalfHub;
        for (i = 0; i < 5; i++) {
            poly[i].x = (LONG)floor(cx + corner[i].r * rx + corner[i].off * px + 0.5);
            poly[i].y = (LONG)floor(cy + corner[i].r * ry + corner[i].off * py + 0.5);
        }
        fill_poly(hdc, poly, 5, t->windowtext);
    }
    ellipse_outline(hdc, cx, cy, M.rHub, t->face, t->windowtext, u);
    fill_ellipse(hdc, cx, cy, M.hubDot, t->windowtext);

    /* 10. read-out embedded in the dial face, under the unit label.
     *     Metrics expresses it relative to the panel; re-anchor it on whatever
     *     centre this call is drawing around. */
    i = cx - M.dialCx;                  /* shift from panel centre to here */
    n = cy - M.dialCy;
    rc = M.dialEmbed;
    OffsetRect(&rc, -M.dialPanel.left + i, -M.dialPanel.top + n);
    Theme_Panel(hdc, &rc, t->window, TK_SUNKEN, M.bevel);
    wsprintfA(buf, "%d", (int)(g->value + 0.5));
    Widget_TextCentered(hdc, &rc, buf, M.fReadout, t->windowtext, t->window);
}
