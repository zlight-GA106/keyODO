/* ===========================================================================
 * app.c -- wiring, and nothing else.
 *
 * Timers
 *   TIMER_ANIM (33 ms) drives the statistics tick, the needle smoothing and the
 *   repaint. The UI is only invalidated when a displayed value actually
 *   changed, so an idle KeyODO costs nothing.
 *
 * Windows
 *   Two of them: the main window and the optional detached desktop gauge. Both
 *   are fed from the same snapshot; neither knows about the other.
 *
 * Privacy
 *   Only virtual key codes and scan codes ever reach this program. No keyboard
 *   layout translation happens, no character is produced, nothing typed is
 *   stored or transmitted. The ini file holds counters and view settings only.
 * ======================================================================== */

#include "app.h"

#include "common.h"
#include "ui/ui.h"
#include "ui/widget.h"
#include "ui/metrics.h"
#include "stats/stats.h"
#include "input/input.h"
#include "storage/storage.h"
#include "platform/platform.h"

#include "keyodo_layout.h"
#include "resource.h"

#include <stdio.h>
#include <string.h>

#define TIMER_ANIM 1
#define ANIM_MS    33

static HINSTANCE s_inst = NULL;
static HWND      s_hwnd = NULL;
static HWND      s_widget = NULL;
static StoreData s_store;
static const InputBackend *s_input = NULL;
static int       s_running = 0;
static UiModel   s_ui;
static int       s_dpiOverride = 0;   /* this run only; never written back */

static void on_command(HWND hwnd, int id);

/* ------------------------------------------------------------------ input */

static void key_sink(unsigned int vkey, int isDown, int isRepeat,
                     unsigned int scan, void *user)
{
    UNUSED(scan);
    UNUSED(user);
    if (!isDown)
        return;                     /* statistics only needs presses */
    Stats_OnKey(vkey, isRepeat, Plat_TickMs());
}

/* --------------------------------------------------------------- helpers */

static void format_uptime(unsigned long secs, char *buf, int cch)
{
    unsigned long h = secs / 3600UL;
    unsigned long m = (secs / 60UL) % 60UL;
    unsigned long s = secs % 60UL;
    wsprintfA(buf, "%02lu:%02lu:%02lu", h, m, s);
}

static void format_scale(char *buf, int cch)
{
    UNUSED(cch);
    wsprintfA(buf, "%d DPI %d%%", M.dpi, M.scalePct);
}

static void build_model(void)
{
    StatsSnapshot snap;
    const char *unit;
    int shown;

    Stats_Get(&snap);

    unit = s_store.mode_kpm ? KDM_UNIT_KPM : KDM_UNIT_WPM;
    shown = (int)((s_store.mode_kpm ? snap.kpm : snap.wpm) + 0.5);

    if (shown == 0)
        shown = 0;
    KeyU64_FormatPad(snap.odo, 7, s_ui.odo, sizeof(s_ui.odo));
    KeyU64_FormatPad(snap.trip, 4, s_ui.trip, sizeof(s_ui.trip));
    wsprintfA(s_ui.value, "%d", shown);
    wsprintfA(s_ui.kpm, "%d", (int)(snap.kpm + 0.5));
    wsprintfA(s_ui.maxv, "%d", (int)(snap.max_wpm + 0.5));
    wsprintfA(s_ui.status, "%d %s", shown, unit);
    format_uptime(snap.sessionSecs, s_ui.uptime, sizeof(s_ui.uptime));
    format_scale(s_ui.scale, sizeof(s_ui.scale));

    s_ui.gauge_value = (double)shown;
    s_ui.needle = snap.needle;
    s_ui.peak = snap.max_wpm;
    s_ui.mode_kpm = s_store.mode_kpm;
    s_ui.show_gate = s_store.show_gate;
    s_ui.transmission = s_store.transmission;
    /* Copy, do not alias: `snap` is a local and gear would dangle. */
    lstrcpynA(s_ui.gear, snap.gear, sizeof(s_ui.gear));

    /* pressed and active are owned by the UI module; do not stomp them. */
    s_ui.pressed = Ui_GetModel()->pressed;
    s_ui.active = Ui_GetModel()->active;
    s_ui.widget = (s_widget != NULL) && Widget_IsVisible(s_widget);
}

static void build_widget(void)
{
    WidgetModel wm;

    if (s_widget == NULL)
        return;
    wm.gauge_value = s_ui.gauge_value;
    wm.needle = s_ui.needle;
    wm.peak = s_ui.peak;
    wm.mode_kpm = s_store.mode_kpm;
    Widget_SetModel(s_widget, &wm);
}

static void refresh(void)
{
    build_model();
    Ui_SetModel(s_hwnd, &s_ui);
    build_widget();
}

/* ------------------------------------------------------------- the gauge */

