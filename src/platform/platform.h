/* ===========================================================================
 * platform.h -- the only module allowed to know which Windows we are on.
 *
 * Everything that differs between Windows 98 SE and modern Windows is kept
 * here behind a small, boring interface. No other module calls a Version API,
 * loads a DLL by hand, or touches DWM.
 * ======================================================================== */

#ifndef KEYODO_PLATFORM_H
#define KEYODO_PLATFORM_H

#include <windows.h>

/* Called once from WinMain. Never fails; degrades silently. */
void Plat_Init(void);

/* Monotonic millisecond clock, wrapped at 2^32. Safe on Windows 98 SE,
 * where GetTickCount64 does not exist. */
DWORD Plat_TickMs(void);

/* Signed difference between two Plat_TickMs() readings. */
int Plat_TickDelta(DWORD now, DWORD then);

/* Full path of the settings file, e.g.
 *   C:\Users\me\AppData\Roaming\KeyODO\keyodo.ini
 * Falls back to the executable's directory when there is no shell folder. */
void Plat_SettingsPath(char *out, int cch);

/* Creates every missing directory in `path`'s parent chain. */
void Plat_EnsureParentDir(const char *path);

/* Smallest / largest allowed window origin, keeps a saved position sane. */
void Plat_ClampToWorkArea(int *x, int *y, int cx, int cy);

/* Marker for the modern-Windows-only tweaks. Returns 0 on Windows 98 SE. */
int Plat_IsModernWindows(void);

/* Asks DWM not to round the corners of the self-drawn square frame. Resolves
 * dwmapi.dll dynamically, so it is a no-op on Windows 98 SE. */
void Plat_ApplyFrameTweaks(HWND hwnd);

/* Work area of the monitor `hwnd` is on, or of the primary monitor when hwnd
 * is NULL. Returns 0 if the query failed and `out` was left untouched. */
int Plat_WorkArea(HWND hwnd, RECT *out);

/* ------------------------------------------------------- run at startup --
 * A value under HKCU\...\CurrentVersion\Run. Per-user on purpose: installing
 * to Program Files needs administrator rights, but turning autostart on must
 * not prompt for elevation. */

int Plat_AutostartEnabled(void);
/* Returns 1 on success. Fails only if the registry is unwritable. */
int Plat_AutostartSet(int on);

#endif /* KEYODO_PLATFORM_H */
