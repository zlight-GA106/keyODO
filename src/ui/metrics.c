/* ===========================================================================
 * metrics.c -- turns the 96-DPI spec into device values for the current monitor.
 *
 * Detection order:
 *   1. GetDpiForWindow  -- per-monitor DPI, Windows 10 1607 and later
 *   2. GetDeviceCaps(LOGPIXELSX) -- system DPI, works on Windows 98 SE
 * Both are reached through GetProcAddress/GetDeviceCaps, so nothing here is a
 * hard dependency on a modern API.
 *
 * The font list is rebuilt on every DPI change so text is rasterised at the
 * size it is actually drawn, never scaled from a bitmap.
 * ======================================================================== */

#include "metrics.h"

#include "keyodo_layout.h"
#include "../platform/platform.h"
#include "../i18n/i18n.h"

Metrics M;

static int s_override = 0;

void Metrics_SetOverride(int dpi)
{
    if (dpi < 0) dpi = 0;
    if (dpi > 480) dpi = 480;
    s_override = dpi;
}

int Metrics_GetOverride(void)
{
    return s_override;
}

int Metrics_S(int v)
{
    return MulDiv(v, M.dpi, 96);
}

static RECT sc4(int l, int t, int r, int b)
{
    RECT rc;
    rc.left = Metrics_S(l);
    rc.top = Metrics_S(t);
    rc.right = Metrics_S(r);
    rc.bottom = Metrics_S(b);
    return rc;
}

/* ---------------------------------------------------------------- DPI ---- */

typedef UINT (WINAPI *PFN_GetDpiForWindow)(HWND);

static int query_dpi(HWND hwnd)
{
    static PFN_GetDpiForWindow pFn = NULL;
    static int tried = 0;
    HDC dc;
    int dpi;

    if (s_override > 0)
        return s_override;

    if (!tried) {
        HMODULE user32 = GetModuleHandleA("user32.dll");
        if (user32 != NULL)
            pFn = (PFN_GetDpiForWindow)GetProcAddress(user32, "GetDpiForWindow");
        tried = 1;
    }
    if (pFn != NULL && hwnd != NULL) {
        UINT d = pFn(hwnd);
        if (d >= 48 && d <= 480)
            return (int)d;
    }

    dc = GetDC(NULL);
    dpi = GetDeviceCaps(dc, LOGPIXELSX);
    ReleaseDC(NULL, dc);
    if (dpi < 48 || dpi > 480)
        dpi = 96;
    return dpi;
}

/* --------------------------------------------------------------- fonts --- */

static HFONT make_font(const char *face, int px, int weight, BYTE quality)
{
    if (px < 1)
        px = 1;
    return CreateFontA(-px, 0, 0, 0, weight, FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       quality, DEFAULT_PITCH, face);
}

static void free_fonts(void)
{
    HFONT *p = &M.fUi;
    int i;
    for (i = 0; i < 8; i++) {
        if (p[i] != NULL) {
            DeleteObject(p[i]);
            p[i] = NULL;
        }
    }
}

static void make_fonts(void)
{
    /* The UI face and its size follow the active language: Chinese needs a
     * bitmap CJK face, English the classic Latin one. Everything else keeps
     * the faces the reference render used. */
    const char *face = I18n_UiFace();
    int uiPx = Metrics_S(I18n_UiFontPx());

    M.fUi      = make_font(face, uiPx, FW_NORMAL, DEFAULT_QUALITY);
    M.fUiBold  = make_font(face, uiPx, FW_BOLD,   DEFAULT_QUALITY);
    M.fDial    = make_font("Arial", Metrics_S(11), FW_BOLD, NONANTIALIASED_QUALITY);
    M.fDrum    = make_font("Arial", Metrics_S(20), FW_BOLD, NONANTIALIASED_QUALITY);
    M.fReadout = make_font("Arial", Metrics_S(17), FW_BOLD, NONANTIALIASED_QUALITY);
    M.fHuge    = make_font("Arial", Metrics_S(29), FW_BOLD, ANTIALIASED_QUALITY);
    M.fGearSm  = make_font("Arial", Metrics_S(20), FW_BOLD, NONANTIALIASED_QUALITY);
    M.fGate    = make_font("Arial", Metrics_S(14), FW_BOLD, NONANTIALIASED_QUALITY);
}

