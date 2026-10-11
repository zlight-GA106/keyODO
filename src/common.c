/* ===========================================================================
 * common.c -- small shared helpers.
 * ======================================================================== */

#include "common.h"

void KeyU64_Format(KeyU64 v, char *buf, int cch)
{
    char tmp[24];
    int n = 0;
    int i;

    if (cch <= 0)
        return;
    if (v == 0) {
        buf[0] = '0';
        if (cch > 1) buf[1] = 0;
        return;
    }
    while (v > 0 && n < (int)sizeof(tmp)) {
        tmp[n++] = (char)('0' + (int)(v % 10));
        v /= 10;
    }
    if (n >= cch)
        n = cch - 1;
    for (i = 0; i < n; i++)
        buf[i] = tmp[n - 1 - i];
    buf[n] = 0;
}

void KeyU64_FormatPad(KeyU64 v, int digits, char *buf, int cch)
{
    char tmp[24];
    int n = 0;
    int i, pad;

    if (cch <= 0)
        return;
    if (v == 0) {
        n = 1;
        tmp[0] = '0';
    } else {
        while (v > 0 && n < (int)sizeof(tmp)) {
            tmp[n++] = (char)('0' + (int)(v % 10));
            v /= 10;
        }
    }
    if (n > digits)
        pad = 0;
    else
        pad = digits - n;
    if (n + pad >= cch)
        pad = (n >= cch) ? 0 : cch - 1 - n;

    for (i = 0; i < pad; i++)
        buf[i] = '0';
    for (i = 0; i < n && pad + i < cch - 1; i++)
        buf[pad + i] = tmp[n - 1 - i];
    buf[pad + i] = 0;
}
