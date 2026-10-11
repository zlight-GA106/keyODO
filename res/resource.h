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

/* Dropdown menu items. The menus are drawn by src/ui/menu.c, so their text
 * lives here with every other user-visible string. A tab separates the label
 * from the shortcut column, and & marks the mnemonic. */
#define IDS_MENU_EXIT           2080
#define IDS_MENU_RESET_TRIP     2081
#define IDS_MENU_RESET_MAX      2082
#define IDS_MENU_WIDGET         2083
#define IDS_MENU_KPM            2084
#define IDS_MENU_GEAR           2085
#define IDS_MENU_AUTO           2086
#define IDS_MENU_MANUAL         2087
#define IDS_MENU_TOPMOST        2088
#define IDS_MENU_WIN5           2089
#define IDS_MENU_WIN10          2090
#define IDS_MENU_WIN30          2091
#define IDS_MENU_ABOUT          2092

/* ------------------------------------------------------ second language --
 * Every translated string has a twin exactly I18N_STRIDE above it, so adding
 * a language is: one more block in the string table, one more set of defines
 * here, and one more entry in src/i18n/i18n.c. No code outside i18n.c knows
 * which language is active. */
#define IDS_LANG_STRIDE         1000
#define IDS_EN(id)              ((id) + IDS_LANG_STRIDE)

#define IDS_LANG_ZH             2095   /* language names, shown in-language  */
#define IDS_LANG_EN             2096

#define IDS_APP_TITLE_EN        IDS_EN(IDS_APP_TITLE)

#define IDS_MENU_FILE_EN        IDS_EN(IDS_MENU_FILE)
#define IDS_MENU_EDIT_EN        IDS_EN(IDS_MENU_EDIT)
#define IDS_MENU_VIEW_EN        IDS_EN(IDS_MENU_VIEW)
#define IDS_MENU_STATS_EN       IDS_EN(IDS_MENU_STATS)
#define IDS_MENU_HELP_EN        IDS_EN(IDS_MENU_HELP)

#define IDS_GB_SPEED_EN         IDS_EN(IDS_GB_SPEED)
#define IDS_GB_ODO_EN           IDS_EN(IDS_GB_ODO)
#define IDS_GB_RATE_EN          IDS_EN(IDS_GB_RATE)

#define IDS_LBL_ODO_EN          IDS_EN(IDS_LBL_ODO)
#define IDS_LBL_TRIP_EN         IDS_EN(IDS_LBL_TRIP)
#define IDS_LBL_KPM_EN          IDS_EN(IDS_LBL_KPM)
#define IDS_LBL_MAX_EN          IDS_EN(IDS_LBL_MAX)

#define IDS_BTN_TRIP_EN         IDS_EN(IDS_BTN_TRIP)
#define IDS_BTN_PEAK_EN         IDS_EN(IDS_BTN_PEAK)

#define IDS_STATUS_READY_EN     IDS_EN(IDS_STATUS_READY)
#define IDS_ABOUT_TITLE_EN      IDS_EN(IDS_ABOUT_TITLE)
#define IDS_ABOUT_TEXT_EN       IDS_EN(IDS_ABOUT_TEXT)

#define IDS_WIDGET_MENU_MAIN_EN IDS_EN(IDS_WIDGET_MENU_MAIN)
#define IDS_WIDGET_MENU_HIDE_EN IDS_EN(IDS_WIDGET_MENU_HIDE)
#define IDS_WIDGET_MENU_EXIT_EN IDS_EN(IDS_WIDGET_MENU_EXIT)
#define IDS_FMT_SCALE_EN        IDS_EN(IDS_FMT_SCALE)

#define IDS_MENU_EXIT_EN        IDS_EN(IDS_MENU_EXIT)
#define IDS_MENU_RESET_TRIP_EN  IDS_EN(IDS_MENU_RESET_TRIP)
#define IDS_MENU_RESET_MAX_EN   IDS_EN(IDS_MENU_RESET_MAX)
#define IDS_MENU_WIDGET_EN      IDS_EN(IDS_MENU_WIDGET)
#define IDS_MENU_KPM_EN         IDS_EN(IDS_MENU_KPM)
#define IDS_MENU_GEAR_EN        IDS_EN(IDS_MENU_GEAR)
#define IDS_MENU_AUTO_EN        IDS_EN(IDS_MENU_AUTO)
#define IDS_MENU_MANUAL_EN      IDS_EN(IDS_MENU_MANUAL)
#define IDS_MENU_TOPMOST_EN     IDS_EN(IDS_MENU_TOPMOST)
#define IDS_MENU_WIN5_EN        IDS_EN(IDS_MENU_WIN5)
#define IDS_MENU_WIN10_EN       IDS_EN(IDS_MENU_WIN10)
#define IDS_MENU_WIN30_EN       IDS_EN(IDS_MENU_WIN30)
#define IDS_MENU_ABOUT_EN       IDS_EN(IDS_MENU_ABOUT)

#define IDS_LANG_ZH_EN          IDS_EN(IDS_LANG_ZH)
#define IDS_LANG_EN_EN          IDS_EN(IDS_LANG_EN)

/* --------------------------------------------- tray and startup (0.2) --- */
#define IDM_TRAY_SHOW           40050
#define IDM_TRAY_ENABLE         40051   /* 显示托盘图标                       */
#define IDM_AUTOSTART           40052   /* 开机自启                           */
#define IDM_TRAY_HIDE           40053   /* hide the gauge, from the tray menu */
#define IDM_LANG_ZH             40060
#define IDM_LANG_EN             40061
#define IDM_FIT_TO_SCREEN       40062   /* 适应屏幕（小屏自动缩小）           */

#define IDS_MENU_TRAY           2100
#define IDS_MENU_AUTOSTART      2101
#define IDS_MENU_LANG           2102
#define IDS_MENU_FIT            2103
#define IDS_TRAY_TIP            2104    /* tray tooltip                       */
#define IDS_MENU_TRAY_SHOW      2105
#define IDS_MENU_TRAY_EXIT      2106
#define IDS_MENU_TRAY_HIDE      2107
#define IDS_AUTOSTART_FAIL      2108   /* registry write refused            */

#define IDS_MENU_TRAY_EN        IDS_EN(IDS_MENU_TRAY)
#define IDS_MENU_AUTOSTART_EN   IDS_EN(IDS_MENU_AUTOSTART)
#define IDS_MENU_LANG_EN        IDS_EN(IDS_MENU_LANG)
#define IDS_MENU_FIT_EN         IDS_EN(IDS_MENU_FIT)
#define IDS_TRAY_TIP_EN         IDS_EN(IDS_TRAY_TIP)
#define IDS_MENU_TRAY_SHOW_EN   IDS_EN(IDS_MENU_TRAY_SHOW)
#define IDS_MENU_TRAY_EXIT_EN   IDS_EN(IDS_MENU_TRAY_EXIT)
#define IDS_MENU_TRAY_HIDE_EN   IDS_EN(IDS_MENU_TRAY_HIDE)
#define IDS_AUTOSTART_FAIL_EN   IDS_EN(IDS_AUTOSTART_FAIL)

#endif /* KEYODO_RESOURCE_H */
