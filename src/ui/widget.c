/* ===========================================================================
 * widget.c -- the detached desktop gauge window.
 *
 * A WS_POPUP with no system chrome: it is exactly the dial panel, nothing
 * more. WM_NCHITTEST reports HTCAPTION so DefWindowProc handles dragging, and
 * a right click opens the owner's context menu.
 *
 * WS_EX_TOOLWINDOW keeps it out of the taskbar and the Alt+Tab list -- it is a
 * desktop gadget, not a second document.
 * ======================================================================== */

#include "widget.h"
#include "gauge.h"
#include "theme.h"
#include "metrics.h"
#include "../platform/platform.h"

#include <string.h>

#define WIDGET_CLASS "KeyODO_Gauge"

static const WidgetHooks *s_hooks = NULL;
static WidgetModel s_model;
static int s_haveModel = 0;

static HDC     s_memDC = NULL;
static HBITMAP s_memBmp = NULL;
static HGDIOBJ s_memOld = NULL;
static int     s_memW = 0, s_memH = 0;

static void free_backbuffer(void)
{
    if (s_memDC != NULL) {
        if (s_memOld != NULL) SelectObject(s_memDC, s_memOld);
        if (s_memBmp != NULL) DeleteObject(s_memBmp);
        DeleteDC(s_memDC);
    }
    s_memDC = NULL; s_memBmp = NULL; s_memOld = NULL;
    s_memW = s_memH = 0;
}

static int ensure_backbuffer(HDC hdc, int w, int h)
{
    if (w <= 0 || h <= 0)
        return 0;
    if (s_memDC != NULL && s_memW == w && s_memH == h)
        return 1;
    free_backbuffer();
    s_memDC = CreateCompatibleDC(hdc);
    if (s_memDC == NULL)
        return 0;
    s_memBmp = CreateCompatibleBitmap(hdc, w, h);
    if (s_memBmp == NULL) {
        DeleteDC(s_memDC);
        s_memDC = NULL;
        return 0;
    }
    s_memOld = SelectObject(s_memDC, s_memBmp);
    s_memW = w;
    s_memH = h;
    return 1;
}

static void paint(HWND hwnd, HDC hdc)
{
    GaugeSpec g;

    g.value = s_model.gauge_value;
    g.needle = s_model.needle;
    g.peak = s_model.peak;
    g.mode_kpm = s_model.mode_kpm;
    g.bare = 1;                 /* no square panel: the dial only */
    g.gear = s_model.gear;

    if (ensure_backbuffer(hdc, s_memW ? s_memW : M.rBezel * 2,
                          s_memH ? s_memH : M.rBezel * 2)) {
        /* Clear first. The dial only covers its own circle, so without this the
         * surround would hold whatever CreateCompatibleBitmap left there --
         * uninitialised memory, which shows up as artefacts inside the round
         * window region. */
        RECT all;
        all.left = 0; all.top = 0;
        all.right = s_memW; all.bottom = s_memH;
        Theme_Fill(s_memDC, &all, g_theme.face);

        Gauge_Draw(s_memDC, &g);
        BitBlt(hdc, 0, 0, s_memW, s_memH, s_memDC, 0, 0, SRCCOPY);
    } else {
        Gauge_Draw(hdc, &g);
    }
}

/* Clips the window to the dial's circle.
 *
 * SetWindowRgn gives a genuinely transparent surround without layered windows
 * (which would be Windows 2000 and later) and without a colour key (which
 * breaks on themes that reuse the key colour). It also makes the transparent
 * corners click-through, so the widget only catches the mouse where it is
 * actually visible. */
static void apply_round_region(HWND hwnd)
{
    int d = 2 * Gauge_DialRadius();
    HRGN rgn = CreateEllipticRgn(0, 0, d, d);
    if (rgn != NULL)
        SetWindowRgn(hwnd, rgn, TRUE);   /* the window owns the region now */
}

