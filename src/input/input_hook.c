/* ===========================================================================
 * input_hook.c -- low-level keyboard hook backend.
 *
 * This exists to prove the InputBackend seam is real: switching the INI key
 * InputBackend from "raw" to "hook" swaps the capture mechanism with no change
 * anywhere else in the program.
 *
 * WINDOWS 98 SE NOTE -----------------------------------------------------
 * WH_KEYBOARD_LL is documented from Windows 2000 onward. On Windows 98 SE the
 * period-correct approach is a global WH_KEYBOARD hook living in a 32-bit DLL
 * that the system maps into every process, replaying events to the main window
 * through a private message. That backend plugs in here as a third
 * InputBackend and needs no changes above this file.
 * ------------------------------------------------------------------------
 * ======================================================================== */

#include "input.h"

#include <windows.h>

#include "../common.h"

void Input_SetActiveName(const char *name);

static InputSink s_sink = NULL;
static void     *s_user = NULL;
static HHOOK     s_hook = NULL;
static unsigned char s_down[256];

static LRESULT CALLBACK ll_key_proc(int code, WPARAM wp, LPARAM lp)
{
    if (code == HC_ACTION && s_sink != NULL) {
        const KBDLLHOOKSTRUCT *kb = (const KBDLLHOOKSTRUCT *)lp;
        unsigned int vkey = kb->vkCode;
        unsigned int scan = kb->scanCode;
        int isDown = ((kb->flags & LLKHF_UP) == 0);
        int isRepeat = 0;

        if (vkey < 256) {
            if (isDown) {
                isRepeat = s_down[vkey] ? 1 : 0;
                s_down[vkey] = 1;
            } else {
                s_down[vkey] = 0;
            }
        }
        s_sink(vkey, isDown, isRepeat, scan, s_user);
    }
    return CallNextHookEx(s_hook, code, wp, lp);
}

static int hook_start(void *nativeWindow, InputSink sink, void *user)
{
    HINSTANCE inst = (HINSTANCE)GetModuleHandleA(NULL);

    UNUSED(nativeWindow);
    s_sink = sink;
    s_user = user;
    ZeroMemory(s_down, sizeof(s_down));

    s_hook = SetWindowsHookExA(WH_KEYBOARD_LL, ll_key_proc, inst, 0);
    if (s_hook == NULL)
        return 0;

    Input_SetActiveName("hook");
    return 1;
}

static void hook_stop(void)
{
    if (s_hook != NULL) {
        UnhookWindowsHookEx(s_hook);
        s_hook = NULL;
    }
    s_sink = NULL;
    Input_SetActiveName("");
}

static int hook_on_message(void *nativeWindow, unsigned int msg,
                           unsigned long wp, unsigned long lp, long *result)
{
    UNUSED(nativeWindow); UNUSED(msg); UNUSED(wp); UNUSED(lp); UNUSED(result);
    /* A low-level hook is delivered on the installing thread's message queue;
     * no window message is involved. */
    return 0;
}

static const InputBackend kHookBackend = {
    "hook",
    hook_start,
    hook_stop,
    hook_on_message
};

const InputBackend *Input_LowLevelHook(void)
{
    return &kHookBackend;
}
