/* ===========================================================================
 * gear.h -- the gear read-out and the selector gate.
 *
 * Both are pure rendering: the transmission mode and the gear NAME arrive from
 * the caller. Nothing here knows how a gear is decided.
 * ======================================================================== */

#ifndef KEYODO_GEAR_H
#define KEYODO_GEAR_H

#include <windows.h>

/* One panel reading "<gear> <value>", e.g. "D 62". */
void Gear_DrawValue(HDC hdc, const RECT *box, const char *gear, const char *value);

/* Selector gate: only the ratios of the active transmission are drawn. */
void Gear_DrawGate(HDC hdc, const RECT *box,
                   const char *const *names, int count, const char *active);

#endif /* KEYODO_GEAR_H */
