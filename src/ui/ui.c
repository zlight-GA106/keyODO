/* ===========================================================================
 * ui.c -- KeyODO main window: Windows 98 chrome, layout and painting.
 *
 * The window is a WS_POPUP with no system caption. Everything the reference
 * screenshot shows -- 1 px black outline, frame, gradient caption, minimise and
 * close buttons, menu bar, group boxes, dial, drums, gear gate, status bar --
 * is painted here with GDI using the pinned classic palette. That is the only
 * way to get the same picture on Windows 98 SE and on a modern Windows, where
 * the native frame and menu are themed.
 *
 * All geometry comes from Metrics, which scales the 96-DPI specification in
 * include/keyodo_layout.h by the monitor DPI. At 100% every value equals the
 * constant, which is what keeps the reference-render comparison exact.
 * ======================================================================== */

#include "ui.h"
#include "theme.h"
#include "widgets.h"
#include "gauge.h"
#include "gear.h"
#include "metrics.h"

#include "keyodo_layout.h"
#include "../common.h"
#include "../platform/platform.h"
#include "resource.h"

#include <string.h>

/* ======================================================================
 * cached strings -- all Chinese text lives in the resource string table so
 * that no source file ever contains a non-ASCII byte
 * ==================================================================== */

typedef struct UiStrings {
    char menu[5][32];
    char gbSpeed[32], gbOdo[32], gbRate[32];
    char lblOdo[16], lblTrip[16], lblKpm[16], lblMax[16];
    char btnTrip[24], btnPeak[24];
    char statusReady[24];
} UiStrings;

static UiStrings S;
static HINSTANCE s_inst = NULL;
static const UiHooks *s_hooks = NULL;
static UiModel s_model;
static int s_haveModel = 0;
static HMENU s_menu = NULL;

static void load_str(UINT id, char *buf, int cch)
{
    buf[0] = 0;
    if (s_inst != NULL)
        LoadStringA(s_inst, id, buf, cch);
}

static void load_strings(void)
{
    load_str(IDS_MENU_FILE,  S.menu[0], sizeof(S.menu[0]));
    load_str(IDS_MENU_EDIT,  S.menu[1], sizeof(S.menu[1]));
    load_str(IDS_MENU_VIEW,  S.menu[2], sizeof(S.menu[2]));
    load_str(IDS_MENU_STATS, S.menu[3], sizeof(S.menu[3]));
    load_str(IDS_MENU_HELP,  S.menu[4], sizeof(S.menu[4]));

    load_str(IDS_GB_SPEED, S.gbSpeed, sizeof(S.gbSpeed));
    load_str(IDS_GB_ODO,   S.gbOdo,   sizeof(S.gbOdo));
    load_str(IDS_GB_RATE,  S.gbRate,  sizeof(S.gbRate));

    load_str(IDS_LBL_ODO,  S.lblOdo,  sizeof(S.lblOdo));
    load_str(IDS_LBL_TRIP, S.lblTrip, sizeof(S.lblTrip));
    load_str(IDS_LBL_KPM,  S.lblKpm,  sizeof(S.lblKpm));
    load_str(IDS_LBL_MAX,  S.lblMax,  sizeof(S.lblMax));

    load_str(IDS_BTN_TRIP, S.btnTrip, sizeof(S.btnTrip));
    load_str(IDS_BTN_PEAK, S.btnPeak, sizeof(S.btnPeak));

    load_str(IDS_STATUS_READY, S.statusReady, sizeof(S.statusReady));
}

/* ======================================================================
 * application icon, drawn with the pinned palette
 * ==================================================================== */

static const char *kIcon16[16] = {
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
    "................"
};

static void draw_icon(HDC hdc, int ox, int oy)
{
    const Theme *t = &g_theme;
    int y, x;
    int u = M.unit;
    RECT r;

    for (y = 0; y < 16; y++) {
        const char *row = kIcon16[y];
        for (x = 0; x < 16; x++) {
            COLORREF c;
            switch (row[x]) {
            case '#': c = t->frame;   break;
            case 'w': c = t->hilight; break;
            case 'f': c = t->face;    break;
            case 'k': c = t->shadow;  break;
            default:  continue;
            }
            r.left = ox + x * u;
            r.top = oy + y * u;
            r.right = r.left + u;
            r.bottom = r.top + u;
            Theme_Fill(hdc, &r, c);
        }
    }
}

