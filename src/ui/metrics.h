/* ===========================================================================
 * metrics.h -- DPI-aware layout.
 *
 * include/keyodo_layout.h stays the single 96-DPI specification. This module
 * turns it into the values a particular monitor actually needs, so the window
 * is legible and correctly sized at 100%, 125%, 150% and 200% -- the scalings
 * that 1080p, 2K and 4K displays normally use.
 *
 * Scaling is explicit rather than done with a GDI world transform:
 *   - every coordinate is MulDiv'd against the monitor DPI, so at 200% the
 *     picture is exactly twice the 96-DPI one, bevels included;
 *   - fonts are created at the scaled pixel size, so text stays crisp instead
 *     of being stretched from a 96-DPI bitmap;
 *   - nothing is ever bitmap-stretched, so no size is blurry.
 * At 100% every value below equals the constant in keyodo_layout.h, which is
 * what keeps the reference-render comparison exact.
 * ======================================================================== */

#ifndef KEYODO_METRICS_H
#define KEYODO_METRICS_H

#include <windows.h>

typedef struct Metrics {
    int dpi;            /* 96 = 100%, 120 = 125%, 144 = 150%, 192 = 200%      */
    int scalePct;       /* dpi * 100 / 96, for the status bar                 */
    int bevel;          /* 3D edge thickness in device pixels                 */
    int unit;           /* one 96-DPI pixel in device pixels (>= 1)           */

    /* window chrome */
    int frame, caption, menubar, statusbar;
    int clientW, clientH, winW, winH;
    int contentY, contentH;

    int capBtnW, capBtnH, capBtnY, capBtnCloseX, capBtnMinX;
    int iconX, iconY, titleX, titleY;

    /* group boxes */
    RECT gbSpeed, gbOdo, gbRate;

    /* speedometer: the dial is drawn in its own local coordinates, so only its
     * size matters here plus the panel rect used by the main window */
    RECT dialPanel;
    int  dialPanelW, dialPanelH;
    int  dialCx, dialCy;
    int  rBezel, rFace, rRedIn, rRedOut, rTickOut;
    int  rMajIn, rMedIn, rMinIn, rLabel, rNeedle, rHub;
    int  rTelltaleIn, rTelltaleOut, rUnitLabel;
    int  needleHalfHub, needleHalfTail, hubDot;

    /* read-outs */
    RECT dialEmbed, gearValue;
    int  gvGearX, gvValueX;

    RECT odoDrum, tripDrum, tripReset;
    int  odoCapY, tripCapY, cellW;

    RECT kpmPanel, maxPanel, gearGate, peakReset;
    int  kpmCapY, maxCapY;

    int  sbPane2X, sbPane3X;

    /* fonts, created at the scaled pixel sizes */
    HFONT fUi, fUiBold, fDial, fDrum, fReadout, fHuge, fGearSm, fGate;
} Metrics;

extern Metrics M;

/* Detects the DPI of `hwnd`'s monitor (or the system DPI when hwnd is NULL),
 * recomputes every value and recreates the fonts. Returns the DPI used. */
int  Metrics_Init(HWND hwnd);
void Metrics_Done(void);

/* Forces a DPI instead of the monitor's own. 0 restores automatic detection.
 * Exposed because it lets the scaling be exercised and compared on one machine
 * (--dpi=96 on a 200% display renders the 100% layout), and because some users
 * want the panel larger or smaller than the system scaling implies. */
void Metrics_SetOverride(int dpi);
int  Metrics_GetOverride(void);

/* When on (the default), the layout is shrunk if the monitor's work area
 * cannot show it at its natural size, so the whole window stays reachable on
 * a small screen or at a high scaling. */
void Metrics_SetFitToScreen(int on);
int  Metrics_GetFitToScreen(void);

/* Diagnostics: pretend the work area is w x h, so the fit logic can be
 * exercised on any machine. 0 restores the real work area. */
void Metrics_SetWorkAreaOverride(int w, int h);

/* Scales a 96-DPI value to device pixels. */
int  Metrics_S(int v);

#endif /* KEYODO_METRICS_H */