static void widget_show(int show)
{
    s_store.widget_shown = show ? 1 : 0;
    if (s_widget == NULL)
        return;
    if (show && s_store.widget_x != 0x7FFFFFFF) {
        Widget_PlaceClamped(s_widget, s_store.widget_x, s_store.widget_y);
    }
    Widget_Show(s_widget, show);
    if (show && s_store.widget_x == 0x7FFFFFFF) {
        int x, y;
        Widget_GetPlace(s_widget, &x, &y);
        Widget_PlaceClamped(s_widget, x, y);
    }
    refresh();
    Ui_SyncMenu(s_hwnd, &s_ui);
}

static void widget_remember(void)
{
    int x, y;
    if (s_widget == NULL)
        return;
    Widget_GetPlace(s_widget, &x, &y);
    s_store.widget_x = x;
    s_store.widget_y = y;
}

static void widget_context(HWND hwnd, POINT pt)
{
    HMENU m;
    char sMain[64], sHide[64], sExit[64];
    int cmd;

    UNUSED(hwnd);
    m = CreatePopupMenu();
    if (m == NULL)
        return;
    LoadStringA(s_inst, IDS_WIDGET_MENU_MAIN, sMain, sizeof(sMain));
    LoadStringA(s_inst, IDS_WIDGET_MENU_HIDE, sHide, sizeof(sHide));
    LoadStringA(s_inst, IDS_WIDGET_MENU_EXIT, sExit, sizeof(sExit));

    AppendMenuA(m, MF_STRING, IDM_WIDGET_SHOWMAIN, sMain);
    AppendMenuA(m, MF_STRING, IDM_WIDGET_HIDE, sHide);
    AppendMenuA(m, MF_SEPARATOR, 0, NULL);
    AppendMenuA(m, MF_STRING, IDM_EXIT, sExit);

    SetForegroundWindow(hwnd);
    cmd = (int)TrackPopupMenu(m, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
                              pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(m);

    if (cmd != 0)
        on_command(s_hwnd, cmd);
}

static void widget_moved(HWND hwnd)
{
    UNUSED(hwnd);
    widget_remember();
}

static void widget_dpi_changed(HWND hwnd)
{
    /* Metrics is a single global describing one scaling, so both windows are
     * rebuilt together rather than left at different scales. */
    Metrics_Init(hwnd);
    Widget_Resize(hwnd);
    if (s_hwnd != NULL)
        PostMessage(s_hwnd, UIWM_DPI_REFRESH, 0, 0);
}

static const WidgetHooks kWigHooks = {
    widget_context,
    widget_moved,
    widget_dpi_changed
};

/* -------------------------------------------------------------- commands */

static void apply_topmost(void)
{
    SetWindowPos(s_hwnd, s_store.always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST,
                 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

static void on_command(HWND hwnd, int id)
{
    int needMenuSync = 0;

    switch (id) {
    case IDM_EXIT:
        PostMessage(hwnd, WM_CLOSE, 0, 0);
        return;

    case IDM_RESET_TRIP:
    case IDC_BTN_TRIP:
        Stats_ResetTrip();
        break;

    case IDM_RESET_MAX:
    case IDC_BTN_PEAK:
        Stats_ResetMax();
        break;

    case IDM_SHOW_WIDGET:
        widget_show(!s_store.widget_shown);
        return;

    case IDM_WIDGET_SHOWMAIN:
        ShowWindow(hwnd, SW_SHOW);
        ShowWindow(hwnd, SW_RESTORE);
        SetForegroundWindow(hwnd);
        return;

    case IDM_WIDGET_HIDE:
        widget_show(0);
        return;

    case IDM_SHOW_KPM:
        s_store.mode_kpm = !s_store.mode_kpm;
        needMenuSync = 1;
        break;

    case IDM_SHOW_GEAR:
        s_store.show_gate = !s_store.show_gate;
        needMenuSync = 1;
        break;

    case IDM_TM_AUTO:
        s_store.transmission = KTM_AUTO;
        Stats_SetTransmission(KTM_AUTO);
        needMenuSync = 1;
        break;

    case IDM_TM_MANUAL:
        s_store.transmission = KTM_MANUAL;
        Stats_SetTransmission(KTM_MANUAL);
        needMenuSync = 1;
        break;

    case IDM_TOPMOST:
        s_store.always_on_top = !s_store.always_on_top;
        apply_topmost();
        needMenuSync = 1;
        break;

    case IDM_WIN5:
        s_store.window_secs = 5;
        Stats_SetWindow(5);
        break;
    case IDM_WIN10:
        s_store.window_secs = 10;
        Stats_SetWindow(10);
        break;
    case IDM_WIN30:
        s_store.window_secs = 30;
        Stats_SetWindow(30);
        break;

    case IDM_ABOUT:
        Ui_ShowAbout(hwnd);
        return;

    default:
        return;
    }

    Stats_Tick(Plat_TickMs());
    refresh();
    if (needMenuSync)
        Ui_SyncMenu(hwnd, &s_ui);
    Ui_InvalidateDynamic(hwnd);
}

/* ----------------------------------------------------------------- hooks */

static void on_timer(HWND hwnd)
{
    Stats_Tick(Plat_TickMs());
    refresh();
    UNUSED(hwnd);
}

static void on_close(HWND hwnd)
{
    RECT wr;
    StatsSnapshot snap;

    /* Pull the live totals back into the record that gets written. Without
     * this the file keeps whatever was loaded at startup. */
    Stats_Get(&snap);
    s_store.odo = snap.odo;
    s_store.trip = snap.trip;
    s_store.max_wpm = snap.max_wpm;

    /* Remember where the user put the windows. */
    if (GetWindowRect(hwnd, &wr)) {
        s_store.win_x = wr.left;
        s_store.win_y = wr.top;
    }
    widget_remember();

    Storage_Save(&s_store);

    KillTimer(hwnd, TIMER_ANIM);
    if (s_input != NULL)
        s_input->stop();
    DestroyWindow(hwnd);
}

static void on_destroy(HWND hwnd)
{
    UNUSED(hwnd);
    if (s_widget != NULL) {
        DestroyWindow(s_widget);
        s_widget = NULL;
    }
    PostQuitMessage(0);
}

static const UiHooks kHooks = {
    on_command,
    on_timer,
    on_close,
    on_destroy
};

/* ------------------------------------------------------------------- life */

int App_Init(HINSTANCE hInst, const char *cmdLine)
{
    StatsSnapshot snap;
    int x, y;

    s_inst = hInst;

    Storage_Load(&s_store);

    /* Command line overrides, mainly so scaling and the two input backends can
     * be exercised without editing the ini file.
     *
     * The DPI override is deliberately kept OUT of s_store: it applies to this
     * run only. Writing it back would mean one `--dpi=96` debugging run pinned
     * the saved setting to 96 permanently. */
    s_dpiOverride = s_store.dpi_override;
    if (cmdLine != NULL) {
        const char *p;
        if (strstr(cmdLine, "--input=hook") != NULL)
            lstrcpynA(s_store.input_backend, "hook", sizeof(s_store.input_backend));
        else if (strstr(cmdLine, "--input=raw") != NULL)
            lstrcpynA(s_store.input_backend, "raw", sizeof(s_store.input_backend));

        p = strstr(cmdLine, "--dpi=");
        if (p != NULL) {
            int d = 0;
            p += 6;
            while (*p >= '0' && *p <= '9') {
                d = d * 10 + (*p - '0');
                p++;
            }
            if (d >= 72 && d <= 480)
                s_dpiOverride = d;
        }
    }

    Metrics_SetOverride(s_dpiOverride);

    Stats_Init(s_store.window_secs, s_store.transmission);
    Stats_Restore(s_store.odo, s_store.trip, s_store.max_wpm);
    Stats_Tick(Plat_TickMs());
    Stats_Get(&snap);

    s_hwnd = Ui_Create(hInst, &kHooks);
    if (s_hwnd == NULL)
        return 0;

    /* The detached gauge is created once and shown or hidden; recreating it
     * would lose its position. */
    s_widget = Widget_Create(hInst, &kWigHooks);
    if (s_widget != NULL && s_store.widget_x != 0x7FFFFFFF)
        Widget_PlaceClamped(s_widget, s_store.widget_x, s_store.widget_y);

    /* Input starts after the window exists: Raw Input needs an HWND. */
    s_input = Input_ByName(s_store.input_backend);
    if (s_input == NULL || !s_input->start(s_hwnd, key_sink, NULL)) {
        /* Fall back rather than run blind. */
        s_input = Input_RawInput();
        if (!s_input->start(s_hwnd, key_sink, NULL))
            return 0;
    }

    if (s_store.win_x == 0x7FFFFFFF || s_store.win_y == 0x7FFFFFFF) {
        x = (GetSystemMetrics(SM_CXSCREEN) - M.winW) / 2;
        y = (GetSystemMetrics(SM_CYSCREEN) - M.winH) / 2;
    } else {
        x = s_store.win_x;
        y = s_store.win_y;
    }
    Plat_ClampToWorkArea(&x, &y, M.winW, M.winH);
    SetWindowPos(s_hwnd, NULL, x, y, 0, 0,
                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

    if (s_store.always_on_top)
        apply_topmost();

    refresh();
    Ui_SyncMenu(s_hwnd, &s_ui);

    if (s_store.widget_shown)
        widget_show(1);

    SetTimer(s_hwnd, TIMER_ANIM, ANIM_MS, NULL);

    s_running = 1;
    return 1;
}

void App_Shutdown(void)
{
    if (s_running && s_input != NULL)
        s_input->stop();
    Stats_Shutdown();
    Metrics_Done();
    s_running = 0;
}

HWND App_Window(void)
{
    return s_hwnd;
}

int App_PreTranslate(MSG *msg)
{
    long result = 0;
    if (msg == NULL || s_input == NULL || s_input->on_message == NULL)
        return 0;
    if (s_input->on_message((void *)msg->hwnd, msg->message,
                            (unsigned long)msg->wParam,
                            (unsigned long)msg->lParam, &result))
        return 1;
    return 0;
}