/* ======================================================================
 * back buffer and the cached caption gradient
 * ==================================================================== */

static HDC     s_memDC = NULL;
static HBITMAP s_memBmp = NULL;
static HGDIOBJ s_memOld = NULL;
static int     s_memW = 0, s_memH = 0;

static HBITMAP s_gradBmp = NULL;
static int     s_gradW = 0;

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

static void free_gradient(void)
{
    if (s_gradBmp != NULL) {
        DeleteObject(s_gradBmp);
        s_gradBmp = NULL;
        s_gradW = 0;
    }
}

static int ensure_backbuffer(HWND hwnd, HDC hdc)
{
    RECT rc;
    GetClientRect(hwnd, &rc);
    if (rc.right <= 0 || rc.bottom <= 0)
        return 0;
    if (s_memDC != NULL && s_memW == rc.right && s_memH == rc.bottom)
        return 1;

    free_backbuffer();
    s_memDC = CreateCompatibleDC(hdc);
    if (s_memDC == NULL)
        return 0;
    s_memBmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
    if (s_memBmp == NULL) {
        DeleteDC(s_memDC);
        s_memDC = NULL;
        return 0;
    }
    s_memOld = SelectObject(s_memDC, s_memBmp);
    s_memW = rc.right;
    s_memH = rc.bottom;
    return 1;
}

/* The caption fade never changes for a given width, so it is rasterised once.
 * One FillRect per column on every frame would be far too much for a 1998 CPU,
 * and at 200% there are twice as many columns. */
static void ensure_caption_gradient(HDC hdc)
{
    const Theme *t = &g_theme;
    int w = M.winW - 2 * M.frame;
    int h = M.caption;
    HDC mdc;
    HGDIOBJ old;
    int x;
    int r0, g0, b0, r1, g1, b1;

    if (s_gradBmp != NULL && s_gradW == w)
        return;
    free_gradient();

    s_gradBmp = CreateCompatibleBitmap(hdc, w, h);
    if (s_gradBmp == NULL)
        return;
    mdc = CreateCompatibleDC(hdc);
    old = SelectObject(mdc, s_gradBmp);

    r0 = GetRValue(t->caption); g0 = GetGValue(t->caption); b0 = GetBValue(t->caption);
    r1 = GetRValue(t->grad_caption);
    g1 = GetGValue(t->grad_caption);
    b1 = GetBValue(t->grad_caption);

    for (x = 0; x < w; x++) {
        double f = (double)x / (double)(w - 1);
        HBRUSH br = CreateSolidBrush(RGB(r0 + (int)((r1 - r0) * f),
                                         g0 + (int)((g1 - g0) * f),
                                         b0 + (int)((b1 - b0) * f)));
        RECT col;
        col.left = x; col.top = 0; col.right = x + 1; col.bottom = h;
        FillRect(mdc, &col, br);
        DeleteObject(br);
    }

    SelectObject(mdc, old);
    DeleteDC(mdc);
    s_gradW = w;
}

/* ======================================================================
 * menu bar geometry -- shared by painting and hit testing
 * ==================================================================== */

#define MENU_ITEM_COUNT 5

static int menubar_item_rect(HDC hdc, int index, RECT *out)
{
    int x, i;
    int gap = Metrics_S(12);
    if (index < 0 || index >= MENU_ITEM_COUNT)
        return 0;
    x = Metrics_S(2);
    for (i = 0; i < MENU_ITEM_COUNT; i++) {
        int w = Widget_TextWidth(hdc, M.fUi, S.menu[i]) + gap;
        if (i == index) {
            out->left = x;
            out->top = 0;
            out->right = x + w;
            out->bottom = M.menubar;
            return 1;
        }
        x += w;
    }
    return 0;
}

static int menubar_index_at(HDC hdc, int x)
{
    int i;
    RECT r;
    POINT p;
    p.x = x;
    p.y = 1;
    for (i = 0; i < MENU_ITEM_COUNT; i++) {
        if (menubar_item_rect(hdc, i, &r) && PtInRect(&r, p))
            return i;
    }
    return -1;
}

/* ======================================================================
 * chrome
 * ==================================================================== */

static void draw_frame(HDC hdc)
{
    const Theme *t = &g_theme;
    RECT rc;
    int i;

    rc.left = 0; rc.top = 0; rc.right = M.winW; rc.bottom = M.winH;
    Theme_Rect1(hdc, &rc, t->frame);

    for (i = 1; i < M.frame; i++) {
        rc.left = i; rc.top = i; rc.right = M.winW - i; rc.bottom = M.winH - i;
        Theme_Rect1(hdc, &rc, t->face);
    }
}

