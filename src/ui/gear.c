/* ===========================================================================
 * gear.c -- combined gear/value read-out and the selector gate.
 *
 * Port of draw_gear_value() and draw_gear_gate() from
 * mockup/keyodo_mockup.py. Both parts of the read-out are left-anchored at
 * fixed offsets so the display does not jitter when the digit count changes.
 * All sizes come from Metrics, so it scales with the monitor DPI.
 * ======================================================================== */

#include "gear.h"
#include "theme.h"
#include "widgets.h"
#include "metrics.h"

#include "keyodo_layout.h"
#include "../common.h"

#include <string.h>

void Gear_DrawValue(HDC hdc, const RECT *box, const char *gear, const char *value)
{
    const Theme *t = &g_theme;
    HFONT gf = (lstrlenA(gear) == 1) ? M.fHuge : M.fGearSm;
    COLORREF gc = (gear[0] == KG_RED_ONLY) ? t->red : t->windowtext;

    Theme_Panel(hdc, box, t->window, TK_SUNKEN, M.bevel);
    Widget_TextLeft(hdc, box->left + M.gvGearX, box, gear, gf, gc, t->window);
    Widget_TextLeft(hdc, box->left + M.gvValueX, box, value, M.fHuge,
                    t->windowtext, t->window);
}

void Gear_DrawGate(HDC hdc, const RECT *box,
                   const char *const *names, int count, const char *active)
{
    const Theme *t = &g_theme;
    int iw, slot, x0, i;
    int maxSlot = Metrics_S(34);

    Theme_Panel(hdc, box, t->window, TK_SUNKEN, M.bevel);

    if (count <= 0)
        return;
    iw = (box->right - box->left) - 2 * M.bevel;
    slot = iw / count;
    if (slot > maxSlot)
        slot = maxSlot;
    x0 = box->left + M.bevel + (iw - slot * count) / 2;

    for (i = 0; i < count; i++) {
        int sx = x0 + i * slot;
        RECT cell;
        int isActive = (lstrcmpA(names[i], active) == 0);
        COLORREF fg = isActive ? t->highlight_text : t->graytext;
        COLORREF bg = isActive ? t->highlight : t->window;

        cell.left = sx;
        cell.top = box->top + M.bevel;
        cell.right = sx + slot;
        cell.bottom = box->bottom - M.bevel;

        if (isActive)
            Theme_Fill(hdc, &cell, t->highlight);
        Widget_TextCentered(hdc, &cell, names[i], M.fGate, fg, bg);
    }
}
