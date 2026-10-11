/* ===========================================================================
 * tray.h -- notification-area icon.
 *
 * Deliberately uses the version-1 NOTIFYICONDATA layout and its own struct
 * rather than the shell's: the modern NOTIFYICONDATA is much larger, and
 * Shell_NotifyIcon rejects a dwSize it does not recognise, so passing the
 * modern one would silently do nothing on Windows 98 SE.
 * ======================================================================== */

#ifndef KEYODO_TRAY_H
#define KEYODO_TRAY_H

#include <windows.h>

/* Adds the icon. `callbackMsg` is posted to `owner` with lParam set to the
 * mouse message (WM_LBUTTONUP, WM_RBUTTONUP, ...) and wParam set to the id. */
int  Tray_Add(HWND owner, HINSTANCE inst, UINT callbackMsg, UINT iconId,
              const char *tip);
void Tray_Remove(HWND owner);
int  Tray_Added(void);
void Tray_SetTip(HWND owner, const char *tip);

#endif /* KEYODO_TRAY_H */