static void draw_caption(HDC hdc)
{
    const Theme *t = &g_theme;
    RECT rc, b;
    char title[64];

    rc.left = M.frame;
    rc.top = M.frame;
    rc.right = M.winW - M.frame;
    rc.bottom = M.frame + M.caption;

    if (s_model.active) {
        ensure_caption_gradient(hdc);
        if (s_gradBmp != NULL) {
            HDC mdc = CreateCompatibleDC(hdc);
            HGDIOBJ old = SelectObject(mdc, s_gradBmp);
            BitBlt(hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
                   mdc, 0, 0, SRCCOPY);
            SelectObject(mdc, old);
            DeleteDC(mdc);
        } else {
            Theme_Fill(hdc, &rc, t->caption);
        }
    } else {
        Theme_Fill(hdc, &rc, t->caption_inactive);
    }

    draw_icon(hdc, M.iconX, M.iconY);

    load_str(IDS_APP_TITLE, title, sizeof(title));
    Widget_TextAtClear(hdc, M.titleX, M.titleY, title, M.fUiBold,
                       s_model.active ? t->caption_text : t->caption_inactive_text);

    b.left = M.capBtnMinX; b.top = M.capBtnY;
    b.right = b.left + M.capBtnW; b.bottom = b.top + M.capBtnH;
    Widget_CaptionButton(hdc, &b, kGlyphMin, s_model.pressed == IDC_CAP_MIN);

    b.left = M.capBtnCloseX; b.top = M.capBtnY;
    b.right = b.left + M.capBtnW; b.bottom = b.top + M.capBtnH;
    Widget_CaptionButton(hdc, &b, kGlyphClose, s_model.pressed == IDC_CAP_CLOSE);
}

static void draw_menubar(HDC hdc)
{
    const Theme *t = &g_theme;
    RECT rc;
    int i;
    int padX = Metrics_S(6);

    rc.left = 0; rc.top = 0; rc.right = M.clientW; rc.bottom = M.menubar;
    Theme_Fill(hdc, &rc, t->menu);

    for (i = 0; i < MENU_ITEM_COUNT; i++) {
        const char *label = S.menu[i];
        const char *lp;
        RECT cell;
        int px, aw;

        if (!menubar_item_rect(hdc, i, &cell))
            break;

        Widget_TextAt(hdc, cell.left + padX, Metrics_S(4), label,
                      M.fUi, t->menutext, t->menu);

        /* accelerator underline under the letter inside the parentheses */
        px = cell.left + padX;
        for (lp = label; *lp != 0 && *lp != '('; lp++)
            ;
        if (*lp == '(') {
            char prefix[32];
            char one[2];
            int n = (int)(lp - label) + 1;
            if (n > (int)sizeof(prefix) - 1) n = (int)sizeof(prefix) - 1;
            memcpy(prefix, label, n);
            prefix[n] = 0;
            px += Widget_TextWidth(hdc, M.fUi, prefix);
            aw = Metrics_S(6);
            lp++;
            if (*lp != 0 && *lp != ')') {
                one[0] = *lp;
                one[1] = 0;
                aw = Widget_TextWidth(hdc, M.fUi, one);
            }
            Theme_HLineT(hdc, px, px + aw, M.menubar - Metrics_S(3),
                         M.unit, t->menutext);
        }
    }
}

static void draw_statusbar(HDC hdc)
{
    const Theme *t = &g_theme;
    RECT rc;
    int t0 = M.clientH - M.statusbar;
    int pad = Metrics_S(4);
    int u = M.unit;

    rc.left = 0; rc.top = t0; rc.right = M.clientW; rc.bottom = M.clientH;
    Theme_Fill(hdc, &rc, t->face);
    Theme_DrawEdge(hdc, &rc, TK_SUNKEN, u);

    Theme_VLineT(hdc, M.sbPane2X - 2 * u, t0 + u, M.clientH - u, u, t->shadow);
    Theme_VLineT(hdc, M.sbPane2X - u, t0 + u, M.clientH - u, u, t->hilight);
    Theme_VLineT(hdc, M.sbPane3X - 2 * u, t0 + u, M.clientH - u, u, t->shadow);
    Theme_VLineT(hdc, M.sbPane3X - u, t0 + u, M.clientH - u, u, t->hilight);

    Widget_TextAt(hdc, Metrics_S(5), t0 + pad, S.statusReady,
                  M.fUi, t->btntext, t->face);
    Widget_TextAt(hdc, M.sbPane2X + Metrics_S(3), t0 + pad, s_model.scale,
                  M.fUi, t->btntext, t->face);
    Widget_TextAt(hdc, M.sbPane3X + Metrics_S(3), t0 + pad, s_model.status,
                  M.fUi, t->btntext, t->face);
}

