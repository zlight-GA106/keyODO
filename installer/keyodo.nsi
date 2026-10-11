/* ===========================================================================
 * keyodo.nsi -- KeyODO installer
 *
 * Build (normally via build.cmd, which supplies the defines):
 *   makensis /DSRCDIR=<repo> /DOUTDIR=<repo>\build /DDISTDIR=<repo>\dist ^
 *            installer\keyodo.nsi
 *
 * Produces dist\KeyODO_Setup.exe, which
 *   - installs into %ProgramFiles%\KeyODO
 *   - creates a Start Menu shortcut (always)
 *   - optionally creates a desktop shortcut
 *   - writes the standard Add/Remove Programs entry
 *   - ships its own uninstaller
 *   - offers to launch KeyODO when finished
 *   - supports silent install:  KeyODO_Setup.exe /S
 *                               KeyODO_Setup.exe /S /D=C:\Path\To\KeyODO
 *                               KeyODO_Setup.exe /S /COMPONENTS="main,startmenu"
 * ======================================================================== */

!include "MUI2.nsh"
!include "FileFunc.nsh"
!include "LogicLib.nsh"

/* --- build inputs ------------------------------------------------------- */
!ifndef SRCDIR
  !define SRCDIR ".."
!endif
!ifndef OUTDIR
  !define OUTDIR "${SRCDIR}\build"
!endif
!ifndef DISTDIR
  !define DISTDIR "${SRCDIR}\dist"
!endif

!define APP_NAME       "KeyODO"
!define APP_VERSION    "0.1"
!define APP_PUBLISHER  "zlight-GA106"
!define APP_EXE        "keyodo.exe"
!define APP_ICON       "keyodo.ico"
!define APP_KEY        "Software\KeyODO"
!define UNINST_KEY     "Software\Microsoft\Windows\CurrentVersion\Uninstall\KeyODO"

!define MUI_ICON   "${SRCDIR}\res\keyodo.ico"
!define MUI_UNICON "${SRCDIR}\res\keyodo.ico"

Name "${APP_NAME} ${APP_VERSION}"
/* tools/installer_ui_test.py builds with /DUITEST to get a copy it can launch
 * and walk through: an admin installer raises a UAC prompt a script cannot
 * answer, and a wizard nobody can open is a wizard nobody checks. A shipped
 * build never defines this. */
!ifdef UITEST
  RequestExecutionLevel user
  !define SETUP_NAME "KeyODO_Setup_uitest.exe"
!else
  RequestExecutionLevel admin
  !ifndef SETUP_NAME
    !define SETUP_NAME "KeyODO_Setup.exe"
  !endif
!endif
OutFile "${DISTDIR}\${SETUP_NAME}"
/* The UI test build installs into a scratch directory and leaves the
 * registry and the shortcuts alone: otherwise walking the wizard would
 * overwrite a real installation's Add/Remove entry. */
!ifdef UITEST
  InstallDir "$TEMP\KeyODO_uitest"
!else
  InstallDir "$PROGRAMFILES\${APP_NAME}"
!endif
!ifndef UITEST
  /* Remembering the previous location is right for a real install, but it
   * would drag the UI test build back into the real Program Files folder. */
  InstallDirRegKey HKLM "${APP_KEY}" "InstallDir"
!endif
SetCompressor /SOLID lzma
ShowInstDetails show
ShowUninstDetails show

VIProductVersion "0.1.0.0"
VIAddVersionKey "ProductName"     "${APP_NAME}"
VIAddVersionKey "FileDescription" "${APP_NAME} Setup"
VIAddVersionKey "FileVersion"     "${APP_VERSION}"
VIAddVersionKey "ProductVersion"  "${APP_VERSION}"
VIAddVersionKey "CompanyName"     "${APP_PUBLISHER}"
VIAddVersionKey "LegalCopyright"  "${APP_PUBLISHER}"

/* --- pages -------------------------------------------------------------- */
!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_RUN "$INSTDIR\${APP_EXE}"
/* Via LangString, like every other visible string: a literal here would go
 * through the script's parse-time codepage conversion and come out as ?????. */
!define MUI_FINISHPAGE_RUN_TEXT "$(FINISH_RUN_TEXT)"

