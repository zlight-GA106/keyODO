/* ===========================================================================
 * platform.c -- Windows 98 SE / modern Windows differences, in one place.
 *
 * RULES OBSERVED HERE
 *   - Nothing newer than the Windows 98 SE API set is referenced at link time.
 *     Anything modern (DWM, SHGetKnownFolderPath, GetTickCount64) is resolved
 *     with GetProcAddress/GetModuleHandle and silently skipped when absent.
 *   - No GetWindowLongPtr, no LONG_PTR: Windows 98 SE has the 32-bit form only.
 * ======================================================================== */

#include "platform.h"

#include <shlobj.h>
#include <stdio.h>
#include <string.h>

#include "../common.h"

/* GetVersionExA is deprecated for manifested applications on modern Windows,
 * but it is the only version call Windows 98 SE understands, so it stays. The
 * result is only used to decide whether modern-only tweaks are available. */
#pragma warning(disable: 4996)

/* ---------------------------------------------------------------- modern? */

typedef BOOL (WINAPI *PFN_SetProcessDPIAware)(void);
typedef HRESULT (WINAPI *PFN_DwmSetWindowAttribute)(HWND, DWORD, LPCVOID, DWORD);

#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#define DWMWCP_DONOTROUND 1

static int s_modern = -1;

int Plat_IsModernWindows(void)
{
    if (s_modern < 0) {
        OSVERSIONINFOA vi;
        ZeroMemory(&vi, sizeof(vi));
        vi.dwOSVersionInfoSize = sizeof(vi);
        /* GetVersionExA is deprecated for manifests but still works and is the
         * only version call Windows 98 SE understands. */
        s_modern = 0;
        if (GetVersionExA(&vi)) {
            if (vi.dwPlatformId == VER_PLATFORM_WIN32_NT && vi.dwMajorVersion >= 6)
                s_modern = 1;
        }
    }
    return s_modern;
}

void Plat_Init(void)
{
    HMODULE user32;
    PFN_SetProcessDPIAware pSetDpi;

    /* 96 DPI is the design basis. Ask for DPI awareness so modern Windows does
     * not bitmap-stretch the window; Windows 98 SE has no such API, so the
     * lookup simply fails there and we carry on at 96 DPI anyway. */
    user32 = GetModuleHandleA("user32.dll");
    if (user32 != NULL) {
        pSetDpi = (PFN_SetProcessDPIAware)GetProcAddress(user32, "SetProcessDPIAware");
        if (pSetDpi != NULL)
            pSetDpi();
    }
}

/* ------------------------------------------------------------------ clock */

DWORD Plat_TickMs(void)
{
    return GetTickCount();
}

int Plat_TickDelta(DWORD now, DWORD then)
{
    return (int)(now - then);   /* unsigned arithmetic wraps correctly */
}

/* ------------------------------------------------------------------ paths */

static int join_path(char *out, int cch, const char *dir, const char *leaf)
{
    int n;
    if (dir == NULL || dir[0] == 0)
        return 0;
    n = wsprintfA(out, "%s\\%s", dir, leaf);
    UNUSED(cch);
    return n > 0;
}

void Plat_SettingsPath(char *out, int cch)
{
    char dir[MAX_PATH];

    out[0] = 0;
    dir[0] = 0;

    /* CSIDL_APPDATA exists on Windows 98 SE (Application Data) and on every
     * later system. SHGetSpecialFolderPathA is the correct period API. */
    if (SHGetSpecialFolderPathA(NULL, dir, CSIDL_APPDATA, TRUE)) {
        char sub[MAX_PATH];
        wsprintfA(sub, "%s\\%s", dir, KEYODO_NAME);
        if (join_path(out, cch, sub, "keyodo.ini"))
            return;
    }

    /* Fallback: beside the executable. */
    if (GetModuleFileNameA(NULL, dir, sizeof(dir)) > 0) {
        char *p = strrchr(dir, '\\');
        if (p != NULL) {
            *p = 0;
            join_path(out, cch, dir, "keyodo.ini");
            return;
        }
    }

    lstrcpynA(out, "keyodo.ini", cch);
}

void Plat_EnsureParentDir(const char *path)
{
    char buf[MAX_PATH];
    char *p;
    int i, n;

    if (path == NULL)
        return;
    lstrcpynA(buf, path, sizeof(buf));

    /* Walk the string and create each directory in turn. */
    n = lstrlenA(buf);
    for (i = 3; i < n; i++) {           /* skip "C:\" */
        if (buf[i] == '\\') {
            buf[i] = 0;
            CreateDirectoryA(buf, NULL);
            buf[i] = '\\';
        }
    }
    p = strrchr(buf, '\\');
    if (p != NULL) {
        *p = 0;
        CreateDirectoryA(buf, NULL);
    }
}

/* ---------------------------------------------------------------- display */

void Plat_ClampToWorkArea(int *x, int *y, int cx, int cy)
{
    RECT wa;
    int w = GetSystemMetrics(SM_CXSCREEN);
    int h = GetSystemMetrics(SM_CYSCREEN);

    if (SystemParametersInfoA(SPI_GETWORKAREA, 0, &wa, 0)) {
        w = wa.right;
        h = wa.bottom;
        if (*x < wa.left) *x = wa.left;
        if (*y < wa.top)  *y = wa.top;
    } else {
        if (*x < 0) *x = 0;
        if (*y < 0) *y = 0;
    }
    if (*x + cx > w) *x = w - cx;
    if (*y + cy > h) *y = h - cy;
    if (*x < 0) *x = 0;
    if (*y < 0) *y = 0;
}

/* -------------------------------------------------- modern-only niceties */

void Plat_ApplyFrameTweaks(HWND hwnd)
{
    HMODULE dwm;
    PFN_DwmSetWindowAttribute pSet;

    if (!Plat_IsModernWindows() || hwnd == NULL)
        return;

    /* The window draws its own square Windows 98 frame; ask DWM not to round
     * the corners on Windows 11. Absent on older systems, so this is a probe,
     * not a requirement. */
    dwm = LoadLibraryA("dwmapi.dll");
    if (dwm == NULL)
        return;
    pSet = (PFN_DwmSetWindowAttribute)GetProcAddress(dwm, "DwmSetWindowAttribute");
    if (pSet != NULL) {
        DWORD pref = DWMWCP_DONOTROUND;
        pSet(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &pref, sizeof(pref));
    }
    FreeLibrary(dwm);
}