/* ======================================================================
 * content
 * ==================================================================== */

static void draw_content(HDC hdc)
{
    const Theme *t = &g_theme;
    RECT rc, band;
    static const char *const autoGears[] = KTM_GEARS_AUTO;
    static const char *const manGears[] = KTM_GEARS_MANUAL;
    int saved;

    band.left = 0; band.top = M.contentY;
    band.right = M.clientW; band.bottom = M.contentY + M.contentH;
    Theme_Fill(hdc, &band, t->face);

    /* ---- speedometer ---- */
    rc = M.gbSpeed;
    Widget_GroupBox(hdc, &rc, S.gbSpeed);

    /* The dial draws in its own coordinates. OffsetViewportOrgEx adds to the
     * current client origin; SetViewportOrgEx would REPLACE it and drop the
     * client offset, painting the dial over the group box caption. */
    saved = SaveDC(hdc);
    OffsetViewportOrgEx(hdc, M.dialPanel.left, M.dialPanel.top, NULL);
    {
        GaugeSpec g;
        g.value = s_model.gauge_value;
        g.peak = s_model.peak;
        g.needle = s_model.needle;
        g.mode_kpm = s_model.mode_kpm;
        Gauge_Draw(hdc, &g);
    }
    RestoreDC(hdc, saved);

    rc = M.gearValue;
    Gear_DrawValue(hdc, &rc, s_model.gear, s_model.value);

    /* ---- odometer ---- */
    rc = M.gbOdo;
    Widget_GroupBox(hdc, &rc, S.gbOdo);

    Widget_TextAt(hdc, Metrics_S(270), M.odoCapY, S.lblOdo,
                  M.fUi, t->btntext, t->face);
    rc = M.odoDrum;
    Widget_Drum(hdc, &rc, s_model.odo, M.cellW);

    Widget_TextAt(hdc, Metrics_S(270), M.tripCapY, S.lblTrip,
                  M.fUi, t->btntext, t->face);
    rc = M.tripDrum;
    Widget_Drum(hdc, &rc, s_model.trip, M.cellW);

    rc = M.tripReset;
    Widget_PushButton(hdc, &rc, S.btnTrip, s_model.pressed == IDC_BTN_TRIP);

    /* ---- rate ---- */
    rc = M.gbRate;
    Widget_GroupBox(hdc, &rc, S.gbRate);

    Widget_TextAt(hdc, Metrics_S(270), M.kpmCapY, S.lblKpm,
                  M.fUi, t->btntext, t->face);
    rc = M.kpmPanel;
    Widget_Readout(hdc, &rc, s_model.kpm, M.fReadout);

    Widget_TextAt(hdc, Metrics_S(270), M.maxCapY, S.lblMax,
                  M.fUi, t->btntext, t->face);
    rc = M.maxPanel;
    Widget_Readout(hdc, &rc, s_model.maxv, M.fReadout);

    if (s_model.show_gate) {
        rc = M.gearGate;
        if (s_model.transmission == KTM_MANUAL)
            Gear_DrawGate(hdc, &rc, manGears, KTM_COUNT_MANUAL, s_model.gear);
        else
            Gear_DrawGate(hdc, &rc, autoGears, KTM_COUNT_AUTO, s_model.gear);
    }

    rc = M.peakReset;
    Widget_PushButton(hdc, &rc, S.btnPeak, s_model.pressed == IDC_BTN_PEAK);
}

static void paint_all(HDC hdc)
{
    int saved = SaveDC(hdc);

    draw_frame(hdc);
    draw_caption(hdc);

    SetViewportOrgEx(hdc, M.frame, M.frame + M.caption, NULL);
    draw_menubar(hdc);
    draw_content(hdc);
    draw_statusbar(hdc);

    RestoreDC(hdc, saved);
}

/* ======================================================================
 * hit testing
 * ==================================================================== */