/* ------------------------------------------------------------ geometry --- */

static void compute(void)
{
    M.scalePct = M.dpi * 100 / 96;
    M.unit = Metrics_S(1);
    if (M.unit < 1)
        M.unit = 1;
    M.bevel = Metrics_S(2);
    if (M.bevel < 1)
        M.bevel = 1;

    M.frame     = Metrics_S(KW_FRAME);
    M.caption   = Metrics_S(KW_CAPTION_H);
    M.menubar   = Metrics_S(KW_MENUBAR_H);
    M.statusbar = Metrics_S(KW_STATUSBAR_H);

    /* Scale the WINDOW and derive the client size from it. Scaling the client
     * width and the frame independently would round twice and land one pixel
     * off at 125% and 150% (e.g. 601x401 instead of 600x400). */
    M.winW      = Metrics_S(KW_WIN_W);
    M.winH      = Metrics_S(KW_WIN_H);
    M.clientW   = M.winW - 2 * M.frame;
    M.clientH   = M.winH - 2 * M.frame - M.caption;
    M.contentY  = M.menubar;
    M.contentH  = M.clientH - M.menubar - M.statusbar;

    M.capBtnW = Metrics_S(KW_CAPBTN_W);
    M.capBtnH = Metrics_S(KW_CAPBTN_H);
    M.capBtnY = Metrics_S(KW_CAPBTN_Y);
    M.capBtnCloseX = M.winW - M.frame - Metrics_S(2) - M.capBtnW;
    M.capBtnMinX   = M.capBtnCloseX - Metrics_S(2) - M.capBtnW;
    M.iconX  = Metrics_S(KW_ICON_X);
    M.iconY  = Metrics_S(KW_ICON_Y);
    M.titleX = Metrics_S(KW_TITLE_X);
    M.titleY = Metrics_S(KW_TITLE_Y);

    M.gbSpeed = sc4(4, KW_BOX_TOP, 256, KW_BOX_BOT);
    M.gbOdo   = sc4(260, KW_BOX_TOP, 470, KW_BOX_TOP + 114);
    M.gbRate  = sc4(260, KW_BOX_TOP + 124, 470, KW_BOX_BOT);

    M.dialPanel = sc4(38, KW_CONTENT_Y + 20, 222, KW_CONTENT_Y + 204);
    M.dialPanelW = M.dialPanel.right - M.dialPanel.left;
    M.dialPanelH = M.dialPanel.bottom - M.dialPanel.top;
    M.dialCx = M.dialPanelW / 2;
    M.dialCy = M.dialPanelH / 2;

    M.rBezel      = Metrics_S(R_BEZEL);
    M.rFace       = Metrics_S(R_FACE);
    M.rRedIn      = Metrics_S(R_RED_IN);
    M.rRedOut     = Metrics_S(R_RED_OUT);
    M.rTickOut    = Metrics_S(R_TICK_OUT);
    M.rMajIn      = Metrics_S(R_MAJ_IN);
    M.rMedIn      = Metrics_S(R_MED_IN);
    M.rMinIn      = Metrics_S(R_MIN_IN);
    M.rLabel      = Metrics_S(R_LABEL);
    M.rNeedle     = Metrics_S(R_NEEDLE);
    M.rHub        = Metrics_S(R_HUB);
    M.rTelltaleIn = Metrics_S(R_TELLTALE_IN);
    M.rTelltaleOut= Metrics_S(R_TELLTALE_OUT);
    M.rUnitLabel  = Metrics_S(R_UNIT_LABEL);

    M.needleHalfHub  = (int)(3.2 * M.dpi / 96.0 + 0.5);
    M.needleHalfTail = (int)(2.4 * M.dpi / 96.0 + 0.5);
    M.hubDot         = Metrics_S(2);

    M.dialEmbed = sc4(88, KW_CONTENT_Y + 151, 172, KW_CONTENT_Y + 173);
    M.gearValue = sc4(72, KW_CONTENT_Y + 208, 188, KW_CONTENT_Y + 244);
    M.gvGearX   = Metrics_S(GV_GEAR_X);
    M.gvValueX  = Metrics_S(GV_VALUE_X);

    M.odoCapY  = Metrics_S(ODO_CAP_Y);
    M.odoDrum  = sc4(270, KW_CONTENT_Y + 33, 460, KW_CONTENT_Y + 63);
    M.tripCapY = Metrics_S(TRIP_CAP_Y);
    M.tripDrum = sc4(348, KW_CONTENT_Y + 83, 460, KW_CONTENT_Y + 113);
    M.tripReset= sc4(270, KW_CONTENT_Y + 85, 340, KW_CONTENT_Y + 111);
    M.cellW    = Metrics_S(DRUM_CELL_W);

    M.kpmCapY  = Metrics_S(KPM_CAP_Y);
    M.kpmPanel = sc4(368, KW_CONTENT_Y + 140, 460, KW_CONTENT_Y + 164);
    M.maxCapY  = Metrics_S(MAX_CAP_Y);
    M.maxPanel = sc4(368, KW_CONTENT_Y + 168, 460, KW_CONTENT_Y + 192);
    M.gearGate = sc4(270, KW_CONTENT_Y + 196, 460, KW_CONTENT_Y + 218);
    M.peakReset= sc4(270, KW_CONTENT_Y + 221, 382, KW_CONTENT_Y + 247);

    M.sbPane3X = Metrics_S(SB_PANE3_X);
    M.sbPane2X = Metrics_S(SB_PANE2_X);
}

