/* ===========================================================================
 * menu.c -- self-drawn Windows 98 popup menus.
 *
 * Every measurement comes from Metrics, so the menu scales with the monitor
 * DPI exactly like the rest of the window, and every colour comes from the
 * pinned classic palette rather than the live system scheme.
 *
 * The tracking loop mirrors what TrackPopupMenu does: take the mouse capture,
 * pump messages until something is chosen or the user clicks away, and keep
 * dispatching everything else so the application stays alive behind the menu.
 * ======================================================================== */

#include "menu.h"
#include "theme.h"
#include "widgets.h"
#include "metrics.h"

#include "../common.h"

#include <string.h>

#define MENU_CLASS "KeyODO_Menu"
#define MENU_MAX   16

/* Classic tick mark, 8 columns by 7 rows, MSB is the leftmost column. */
static const unsigned short kTick[7] = {
    0x0100,     /* .......# */
    0x0300,     /* ......## */
    0x0600,     /* .....##. */
    0x8C00,     /* #...##.. */
    0xD800,     /* ##.##... */
    0x7000,     /* .###.... */
    0x2000      /* ..#..... */
};

/* Classic radio mark: a small round bullet. */
static const unsigned short kBullet[7] = {
    0x0000, 0x1800, 0x3C00, 0x3C00, 0x3C00, 0x1800, 0x0000
};

typedef struct ItemBox {
    UINT id;
    UINT flags;
    char label[64];
    char accel[32];
    int  y, h;
    int  labelW;
} ItemBox;

/* One menu at a time, so the state can live here. */
static HWND      s_popup = NULL;
static HINSTANCE s_inst = NULL;
static ItemBox   s_box[MENU_MAX];
static int       s_count = 0;
static int       s_hot = -1;
static UINT      s_result = 0;
static int       s_done = 0;
static int       s_labelX = 0;      /* label origin, client space */
static int       s_checkW = 0;
static int       s_textH = 0;       /* font cell height, for centring */
static int       s_w = 0, s_h = 0;

void Menu_Item(MenuItem *m, UINT id, UINT textId, UINT flags)
{
    m->id = id;
    m->textId = textId;
    m->flags = flags;
}

/* --------------------------------------------------------------- helpers */

static void load(UINT id, char *buf, int cch)
{
    buf[0] = 0;
    if (id != 0)
        LoadStringA(s_inst, id, buf, cch);
}

/* Splits "label\tshortcut" in place. */
static void split_tab(char *s, char *accel, int accelCch)
{
    char *tab = s;
    accel[0] = 0;
    while (*tab != 0 && *tab != '\t')
        tab++;
    if (*tab == '\t') {
        *tab = 0;
        lstrcpynA(accel, tab + 1, accelCch);
    }
}

/* Draws `text` with the mnemonic underlined; `&` marks it and is not shown. */
static void draw_text(HDC hdc, int x, int y, const char *text,
                      COLORREF fg, COLORREF bg)
{
    char clean[64];
    int i = 0, mnemonic = -1;
    const char *p;
    HGDIOBJ old;
    SIZE sz;

    for (p = text; *p != 0 && i < (int)sizeof(clean) - 1; p++) {
        if (*p == '&') {
            if (p[1] == '&') {          /* escaped ampersand */
                clean[i++] = '&';
                p++;
                continue;
            }
            mnemonic = i;
            continue;
        }
        clean[i++] = *p;
    }
    clean[i] = 0;

    old = SelectObject(hdc, M.fUi);
    SetBkMode(hdc, OPAQUE);
    SetBkColor(hdc, bg);
    SetTextColor(hdc, fg);
    TextOutA(hdc, x, y, clean, i);
    sz.cx = 0; sz.cy = 0;
    GetTextExtentPoint32A(hdc, clean, i, &sz);
    SelectObject(hdc, old);

    if (mnemonic >= 0 && mnemonic < i) {
        SIZE pre, one;
        char mc[2];
        mc[0] = clean[mnemonic];
        mc[1] = 0;
        GetTextExtentPoint32A(hdc, clean, mnemonic, &pre);
        GetTextExtentPoint32A(hdc, mc, 1, &one);
        Theme_HLine(hdc, x + pre.cx, x + pre.cx + one.cx, y + sz.cy - 1, fg);
    }
}

