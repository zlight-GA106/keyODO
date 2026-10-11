/* ===========================================================================
 * storage.h -- settings persistence. Knows nothing about statistics or UI:
 * it reads and writes one plain data record.
 * ======================================================================== */

#ifndef KEYODO_STORAGE_H
#define KEYODO_STORAGE_H

#include "../common.h"

typedef struct StoreData {
    KeyU64 odo;
    KeyU64 trip;
    double max_wpm;

    int mode_kpm;          /* 0 = show WPM, 1 = show KPM          */
    int transmission;      /* KTM_AUTO / KTM_MANUAL               */
    int show_gate;         /* draw the selector gate              */
    int always_on_top;
    int window_secs;       /* WPM / KPM sampling window           */

    int win_x, win_y;

    char input_backend[16]; /* "raw" or "hook"                    */
} StoreData;

void Storage_Defaults(StoreData *d);

/* Both return non-zero on success. Missing values fall back to defaults, so a
 * partially written or hand-edited file is not fatal. */
int Storage_Load(StoreData *d);
int Storage_Save(const StoreData *d);

/* Full path of the ini file, for diagnostics and the about box. */
const char *Storage_Path(void);

#endif /* KEYODO_STORAGE_H */