/* kind: 0 = nothing, 1 = control (id set), 2 = caption drag */
static int hit_test(POINT pt, int *id)
{
    RECT r;

    *id = 0;

    r.left = M.capBtnCloseX; r.top = M.capBtnY;
    r.right = r.left + M.capBtnW; r.bottom = r.top + M.capBtnH;
    if (PtInRect(&r, pt)) { *id = IDC_CAP_CLOSE; return 1; }

    r.left = M.capBtnMinX; r.top = M.capBtnY;
    r.right = r.left + M.capBtnW; r.bottom = r.top + M.capBtnH;
    if (PtInRect(&r, pt)) { *id = IDC_CAP_MIN; return 1; }

    r.left = M.frame; r.top = M.frame;
    r.right = M.winW - M.frame; r.bottom = M.frame + M.caption;
    if (PtInRect(&r, pt))
        return 2;

    /* client-area buttons */
    {
        POINT c;
        c.x = pt.x - M.frame;
        c.y = pt.y - (M.frame + M.caption);
        r = M.tripReset;
        if (PtInRect(&r, c)) { *id = IDC_BTN_TRIP; return 1; }
        r = M.peakReset;
        if (PtInRect(&r, c)) { *id = IDC_BTN_PEAK; return 1; }
    }
    return 0;
}

static void show_menu(HWND hwnd, int index)
{
    HMENU sub;
    HDC hdc;
    RECT cell, wr;
    POINT pt;
    int cmd;

    if (s_menu == NULL || index < 0)
        return;
    sub = GetSubMenu(s_menu, index);
    if (sub == NULL)
        return;

    hdc = GetDC(hwnd);
    if (!menubar_item_rect(hdc, index, &cell))
        cell.left = Metrics_S(2);
    ReleaseDC(hwnd, hdc);

    GetWindowRect(hwnd, &wr);
    pt.x = wr.left + M.frame + cell.left;
    pt.y = wr.top + M.frame + M.caption + M.menubar;

    cmd = (int)TrackPopupMenu(sub,
                              TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD |
                              TPM_RIGHTBUTTON,
                              pt.x, pt.y, 0, hwnd, NULL);
    if (cmd != 0 && s_hooks != NULL && s_hooks->on_command != NULL)
        s_hooks->on_command(hwnd, cmd);
    InvalidateRect(hwnd, NULL, FALSE);
}

/* ======================================================================
 * window procedure
 * ==================================================================== */

