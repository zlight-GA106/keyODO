/* ===========================================================================
 * ui.h -- the KeyODO window.
 *
 * The UI module renders a plain data snapshot and reports user commands. It
 * does not know that keystrokes exist, where statistics come from, or that
 * anything is ever written to disk.
 *
 * The window draws its own Windows 98 chrome: frame, gradient caption, caption
 * buttons and menu bar are all painted with GDI, because on a modern Windows
 * the native frame and menu look nothing like the reference screenshots.
 * ======================================================================== */

#ifndef KEYODO_UI_H
#define KEYODO_UI_H

#include <windows.h>

/* Sent to the main window when the detached widget reports a DPI change;
 * Metrics is global, so both windows must rescale together. */
#define UIWM_DPI_REFRESH (WM_APP + 1)

typedef struct UiModel {
    /* formatted read-outs */
    char odo[24];
    char trip[24];
    char value[24];      /* the switchable reading, formatted */
    char kpm[24];
    char maxv[24];
    char uptime[24];
    char status[24];     /* status-bar pane 3, e.g. "62 WPM"  */
    char scale[24];      /* status-bar pane 2, e.g. "192 DPI 200%" */

    /* numeric values the dial needs */
    double gauge_value;  /* the switchable reading, for the embedded read-out */
    double needle;       /* smoothed needle position, always in WPM           */
    double peak;         /* peak WPM                                          */

    /* state */
    int    mode_kpm;
    int    show_gate;
    int    transmission; /* KTM_AUTO / KTM_MANUAL */
    char   gear[8];      /* current ratio NAME, owned by the model */
    int    pressed;      /* IDC_* currently drawn pressed, or 0 */
    int    active;       /* window has focus, caption is drawn active */
    int    widget;       /* detached desktop gauge is visible */
    int    topmost;      /* window is pinned above others      */
    int    window_secs;  /* statistics window: 5, 10 or 30     */
} UiModel;

typedef struct UiHooks {
    void (*on_command)(HWND hwnd, int id);
    void (*on_timer)(HWND hwnd);
    void (*on_close)(HWND hwnd);
    void (*on_destroy)(HWND hwnd);
} UiHooks;

/* Registers the window class and creates the window (not shown). */
HWND Ui_Create(HINSTANCE hInst, const UiHooks *hooks);
void Ui_Destroy(HWND hwnd);

/* Replaces the rendered snapshot and repaints what changed. */
void Ui_SetModel(HWND hwnd, const UiModel *m);
const UiModel *Ui_GetModel(void);

/* Rebuilds the popup menu and ticks the current state. */

/* Shows the modal about box. */
void Ui_ShowAbout(HWND hwnd);

/* Client rectangle that the animated parts live in. */
void Ui_InvalidateDynamic(HWND hwnd);

#endif /* KEYODO_UI_H */
