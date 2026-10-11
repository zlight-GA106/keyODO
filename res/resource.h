/* ===========================================================================
 * resource.h -- resource and command identifiers for KeyODO.
 * ======================================================================== */

#ifndef KEYODO_RESOURCE_H
#define KEYODO_RESOURCE_H

/* --------------------------------------------------------------- version */
/* Kept here rather than in src/common.h because the resource compiler cannot
 * parse common.h (it contains typedefs). This is the single source of truth
 * for the version, used by both keyodo.rc and the C sources. */
#define KEYODO_VERSION_MAJOR 0
#define KEYODO_VERSION_MINOR 1
#define KEYODO_VERSION_PATCH 0
#define KEYODO_VERSION_STR   "0.1"
#define KEYODO_VERSION_NUM   0,1,0,0
#define KEYODO_VERSION_NUM_S "0.1.0.0"

/* -------------------------------------------------------------------- icons */
#define IDI_KEYODO              100

/* ------------------------------------------------------------------- menus */
#define IDM_MAIN                100

/* ---------------------------------------------------------- menu commands */
#define IDM_EXIT                40001

#define IDM_RESET_TRIP          40010
#define IDM_RESET_MAX           40011

#define IDM_SHOW_KPM            40020
#define IDM_SHOW_GEAR           40021
#define IDM_TM_AUTO             40022
#define IDM_TM_MANUAL           40023
#define IDM_TOPMOST             40024
#define IDM_SHOW_WIDGET         40025    /* detached desktop gauge          */
#define IDM_WIDGET_HIDE         40026    /* from the widget's own menu      */
#define IDM_WIDGET_SHOWMAIN     40027    /* bring the main window forward   */

#define IDM_WIN5                40030
#define IDM_WIN10               40031
#define IDM_WIN30               40032

#define IDM_ABOUT               40040

/* ------------------------------------------- internal hit-test identifiers */
/* Not menu commands: these identify which drawn control is under the mouse. */
#define IDC_CAP_MIN             500
#define IDC_CAP_CLOSE           501
#define IDC_BTN_TRIP            502
#define IDC_BTN_PEAK            503

/* ---------------------------------------------------------- string table */
#define IDS_APP_TITLE           2000

#define IDS_MENU_FILE           2010
#define IDS_MENU_EDIT           2011
#define IDS_MENU_VIEW           2012
#define IDS_MENU_STATS          2013
#define IDS_MENU_HELP           2014

#define IDS_GB_SPEED            2020
#define IDS_GB_ODO              2021
#define IDS_GB_RATE             2022

#define IDS_LBL_ODO             2030
#define IDS_LBL_TRIP            2031
#define IDS_LBL_KPM             2032
#define IDS_LBL_MAX             2033

#define IDS_BTN_TRIP            2040
#define IDS_BTN_PEAK            2041

#define IDS_STATUS_READY        2050
#define IDS_ABOUT_TITLE         2060
#define IDS_ABOUT_TEXT          2061

#define IDS_WIDGET_MENU_MAIN    2070
#define IDS_WIDGET_MENU_HIDE    2071
#define IDS_WIDGET_MENU_EXIT    2072
#define IDS_FMT_SCALE           2073   /* "%d DPI  %d%%" */

#endif /* KEYODO_RESOURCE_H */
