/* ===========================================================================
 * tray.c -- notification-area icon.
 * ======================================================================== */

#include "tray.h"

#include <shellapi.h>
#include <string.h>

#define TRAY_ID 1

/* The Windows 95/98/NT4 shape of NOTIFYICONDATA. Every later version is a
 * superset, and the shell switches on dwSize, so this stays valid. */
typedef struct TrayIconData {
    DWORD cbSize;
    HWND  hWnd;
    UINT  uID;
    UINT  uFlags;
    UINT  uCallbackMessage;
    HICON hIcon;
    char  szTip[64];
} TrayIconData;

static HWND  s_owner = NULL;
static HICON s_icon = NULL;
static int   s_added = 0;

#ifndef NIF_STATE
#define NIF_STATE 0x00000008
#endif

static void fill(TrayIconData *d, HWND owner, const char *tip)
{
    ZeroMemory(d, sizeof(*d));
    d->cbSize = sizeof(TrayIconData);
    d->hWnd = owner;
    d->uID = TRAY_ID;
}

int Tray_Add(HWND owner, HINSTANCE inst, UINT callbackMsg, UINT iconId,
             const char *tip)
{
    TrayIconData d;
    int cx, cy;

    if (s_added)
        Tray_Remove(s_owner);

    cx = GetSystemMetrics(SM_CXSMICON);
    cy = GetSystemMetrics(SM_CYSMICON);
    if (cx <= 0) cx = 16;
    if (cy <= 0) cy = 16;

    s_icon = (HICON)LoadImageA(inst, MAKEINTRESOURCEA(iconId), IMAGE_ICON,
                               cx, cy, LR_DEFAULTCOLOR);
    if (s_icon == NULL)
        s_icon = LoadIconA(NULL, IDI_APPLICATION);

    fill(&d, owner, tip);
    d.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    d.uCallbackMessage = callbackMsg;
    d.hIcon = s_icon;
    if (tip != NULL)
        lstrcpynA(d.szTip, tip, sizeof(d.szTip));

    if (!Shell_NotifyIconA(NIM_ADD, (NOTIFYICONDATAA *)&d)) {
        if (s_icon != NULL) {
            DestroyIcon(s_icon);
            s_icon = NULL;
        }
        return 0;
    }
    s_owner = owner;
    s_added = 1;
    return 1;
}

void Tray_Remove(HWND owner)
{
    TrayIconData d;
    if (!s_added)
        return;
    fill(&d, (owner != NULL) ? owner : s_owner, NULL);
    Shell_NotifyIconA(NIM_DELETE, (NOTIFYICONDATAA *)&d);
    if (s_icon != NULL) {
        DestroyIcon(s_icon);
        s_icon = NULL;
    }
    s_added = 0;
    s_owner = NULL;
}

int Tray_Added(void)
{
    return s_added;
}

void Tray_SetTip(HWND owner, const char *tip)
{
    TrayIconData d;
    if (!s_added || tip == NULL)
        return;
    fill(&d, owner, tip);
    d.uFlags = NIF_TIP;
    lstrcpynA(d.szTip, tip, sizeof(d.szTip));
    Shell_NotifyIconA(NIM_MODIFY, (NOTIFYICONDATAA *)&d);
}
