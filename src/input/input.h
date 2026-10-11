/* ===========================================================================
 * input.h -- the keyboard input backend interface.
 *
 * The rest of the program never calls RegisterRawInputDevices or
 * SetWindowsHookEx. It selects a backend once and feeds key events to the
 * statistics module. Replacing Raw Input with a Windows 98 hook therefore
 * means adding one file and one table entry -- nothing else changes.
 *
 * The interface deliberately uses 32-bit integers for the message parameters
 * so it stays valid on Windows 98 SE, where there is no LONG_PTR.
 * ======================================================================== */

#ifndef KEYODO_INPUT_H
#define KEYODO_INPUT_H

/* vkey   : virtual key code
 * isDown : 1 for press, 0 for release
 * isRepeat: 1 when the key was already held (auto-repeat)
 * scan   : hardware scan code
 */
typedef void (*InputSink)(unsigned int vkey, int isDown, int isRepeat,
                          unsigned int scan, void *user);

typedef struct InputBackend {
    const char *name;
    /* nativeWindow is an HWND; kept as void* so this header does not need
     * windows.h. Returns non-zero on success. */
    int  (*start)(void *nativeWindow, InputSink sink, void *user);
    void (*stop)(void);
    /* Returns 1 when the message was consumed. `result` receives the value the
     * window procedure should return. */
    int  (*on_message)(void *nativeWindow, unsigned int msg,
                       unsigned long wp, unsigned long lp, long *result);
} InputBackend;

/* Available backends. */
const InputBackend *Input_RawInput(void);
const InputBackend *Input_LowLevelHook(void);

/* Returns the backend named "raw" or "hook"; falls back to Raw Input. */
const InputBackend *Input_ByName(const char *name);

/* Name of the backend that is currently running ("" before start). */
const char *Input_ActiveName(void);

#endif /* KEYODO_INPUT_H */
