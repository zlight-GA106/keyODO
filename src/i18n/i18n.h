/* ===========================================================================
 * i18n.h -- one place that knows which language is active.
 *
 * The design rule for this project is that every user-visible string lives in
 * the resource string table and no C source contains a non-ASCII byte. Adding
 * a language therefore means: one more STRINGTABLE block in res/keyodo.rc, one
 * more set of IDS_EN(...) defines in res/resource.h, and one more entry in the
 * table in i18n.c. Nothing else changes.
 *
 * Strings are cached on first use, so I18n_Str can be called from painting
 * code without a resource lookup per frame.
 * ======================================================================== */

#ifndef KEYODO_I18N_H
#define KEYODO_I18N_H

#include <windows.h>

#define I18N_LANG_ZH    0
#define I18N_LANG_EN    1
#define I18N_LANG_COUNT 2

void I18n_Init(HINSTANCE hInst, int lang);

/* Switching reloads every cached string; the caller must rebuild the fonts
 * (the UI face depends on the language) and repaint. */
void I18n_SetLang(int lang);
int  I18n_Lang(void);

/* Localized text for a string-table id. Falls back to the built-in language
 * when a translation is missing, and never returns NULL. */
const char *I18n_Str(UINT id);

/* Language name, shown in its own language. */
const char *I18n_LangName(int lang);

/* UI font. Chinese wants a bitmap CJK face at its own pixel size; English
 * wants the classic Windows 98 Latin face, which is smaller. */
const char *I18n_UiFace(void);
int         I18n_UiFontPx(void);

#endif /* KEYODO_I18N_H */
