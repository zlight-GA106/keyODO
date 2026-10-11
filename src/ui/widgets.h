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