/* ------------------------------------------------------- fit to screen --- */

static int s_fit = 1;
static int s_workOverrideW = 0;     /* diagnostics: pretend the screen is this */
static int s_workOverrideH = 0;

void Metrics_SetFitToScreen(int on)
{
    s_fit = on ? 1 : 0;
}

int Metrics_GetFitToScreen(void)
{
    return s_fit;
}

/* Pretend the work area is w x h. Lets the fit logic be exercised on a large
 * monitor (--work=800x600 answers "would this fit on the old machine?"), and
 * is how tools/feature_test.py verifies it. 0 restores the real work area. */
void Metrics_SetWorkAreaOverride(int w, int h)
{
    s_workOverrideW = (w > 0) ? w : 0;
    s_workOverrideH = (h > 0) ? h : 0;
}

static int work_area(HWND hwnd, RECT *out)
{
    if (s_workOverrideW > 0 && s_workOverrideH > 0) {
        out->left = 0;
        out->top = 0;
        out->right = s_workOverrideW;
        out->bottom = s_workOverrideH;
        return 1;
    }
    return Plat_WorkArea(hwnd, out);
}

/* Shrinks the whole layout when the monitor cannot show it at its natural
 * size -- a 200% scaling on a 1366x768 panel, or an 800x600 machine. Without
 * this the window would simply be clamped to the work area and the bottom of
 * the status bar would never be reachable.
 *
 * The window is proportional to the DPI (480x320 at 96), so the largest DPI
 * that fits is exact arithmetic rather than a search. */
static void apply_fit(HWND hwnd)
{
    RECT wa;
    int maxW, maxH, dpiW, dpiH, fit;

    if (!s_fit)
        return;
    if (!work_area(hwnd, &wa))
        return;

    maxW = wa.right - wa.left;
    maxH = wa.bottom - wa.top;
    /* Leave a margin so the frame is not flush against the screen edge. */
    maxW -= maxW / 24;
    maxH -= maxH / 24;
    if (maxW < 160 || maxH < 120)
        return;

    dpiW = MulDiv(maxW, 96, KW_WIN_W);
    dpiH = MulDiv(maxH, 96, KW_WIN_H);
    fit = (dpiW < dpiH) ? dpiW : dpiH;
    if (fit >= M.dpi)
        return;
    if (fit < 48)               /* never shrink below 50%: unreadable */
        fit = 48;

    M.dpi = fit;
    compute();
}

int Metrics_Init(HWND hwnd)
{
    M.dpi = query_dpi(hwnd);
    compute();
    apply_fit(hwnd);
    free_fonts();
    make_fonts();
    return M.dpi;
}

void Metrics_Done(void)
{
    free_fonts();
}