!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "SimpChinese"

/* --- strings ------------------------------------------------------------ */
LangString DESC_MAIN      ${LANG_SIMPCHINESE} "KeyODO 程序文件（必需）"
LangString DESC_STARTMENU ${LANG_SIMPCHINESE} "在开始菜单中创建快捷方式"
LangString DESC_DESKTOP   ${LANG_SIMPCHINESE} "在桌面上创建快捷方式"
LangString UNINST_RUNNING ${LANG_SIMPCHINESE} "KeyODO 正在运行。请先退出 KeyODO，然后再继续。"
LangString NEED_ADMIN     ${LANG_SIMPCHINESE} "安装 KeyODO 需要管理员权限。$\r$\n$\r$\n请右键单击安装程序，选择“以管理员身份运行”。"
LangString LNK_APP        ${LANG_SIMPCHINESE} "KeyODO"
LangString LNK_UNINST     ${LANG_SIMPCHINESE} "卸载 KeyODO"
LangString SM_GROUP       ${LANG_SIMPCHINESE} "KeyODO"
LangString FINISH_RUN_TEXT ${LANG_SIMPCHINESE} "立即运行 KeyODO"
LangString UNINST_KEEP    ${LANG_SIMPCHINESE} "已卸载 KeyODO。$\r$\n$\r$\n个人统计数据仍保留在：$\r$\n$APPDATA\KeyODO$\r$\n如需彻底清除，请手动删除该文件夹。"

/* --- init --------------------------------------------------------------- */
/* Installing under %ProgramFiles% and writing the Add/Remove Programs entry
 * both need administrator rights. Without an explicit check a non-elevated run
 * still copies the files and then silently skips the registry and shortcuts,
 * which looks like a successful install but is not. Fail loudly instead.
 *
 * The test is a real capability probe rather than UserInfo::GetAccountType:
 * on a UAC-filtered account that call reports the group membership ("Admin")
 * even though the token is not elevated and HKLM writes will be denied. */
Function CheckAdmin
  ClearErrors
  WriteRegStr HKLM "${APP_KEY}" "ElevationProbe" "1"
  IfErrors notElevated
  DeleteRegValue HKLM "${APP_KEY}" "ElevationProbe"
  Return

notElevated:
  IfSilent +3 0
  MessageBox MB_OK|MB_ICONSTOP "$(NEED_ADMIN)"
  Abort
  SetErrorLevel 5
  Abort
FunctionEnd

Function .onInit
  /* The UI test build must reach the pages, so it skips the privilege probe. */
!ifndef UITEST
  Call CheckAdmin
!endif

  /* Refuse to overwrite a running instance: keyodo.exe would be locked. */
  FindWindow $0 "KeyODO_Main" ""
  StrCmp $0 0 done
  IfSilent silentFail
  MessageBox MB_OK|MB_ICONEXCLAMATION "$(UNINST_RUNNING)"
  Abort
silentFail:
  SetErrorLevel 2
  Abort
done:
FunctionEnd

Function un.onInit
  FindWindow $0 "KeyODO_Main" ""
  StrCmp $0 0 done
  IfSilent silentFail
  MessageBox MB_OK|MB_ICONEXCLAMATION "$(UNINST_RUNNING)"
  Abort
silentFail:
  SetErrorLevel 2
  Abort
done:
FunctionEnd

/* --- sections ----------------------------------------------------------- */
/* Section names go through LangString rather than being written as literals.
 * A literal non-ASCII string in the script is converted once, at parse time,
 * from the script's charset into the installer's codepage -- and that
 * conversion is what turned these into "????????" in the component list.
 * LangString values live in the language table, which NSIS encodes per
 * language, so they survive (the shortcut names already relied on this). */
LangString SEC_MAIN_NAME      ${LANG_SIMPCHINESE} "KeyODO 程序文件（必需）"
LangString SEC_STARTMENU_NAME ${LANG_SIMPCHINESE} "开始菜单快捷方式"
LangString SEC_DESKTOP_NAME   ${LANG_SIMPCHINESE} "桌面快捷方式"

