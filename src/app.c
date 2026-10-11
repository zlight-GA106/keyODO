/* ===========================================================================
 * app.c -- wiring, and nothing else.
 *
 * Timers
 *   TIMER_ANIM (33 ms) drives the statistics tick, the needle smoothing and the
 *   repaint. The UI is only invalidated when a displayed value actually
 *   changed, so an idle KeyODO costs nothing.
 *
 * Privacy
 *   Only virtual key codes and scan codes ever reach this program. No keyboard
 *   layout translation happens, no character is produced, nothing typed is
 *   stored or transmitted. The ini file holds counters and view settings only.
 * ======================================================================== */

#include "app.h"

#include "common.h"
#include "ui/ui.h"
#include "stats/stats.h"
#include "input/input.h"
#include "storage/storage.h"
#include "platform/platform.h"
#include "ui/ui.h"

#include "keyodo_layout.h"
#include "resource.h"

#include <stdio.h>
#include <string.h>

#define TIMER_ANIM 1
#define ANIM_MS    33

static HINSTANCE s_inst = NULL;
static HWND      s_hwnd = NULL;
static StoreData s_store;
static const InputBackend *s_input = NULL;
static int       s_running = 0;
static UiModel   s_ui;

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

static void build_model(void)
{
    StatsSnapshot snap;
    const char *unit;
    int shown;

    Stats_Get(&snap);

    unit = s_store.mode_kpm ? KDM_UNIT_KPM : KDM_UNIT_WPM;
    shown = (int)((s_store.mode_kpm ? snap.kpm : snap.wpm) + 0.5);

    KeyU64_FormatPad(snap.odo, 7, s_ui.odo, sizeof(s_ui.odo));
    KeyU64_FormatPad(snap.trip, 4, s_ui.trip, sizeof(s_ui.trip));
    wsprintfA(s_ui.value, "%d", shown);
    wsprintfA(s_ui.kpm, "%d", (int)(snap.kpm + 0.5));
    wsprintfA(s_ui.maxv, "%d", (int)(snap.max_wpm + 0.5));
    wsprintfA(s_ui.status, "%d %s", shown, unit);
    format_uptime(snap.sessionSecs, s_ui.uptime, sizeof(s_ui.uptime));

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
}

/* -------------------------------------------------------------- commands */

static void apply_topmost(void)
{
    SetWindowPos(s_hwnd, s_store.always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST,
                 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

static void on_command(HWND hwnd, int id)
{
    StatsSnapshot snap;
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

    Stats_Get(&snap);
    build_model();
    Ui_SetModel(hwnd, &s_ui);
    if (needMenuSync)
        Ui_SyncMenu(hwnd, &s_ui);
    Ui_InvalidateDynamic(hwnd);
}

/* ----------------------------------------------------------------- hooks */

static void on_timer(HWND hwnd)
{
    Stats_Tick(Plat_TickMs());
    build_model();
    Ui_SetModel(hwnd, &s_ui);
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

    /* Remember where the user put the window. */
    if (GetWindowRect(hwnd, &wr)) {
        s_store.win_x = wr.left;
        s_store.win_y = wr.top;
    }
    Storage_Save(&s_store);

    KillTimer(hwnd, TIMER_ANIM);
    if (s_input != NULL)
        s_input->stop();
    DestroyWindow(hwnd);
}

static void on_destroy(HWND hwnd)
{
    UNUSED(hwnd);
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

    /* Command line override, mainly so the two input backends can be compared
     * without editing the ini file. */
    if (cmdLine != NULL) {
        if (strstr(cmdLine, "--input=hook") != NULL)
            lstrcpynA(s_store.input_backend, "hook", sizeof(s_store.input_backend));
        else if (strstr(cmdLine, "--input=raw") != NULL)
            lstrcpynA(s_store.input_backend, "raw", sizeof(s_store.input_backend));
    }

    Stats_Init(s_store.window_secs, s_store.transmission);
    Stats_Restore(s_store.odo, s_store.trip, s_store.max_wpm);
    Stats_Tick(Plat_TickMs());
    Stats_Get(&snap);

    s_hwnd = Ui_Create(hInst, &kHooks);
    if (s_hwnd == NULL)
        return 0;

    /* Input starts after the window exists: Raw Input needs an HWND. */
    s_input = Input_ByName(s_store.input_backend);
    if (s_input == NULL || !s_input->start(s_hwnd, key_sink, NULL)) {
        /* Fall back rather than run blind. */
        s_input = Input_RawInput();
        if (!s_input->start(s_hwnd, key_sink, NULL))
            return 0;
    }

    if (s_store.win_x == 0x7FFFFFFF || s_store.win_y == 0x7FFFFFFF) {
        x = (GetSystemMetrics(SM_CXSCREEN) - KW_WIN_W) / 2;
        y = (GetSystemMetrics(SM_CYSCREEN) - KW_WIN_H) / 2;
    } else {
        x = s_store.win_x;
        y = s_store.win_y;
    }
    Plat_ClampToWorkArea(&x, &y, KW_WIN_W, KW_WIN_H);
    SetWindowPos(s_hwnd, NULL, x, y, 0, 0,
                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

    if (s_store.always_on_top)
        apply_topmost();

    build_model();
    Ui_SetModel(s_hwnd, &s_ui);
    Ui_SyncMenu(s_hwnd, &s_ui);

    SetTimer(s_hwnd, TIMER_ANIM, ANIM_MS, NULL);

    s_running = 1;
    return 1;
}

void App_Shutdown(void)
{
    if (s_running && s_input != NULL)
        s_input->stop();
    Stats_Shutdown();
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
