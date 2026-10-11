/* ===========================================================================
 * ui.c -- KeyODO main window: Windows 98 chrome, layout and painting.
 *
 * The window is a WS_POPUP with no system caption. Everything the reference
 * screenshot shows -- 1 px black outline, 3 px frame, gradient caption,
 * minimise and close buttons, menu bar, group boxes, dial, drums, gear gate,
 * status bar -- is painted here with GDI using the pinned classic palette.
 * That is the only way to get the same picture on Windows 98 SE and on a
 * modern Windows, where the native frame and menu are themed.
 *
 * Layout constants come from include/keyodo_layout.h and are the same numbers
 * the reference renderer uses. Content is drawn with the viewport origin moved
 * to (KW_FRAME, KW_FRAME + KW_CAPTION_H), so client-area coordinates apply
 * unchanged.
 * ======================================================================== */

#include "ui.h"
#include "theme.h"
#include "widgets.h"
#include "gauge.h"
#include "gear.h"

#include "keyodo_layout.h"
#include "../common.h"
#include "../platform/platform.h"
#include "resource.h"

#include <string.h>

/* The layout header names these KW_*; the drawing code reads better with the
 * short forms, and they are the same numbers. */
#define WIN_W  KW_WIN_W
#define WIN_H  KW_WIN_H

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
            SetPixelV(hdc, ox + x, oy + y, c);
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

/* The caption fade never changes, so it is rasterised once. Drawing 474
 * FillRect calls on every frame would be far too much for a 1998 CPU. */
