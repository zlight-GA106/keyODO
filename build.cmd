@echo off
rem ===========================================================================
rem  KeyODO one-shot build
rem
rem    build.cmd              build keyodo.exe and KeyODO_Setup.exe
rem    build.cmd app          build only keyodo.exe
rem    build.cmd setup        build only the installer (needs keyodo.exe)
rem    build.cmd clean        remove build\ and dist\
rem
rem  Requires:
rem    - Visual Studio with the x86 C++ tools (found through vswhere)
rem    - NSIS makensis.exe (PATH, Program Files, or KEYODO_NSIS env var)
rem  Output:
rem    build\keyodo.exe       32-bit application
rem    dist\KeyODO_Setup.exe  installer
rem ===========================================================================
setlocal enabledelayedexpansion
cd /d "%~dp0"

set "OUTDIR=build"
set "DISTDIR=dist"
set "TARGET=%~1"
if "%TARGET%"=="" set "TARGET=all"

if /i "%TARGET%"=="clean" (
    if exist "%OUTDIR%" rmdir /s /q "%OUTDIR%"
    if exist "%DISTDIR%" rmdir /s /q "%DISTDIR%"
    echo [clean] removed build\ and dist\
    exit /b 0
)

rem ---------------------------------------------------------------- sources
set "SRCS=src\main.c src\app.c src\common.c src\platform\platform.c src\ui\ui.c src\ui\theme.c src\ui\widgets.c src\ui\gauge.c src\ui\gear.c src\stats\stats.c src\input\input.c src\input\input_raw.c src\input\input_hook.c src\storage\storage.c"

set "OBJS=%OUTDIR%\main.obj %OUTDIR%\app.obj %OUTDIR%\common.obj %OUTDIR%\platform.obj %OUTDIR%\ui.obj %OUTDIR%\theme.obj %OUTDIR%\widgets.obj %OUTDIR%\gauge.obj %OUTDIR%\gear.obj %OUTDIR%\stats.obj %OUTDIR%\input.obj %OUTDIR%\input_raw.obj %OUTDIR%\input_hook.obj %OUTDIR%\storage.obj"

if not exist "%OUTDIR%" mkdir "%OUTDIR%"
if not exist "%DISTDIR%" mkdir "%DISTDIR%"

if /i not "%TARGET%"=="setup" goto :build_app
goto :build_setup

rem ------------------------------------------------------------------- app
:build_app
echo [1/4] locating the 32-bit toolchain
if not defined VCINSTALLDIR (
    set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if not exist "!VSWHERE!" (
        echo ERROR: vswhere.exe not found. Install Visual Studio with the C++ workload.
        exit /b 1
    )
    for /f "usebackq delims=" %%i in (`"!VSWHERE!" -latest -products * -property installationPath`) do set "VSPATH=%%i"
    if not defined VSPATH (
        echo ERROR: Visual Studio installation not found.
        exit /b 1
    )
    call "!VSPATH!\VC\Auxiliary\Build\vcvars32.bat" >nul
    if errorlevel 1 (
        echo ERROR: vcvars32.bat failed.
        exit /b 1
    )
)
echo       cl: & where cl

echo [2/4] compiling (x86, /MT)
cl /nologo /c /TC /W3 /O2 /MT /GS- /DWIN32 /D_WINDOWS /D_CRT_SECURE_NO_WARNINGS ^
   /I include /I res /Fo"%OUTDIR%\\" %SRCS%
if errorlevel 1 (
    echo ERROR: compile failed.
    exit /b 1
)

echo [3/4] resources
rem Windows 98 SE needs the resource script as code page 936. The file under
rem version control is UTF-8, so convert it into build\ first.
python tools\to_cp936.py res\keyodo.rc --out "%OUTDIR%\keyodo_cp936.rc"
if errorlevel 1 (
    echo ERROR: resource encoding step failed.
    exit /b 1
)
rc /nologo /i res /fo "%OUTDIR%\keyodo.res" "%OUTDIR%\keyodo_cp936.rc"
if errorlevel 1 (
    echo ERROR: rc failed.
    exit /b 1
)

echo [4/4] linking
rem No manifest is embedded on purpose: without a comctl32 v6 manifest the
rem program keeps the classic, unthemed control look on modern Windows too.
link /nologo /SUBSYSTEM:WINDOWS /MACHINE:X86 /INCREMENTAL:NO ^
     /OUT:"%OUTDIR%\keyodo.exe" %OBJS% "%OUTDIR%\keyodo.res" ^
     user32.lib gdi32.lib shell32.lib
if errorlevel 1 (
    echo ERROR: link failed.
    exit /b 1
)
echo       built %OUTDIR%\keyodo.exe

if /i "%TARGET%"=="app" goto :done

rem --------------------------------------------------------------- installer
:build_setup
if not exist "%OUTDIR%\keyodo.exe" (
    echo ERROR: %OUTDIR%\keyodo.exe missing. Run "build.cmd app" first.
    exit /b 1
)

echo [setup] locating makensis
set "MAKENSIS="
if defined KEYODO_NSIS if exist "%KEYODO_NSIS%" set "MAKENSIS=%KEYODO_NSIS%"
if not defined MAKENSIS for %%p in (makensis.exe) do if not "%%~$PATH:p"=="" set "MAKENSIS=%%~$PATH:p"
if not defined MAKENSIS if exist "%ProgramFiles(x86)%\NSIS\makensis.exe" set "MAKENSIS=%ProgramFiles(x86)%\NSIS\makensis.exe"
if not defined MAKENSIS if exist "%ProgramFiles%\NSIS\makensis.exe" set "MAKENSIS=%ProgramFiles%\NSIS\makensis.exe"
if not defined MAKENSIS if exist "%USERPROFILE%\.keyodo-tools\nsis\makensis.exe" set "MAKENSIS=%USERPROFILE%\.keyodo-tools\nsis\makensis.exe"
if not defined MAKENSIS (
    echo ERROR: makensis.exe not found.
    echo        Install NSIS, or set KEYODO_NSIS to the full path of makensis.exe.
    exit /b 1
)
echo       !MAKENSIS!

rem NSIS reads a script as the system code page unless it carries a UTF-8 BOM.
rem The repository copy is BOM-less; give the build a BOM'd copy so the result
rem does not depend on the code page of the machine running the build.
python tools\to_utf8bom.py installer\keyodo.nsi --out "%OUTDIR%\keyodo_utf8.nsi"
if errorlevel 1 (
    echo ERROR: installer encoding step failed.
    exit /b 1
)

"%MAKENSIS%" /V2 /DSRCDIR="%CD%" /DOUTDIR="%CD%\%OUTDIR%" /DDISTDIR="%CD%\%DISTDIR%" "%OUTDIR%\keyodo_utf8.nsi"
if errorlevel 1 (
    echo ERROR: makensis failed.
    exit /b 1
)
echo       built %DISTDIR%\KeyODO_Setup.exe

:done
echo.
echo Build complete.
endlocal
exit /b 0