static LRESULT CALLBACK Widget_WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        paint(hwnd, hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_NCHITTEST:
        /* The whole surface drags. */
        return HTCAPTION;

    case WM_RBUTTONUP:
        if (s_hooks != NULL && s_hooks->on_context != NULL) {
            POINT pt;
            pt.x = (short)LOWORD(lp);
            pt.y = (short)HIWORD(lp);
            ClientToScreen(hwnd, &pt);
            s_hooks->on_context(hwnd, pt);
        }
        return 0;

    case WM_EXITSIZEMOVE:
        if (s_hooks != NULL && s_hooks->on_moved != NULL)
            s_hooks->on_moved(hwnd);
        return 0;

    case WM_DPICHANGED:
        if (s_hooks != NULL && s_hooks->on_dpi_changed != NULL)
            s_hooks->on_dpi_changed(hwnd);
        return 0;

    case WM_CLOSE:
        ShowWindow(hwnd, SW_HIDE);
        return 0;

    case WM_DESTROY:
        free_backbuffer();
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wp, lp);
}

HWND Widget_Create(HINSTANCE hInst, const WidgetHooks *hooks)
{
    WNDCLASSA wc;
    HWND hwnd;
    int w, h, x, y;

    s_hooks = hooks;
    ZeroMemory(&s_model, sizeof(s_model));

    ZeroMemory(&wc, sizeof(wc));
    wc.style = CS_DROPSHADOW;
    wc.lpfnWndProc = Widget_WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszClassName = WIDGET_CLASS;
    if (RegisterClassA(&wc) == 0)
        return NULL;

    Gauge_DialSize(&w, &h);
    x = GetSystemMetrics(SM_CXSCREEN) - w - 40;
    y = 40;

    hwnd = CreateWindowExA(WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
                           WIDGET_CLASS, "KeyODO Gauge",
                           WS_POPUP | WS_CLIPCHILDREN,
                           x, y, w, h, NULL, NULL, hInst, NULL);
    if (hwnd == NULL)
        return NULL;
    apply_round_region(hwnd);
    return hwnd;
}

void Widget_Destroy(HWND hwnd)
{
    if (hwnd != NULL)
        DestroyWindow(hwnd);
}

void Widget_SetModel(HWND hwnd, const WidgetModel *m)
{
    int changed = 0;

    if (!s_haveModel) {
        changed = 1;
        s_haveModel = 1;
    } else {
        if (s_model.needle != m->needle) changed = 1;
        if (s_model.peak != m->peak) changed = 1;
        if (s_model.gauge_value != m->gauge_value) changed = 1;
        if (s_model.mode_kpm != m->mode_kpm) changed = 1;
        if (lstrcmpA(s_model.gear, m->gear) != 0) changed = 1;
    }
    s_model = *m;

    if (changed && hwnd != NULL)
        InvalidateRect(hwnd, NULL, FALSE);
}

void Widget_Place(HWND hwnd, int x, int y)
{
    if (hwnd != NULL)
        SetWindowPos(hwnd, NULL, x, y, 0, 0,
                     SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void Widget_PlaceClamped(HWND hwnd, int x, int y)
{
    int w, h;
    Gauge_DialSize(&w, &h);
    Plat_ClampToWorkArea(&x, &y, w, h);
    Widget_Place(hwnd, x, y);
}

void Widget_GetPlace(HWND hwnd, int *x, int *y)
{
    RECT r;
    if (hwnd != NULL && GetWindowRect(hwnd, &r)) {
        if (x) *x = r.left;
        if (y) *y = r.top;
    } else {
        if (x) *x = 0;
        if (y) *y = 0;
    }
}

void Widget_Show(HWND hwnd, int show)
{
    if (hwnd == NULL)
        return;
    ShowWindow(hwnd, show ? SW_SHOWNOACTIVATE : SW_HIDE);
    if (show) {
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        InvalidateRect(hwnd, NULL, FALSE);
    }
}

int Widget_IsVisible(HWND hwnd)
{
    return (hwnd != NULL) && IsWindowVisible(hwnd);
}

void Widget_Resize(HWND hwnd)
{
    int w, h, x, y;
    if (hwnd == NULL)
        return;
    free_backbuffer();
    Gauge_DialSize(&w, &h);
    Widget_GetPlace(hwnd, &x, &y);
    SetWindowPos(hwnd, NULL, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
    apply_round_region(hwnd);
    Plat_ClampToWorkArea(&x, &y, w, h);
    Widget_Place(hwnd, x, y);
    InvalidateRect(hwnd, NULL, FALSE);
}
