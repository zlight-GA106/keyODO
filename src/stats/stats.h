/* ===========================================================================
 * stats.h -- keystroke statistics. No UI, no input mechanism, no file I/O.
 *
 * The module is fed key events and a monotonic clock, and produces a snapshot.
 * Swapping Raw Input for a Windows 98 hook changes nothing here.
 * ======================================================================== */

#ifndef KEYODO_STATS_H
#define KEYODO_STATS_H

#include "../common.h"

#define STATS_GEAR_MAX 4

typedef struct StatsSnapshot {
    KeyU64 odo;            /* every real key press since first run   */
    KeyU64 trip;           /* key presses since the last trip reset  */
    double wpm;            /* rate over the sampling window          */
    double kpm;
    double max_wpm;        /* highest wpm ever seen                  */
    double needle;         /* smoothed wpm, for the dial             */
    int    transmission;   /* KTM_AUTO / KTM_MANUAL                  */
    char   gear[STATS_GEAR_MAX];
    unsigned long sessionSecs;
} StatsSnapshot;

/* windowSecs: sampling window used for WPM / KPM, 1..60.
 * transmission: KTM_AUTO or KTM_MANUAL from keyodo_layout.h. */
void Stats_Init(int windowSecs, int transmission);
void Stats_Shutdown(void);

/* A real physical key press. `isRepeat` marks an auto-repeat, which must NOT
 * increase ODO or TRIP -- it is not a new press. */
void Stats_OnKey(unsigned int vkey, int isRepeat, unsigned long tickMs);

/* Call at least as often as the animation timer; `tickMs` is the same clock. */
void Stats_Tick(unsigned long tickMs);

void Stats_ResetTrip(void);
void Stats_ResetMax(void);

void Stats_SetWindow(int secs);
int  Stats_GetWindow(void);
void Stats_SetTransmission(int transmission);
int  Stats_GetTransmission(void);

void Stats_Get(StatsSnapshot *out);

/* Restores persisted totals; call right after Stats_Init. */
void Stats_Restore(KeyU64 odo, KeyU64 trip, double maxWpm);

#endif /* KEYODO_STATS_H */
