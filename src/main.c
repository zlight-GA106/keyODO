/* ===========================================================================
 * main.c -- process entry point.
 * ======================================================================== */

#include <windows.h>

#include "app.h"
#include "common.h"
#include "platform/platform.h"
#include "resource.h"

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow)
{
    MSG msg;
    HACCEL accel;

    UNUSED(hPrev);

    Plat_Init();

    if (!App_Init(hInst, lpCmdLine))
        return 1;

    accel = LoadAcceleratorsA(hInst, MAKEINTRESOURCEA(IDM_MAIN));

    ShowWindow(App_Window(), nCmdShow);
    UpdateWindow(App_Window());

    while (GetMessageA(&msg, NULL, 0, 0) > 0) {
        if (accel != NULL && TranslateAcceleratorA(App_Window(), accel, &msg))
            continue;
        if (App_PreTranslate(&msg))
            continue;
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    App_Shutdown();
    return (int)msg.wParam;
}
