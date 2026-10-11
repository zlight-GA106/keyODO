/* ===========================================================================
 * widget.h -- the detached desktop gauge.
 *
 * Shows ONLY the speedometer component: bezel, dial face, ticks, numerals,
 * needle, peak telltale and the embedded read-out. No square instrument panel,
 * no group box, no caption, no menu, no other read-out.
 *
 * The window is clipped to the dial's circle with SetWindowRgn, so what floats
 * on the desktop is the round instrument itself. The surround is genuinely
 * transparent and click-through, and the corners do not eat mouse input.
 *
 * It is a separate top-level window so it can be parked anywhere on the
 * desktop, on top of other windows, independently of the main window.
 * ======================================================================== */

#ifndef KEYODO_WIDGET_H
#define KEYODO_WIDGET_H

#include <windows.h>

typedef struct WidgetModel {
    double gauge_value;   /* the switchable reading */
    double needle;        /* smoothed needle position, in WPM */
    double peak;          /* peak WPM, for the telltale */
    int    mode_kpm;      /* selects the printed unit */
    char   gear[8];       /* shown beside the value, e.g. "P" */
} WidgetModel;

typedef struct WidgetHooks {
    /* Right click: the owner shows the widget's context menu at this point
     * (screen coordinates). */
    void (*on_context)(HWND hwnd, POINT screenPt);
    /* The widget was dragged: persist the new position. */
    void (*on_moved)(HWND hwnd);
    /* The widget landed on a monitor with a different DPI. Metrics is a single
     * global, so the owner must rescale both windows together. */
    void (*on_dpi_changed)(HWND hwnd);
} WidgetHooks;

HWND Widget_Create(HINSTANCE hInst, const WidgetHooks *hooks);
void Widget_Destroy(HWND hwnd);

void Widget_SetModel(HWND hwnd, const WidgetModel *m);

/* Position and visibility, both persisted by the owner. */
void Widget_Place(HWND hwnd, int x, int y);
void Widget_PlaceClamped(HWND hwnd, int x, int y);
void Widget_GetPlace(HWND hwnd, int *x, int *y);
void Widget_Show(HWND hwnd, int show);
int  Widget_IsVisible(HWND hwnd);

/* Re-reads the DPI-dependent size; call after a DPI change. */
void Widget_Resize(HWND hwnd);

#endif /* KEYODO_WIDGET_H */