Section "$(SEC_MAIN_NAME)" SEC_MAIN
  SectionIn RO
  SetShellVarContext all
  SetOutPath "$INSTDIR"
  SetOverwrite on

  File "${OUTDIR}\${APP_EXE}"
  File "/oname=${APP_ICON}" "${SRCDIR}\res\${APP_ICON}"

  WriteUninstaller "$INSTDIR\uninstall.exe"

!ifndef UITEST
  WriteRegStr HKLM "${APP_KEY}" "InstallDir" "$INSTDIR"
  WriteRegStr HKLM "${APP_KEY}" "Version" "${APP_VERSION}"

  /* Standard Add/Remove Programs entry. A 32-bit installer writes to the
   * Wow6432Node view on 64-bit Windows, which is where 32-bit entries belong. */
  WriteRegStr   HKLM "${UNINST_KEY}" "DisplayName"     "${APP_NAME} ${APP_VERSION}"
  WriteRegStr   HKLM "${UNINST_KEY}" "DisplayVersion"  "${APP_VERSION}"
  WriteRegStr   HKLM "${UNINST_KEY}" "DisplayIcon"     "$INSTDIR\${APP_EXE},0"
  WriteRegStr   HKLM "${UNINST_KEY}" "Publisher"       "${APP_PUBLISHER}"
  WriteRegStr   HKLM "${UNINST_KEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr   HKLM "${UNINST_KEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr   HKLM "${UNINST_KEY}" "QuietUninstallString" '"$INSTDIR\uninstall.exe" /S'
  WriteRegDWORD HKLM "${UNINST_KEY}" "NoModify" 1
  WriteRegDWORD HKLM "${UNINST_KEY}" "NoRepair" 1

  ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
  IntFmt $0 "0x%08X" $0
  WriteRegDWORD HKLM "${UNINST_KEY}" "EstimatedSize" "$0"
!endif
SectionEnd

Section "$(SEC_STARTMENU_NAME)" SEC_STARTMENU
!ifndef UITEST
  SectionIn RO
  SetShellVarContext all
  CreateDirectory "$SMPROGRAMS\$(SM_GROUP)"
  CreateShortCut "$SMPROGRAMS\$(SM_GROUP)\$(LNK_APP).lnk" \
                 "$INSTDIR\${APP_EXE}" "" "$INSTDIR\${APP_EXE}" 0
  CreateShortCut "$SMPROGRAMS\$(SM_GROUP)\$(LNK_UNINST).lnk" \
                 "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
!endif
SectionEnd

Section "$(SEC_DESKTOP_NAME)" SEC_DESKTOP
!ifndef UITEST
  SetShellVarContext all
  CreateShortCut "$DESKTOP\$(LNK_APP).lnk" \
                 "$INSTDIR\${APP_EXE}" "" "$INSTDIR\${APP_EXE}" 0
!endif
SectionEnd

/* --- descriptions ------------------------------------------------------- */
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_MAIN}      "$(DESC_MAIN)"
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_STARTMENU} "$(DESC_STARTMENU)"
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_DESKTOP}   "$(DESC_DESKTOP)"
!insertmacro MUI_FUNCTION_DESCRIPTION_END

/* --- uninstall ---------------------------------------------------------- */
Section "Uninstall"
  SetShellVarContext all

  Delete "$INSTDIR\${APP_EXE}"
  Delete "$INSTDIR\${APP_ICON}"
  Delete "$INSTDIR\uninstall.exe"
  RMDir "$INSTDIR"

  /* Wildcards, not exact names: a shortcut left behind would keep the Start
   * Menu folder alive and make RMDir fail. */
  Delete "$SMPROGRAMS\$(SM_GROUP)\*.lnk"
  RMDir "$SMPROGRAMS\$(SM_GROUP)"
  Delete "$DESKTOP\$(LNK_APP).lnk"
  Delete "$DESKTOP\${APP_NAME}.lnk"

  DeleteRegKey HKLM "${UNINST_KEY}"
  DeleteRegKey HKLM "${APP_KEY}"

  /* %APPDATA%\KeyODO is deliberately kept: it holds the user's odometer
   * totals. Removing it silently would throw away their data. */
  IfSilent +2 0
  MessageBox MB_OK|MB_ICONINFORMATION "$(UNINST_KEEP)"
SectionEnd
