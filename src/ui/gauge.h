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

void Gauge_Draw(HDC hdc, const GaugeSpec *g);

/* Rectangles that change every frame; the caller invalidates only these. */
void Gauge_DirtyRect(RECT *out);

#endif /* KEYODO_GAUGE_H */