static void draw_mask(HDC hdc, int x, int y, const unsigned short *mask,
                      int cols, int rows, COLORREF ink, int px)
{
    int r, c;
    RECT b;
    if (px < 1)
        px = 1;
    for (r = 0; r < rows; r++) {
        for (c = 0; c < cols; c++) {
            if ((mask[r] & (0x8000u >> c)) == 0)
                continue;
            b.left = x + c * px;
            b.top = y + r * px;
            b.right = b.left + px;
            b.bottom = b.top + px;
            Theme_Fill(hdc, &b, ink);
        }
    }
}

/* --------------------------------------------------------------- layout */

static void build_boxes(const MenuDef *def)
{
    int i;
    int w = 0, y = 0;
    int anyMark = 0;
    int itemH = Metrics_S(17);
    int sepH = Metrics_S(7);
    int padL = Metrics_S(8);
    int padR = Metrics_S(14);
    int accelGap = Metrics_S(20);
    HDC hdc;
    SIZE sz;

    s_count = def->count;
    if (s_count > MENU_MAX)
        s_count = MENU_MAX;

    for (i = 0; i < s_count; i++) {
        if ((def->items[i].flags & MIF_SEPARATOR) == 0) {
            anyMark = 1;
            break;
        }
    }
    s_checkW = anyMark ? Metrics_S(16) : 0;

    for (i = 0; i < s_count; i++) {
        ItemBox *b = &s_box[i];
        const MenuItem *it = &def->items[i];

        b->id = it->id;
        b->flags = it->flags;
        b->label[0] = 0;
        b->accel[0] = 0;
        b->labelW = 0;
        b->y = y;
        if (it->flags & MIF_SEPARATOR) {
            b->h = sepH;
            y += sepH;
            continue;
        }
        load(it->textId, b->label, sizeof(b->label));
        split_tab(b->label, b->accel, sizeof(b->accel));
        b->h = itemH;
        y += itemH;
    }

    hdc = GetDC(NULL);
    {
        HGDIOBJ old = SelectObject(hdc, M.fUi);
        const char *sample = "Ag";
        sz.cx = 0; sz.cy = 0;
        GetTextExtentPoint32A(hdc, sample, 2, &sz);
        SelectObject(hdc, old);
    }
    s_textH = sz.cy;
    if (s_textH <= 0)
        s_textH = Metrics_S(12);

    for (i = 0; i < s_count; i++) {
        ItemBox *b = &s_box[i];
        int need;
        if (b->flags & MIF_SEPARATOR)
            continue;
        b->labelW = b->label[0] ? Widget_TextWidth(hdc, M.fUi, b->label) : 0;
        need = s_checkW + padL + b->labelW + padR;
        if (b->accel[0])
            need += accelGap + Widget_TextWidth(hdc, M.fUi, b->accel);
        if (need > w)
            w = need;
    }
    ReleaseDC(NULL, hdc);

    s_labelX = s_checkW + padL;
    s_w = w;
    s_h = y;
}

/* --------------------------------------------------------------- painting */

