/* ===========================================================================
 * input.c -- backend selection.
 *
 * Kept separate from the implementations so the choice is a single, obvious
 * line of policy rather than something buried in WinMain.
 * ======================================================================== */

#include "input.h"

#include <windows.h>
#include <string.h>

static const char *s_active = "";

const InputBackend *Input_ByName(const char *name)
{
    if (name != NULL && (lstrcmpiA(name, "hook") == 0))
        return Input_LowLevelHook();
    return Input_RawInput();
}

const char *Input_ActiveName(void)
{
    return s_active;
}

/* The implementations report their name through this hook so Input_ByName and
 * the active name never disagree. */
void Input_SetActiveName(const char *name)
{
    s_active = (name != NULL) ? name : "";
}