static void ensure_caption_gradient(HDC hdc)
{
    const Theme *t = &g_theme;
    int w = WIN_W - 2 * KW_FRAME;
    int h = KW_CAPTION_H;
    HDC mdc;
    HGDIOBJ old;
    int x;
    int r0, g0, b0, r1, g1, b1;

    if (s_gradBmp != NULL && s_gradW == w)
        return;
    if (s_gradBmp != NULL) {
        DeleteObject(s_gradBmp);
        s_gradBmp = NULL;
    }

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
#define MENU_PAD_X      6
#define MENU_GAP        12

/* Fills `out` with the clickable rectangle of item `index`; 0 if out of range. */
static int menubar_item_rect(HDC hdc, int index, RECT *out)
{
    int x, i;
    if (index < 0 || index >= MENU_ITEM_COUNT)
        return 0;
    x = 2;
    for (i = 0; i < MENU_ITEM_COUNT; i++) {
        int w = Widget_TextWidth(hdc, g_theme.f_ui, S.menu[i]) + MENU_GAP;
        if (i == index) {
            out->left = x;
            out->top = 0;
            out->right = x + w;
            out->bottom = KW_MENUBAR_H;
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

    rc.left = 0; rc.top = 0; rc.right = WIN_W; rc.bottom = WIN_H;
    Theme_Rect1(hdc, &rc, t->frame);

    for (i = 1; i < KW_FRAME; i++) {
        rc.left = i; rc.top = i; rc.right = WIN_W - i; rc.bottom = WIN_H - i;
        Theme_Rect1(hdc, &rc, t->face);
    }
}

static void draw_caption(HDC hdc)
{
    const Theme *t = &g_theme;
    RECT rc;
    char title[64];

    rc.left = KW_FRAME;
    rc.top = KW_FRAME;
    rc.right = WIN_W - KW_FRAME;
    rc.bottom = KW_FRAME + KW_CAPTION_H;

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

    draw_icon(hdc, KW_ICON_X, KW_ICON_Y);

    load_str(IDS_APP_TITLE, title, sizeof(title));
    /* Transparent: the caption is a gradient, an opaque background would
     * paint a flat block across it. */
    Widget_TextAtClear(hdc, KW_TITLE_X, KW_TITLE_Y, title, t->f_ui_bold,
                       s_model.active ? t->caption_text : t->caption_inactive_text);

    {
        RECT b;
        b.left = KW_CAPBTN_MIN_X; b.top = KW_CAPBTN_Y;
        b.right = b.left + KW_CAPBTN_W; b.bottom = b.top + KW_CAPBTN_H;
        Widget_CaptionButton(hdc, &b, kGlyphMin, s_model.pressed == IDC_CAP_MIN);

        b.left = KW_CAPBTN_CLOSE_X; b.top = KW_CAPBTN_Y;
        b.right = b.left + KW_CAPBTN_W; b.bottom = b.top + KW_CAPBTN_H;
        Widget_CaptionButton(hdc, &b, kGlyphClose, s_model.pressed == IDC_CAP_CLOSE);
    }
}

static void draw_menubar(HDC hdc)
{
    const Theme *t = &g_theme;
    RECT rc;
    int i;

    rc.left = 0; rc.top = 0; rc.right = KW_CLIENT_W; rc.bottom = KW_MENUBAR_H;
    Theme_Fill(hdc, &rc, t->menu);

    for (i = 0; i < MENU_ITEM_COUNT; i++) {
        const char *label = S.menu[i];
        const char *lp;
        RECT cell;
        int px, aw;

        if (!menubar_item_rect(hdc, i, &cell))
            break;

        Widget_TextAt(hdc, cell.left + MENU_PAD_X, 4, label,
                      t->f_ui, t->menutext, t->menu);

        /* accelerator underline under the letter inside the parentheses */
        px = cell.left + MENU_PAD_X;
        for (lp = label; *lp != 0 && *lp != '('; lp++)
            ;
        if (*lp == '(') {
            char prefix[32];
            char one[2];
            int n = (int)(lp - label) + 1;
            if (n > (int)sizeof(prefix) - 1) n = (int)sizeof(prefix) - 1;
            memcpy(prefix, label, n);
            prefix[n] = 0;
            px += Widget_TextWidth(hdc, t->f_ui, prefix);
            aw = 6;
            lp++;
            if (*lp != 0 && *lp != ')') {
                one[0] = *lp;
                one[1] = 0;
                aw = Widget_TextWidth(hdc, t->f_ui, one);
            }
            Theme_HLine(hdc, px, px + aw, KW_MENUBAR_H - 3, t->menutext);
        }
    }
}

static void draw_statusbar(HDC hdc)
{
    const Theme *t = &g_theme;
    RECT rc;
    int t0 = KW_CLIENT_H - KW_STATUSBAR_H;

    rc.left = 0; rc.top = t0; rc.right = KW_CLIENT_W; rc.bottom = KW_CLIENT_H;
    Theme_Fill(hdc, &rc, t->face);
    Theme_DrawEdge(hdc, &rc, TK_SUNKEN, 1);

    Theme_VLine(hdc, SB_PANE2_X - 2, t0 + 1, KW_CLIENT_H - 1, t->shadow);
    Theme_VLine(hdc, SB_PANE2_X - 1, t0 + 1, KW_CLIENT_H - 1, t->hilight);
    Theme_VLine(hdc, SB_PANE3_X - 2, t0 + 1, KW_CLIENT_H - 1, t->shadow);
    Theme_VLine(hdc, SB_PANE3_X - 1, t0 + 1, KW_CLIENT_H - 1, t->hilight);

    Widget_TextAt(hdc, 5, t0 + 4, S.statusReady, t->f_ui, t->btntext, t->face);
    Widget_TextAt(hdc, SB_PANE2_X + 3, t0 + 4, s_model.uptime, t->f_ui,
                  t->btntext, t->face);
    Widget_TextAt(hdc, SB_PANE3_X + 3, t0 + 4, s_model.status, t->f_ui,
                  t->btntext, t->face);
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

    band.left = 0; band.top = KW_CONTENT_Y;
    band.right = KW_CLIENT_W; band.bottom = KW_CONTENT_Y + KW_CONTENT_H;
    Theme_Fill(hdc, &band, t->face);

    /* ---- speedometer ---- */
    RECT_SET(rc, RECT_GB_SPEED);
    Widget_GroupBox(hdc, &rc, S.gbSpeed);

    {
        GaugeSpec g;
        g.value = s_model.gauge_value;
        g.peak = s_model.peak;
        g.needle = s_model.needle;
        g.mode_kpm = s_model.mode_kpm;
        Gauge_Draw(hdc, &g);
    }

    RECT_SET(rc, RECT_GEAR_VALUE);
    Gear_DrawValue(hdc, &rc, s_model.gear,
                   s_model.value);

    /* ---- odometer ---- */
    RECT_SET(rc, RECT_GB_ODO);
    Widget_GroupBox(hdc, &rc, S.gbOdo);

    Widget_TextAt(hdc, 270, ODO_CAP_Y, S.lblOdo, t->f_ui, t->btntext, t->face);
    RECT_SET(rc, RECT_ODO_DRUM);
    Widget_Drum(hdc, &rc, s_model.odo, DRUM_CELL_W);

    Widget_TextAt(hdc, 270, TRIP_CAP_Y, S.lblTrip, t->f_ui, t->btntext, t->face);
    RECT_SET(rc, RECT_TRIP_DRUM);
    Widget_Drum(hdc, &rc, s_model.trip, DRUM_CELL_W);

    RECT_SET(rc, RECT_TRIP_RESET);
    Widget_PushButton(hdc, &rc, S.btnTrip, s_model.pressed == IDC_BTN_TRIP);

    /* ---- rate ---- */
    RECT_SET(rc, RECT_GB_RATE);
    Widget_GroupBox(hdc, &rc, S.gbRate);

    Widget_TextAt(hdc, 270, KPM_CAP_Y, S.lblKpm, t->f_ui, t->btntext, t->face);
    RECT_SET(rc, RECT_KPM_PANEL);
    Widget_Readout(hdc, &rc, s_model.kpm, t->f_readout);

    Widget_TextAt(hdc, 270, MAX_CAP_Y, S.lblMax, t->f_ui, t->btntext, t->face);
    RECT_SET(rc, RECT_MAX_PANEL);
    Widget_Readout(hdc, &rc, s_model.maxv, t->f_readout);

    if (s_model.show_gate) {
        RECT_SET(rc, RECT_GEAR_GATE);
        if (s_model.transmission == KTM_MANUAL)
            Gear_DrawGate(hdc, &rc, manGears, KTM_COUNT_MANUAL, s_model.gear);
        else
            Gear_DrawGate(hdc, &rc, autoGears, KTM_COUNT_AUTO, s_model.gear);
    }

    RECT_SET(rc, RECT_PEAK_RESET);
    Widget_PushButton(hdc, &rc, S.btnPeak, s_model.pressed == IDC_BTN_PEAK);
}

static void paint_all(HDC hdc)
{
    int saved = SaveDC(hdc);

    draw_frame(hdc);
    draw_caption(hdc);

    SetViewportOrgEx(hdc, KW_FRAME, KW_FRAME + KW_CAPTION_H, NULL);
    draw_menubar(hdc);
    draw_content(hdc);
    draw_statusbar(hdc);

    RestoreDC(hdc, saved);
}

/* ======================================================================
 * hit testing
 * ==================================================================== */

static void cap_rect(int x, RECT *r)
{
    r->left = x; r->top = KW_CAPBTN_Y;
    r->right = x + KW_CAPBTN_W; r->bottom = KW_CAPBTN_Y + KW_CAPBTN_H;
}

/* kind: 0 = nothing, 1 = control (id set), 2 = caption drag */
static int hit_test(HWND hwnd, POINT pt, int *id)
{
    RECT r;

    *id = 0;

    cap_rect(KW_CAPBTN_CLOSE_X, &r);
    if (PtInRect(&r, pt)) { *id = IDC_CAP_CLOSE; return 1; }
    cap_rect(KW_CAPBTN_MIN_X, &r);
    if (PtInRect(&r, pt)) { *id = IDC_CAP_MIN; return 1; }

    r.left = KW_FRAME; r.top = KW_FRAME;
    r.right = KW_WIN_W - KW_FRAME; r.bottom = KW_FRAME + KW_CAPTION_H;
    if (PtInRect(&r, pt))
        return 2;

    /* client-area buttons */
    {
        POINT c;
        c.x = pt.x - KW_FRAME;
        c.y = pt.y - (KW_FRAME + KW_CAPTION_H);
        RECT_SET(r, RECT_TRIP_RESET);
        if (PtInRect(&r, c)) { *id = IDC_BTN_TRIP; return 1; }
        RECT_SET(r, RECT_PEAK_RESET);
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
        cell.left = 2;
    ReleaseDC(hwnd, hdc);

    GetWindowRect(hwnd, &wr);
    pt.x = wr.left + KW_FRAME + cell.left;
    pt.y = wr.top + KW_FRAME + KW_CAPTION_H + KW_MENUBAR_H;

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

    case WM_NCHITTEST: {
        POINT pt;
        int id;
        pt.x = (short)LOWORD(lp);
        pt.y = (short)HIWORD(lp);
        ScreenToClient(hwnd, &pt);
        if (hit_test(hwnd, pt, &id) == 2)
            return HTCAPTION;
        return HTCLIENT;
    }

    case WM_LBUTTONDOWN: {
        POINT pt;
        int id;
        int kind;

        pt.x = (short)LOWORD(lp);
        pt.y = (short)HIWORD(lp);

        /* menu bar click */
        if (pt.y >= KW_FRAME + KW_CAPTION_H &&
            pt.y < KW_FRAME + KW_CAPTION_H + KW_MENUBAR_H &&
            pt.x >= KW_FRAME && pt.x < KW_FRAME + KW_CLIENT_W) {
            HDC hdc = GetDC(hwnd);
            int idx = menubar_index_at(hdc, pt.x - KW_FRAME);
            ReleaseDC(hwnd, hdc);
            if (idx >= 0) {
                show_menu(hwnd, idx);
                return 0;
            }
        }

        kind = hit_test(hwnd, pt, &id);
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
        kind = hit_test(hwnd, pt, &id);
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
        if (s_gradBmp != NULL) {
            DeleteObject(s_gradBmp);
            s_gradBmp = NULL;
            s_gradW = 0;
        }
        Widget_Shutdown();
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
     * draw_frame()/draw_caption(), which keeps it identical on every Windows.
     * WM_NCHITTEST returns HTCAPTION over the caption so dragging, the system
     * menu and Alt+F4 still work through DefWindowProc. */
    style = WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN;

    x = (GetSystemMetrics(SM_CXSCREEN) - WIN_W) / 2;
    y = (GetSystemMetrics(SM_CYSCREEN) - WIN_H) / 2;

    hwnd = CreateWindowExA(WS_EX_APPWINDOW, "KeyODO_Main", KEYODO_NAME, style,
                           x, y, WIN_W, WIN_H, NULL, NULL, hInst, NULL);
    if (hwnd == NULL)
        return NULL;

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
        s_model.pressed = s_model.pressed;
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
        OffsetRect(&dirty, KW_FRAME, KW_FRAME + KW_CAPTION_H);

        RECT_SET(g, RECT_GEAR_VALUE);
        OffsetRect(&g, KW_FRAME, KW_FRAME + KW_CAPTION_H);
        UnionRect(&dirty, &dirty, &g);

        RECT_SET(g, RECT_KPM_PANEL);
        OffsetRect(&g, KW_FRAME, KW_FRAME + KW_CAPTION_H);
        UnionRect(&dirty, &dirty, &g);

        RECT_SET(g, RECT_MAX_PANEL);
        OffsetRect(&g, KW_FRAME, KW_FRAME + KW_CAPTION_H);
        UnionRect(&dirty, &dirty, &g);

        RECT_SET(g, RECT_GEAR_GATE);
        OffsetRect(&g, KW_FRAME, KW_FRAME + KW_CAPTION_H);
        UnionRect(&dirty, &dirty, &g);

        g.left = 0;
        g.top = KW_FRAME + KW_CAPTION_H + KW_CLIENT_H - KW_STATUSBAR_H;
        g.right = WIN_W;
        g.bottom = WIN_H - KW_FRAME;
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
    }
}

void Ui_ShowAbout(HWND hwnd)
{
    char title[64], text[256];
    load_str(IDS_ABOUT_TITLE, title, sizeof(title));
    load_str(IDS_ABOUT_TEXT, text, sizeof(text));
    MessageBoxA(hwnd, text, title, MB_OK | MB_ICONINFORMATION);
}
