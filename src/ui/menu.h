/* ===========================================================================
 * menu.h -- self-drawn Windows 98 popup menus.
 *
 * WHY NOT TrackPopupMenu
 *   The system menu on a modern Windows is rounded, shadowed and drawn with
 *   the current visual style. Dropping one of those into a window whose every
 *   other pixel is pinned to the classic scheme looks like a mistake, and the
 *   reference screenshots are the specification -- so the menu is drawn here
 *   with the same GDI recipes as the rest of the chrome.
 *
 * The popup is a WS_POPUP window of its own that runs a local tracking loop,
 * which is roughly what the system does internally.
 * ======================================================================== */

#ifndef KEYODO_MENU_H
#define KEYODO_MENU_H

#include <windows.h>

#define MIF_SEPARATOR 0x0001
#define MIF_CHECKED   0x0002   /* tick mark in the check column          */
#define MIF_RADIO     0x0004   /* round mark instead of a tick           */
#define MIF_DISABLED  0x0008

typedef struct MenuItem {
    UINT id;         /* command id returned when chosen                */
    UINT textId;     /* string-table id; a tab splits label | shortcut  */
    UINT flags;
} MenuItem;

typedef struct MenuDef {
    const MenuItem *items;
    int count;
} MenuDef;

/* Shows the menu with its top-left at the given screen point and tracks it.
 * Returns the chosen command id, or 0 if it was dismissed. */
UINT Menu_Track(HWND owner, const MenuDef *def, int x, int y);

/* Fills in one MenuItem; keeps the call sites free of compound literals,
 * which MSVC 6 does not accept. */
void Menu_Item(MenuItem *m, UINT id, UINT textId, UINT flags);

#endif /* KEYODO_MENU_H */
