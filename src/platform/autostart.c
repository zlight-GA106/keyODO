/* ===========================================================================
 * autostart.c -- "run at startup" via the per-user Run key.
 *
 * HKCU, not HKLM: the program installs to Program Files, which needs
 * administrator rights, but toggling a checkbox must not raise a UAC prompt.
 * Windows 98 SE has the same key, so nothing here is version-dependent.
 * ======================================================================== */

#include "platform.h"

#include <windows.h>

#define RUN_KEY  "Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define RUN_NAME "KeyODO"

static int exe_command(char *out, int cch)
{
    char path[MAX_PATH];
    DWORD n = GetModuleFileNameA(NULL, path, sizeof(path));
    if (n == 0 || n >= sizeof(path))
        return 0;
    /* Quoted: the install path contains a space. */
    wsprintfA(out, "\"%s\"", path);
    if ((int)lstrlenA(out) >= cch)
        return 0;
    return 1;
}

int Plat_AutostartEnabled(void)
{
    HKEY key;
    char want[MAX_PATH + 8];
    char have[MAX_PATH + 8];
    DWORD size = sizeof(have);
    DWORD type = 0;
    int ok = 0;

    if (!exe_command(want, sizeof(want)))
        return 0;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, RUN_KEY, 0, KEY_QUERY_VALUE, &key)
        != ERROR_SUCCESS)
        return 0;
    if (RegQueryValueExA(key, RUN_NAME, NULL, &type, (LPBYTE)have, &size)
        == ERROR_SUCCESS && type == REG_SZ) {
        have[sizeof(have) - 1] = 0;
        ok = (lstrcmpiA(have, want) == 0);
    }
    RegCloseKey(key);
    return ok;
}

int Plat_AutostartSet(int on)
{
    HKEY key;
    LONG rc;

    if (RegCreateKeyExA(HKEY_CURRENT_USER, RUN_KEY, 0, NULL, 0,
                        KEY_SET_VALUE, NULL, &key, NULL) != ERROR_SUCCESS)
        return 0;

    if (on) {
        char cmd[MAX_PATH + 8];
        if (!exe_command(cmd, sizeof(cmd))) {
            RegCloseKey(key);
            return 0;
        }
        rc = RegSetValueExA(key, RUN_NAME, 0, REG_SZ,
                            (const BYTE *)cmd, (DWORD)lstrlenA(cmd) + 1);
    } else {
        rc = RegDeleteValueA(key, RUN_NAME);
        if (rc == ERROR_FILE_NOT_FOUND)
            rc = ERROR_SUCCESS;
    }
    RegCloseKey(key);
    return rc == ERROR_SUCCESS;
}
