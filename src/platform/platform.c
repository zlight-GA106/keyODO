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
typedef BOOL (WINAPI *PFN_SetProcessDpiAwarenessContext)(void *);
typedef HRESULT (WINAPI *PFN_SetProcessDpiAwareness)(int);
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
        /* GetVersionExA is deprecated for manifested applications on modern
         * Windows, but it is the only version call Windows 98 SE understands.
         * The answer is only used to decide whether modern-only tweaks are
         * worth attempting. */
        s_modern = 0;
        if (GetVersionExA(&vi)) {
            if (vi.dwPlatformId == VER_PLATFORM_WIN32_NT && vi.dwMajorVersion >= 6)
                s_modern = 1;
        }
    }
    return s_modern;
}

/* Ask for the best DPI awareness this Windows offers, in descending order.
 * Every step is resolved dynamically, so on Windows 98 SE -- which has none of
 * these -- the whole routine is a no-op and the system stays at 96 DPI. */
static void enable_dpi_awareness(void)
{
    HMODULE user32 = GetModuleHandleA("user32.dll");
    HMODULE shcore;

    if (user32 != NULL) {
        PFN_SetProcessDpiAwarenessContext pCtx =
            (PFN_SetProcessDpiAwarenessContext)GetProcAddress(
                user32, "SetProcessDpiAwarenessContext");
        if (pCtx != NULL) {
            /* DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 == (HANDLE)-4 */
            if (pCtx((void *)(INT_PTR)-4))
                return;
        }
    }

    shcore = LoadLibraryA("shcore.dll");
    if (shcore != NULL) {
        PFN_SetProcessDpiAwareness pAware =
            (PFN_SetProcessDpiAwareness)GetProcAddress(
                shcore, "SetProcessDpiAwareness");
        if (pAware != NULL) {
            /* PROCESS_PER_MONITOR_DPI_AWARE == 2 */
            if (pAware(2) == 0) {
                FreeLibrary(shcore);
                return;
            }
        }
        FreeLibrary(shcore);
    }

    if (user32 != NULL) {
        PFN_SetProcessDPIAware pSys =
            (PFN_SetProcessDPIAware)GetProcAddress(user32, "SetProcessDPIAware");
        if (pSys != NULL)
            pSys();
    }
}

void Plat_Init(void)
{
    enable_dpi_awareness();
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

/* Multi-monitor placement.
 *
 * MonitorFromPoint / GetMonitorInfoA exist on Windows 98 SE and later, but the
 * headers only declare them above a WINVER threshold, and this project must
 * also compile with the Windows 98 era SDK. Declaring the two prototypes and
 * the struct locally keeps the source independent of that threshold.
 */
#define KMONITOR_DEFAULTTONEAREST 2

typedef struct KMonitorInfo {
    DWORD cbSize;
    RECT  rcMonitor;
    RECT  rcWork;
    DWORD dwFlags;
} KMonitorInfo;

typedef HMONITOR (WINAPI *PFN_MonitorFromPoint)(POINT, DWORD);
typedef HMONITOR (WINAPI *PFN_MonitorFromWindow)(HWND, DWORD);
typedef BOOL (WINAPI *PFN_GetMonitorInfoA)(HMONITOR, KMonitorInfo *);

static int work_area_for_point(const POINT *pt, RECT *out)
{
    static PFN_MonitorFromPoint pFromPt = NULL;
    static PFN_GetMonitorInfoA pInfo = NULL;
    static int tried = 0;
    HMONITOR mon;
    KMonitorInfo mi;

    if (!tried) {
        HMODULE user32 = GetModuleHandleA("user32.dll");
        if (user32 != NULL) {
            pFromPt = (PFN_MonitorFromPoint)GetProcAddress(user32, "MonitorFromPoint");
            pInfo   = (PFN_GetMonitorInfoA)GetProcAddress(user32, "GetMonitorInfoA");
        }
        tried = 1;
    }

    if (pFromPt != NULL && pInfo != NULL) {
        mon = pFromPt(*pt, KMONITOR_DEFAULTTONEAREST);
        if (mon != NULL) {
            mi.cbSize = sizeof(mi);
            if (pInfo(mon, &mi)) {
                *out = mi.rcWork;
                return 1;
            }
        }
    }
    return 0;
}

int Plat_WorkArea(HWND hwnd, RECT *out)
{
    static PFN_MonitorFromWindow pFromWnd = NULL;
    static PFN_GetMonitorInfoA pInfo = NULL;
    static int tried = 0;
    KMonitorInfo mi;

    if (hwnd != NULL) {
        if (!tried) {
            HMODULE user32 = GetModuleHandleA("user32.dll");
            if (user32 != NULL) {
                pFromWnd = (PFN_MonitorFromWindow)GetProcAddress(user32, "MonitorFromWindow");
                pInfo    = (PFN_GetMonitorInfoA)GetProcAddress(user32, "GetMonitorInfoA");
            }
            tried = 1;
        }
        if (pFromWnd != NULL && pInfo != NULL) {
            HMONITOR mon = pFromWnd(hwnd, KMONITOR_DEFAULTTONEAREST);
            if (mon != NULL) {
                mi.cbSize = sizeof(mi);
                if (pInfo(mon, &mi)) {
                    *out = mi.rcWork;
                    return 1;
                }
            }
        }
    }
    if (SystemParametersInfoA(SPI_GETWORKAREA, 0, out, 0))
        return 1;
    out->left = 0;
    out->top = 0;
    out->right = GetSystemMetrics(SM_CXSCREEN);
    out->bottom = GetSystemMetrics(SM_CYSCREEN);
    return 0;
}

void Plat_ClampToWorkArea(int *x, int *y, int cx, int cy)
{
    RECT wa;
    POINT pt;

    if (x == NULL || y == NULL)
        return;

    pt.x = *x;
    pt.y = *y;
    /* MonitorFromPoint needs a real coordinate; the 0x7FFFFFFF sentinel used
     * for "not saved yet" would miss every monitor, so fall back to the
     * primary work area in that case. */
    if (*x == 0x7FFFFFFF || *y == 0x7FFFFFFF) {
        pt.x = 0;
        pt.y = 0;
        if (!work_area_for_point(&pt, &wa) &&
            !SystemParametersInfoA(SPI_GETWORKAREA, 0, &wa, 0)) {
            wa.left = 0; wa.top = 0;
            wa.right = GetSystemMetrics(SM_CXSCREEN);
            wa.bottom = GetSystemMetrics(SM_CYSCREEN);
        }
    } else if (!work_area_for_point(&pt, &wa) &&
               !SystemParametersInfoA(SPI_GETWORKAREA, 0, &wa, 0)) {
        wa.left = 0; wa.top = 0;
        wa.right = GetSystemMetrics(SM_CXSCREEN);
        wa.bottom = GetSystemMetrics(SM_CYSCREEN);
    }

    if (*x + cx > wa.right)  *x = wa.right - cx;
    if (*y + cy > wa.bottom) *y = wa.bottom - cy;
    if (*x < wa.left) *x = wa.left;
    if (*y < wa.top)  *y = wa.top;
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
