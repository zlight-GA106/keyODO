/* ===========================================================================
 * input_raw.c -- Raw Input keyboard backend.
 *
 * RIDEV_INPUTSINK is what makes this useful for an odometer: the window keeps
 * receiving WM_INPUT while it is in the background, so keystrokes are counted
 * whatever the user is typing into.
 *
 * Auto-repeat: Raw Input does not flag repeats, so a 256-entry pressed table is
 * maintained here. The first make code for a key is a real press; further make
 * codes without an intervening break are repeats. Statistics counts ODO/TRIP
 * from real presses only, but still feeds repeats into the rate window.
 *
 * Only virtual key codes and scan codes leave this file. No character, no
 * keyboard layout translation, no text is ever produced or stored.
 * ======================================================================== */

#include "input.h"

#include <windows.h>

#include "../common.h"

void Input_SetActiveName(const char *name);

static InputSink s_sink = NULL;
static void     *s_user = NULL;
static unsigned char s_down[256];
static HWND      s_hwnd = NULL;

static int raw_start(void *nativeWindow, InputSink sink, void *user)
{
    RAWINPUTDEVICE rid;

    s_hwnd = (HWND)nativeWindow;
    s_sink = sink;
    s_user = user;
    ZeroMemory(s_down, sizeof(s_down));

    /* Generic Desktop / Keyboard. hwndTarget is required by RIDEV_INPUTSINK. */
    rid.usUsagePage = 0x01;
    rid.usUsage = 0x06;
    rid.dwFlags = RIDEV_INPUTSINK;
    rid.hwndTarget = s_hwnd;

    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid)))
        return 0;

    Input_SetActiveName("raw");
    return 1;
}

static void raw_stop(void)
{
    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x01;
    rid.usUsage = 0x06;
    rid.dwFlags = RIDEV_REMOVE;
    rid.hwndTarget = NULL;
    RegisterRawInputDevices(&rid, 1, sizeof(rid));
    Input_SetActiveName("");
}

static int raw_on_message(void *nativeWindow, unsigned int msg,
                          unsigned long wp, unsigned long lp, long *result)
{
    UINT size = 0;
    static BYTE buffer[64];
    RAWINPUT *ri;
    const RAWKEYBOARD *kb;
    int isDown, isRepeat;
    unsigned int vkey;
    unsigned int scan;

    UNUSED(nativeWindow);
    UNUSED(wp);
    UNUSED(result);

    if (msg != WM_INPUT)
        return 0;

    if (GetRawInputData((HRAWINPUT)lp, RID_INPUT, NULL, &size,
                        sizeof(RAWINPUTHEADER)) != 0)
        return 1;
    if (size == 0 || size > sizeof(buffer))
        return 1;
    if (GetRawInputData((HRAWINPUT)lp, RID_INPUT, buffer, &size,
                        sizeof(RAWINPUTHEADER)) != size)
        return 1;

    ri = (RAWINPUT *)buffer;
    if (ri->header.dwType != RIM_TYPEKEYBOARD)
        return 1;

    kb = &ri->data.keyboard;
    vkey = kb->VKey;
    scan = kb->MakeCode;

    /* 255 is the fake shift that some keyboards emit around arrow keys. */
    if (vkey == 0xFF || vkey >= 256)
        return 1;

    isDown = ((kb->Flags & RI_KEY_BREAK) == 0);

    if (isDown) {
        isRepeat = s_down[vkey] ? 1 : 0;
        s_down[vkey] = 1;
    } else {
        isRepeat = 0;
        s_down[vkey] = 0;
    }

    if (s_sink != NULL)
        s_sink(vkey, isDown, isRepeat, scan, s_user);
    return 1;
}

static const InputBackend kRawBackend = {
    "raw",
    raw_start,
    raw_stop,
    raw_on_message
};

const InputBackend *Input_RawInput(void)
{
    return &kRawBackend;
}
