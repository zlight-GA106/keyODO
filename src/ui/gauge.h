/* ===========================================================================
 * gauge.h -- the WPM speedometer.
 * ======================================================================== */

#ifndef KEYODO_GAUGE_H
#define KEYODO_GAUGE_H

#include <windows.h>

typedef struct GaugeSpec {
    double value;      /* the switchable reading: WPM or KPM               */
    double peak;       /* peak WPM, drives the telltale -- always a speed  */
    double needle;     /* smoothed needle position, always in WPM          */
    int    mode_kpm;   /* selects the printed unit and the value scaling   */
} GaugeSpec;

/* Draws the dial at the current DC origin. The dial owns the rectangle
 * (0,0)-(w,h) reported by Gauge_Size, so the caller positions it either with a
 * viewport origin (main window) or by just painting at 0,0 (desktop widget). */
void Gauge_Draw(HDC hdc, const GaugeSpec *g);

/* Size of the dial panel in device pixels, for the current metrics. */
void Gauge_Size(int *w, int *h);

/* Dirty rectangle in dial-local coordinates. */
void Gauge_DirtyRect(RECT *out);

#endif /* KEYODO_GAUGE_H */