static void paint(HDC hdc)
{
    const Theme *t = &g_theme;
    RECT rc, sep;
    int i, x;
    int tickW = 8 * M.unit;
    int tickH = 7 * M.unit;

    rc.left = 0; rc.top = 0; rc.right = s_w; rc.bottom = s_h;
    Theme_Fill(hdc, &rc, t->menu);
    Theme_DrawEdge(hdc, &rc, TK_RAISED, M.bevel);

    for (i = 0; i < s_count; i++) {
        ItemBox *b = &s_box[i];
        RECT ir;
        COLORREF bg, fg;
        int hot;

        ir.left = 0; ir.top = b->y;
        ir.right = s_w; ir.bottom = b->y + b->h;

        if (b->flags & MIF_SEPARATOR) {
            int mid = b->y + b->h / 2;
            sep.left = ir.left + Metrics_S(3);
            sep.right = ir.right - Metrics_S(3);
            sep.top = mid; sep.bottom = mid + 1;
            Theme_Fill(hdc, &sep, t->shadow);
            sep.top = mid + 1; sep.bottom = mid + 2;
            Theme_Fill(hdc, &sep, t->hilight);
            continue;
        }

        hot = (i == s_hot) && ((b->flags & MIF_DISABLED) == 0);
        bg = hot ? t->highlight : t->menu;
        if (b->flags & MIF_DISABLED)
            fg = t->graytext;
        else
            fg = hot ? t->highlight_text : t->menutext;

        Theme_Fill(hdc, &ir, bg);

        if (b->flags & (MIF_CHECKED | MIF_RADIO)) {
            const unsigned short *m = (b->flags & MIF_RADIO) ? kBullet : kTick;
            draw_mask(hdc, (s_checkW - tickW) / 2,
                      ir.top + (b->h - tickH) / 2, m, 8, 7, fg, M.unit);
        }

        {
            int y = ir.top + (b->h - s_textH) / 2;
            x = s_labelX;
            draw_text(hdc, x, y, b->label, fg, bg);
            if (b->accel[0]) {
                int aw = Widget_TextWidth(hdc, M.fUi, b->accel);
                draw_text(hdc, s_w - Metrics_S(12) - aw, y, b->accel, fg, bg);
            }
        }
    }
}

/* ----------------------------------------------------------- window proc */

static int item_at(int mx, int my)
{
    int i;
    for (i = 0; i < s_count; i++) {
        ItemBox *b = &s_box[i];
        if (b->flags & MIF_SEPARATOR)
            continue;
        if (my >= b->y && my < b->y + b->h)
            return i;
    }
    return -1;
}

static void invalidate_item(int i)
{
    RECT r;
    if (s_popup == NULL || i < 0 || i >= s_count)
        return;
    r.left = 0; r.top = s_box[i].y;
    r.right = s_w; r.bottom = s_box[i].y + s_box[i].h;
    InvalidateRect(s_popup, &r, FALSE);
}

static void set_hot(int hot)
{
    int old = s_hot;
    if (hot == old)
        return;
    s_hot = hot;
    invalidate_item(old);
    invalidate_item(hot);
}

static void move_hot(int delta)
{
    int i = s_hot;
    int guard = 0;
    if (s_count == 0)
        return;
    if (i < 0)
        i = (delta > 0) ? -1 : s_count;
    do {
        i += delta;
        if (i < 0) i = s_count - 1;
        if (i >= s_count) i = 0;
        if (++guard > s_count)
            return;
    } while ((s_box[i].flags & (MIF_SEPARATOR | MIF_DISABLED)) != 0);
    set_hot(i);
}

static void pick(void)
{
    if (s_hot >= 0 && (s_box[s_hot].flags & MIF_DISABLED) == 0) {
        s_result = s_box[s_hot].id;
        s_done = 1;
    }
}

