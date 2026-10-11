/* ===========================================================================
 * i18n.c -- string-table lookup keyed by the active language.
 *
 * Translated strings live exactly IDS_LANG_STRIDE above their original, which
 * gives O(1) lookup with no language table to maintain in code. If a
 * translation is missing the original is used, so a half-finished language
 * degrades to mixed text rather than blank labels.
 * ======================================================================== */

#include "i18n.h"

#include "resource.h"

#include <string.h>

#define I18N_FIRST  2000
#define I18N_LAST   2199
#define I18N_SLOTS  (I18N_LAST - I18N_FIRST + 1)
#define I18N_MAXLEN 128

/* The Chinese face is SimSun, written as escapes to keep this file pure ASCII.
 * English uses the stock Windows 98 Latin UI face. */
static const char kFaceZh[] = "\xcb\xce\xcc\xe5";
static const char kFaceEn[] = "MS Sans Serif";

typedef struct LangInfo {
    const char *face;
    int         px;      /* at 96 DPI */
} LangInfo;

static const LangInfo kLangs[I18N_LANG_COUNT] = {
    { kFaceZh, 12 },
    { kFaceEn, 11 }
};

static HINSTANCE s_inst = NULL;
static int       s_lang = -1;
static char      s_cache[I18N_SLOTS][I18N_MAXLEN];
static char      s_loaded[I18N_SLOTS];

void I18n_SetLang(int lang)
{
    if (lang < 0 || lang >= I18N_LANG_COUNT)
        lang = I18N_LANG_ZH;
    if (lang == s_lang)
        return;
    s_lang = lang;
    ZeroMemory(s_loaded, sizeof(s_loaded));
}

void I18n_Init(HINSTANCE hInst, int lang)
{
    s_inst = hInst;
    s_lang = -1;
    I18n_SetLang(lang);
}

int I18n_Lang(void)
{
    return (s_lang < 0) ? I18N_LANG_ZH : s_lang;
}

const char *I18n_Str(UINT id)
{
    int slot;
    int n = 0;

    if (id < I18N_FIRST || id > I18N_LAST)
        return "";
    slot = (int)(id - I18N_FIRST);

    if (!s_loaded[slot]) {
        if (s_inst != NULL) {
            if (s_lang > 0) {
                n = LoadStringA(s_inst, id + IDS_LANG_STRIDE,
                                s_cache[slot], I18N_MAXLEN);
            }
            if (n <= 0)
                n = LoadStringA(s_inst, id, s_cache[slot], I18N_MAXLEN);
        }
        if (n <= 0)
            s_cache[slot][0] = 0;
        s_loaded[slot] = 1;
    }
    return s_cache[slot];
}

const char *I18n_LangName(int lang)
{
    if (lang == I18N_LANG_EN)
        return I18n_Str(IDS_LANG_EN);
    return I18n_Str(IDS_LANG_ZH);
}

const char *I18n_UiFace(void)
{
    return kLangs[I18n_Lang()].face;
}

int I18n_UiFontPx(void)
{
    return kLangs[I18n_Lang()].px;
}
