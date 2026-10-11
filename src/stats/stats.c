/* ===========================================================================
 * stats.c -- keystroke statistics.
 *
 * WPM / KPM are computed over a sliding window of real keystroke timestamps:
 * the window holds the last N seconds of presses, so the rate reacts
 * immediately when typing starts or stops, instead of averaging since launch.
 *
 * The needle is deliberately smoothed here rather than in the UI: it is a
 * numeric concern, and putting it here keeps the renderer a pure function of
 * its input.
 * ======================================================================== */

#include "stats.h"

#include "keyodo_layout.h"      /* KTM_AUTO / KTM_MANUAL / WPM_MAX */

#include <windows.h>
#include <math.h>
#include <string.h>

#define STAMP_CAP       1024    /* keystrokes retained for the window      */
#define MAX_WINDOW_SEC  60
#define IDLE_MS         2500    /* "not moving" threshold for gear P       */

/* Coast-down.
 *
 * The rate is a boxcar window, so after the last keystroke it would hold flat
 * for the whole window and then drop off a cliff -- the needle sat still for
 * several seconds and then snapped down. A speedometer does not behave that
 * way: you lift off and the needle falls away smoothly. So once the keys have
 * been quiet for FALL_GRACE_MS the indicated rate is multiplied by a decaying
 * factor.
 *
 * The grace period matters: normal typing has gaps of 100-300 ms, and a short
 * pause to think must not be mistaken for stopping. */
#define FALL_GRACE_MS   600.0
#define FALL_TAU_MS     900.0

/* Needle inertia. Heavier on the way down than on the way up, which is what
 * makes the fall read as a damped needle rather than a linear ramp. */
#define TAU_RISE_MS     140.0
#define TAU_FALL_MS     260.0

#define MANUAL_M2_WPM   40.0
#define MANUAL_M3_WPM   80.0

static unsigned long s_stamp[STAMP_CAP];
static int      s_head = 0;         /* next write slot                        */
static int      s_count = 0;        /* valid entries, oldest first            */

static KeyU64   s_odo = 0;
static KeyU64   s_trip = 0;
static double   s_maxWpm = 0.0;
static double   s_needle = 0.0;

static int      s_windowSecs = 10;
static int      s_transmission = KTM_AUTO;

static unsigned int s_lastVKey = 0;
static unsigned long s_lastKeyMs = 0;
static unsigned long s_startMs = 0;
static unsigned long s_lastTickMs = 0;
static unsigned long s_lastSmoothMs = 0;
static int      s_hasKeys = 0;

static double   s_wpm = 0.0;
static double   s_kpm = 0.0;
static char     s_gear[STATS_GEAR_MAX] = "P";

void Stats_Init(int windowSecs, int transmission)
{
    s_head = 0;
    s_count = 0;
    s_odo = 0;
    s_trip = 0;
    s_maxWpm = 0.0;
    s_needle = 0.0;
    s_wpm = 0.0;
    s_kpm = 0.0;
    s_hasKeys = 0;
    s_lastVKey = 0;
    s_lastKeyMs = 0;
    s_startMs = 0;
    s_lastTickMs = 0;
    s_lastSmoothMs = 0;
    lstrcpynA(s_gear, "P", STATS_GEAR_MAX);

    Stats_SetWindow(windowSecs);
    Stats_SetTransmission(transmission);
}

void Stats_Shutdown(void)
{
    s_count = 0;
    s_head = 0;
}

void Stats_Restore(KeyU64 odo, KeyU64 trip, double maxWpm)
{
    s_odo = odo;
    s_trip = trip;
    s_maxWpm = maxWpm;
}

void Stats_SetWindow(int secs)
{
    if (secs < 1) secs = 1;
    if (secs > MAX_WINDOW_SEC) secs = MAX_WINDOW_SEC;
    s_windowSecs = secs;
}

int Stats_GetWindow(void)
{
    return s_windowSecs;
}

void Stats_SetTransmission(int transmission)
{
    s_transmission = (transmission == KTM_MANUAL) ? KTM_MANUAL : KTM_AUTO;
}

int Stats_GetTransmission(void)
{
    return s_transmission;
}

void Stats_ResetTrip(void)
{
    s_trip = 0;
}

void Stats_ResetMax(void)
{
    s_maxWpm = 0.0;
    if (s_wpm > s_maxWpm)
        s_maxWpm = s_wpm;
}

void Stats_OnKey(unsigned int vkey, int isRepeat, unsigned long tickMs)
{
    if (s_startMs == 0)
        s_startMs = tickMs;

    s_lastVKey = vkey;
    s_lastKeyMs = tickMs;

    if (!isRepeat) {
        s_odo++;
        s_trip++;
    }

    /* Record the timestamp even for auto-repeats: a held key really is
     * producing characters, so it must count towards WPM / KPM. */
    s_stamp[s_head] = tickMs;
    s_head = (s_head + 1) % STAMP_CAP;
    if (s_count < STAMP_CAP)
        s_count++;
    s_hasKeys = 1;
}

