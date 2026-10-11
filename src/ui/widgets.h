/* ===========================================================================
 * widgets.h -- the handful of Windows 98 controls KeyODO draws by hand.
 *
 * No common controls are used for the chrome: everything is painted with the
 * pinned palette so the window looks identical on Windows 98 SE and on a
 * modern Windows. Text helpers reproduce the exact centring used by the
 * reference renderer (mockup/keyodo_mockup.py): horizontally centred on the
 * advance width, vertically centred on the ink box.
 * ======================================================================== */

#ifndef KEYODO_WIDGETS_H
#define KEYODO_WIDGETS_H

#include <windows.h>

/* ---- text ---- */
int  Widget_TextWidth(HDC hdc, HFONT f, const char *s);
void Widget_TextAt(HDC hdc, int x, int y, const char *s,
                   HFONT f, COLORREF fg, COLORREF bg);
/* Same placement but without painting a background -- for text over the
 * caption gradient. */
void Widget_TextAtClear(HDC hdc, int x, int y, const char *s,
                        HFONT f, COLORREF fg);
void Widget_Shutdown(void);
void Widget_TextLeft(HDC hdc, int x, const RECT *box, const char *s,
                     HFONT f, COLORREF fg, COLORREF bg);
void Widget_TextCentered(HDC hdc, const RECT *box, const char *s,
                         HFONT f, COLORREF fg, COLORREF bg);

/* Draws `text` with the mnemonic underlined; `&` marks it and is not shown,
 * `&&` is a literal ampersand. Returns the width drawn. Handles both the
 * English ("&File") and CJK (letter in brackets) caption styles. */
int  Widget_TextMnemonic(HDC hdc, int x, int y, const char *text,
                         HFONT f, COLORREF fg, COLORREF bg);

/* Same string without its mnemonic markers, measured. Use this wherever a
 * caption has to be laid out, so the box matches what gets drawn. */
int  Widget_TextWidthMnemonic(HDC hdc, HFONT f, const char *s);

/* ---- chrome ---- */
void Widget_GroupBox(HDC hdc, const RECT *box, const char *caption);
void Widget_PushButton(HDC hdc, const RECT *box, const char *caption, int pressed);
void Widget_CaptionButton(HDC hdc, const RECT *box,
                          const unsigned short *glyph, int pressed);

/* ---- read-outs ---- */
/* Sunken white panel with the value centred in the read-out font. */
void Widget_Readout(HDC hdc, const RECT *box, const char *text, HFONT f);
/* Mechanical counter: one cell per character of `digits`. */
void Widget_Drum(HDC hdc, const RECT *box, const char *digits, int cellW);

#endif /* KEYODO_WIDGETS_H */
