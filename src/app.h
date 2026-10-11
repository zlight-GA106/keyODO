/* ===========================================================================
 * app.h -- the composition root.
 *
 * app.c is the only file that knows all four modules. Everything else talks to
 * one neighbour: input feeds statistics, statistics feeds the UI, storage
 * saves a plain record.
 * ======================================================================== */

#ifndef KEYODO_APP_H
#define KEYODO_APP_H

#include <windows.h>

int  App_Init(HINSTANCE hInst, const char *cmdLine);
void App_Shutdown(void);
HWND App_Window(void);

/* Gives the input backend first look at every message. Returns 1 if consumed. */
int  App_PreTranslate(MSG *msg);

#endif /* KEYODO_APP_H */