static void evict(unsigned long now)
{
    unsigned long span = (unsigned long)s_windowSecs * 1000UL;
    /* Drop timestamps that fell out of the window. At most one pass per tick,
     * and the ring is small, so this stays cheap. */
    while (s_count > 0) {
        int oldest = (s_head - s_count + STAMP_CAP) % STAMP_CAP;
        if (now - s_stamp[oldest] > span) {
            s_count--;
        } else {
            break;
        }
    }
}

static void derive_gear(unsigned long now)
{
    unsigned long idle;

    if (!s_hasKeys) {
        lstrcpynA(s_gear, "P", STATS_GEAR_MAX);
        return;
    }

    idle = now - s_lastKeyMs;

    if (s_transmission == KTM_MANUAL) {
        /* Manual gate is R N M1 M2 M3: no P and no D. */
        if (idle > IDLE_MS || s_lastVKey == VK_SPACE) {
            lstrcpynA(s_gear, "N", STATS_GEAR_MAX);
        } else if (s_lastVKey == VK_BACK) {
            lstrcpynA(s_gear, "R", STATS_GEAR_MAX);
        } else if (s_wpm >= MANUAL_M3_WPM) {
            lstrcpynA(s_gear, "M3", STATS_GEAR_MAX);
        } else if (s_wpm >= MANUAL_M2_WPM) {
            lstrcpynA(s_gear, "M2", STATS_GEAR_MAX);
        } else {
            lstrcpynA(s_gear, "M1", STATS_GEAR_MAX);
        }
        return;
    }

    /* Automatic gate is P R N D. */
    if (idle > IDLE_MS) {
        lstrcpynA(s_gear, "P", STATS_GEAR_MAX);
    } else if (s_lastVKey == VK_BACK) {
        lstrcpynA(s_gear, "R", STATS_GEAR_MAX);
    } else if (s_lastVKey == VK_SPACE) {
        lstrcpynA(s_gear, "N", STATS_GEAR_MAX);
    } else {
        lstrcpynA(s_gear, "D", STATS_GEAR_MAX);
    }
}

void Stats_Tick(unsigned long tickMs)
{
    unsigned long dt;
    double alpha;

    if (s_startMs == 0)
        s_startMs = tickMs;

    evict(tickMs);

    s_kpm = (double)s_count * (60000.0 / ((double)s_windowSecs * 1000.0));
    s_wpm = s_kpm / 5.0;

    /* Coast-down once the keyboard has been quiet for longer than the grace
     * period. Applied to the rate itself so the needle, the dial read-out, the
     * KPM panel and the status bar all tell the same story. */
    if (s_lastKeyMs != 0 && tickMs > s_lastKeyMs) {
        double idle = (double)(tickMs - s_lastKeyMs);
        if (idle > FALL_GRACE_MS) {
            double f = exp(-(idle - FALL_GRACE_MS) / FALL_TAU_MS);
            s_kpm *= f;
            s_wpm *= f;
        }
    }

    if (s_wpm > s_maxWpm)
        s_maxWpm = s_wpm;

    /* exponential smoothing towards the indicated rate */
    dt = tickMs - s_lastSmoothMs;
    if (s_lastSmoothMs == 0)
        dt = 33;
    s_lastSmoothMs = tickMs;
    if (dt > 500) dt = 500;             /* do not lurch after a stall */

    alpha = 1.0 - exp(-((double)dt) /
                      (s_wpm >= s_needle ? TAU_RISE_MS : TAU_FALL_MS));
    if (alpha < 0.0) alpha = 0.0;
    if (alpha > 1.0) alpha = 1.0;
    s_needle += (s_wpm - s_needle) * alpha;

    if (s_needle < 0.0) s_needle = 0.0;
    if (s_needle > (double)WPM_MAX) s_needle = (double)WPM_MAX;

    derive_gear(tickMs);

    s_lastTickMs = tickMs;
}

void Stats_Get(StatsSnapshot *out)
{
    if (out == NULL)
        return;
    out->odo = s_odo;
    out->trip = s_trip;
    out->wpm = s_wpm;
    out->kpm = s_kpm;
    out->max_wpm = s_maxWpm;
    out->needle = s_needle;
    out->transmission = s_transmission;
    lstrcpynA(out->gear, s_gear, STATS_GEAR_MAX);
    {
        unsigned long now = s_lastTickMs;
        if (now < s_startMs)
            out->sessionSecs = 0;
        else
            out->sessionSecs = (now - s_startMs) / 1000UL;
    }
}
