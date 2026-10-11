/* ===========================================================================
 * storage.c -- INI persistence.
 *
 * GetPrivateProfileStringA / WritePrivateProfileStringA are used on purpose:
 * they exist on Windows 98 SE, they are still supported on Windows 11, and
 * they produce a file a user can read and edit. No registry, no JSON, no
 * dependency.
 *
 * Counters are stored as decimal strings because the profile API is text based
 * and 64-bit integer formatting is not portable across period compilers.
 * ======================================================================== */

#include "storage.h"

#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "keyodo_layout.h"
#include "../platform/platform.h"

#define SEC_TOTALS   "Totals"
#define SEC_VIEW     "View"
#define SEC_WINDOW   "Window"
#define SEC_INPUT    "Input"

#define INI_NAME     "keyodo.ini"

static char s_path[MAX_PATH];

void Storage_Defaults(StoreData *d)
{
    ZeroMemory(d, sizeof(*d));
    d->odo = 0;
    d->trip = 0;
    d->max_wpm = 0.0;
    d->mode_kpm = 0;
    d->transmission = KTM_AUTO;
    d->show_gate = 1;
    d->always_on_top = 0;
    d->window_secs = 10;
    d->win_x = 0x7FFFFFFF;      /* sentinel: centre the window */
    d->win_y = 0x7FFFFFFF;
    lstrcpynA(d->input_backend, "raw", sizeof(d->input_backend));
}

const char *Storage_Path(void)
{
    if (s_path[0] == 0)
        Plat_SettingsPath(s_path, sizeof(s_path));
    return s_path;
}

static void read_str(const char *sec, const char *key, const char *def,
                     char *out, int cch)
{
    GetPrivateProfileStringA(sec, key, def, out, cch, Storage_Path());
    if (out[0] == 0)
        lstrcpynA(out, def, cch);
}

static int read_int(const char *sec, const char *key, int def)
{
    return (int)GetPrivateProfileIntA(sec, key, def, Storage_Path());
}

static KeyU64 parse_u64(const char *s)
{
    KeyU64 v = 0;
    int i;
    if (s == NULL)
        return 0;
    for (i = 0; s[i] >= '0' && s[i] <= '9'; i++) {
        v = v * 10 + (KeyU64)(s[i] - '0');
        if (v > ((KeyU64)1 << 60))
            break;
    }
    return v;
}

int Storage_Load(StoreData *d)
{
    char buf[64];
    FILE *f;

    if (d == NULL)
        return 0;
    Storage_Defaults(d);

    f = fopen(Storage_Path(), "rb");
    if (f == NULL)
        return 0;               /* first run: defaults are correct */
    fclose(f);

    read_str(SEC_TOTALS, "Odo", "0", buf, sizeof(buf));
    d->odo = parse_u64(buf);
    read_str(SEC_TOTALS, "Trip", "0", buf, sizeof(buf));
    d->trip = parse_u64(buf);
    d->max_wpm = (double)read_int(SEC_TOTALS, "MaxWpmX10", 0) / 10.0;

    d->mode_kpm       = read_int(SEC_VIEW, "ShowKpm", 0) ? 1 : 0;
    d->show_gate      = read_int(SEC_VIEW, "ShowGate", 1) ? 1 : 0;
    d->always_on_top  = read_int(SEC_VIEW, "AlwaysOnTop", 0) ? 1 : 0;
    d->transmission   = read_int(SEC_VIEW, "Manual", 0) ? KTM_MANUAL : KTM_AUTO;

    d->window_secs    = read_int(SEC_WINDOW, "Seconds", 10);
    if (d->window_secs < 1) d->window_secs = 1;
    if (d->window_secs > 60) d->window_secs = 60;

    d->win_x          = read_int(SEC_WINDOW, "X", 0x7FFFFFFF);
    d->win_y          = read_int(SEC_WINDOW, "Y", 0x7FFFFFFF);

    read_str(SEC_INPUT, "Backend", "raw", d->input_backend,
             sizeof(d->input_backend));
    return 1;
}

int Storage_Save(const StoreData *d)
{
    char buf[64];
    const char *p;

    if (d == NULL)
        return 0;
    p = Storage_Path();
    Plat_EnsureParentDir(p);

    KeyU64_Format(d->odo, buf, sizeof(buf));
    WritePrivateProfileStringA(SEC_TOTALS, "Odo", buf, p);

    KeyU64_Format(d->trip, buf, sizeof(buf));
    WritePrivateProfileStringA(SEC_TOTALS, "Trip", buf, p);

    wsprintfA(buf, "%d", (int)(d->max_wpm * 10.0 + 0.5));
    WritePrivateProfileStringA(SEC_TOTALS, "MaxWpmX10", buf, p);

    wsprintfA(buf, "%d", d->mode_kpm ? 1 : 0);
    WritePrivateProfileStringA(SEC_VIEW, "ShowKpm", buf, p);
    wsprintfA(buf, "%d", d->show_gate ? 1 : 0);
    WritePrivateProfileStringA(SEC_VIEW, "ShowGate", buf, p);
    wsprintfA(buf, "%d", d->always_on_top ? 1 : 0);
    WritePrivateProfileStringA(SEC_VIEW, "AlwaysOnTop", buf, p);
    wsprintfA(buf, "%d", (d->transmission == KTM_MANUAL) ? 1 : 0);
    WritePrivateProfileStringA(SEC_VIEW, "Manual", buf, p);

    wsprintfA(buf, "%d", d->window_secs);
    WritePrivateProfileStringA(SEC_WINDOW, "Seconds", buf, p);
    wsprintfA(buf, "%d", d->win_x);
    WritePrivateProfileStringA(SEC_WINDOW, "X", buf, p);
    wsprintfA(buf, "%d", d->win_y);
    WritePrivateProfileStringA(SEC_WINDOW, "Y", buf, p);

    WritePrivateProfileStringA(SEC_INPUT, "Backend", d->input_backend, p);

    /* Flush so the file is complete even if the process is killed. */
    WritePrivateProfileStringA(NULL, NULL, NULL, p);
    return 1;
}