static LRESULT CALLBACK Ui_WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        if (ensure_backbuffer(hwnd, hdc)) {
            paint_all(s_memDC);
            BitBlt(hdc, ps.rcPaint.left, ps.rcPaint.top,
                   ps.rcPaint.right - ps.rcPaint.left,
                   ps.rcPaint.bottom - ps.rcPaint.top,
                   s_memDC, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);
        }
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DPICHANGED: {
        /* Moved to a monitor with a different scaling: rebuild every metric
         * and font, then resize. The suggested rectangle assumes a system
         * frame, which this window does not have, so only its origin is used
         * and the size is recomputed from Metrics. */
        RECT *suggested = (RECT *)lp;
        Metrics_Init(hwnd);
        free_backbuffer();
        free_gradient();
        SetWindowPos(hwnd, NULL, suggested->left, suggested->top,
                     M.winW, M.winH, SWP_NOZORDER | SWP_NOACTIVATE);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case UIWM_DPI_REFRESH: {
        Metrics_Init(hwnd);
        free_backbuffer();
        free_gradient();
        SetWindowPos(hwnd, NULL, 0, 0, M.winW, M.winH,
                     SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_NCHITTEST: {
        POINT pt;
        int id;
        pt.x = (short)LOWORD(lp);
        pt.y = (short)HIWORD(lp);
        ScreenToClient(hwnd, &pt);
        if (hit_test(pt, &id) == 2)
            return HTCAPTION;
        return HTCLIENT;
    }

    case WM_LBUTTONDOWN: {
        POINT pt;
        int id, kind;

        pt.x = (short)LOWORD(lp);
        pt.y = (short)HIWORD(lp);

        /* menu bar click */
        if (pt.y >= M.frame + M.caption &&
            pt.y < M.frame + M.caption + M.menubar &&
            pt.x >= M.frame && pt.x < M.frame + M.clientW) {
            HDC hdc = GetDC(hwnd);
            int idx = menubar_index_at(hdc, pt.x - M.frame);
            ReleaseDC(hwnd, hdc);
            if (idx >= 0) {
                show_menu(hwnd, idx);
                return 0;
            }
        }

        kind = hit_test(pt, &id);
        if (kind == 1) {
            s_model.pressed = id;
            InvalidateRect(hwnd, NULL, FALSE);
            SetCapture(hwnd);
        }
        return 0;
    }

    case WM_LBUTTONUP: {
        POINT pt;
        int id, kind, was = s_model.pressed;

        if (GetCapture() == hwnd)
            ReleaseCapture();

        pt.x = (short)LOWORD(lp);
        pt.y = (short)HIWORD(lp);
        kind = hit_test(pt, &id);
        s_model.pressed = 0;
        InvalidateRect(hwnd, NULL, FALSE);

        if (kind == 1 && id == was) {
            if (id == IDC_CAP_CLOSE)
                PostMessage(hwnd, WM_CLOSE, 0, 0);
            else if (id == IDC_CAP_MIN)
                ShowWindow(hwnd, SW_MINIMIZE);
            else if (s_hooks != NULL && s_hooks->on_command != NULL)
                s_hooks->on_command(hwnd, id);
        }
        return 0;
    }

    case WM_TIMER:
        if (s_hooks != NULL && s_hooks->on_timer != NULL)
            s_hooks->on_timer(hwnd);
        return 0;

    case WM_COMMAND:
        if (s_hooks != NULL && s_hooks->on_command != NULL)
            s_hooks->on_command(hwnd, (int)LOWORD(wp));
        return 0;

    case WM_ACTIVATE:
        s_model.active = (LOWORD(wp) != WA_INACTIVE);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;

    case WM_CLOSE:
        if (s_hooks != NULL && s_hooks->on_close != NULL)
            s_hooks->on_close(hwnd);
        else
            DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        free_backbuffer();
        free_gradient();
        if (s_hooks != NULL && s_hooks->on_destroy != NULL)
            s_hooks->on_destroy(hwnd);
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wp, lp);
}

/* ======================================================================
 * public
 * ==================================================================== */

HWND Ui_Create(HINSTANCE hInst, const UiHooks *hooks)
{
    WNDCLASSA wc;
    HWND hwnd;
    DWORD style;
    int x, y;

    s_inst = hInst;
    s_hooks = hooks;
    ZeroMemory(&s_model, sizeof(s_model));
    lstrcpynA(s_model.gear, "P", sizeof(s_model.gear));
    s_model.active = 1;
    s_model.transmission = KTM_AUTO;
    s_model.show_gate = 1;

    Theme_Init();
    Metrics_Init(NULL);          /* system DPI, before a window exists */
    load_strings();

    s_menu = LoadMenuA(hInst, MAKEINTRESOURCEA(IDM_MAIN));

    ZeroMemory(&wc, sizeof(wc));
    wc.style = CS_DROPSHADOW | CS_BYTEALIGNWINDOW;
    wc.lpfnWndProc = Ui_WndProc;
    wc.hInstance = hInst;
    wc.hIcon = LoadIconA(hInst, MAKEINTRESOURCEA(IDI_KEYODO));
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszClassName = "KeyODO_Main";
    if (wc.hIcon == NULL)
        wc.hIcon = LoadIconA(NULL, IDI_APPLICATION);
    if (RegisterClassA(&wc) == 0)
        return NULL;

    /* No WS_CAPTION and no WS_THICKFRAME: the classic frame is painted by
     * draw_frame()/draw_caption(), which keeps it identical on every Windows
     * and lets the window size follow the monitor DPI exactly. WM_NCHITTEST
     * returns HTCAPTION over the caption so dragging, the system menu and
     * Alt+F4 still work through DefWindowProc. */
    style = WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN;

    x = (GetSystemMetrics(SM_CXSCREEN) - M.winW) / 2;
    y = (GetSystemMetrics(SM_CYSCREEN) - M.winH) / 2;

    hwnd = CreateWindowExA(WS_EX_APPWINDOW, "KeyODO_Main", KEYODO_NAME, style,
                           x, y, M.winW, M.winH, NULL, NULL, hInst, NULL);
    if (hwnd == NULL)
        return NULL;

    /* Now that the window exists, take the DPI of the monitor it landed on. */
    Metrics_Init(hwnd);
    free_gradient();
    SetWindowPos(hwnd, NULL, 0, 0, M.winW, M.winH,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

    Plat_ApplyFrameTweaks(hwnd);
    return hwnd;
}

void Ui_Destroy(HWND hwnd)
{
    if (hwnd != NULL)
        DestroyWindow(hwnd);
}

const UiModel *Ui_GetModel(void)
{
    return &s_model;
}

void Ui_SetModel(HWND hwnd, const UiModel *m)
{
    int changed = 0;

    if (!s_haveModel) {
        s_model = *m;
        s_haveModel = 1;
        changed = 1;
    } else {
        if (strcmp(s_model.odo, m->odo) != 0) changed = 1;
        if (strcmp(s_model.trip, m->trip) != 0) changed = 1;
        if (strcmp(s_model.value, m->value) != 0) changed = 1;
        if (strcmp(s_model.kpm, m->kpm) != 0) changed = 1;
        if (strcmp(s_model.maxv, m->maxv) != 0) changed = 1;
        if (strcmp(s_model.uptime, m->uptime) != 0) changed = 1;
        if (strcmp(s_model.status, m->status) != 0) changed = 1;
        if (strcmp(s_model.scale, m->scale) != 0) changed = 1;
        if (s_model.mode_kpm != m->mode_kpm) changed = 1;
        if (s_model.show_gate != m->show_gate) changed = 1;
        if (s_model.transmission != m->transmission) changed = 1;
        if (strcmp(s_model.gear, m->gear) != 0) changed = 1;
        if (s_model.needle != m->needle) changed = 1;
        if (s_model.peak != m->peak) changed = 1;
        if (s_model.active != m->active) changed = 1;
        if (s_model.pressed != m->pressed) changed = 1;
        s_model = *m;
    }

    if (!changed || hwnd == NULL)
        return;

    /* The dial, the gear read-out, the rate panels and the status bar are the
     * parts that move; invalidating only those keeps the blit small. */
    {
        RECT dirty, g;
        Gauge_DirtyRect(&dirty);
        OffsetRect(&dirty, M.dialPanel.left, M.dialPanel.top);
        OffsetRect(&dirty, M.frame, M.frame + M.caption);

        g = M.gearValue;
        OffsetRect(&g, M.frame, M.frame + M.caption);
        UnionRect(&dirty, &dirty, &g);

        g = M.kpmPanel;
        OffsetRect(&g, M.frame, M.frame + M.caption);
        UnionRect(&dirty, &dirty, &g);

        g = M.maxPanel;
        OffsetRect(&g, M.frame, M.frame + M.caption);
        UnionRect(&dirty, &dirty, &g);

        g = M.gearGate;
        OffsetRect(&g, M.frame, M.frame + M.caption);
        UnionRect(&dirty, &dirty, &g);

        g.left = 0;
        g.top = M.frame + M.caption + M.clientH - M.statusbar;
        g.right = M.winW;
        g.bottom = M.winH - M.frame;
        UnionRect(&dirty, &dirty, &g);

        InvalidateRect(hwnd, &dirty, FALSE);
    }
}

void Ui_InvalidateDynamic(HWND hwnd)
{
    if (hwnd != NULL)
        InvalidateRect(hwnd, NULL, FALSE);
}

HMENU Ui_BuildMenu(void)
{
    return s_menu;
}

static void check_item(HMENU m, UINT id, int on)
{
    CheckMenuItem(m, id, MF_BYCOMMAND | (on ? MF_CHECKED : MF_UNCHECKED));
}

void Ui_SyncMenu(HWND hwnd, const UiModel *m)
{
    HMENU view;
    UNUSED(hwnd);
    if (s_menu == NULL)
        return;
    view = GetSubMenu(s_menu, 2);
    if (view != NULL) {
        check_item(view, IDM_SHOW_KPM, m->mode_kpm);
        check_item(view, IDM_SHOW_GEAR, m->show_gate);
        check_item(view, IDM_TM_AUTO, m->transmission == KTM_AUTO);
        check_item(view, IDM_TM_MANUAL, m->transmission == KTM_MANUAL);
        check_item(view, IDM_SHOW_WIDGET, m->widget);
    }
}

void Ui_ShowAbout(HWND hwnd)
{
    char title[64], text[256];
    load_str(IDS_ABOUT_TITLE, title, sizeof(title));
    load_str(IDS_ABOUT_TEXT, text, sizeof(text));
    MessageBoxA(hwnd, text, title, MB_OK | MB_ICONINFORMATION);
}