static LRESULT CALLBACK Menu_WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        paint(hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_MOUSEMOVE:
        set_hot(item_at((short)LOWORD(lp), (short)HIWORD(lp)));
        return 0;

    case WM_LBUTTONDOWN:
        /* Pressing outside the item list dismisses the menu the way the
         * system does: the click closes it and goes no further. */
        if (item_at((short)LOWORD(lp), (short)HIWORD(lp)) < 0)
            s_done = 1;
        return 0;

    case WM_LBUTTONUP: {
        int i = item_at((short)LOWORD(lp), (short)HIWORD(lp));
        if (i >= 0 && (s_box[i].flags & MIF_DISABLED) == 0) {
            s_result = s_box[i].id;
            s_done = 1;
        } else if (i < 0) {
            s_done = 1;
        }
        return 0;
    }

    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_NCLBUTTONDOWN:
    case WM_NCRBUTTONDOWN:
        s_done = 1;
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

/* --------------------------------------------------------------- public */

UINT Menu_Track(HWND owner, const MenuDef *def, int x, int y)
{
    static int registered = 0;
    WNDCLASSA wc;
    MSG msg;
    RECT work;

    if (def == NULL || def->count <= 0)
        return 0;

    s_inst = (HINSTANCE)(INT_PTR)GetWindowLongA(owner, GWL_HINSTANCE);
    build_boxes(def);
    if (s_w <= 0 || s_h <= 0)
        return 0;

    if (!registered) {
        ZeroMemory(&wc, sizeof(wc));
        wc.lpfnWndProc = Menu_WndProc;
        wc.hInstance = s_inst;
        wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
        wc.hbrBackground = NULL;
        wc.lpszClassName = MENU_CLASS;
        if (RegisterClassA(&wc) == 0)
            return 0;
        registered = 1;
    }

    /* keep the whole menu on screen */
    if (SystemParametersInfoA(SPI_GETWORKAREA, 0, &work, 0)) {
        if (x + s_w > work.right)  x = work.right - s_w;
        if (y + s_h > work.bottom) y = work.bottom - s_h;
        if (x < work.left) x = work.left;
        if (y < work.top)  y = work.top;
    }

    s_hot = -1;
    s_result = 0;
    s_done = 0;

    s_popup = CreateWindowExA(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, MENU_CLASS, "",
                              WS_POPUP, x, y, s_w, s_h,
                              owner, NULL, s_inst, NULL);
    if (s_popup == NULL)
        return 0;

    ShowWindow(s_popup, SW_SHOWNA);
    UpdateWindow(s_popup);
    SetCapture(s_popup);

    /* Mouse input arrives here because of the capture; key input still goes to
     * the focused window, so it is intercepted in the loop instead. */
    while (!s_done) {
        if (GetMessageA(&msg, NULL, 0, 0) <= 0)
            break;
        if (msg.hwnd == s_popup) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        } else if (msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN) {
            switch (msg.wParam) {
            case VK_UP:    move_hot(-1); break;
            case VK_DOWN:  move_hot(1);  break;
            case VK_LEFT:  move_hot(-1); break;
            case VK_RIGHT: move_hot(1);  break;
            case VK_RETURN: pick();      break;
            case VK_ESCAPE: s_done = 1;  break;
            default: {
                int k;
                for (k = 0; k < s_count; k++) {
                    const char *p;
                    if (s_box[k].flags & (MIF_SEPARATOR | MIF_DISABLED))
                        continue;
                    for (p = s_box[k].label; *p != 0; p++) {
                        char c;
                        if (*p != '&' || p[1] == 0)
                            continue;
                        c = p[1];
                        if (c >= 'a' && c <= 'z')
                            c = (char)(c - 'a' + 'A');
                        if ((int)(unsigned char)c == (int)msg.wParam) {
                            s_hot = k;
                            pick();
                        }
                        break;
                    }
                    if (s_done)
                        break;
                }
                break;
            }
            }
        } else if (msg.message == WM_LBUTTONDOWN || msg.message == WM_RBUTTONDOWN ||
                   msg.message == WM_NCLBUTTONDOWN || msg.message == WM_MBUTTONDOWN ||
                   msg.message == WM_MOUSEWHEEL) {
            break;
        } else {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

    if (GetCapture() == s_popup)
        ReleaseCapture();
    DestroyWindow(s_popup);
    s_popup = NULL;
    return s_result;
}
