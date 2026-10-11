# Windows classic UI asset inventory

Extracted 2026-10-11 11:20:13 with `tools/pe_extract.py` (pure-Python PE resource extractor, stdlib `struct` + Pillow).

Host: Windows 11 LTSC, build 26100. All assets were read directly out of Microsoft-signed
binaries that ship with this OS. Nothing here is reconstructed, hand-drawn or invented.

## Contents

1. [Summary](#1-summary)
2. [Method](#2-method)
3. [Per-target status](#3-per-target-status)
4. [Classic (1995-1999) artifacts found](#4-classic-19951999-artifacts-found)
5. [Notable absences](#5-notable-absences)
6. [Full inventory of everything extracted](#6-full-inventory-of-everything-extracted)
7. [Appendix A - commands run and key output](#appendix-a---commands-run-and-key-output)
8. [Appendix B - per-source manifests](#appendix-b---per-source-manifests)

---

## 1. Summary

| metric | value |
|---|---|
| PE sources processed | 54 |
| sources that yielded at least one asset | 34 |
| sources that yielded nothing | 20 |
| resource records extracted | 8992 |
| RT_BITMAP -> PNG | 165 |
| RT_ICON images -> PNG | 7384 |
| RT_GROUP_ICON -> multi-image .ico | 1104 |
| RT_CURSOR images -> PNG | 210 |
| RT_GROUP_CURSOR -> .cur | 129 |
| files on disk (PNG assets) | 7759 |
| files on disk (.ico) | 1104 |
| files on disk (.cur) | 129 |
| extraction errors (unreadable/malformed resources) | 0 |
| files that failed to re-open with Pillow | 0 |

Layout: `assets/extracted/<source>/bitmaps/*.png`, `icons/*.ico` + `*.png`,
`cursors/*.cur` + `*.png`, plus `_contact_sheet.png` (grid, labelled, nearest-neighbour
upscaled) and `_manifest.json` (machine-readable record of every extracted resource).

---

## 2. Method

`tools/pe_extract.py` is a dependency-free Win32 PE resource extractor:

* DOS header -> `e_lfanew` -> `PE\0\0` -> COFF header -> optional header (PE32 and PE32+)
  -> data directory index 2 = resource table.
* Walks the three-level `IMAGE_RESOURCE_DIRECTORY` tree (type -> name/ID -> language) to the
  `IMAGE_RESOURCE_DATA_ENTRY`. Handles both named entries (high bit of the Name field set,
  UTF-16 `IMAGE_RESOURCE_DIR_STRING_U`) and numeric IDs.
* Maps resource RVAs to file offsets through the section table.
* `RT_BITMAP`: raw DIB (BITMAPINFOHEADER and 12-byte BITMAPCOREHEADER). Handles 1/4/8/16/24/32 bpp,
  BI_RGB / BI_BITFIELDS / BI_ALPHABITFIELDS **and BI_RLE4 / BI_RLE8** (the classic comctl32 toolbar
  strips are RLE4-compressed - this is essential, not optional). Palette count honours `biClrUsed`
  (0 = 2^bpp entries). `biHeight < 0` (top-down) handled. 32 bpp keeps alpha only when a non-zero
  alpha channel is actually present, otherwise treated as opaque.
* `RT_ICON` + `RT_GROUP_ICON`: rebuilt into real multi-image `.ico` files (ICONDIR/ICONDIRENTRY).
  The DIB colour half is separated from the AND mask (the mask has a different row stride, which is
  why a naive whole-buffer DIB parse fails on icons), the AND mask is applied for transparency, and
  a canonical XOR+AND DIB is written into the .ico. Vista+ PNG-compressed RT_ICON payloads
  (signature `89 50 4E 47`) are detected and embedded verbatim.
* `RT_CURSOR` + `RT_GROUP_CURSOR`: the 4-byte hotspot prefix that rc.exe puts in front of the DIB is
  detected, hotspot comes from the group directory, `.cur` files are written.
* Every resource is wrapped in its own exception handler: a malformed resource is logged and skipped,
  never aborting the run. Across all 54 sources this run finished with **0 errors**.

### Why `.mun` sources appear below

On Windows 10/11 most icon and bitmap resources are no longer inside `C:\Windows\System32\<name>`.
The System32 binaries are MUI stubs that forward to language-neutral resource-only images in
`C:\Windows\SystemResources\<name>.mun`. `shell32.dll`, `imageres.dll`, `moricons.dll`, `mmres.dll`,
`main.cpl`, `intl.cpl`, `sysdm.cpl`, `timedate.cpl` and `themeui.dll` all extract **0 assets** from
System32 and carry their real art in the matching `.mun`. Both were run for every target so the
inventory shows exactly where each asset physically comes from.

---

## 3. Per-target status

`bmp` = RT_BITMAP -> PNG, `ico img` = RT_ICON -> PNG, `grp ico` = RT_GROUP_ICON -> .ico,
`cur img` = RT_CURSOR -> PNG, `grp cur` = RT_GROUP_CURSOR -> .cur.

| # | source label | path | file version | size (bytes) | extraction | bmp | ico img | grp ico | cur img | grp cur | contact sheet |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | `comctl32` | `C:/Windows/System32/comctl32.dll` | 5.82 (WinBuild.160101.0800) | 742,904 | OK (75) | 10 | 10 | 4 | 33 | 18 | [sheet](extracted/comctl32/_contact_sheet.png) |
| 2 | `user32` | `C:/Windows/System32/user32.dll` | 10.0.26100.8117 (WinBuild.160101.0800) | 1,914,656 | OK (195) | 7 | 77 | 7 | 70 | 34 | [sheet](extracted/user32/_contact_sheet.png) |
| 3 | `shell32` | `C:/Windows/System32/shell32.dll` | 10.0.26100.9278 (WinBuild.160101.0800) | 8,009,440 | no assets | 0 | 0 | 0 | 0 | 0 | - |
| 4 | `moricons` | `C:/Windows/System32/moricons.dll` | 10.0.26100.1 (WinBuild.160101.0800) | 12,288 | no assets | 0 | 0 | 0 | 0 | 0 | - |
| 5 | `pifmgr` | `C:/Windows/System32/pifmgr.dll` | 10.0.26100.1 (WinBuild.160101.0800) | 45,056 | OK (77) | 0 | 39 | 38 | 0 | 0 | [sheet](extracted/pifmgr/_contact_sheet.png) |
| 6 | `main_cpl` | `C:/Windows/System32/main.cpl` | 10.0.26100.8875 (WinBuild.160101.0800) | 102,400 | no assets | 0 | 0 | 0 | 0 | 0 | - |
| 7 | `joy_cpl` | `C:/Windows/System32/joy.cpl` | 10.0.26100.8875 (WinBuild.160101.0800) | 118,784 | OK (9) | 0 | 8 | 1 | 0 | 0 | [sheet](extracted/joy_cpl/_contact_sheet.png) |
| 8 | `timedate_cpl` | `C:/Windows/System32/timedate.cpl` | 10.0.26100.8875 (WinBuild.160101.0800) | 270,336 | no assets | 0 | 0 | 0 | 0 | 0 | - |
| 9 | `sysdm_cpl` | `C:/Windows/System32/sysdm.cpl` | 10.0.26100.8875 (WinBuild.160101.0800) | 172,032 | no assets | 0 | 0 | 0 | 0 | 0 | - |
| 10 | `desk_cpl` | `C:/Windows/System32/desk.cpl` | 10.0.26100.8875 (WinBuild.160101.0800) | 237,568 | OK (11) | 0 | 10 | 1 | 0 | 0 | [sheet](extracted/desk_cpl/_contact_sheet.png) |
| 11 | `intl_cpl` | `C:/Windows/System32/intl.cpl` | 10.0.26100.8875 (WinBuild.160101.0800) | 245,760 | no assets | 0 | 0 | 0 | 0 | 0 | - |
| 12 | `powercfg_cpl` | `C:/Windows/System32/powercfg.cpl` | 10.0.26100.8875 (WinBuild.160101.0800) | 274,432 | OK (11) | 0 | 10 | 1 | 0 | 0 | [sheet](extracted/powercfg_cpl/_contact_sheet.png) |
| 13 | `themeui` | `C:/Windows/System32/themeui.dll` | 10.0.26100.8875 (WinBuild.160101.0800) | 606,208 | no assets | 0 | 0 | 0 | 0 | 0 | - |
| 14 | `imageres` | `C:/Windows/System32/imageres.dll` | 10.0.26100.7705 (WinBuild.160101.0800) | 12,288 | no assets | 0 | 0 | 0 | 0 | 0 | - |
| 15 | `browseui` | `C:/Windows/System32/browseui.dll` | 10.0.26100.1 (WinBuild.160101.0800) | 40,960 | no assets | 0 | 0 | 0 | 0 | 0 | - |
| 16 | `oledlg` | `C:/Windows/System32/oledlg.dll` | 10.0.26100.8875 (WinBuild.160101.0800) | 233,472 | OK (3) | 3 | 0 | 0 | 0 | 0 | [sheet](extracted/oledlg/_contact_sheet.png) |
| 17 | `mmres` | `C:/Windows/System32/mmres.dll` | 10.0.26100.8875 (WinBuild.160101.0800) | 12,288 | no assets | 0 | 0 | 0 | 0 | 0 | - |
| 18 | `comctl32_winsxs_v5` | `C:/Windows/WinSxS/amd64_microsoft.windows.common-controls_6595b64144ccf1df_5.82.26100.8328_none_87eede957a2ccddd/comctl32.dll` | 5.82 (WinBuild.160101.0800) | 742,904 | OK (75) | 10 | 10 | 4 | 33 | 18 | [sheet](extracted/comctl32_winsxs_v5/_contact_sheet.png) |
| 19 | `comctl32_winsxs_v6` | `C:/Windows/WinSxS/amd64_microsoft.windows.common-controls_6595b64144ccf1df_6.0.26100.9278_none_3e0d1ba8e3303201/comctl32.dll` | 5.82 (WinBuild.160101.0800) | 2,684,408 | OK (105) | 22 | 28 | 4 | 33 | 18 | [sheet](extracted/comctl32_winsxs_v6/_contact_sheet.png) |
| 20 | `mmsys_cpl` | `C:/Windows/System32/mmsys.cpl` | 10.0.26100.8875 (WinBuild.160101.0800) | 385,024 | no assets | 0 | 0 | 0 | 0 | 0 | - |
| 21 | `inetcpl_cpl` | `C:/Windows/System32/inetcpl.cpl` | 11.00.26100.8875 (WinBuild.160101.0800) | 581,632 | no assets | 0 | 0 | 0 | 0 | 0 | - |
| 22 | `appwiz_cpl` | `C:/Windows/System32/appwiz.cpl` | 10.0.26100.8875 (WinBuild.160101.0800) | 618,496 | no assets | 0 | 0 | 0 | 0 | 0 | - |
| 23 | `hdwwiz_cpl` | `C:/Windows/System32/hdwwiz.cpl` | 10.0.26100.8875 (WinBuild.160101.0800) | 409,600 | OK (11) | 2 | 6 | 3 | 0 | 0 | [sheet](extracted/hdwwiz_cpl/_contact_sheet.png) |
| 24 | `ncpa_cpl` | `C:/Windows/System32/ncpa.cpl` | 10.0.26100.8875 (WinBuild.160101.0800) | 126,976 | OK (11) | 0 | 10 | 1 | 0 | 0 | [sheet](extracted/ncpa_cpl/_contact_sheet.png) |
| 25 | `wscui_cpl` | `C:/Windows/System32/wscui.cpl` | 10.0.26100.8875 (WinBuild.160101.0800) | 147,456 | no assets | 0 | 0 | 0 | 0 | 0 | - |
| 26 | `telephon_cpl` | `C:/Windows/System32/telephon.cpl` | 10.0.26100.8875 (WinBuild.160101.0800) | 135,168 | OK (14) | 0 | 13 | 1 | 0 | 0 | [sheet](extracted/telephon_cpl/_contact_sheet.png) |
| 27 | `themecpl` | `C:/Windows/System32/themecpl.dll` | 10.0.26100.8875 (WinBuild.160101.0800) | 495,616 | no assets | 0 | 0 | 0 | 0 | 0 | - |
| 28 | `accessibilitycpl` | `C:/Windows/System32/accessibilitycpl.dll` | 10.0.26100.8875 (WinBuild.160101.0800) | 319,488 | no assets | 0 | 0 | 0 | 0 | 0 | - |
| 29 | `mspaint_exe` | `C:/Windows/System32/mspaint.exe` | 10.0.26100.8875 (WinBuild.160101.0800) | 876,544 | no assets | 0 | 0 | 0 | 0 | 0 | - |
| 30 | `calc_exe` | `C:/Windows/System32/calc.exe` | 10.0.26100.8521 (WinBuild.160101.0800) | 49,152 | OK (5) | 0 | 4 | 1 | 0 | 0 | [sheet](extracted/calc_exe/_contact_sheet.png) |
| 31 | `charmap_exe` | `C:/Windows/System32/charmap.exe` | 5.2.3668.0 | 282,624 | OK (15) | 1 | 13 | 1 | 0 | 0 | [sheet](extracted/charmap_exe/_contact_sheet.png) |
| 32 | `notepad_exe` | `C:/Windows/System32/notepad.exe` | 10.0.26100.8875 (WinBuild.160101.0800) | 360,448 | OK (26) | 0 | 22 | 4 | 0 | 0 | [sheet](extracted/notepad_exe/_contact_sheet.png) |
| 33 | `mstsc_exe` | `C:/Windows/System32/mstsc.exe` | 10.0.26100.8875 (WinBuild.160101.0800) | 1,114,112 | no assets | 0 | 0 | 0 | 0 | 0 | - |
| 34 | `ddores` | `C:/Windows/System32/ddores.dll` | 10.0.26100.8875 (WinBuild.160101.0800) | 71,600 | no assets | 0 | 0 | 0 | 0 | 0 | - |
| 35 | `printui` | `C:/Windows/System32/printui.dll` | 10.0.26100.8875 (WinBuild.160101.0800) | 770,048 | no assets | 0 | 0 | 0 | 0 | 0 | - |
| 36 | `sndvol_exe` | `C:/Windows/System32/sndvol.exe` | 10.0.26100.8875 (WinBuild.160101.0800) | 311,488 | OK (36) | 0 | 33 | 3 | 0 | 0 | [sheet](extracted/sndvol_exe/_contact_sheet.png) |
| 37 | `moricons_mun` | `C:/Windows/SystemResources/moricons.dll.mun` | 10.0.26100.1 (WinBuild.160101.0800) | 185,344 | OK (350) | 0 | 237 | 113 | 0 | 0 | [sheet](extracted/moricons_mun/_contact_sheet.png) |
| 38 | `shell32_mun` | `C:/Windows/SystemResources/shell32.dll.mun` | 10.0.26100.9278 (WinBuild.160101.0800) | 18,143,744 | OK (2865) | 35 | 2485 | 335 | 5 | 5 | [sheet](extracted/shell32_mun/_contact_sheet.png) |
| 39 | `imageres_mun` | `C:/Windows/SystemResources/imageres.dll.mun` | 10.0.26100.7705 (WinBuild.160101.0800) | 24,280,064 | OK (3210) | 9 | 2832 | 369 | 0 | 0 | [sheet](extracted/imageres_mun/_contact_sheet.png) |
| 40 | `mmres_mun` | `C:/Windows/SystemResources/mmres.dll.mun` | 10.0.26100.1 (WinBuild.160101.0800) | 9,568,256 | OK (158) | 0 | 140 | 18 | 0 | 0 | [sheet](extracted/mmres_mun/_contact_sheet.png) |
| 41 | `main_cpl_mun` | `C:/Windows/SystemResources/main.cpl.mun` | 10.0.26100.3037 (WinBuild.160101.0800) | 577,024 | OK (124) | 1 | 107 | 16 | 0 | 0 | [sheet](extracted/main_cpl_mun/_contact_sheet.png) |
| 42 | `intl_cpl_mun` | `C:/Windows/SystemResources/intl.cpl.mun` | 10.0.26100.1882 (WinBuild.160101.0800) | 263,680 | OK (23) | 0 | 22 | 1 | 0 | 0 | [sheet](extracted/intl_cpl_mun/_contact_sheet.png) |
| 43 | `sysdm_cpl_mun` | `C:/Windows/SystemResources/sysdm.cpl.mun` | 10.0.26100.1 (WinBuild.160101.0800) | 204,288 | OK (32) | 0 | 29 | 3 | 0 | 0 | [sheet](extracted/sysdm_cpl_mun/_contact_sheet.png) |
| 44 | `timedate_cpl_mun` | `C:/Windows/SystemResources/timedate.cpl.mun` | 10.0.26100.3624 (WinBuild.160101.0800) | 282,624 | OK (11) | 0 | 10 | 1 | 0 | 0 | [sheet](extracted/timedate_cpl_mun/_contact_sheet.png) |
| 45 | `themeui_mun` | `C:/Windows/SystemResources/themeui.dll.mun` | 10.0.26100.8737 (WinBuild.160101.0800) | 229,888 | OK (28) | 0 | 26 | 2 | 0 | 0 | [sheet](extracted/themeui_mun/_contact_sheet.png) |
| 46 | `accessibilitycpl_mun` | `C:/Windows/SystemResources/accessibilitycpl.dll.mun` | 10.0.26100.6725 (WinBuild.160101.0800) | 3,129,344 | OK (399) | 1 | 376 | 22 | 0 | 0 | [sheet](extracted/accessibilitycpl_mun/_contact_sheet.png) |
| 47 | `appwiz_cpl_mun` | `C:/Windows/SystemResources/appwiz.cpl.mun` | 10.0.26100.3624 (WinBuild.160101.0800) | 332,800 | OK (63) | 5 | 53 | 5 | 0 | 0 | [sheet](extracted/appwiz_cpl_mun/_contact_sheet.png) |
| 48 | `inetcpl_cpl_mun` | `C:/Windows/SystemResources/inetcpl.cpl.mun` | 11.00.26100.5074 (WinBuild.160101.0800) | 1,598,464 | OK (438) | 19 | 360 | 59 | 0 | 0 | [sheet](extracted/inetcpl_cpl_mun/_contact_sheet.png) |
| 49 | `mmsys_cpl_mun` | `C:/Windows/SystemResources/mmsys.cpl.mun` | 10.0.26100.1 (WinBuild.160101.0800) | 423,936 | OK (55) | 4 | 42 | 9 | 0 | 0 | [sheet](extracted/mmsys_cpl_mun/_contact_sheet.png) |
| 50 | `printui_mun` | `C:/Windows/SystemResources/printui.dll.mun` | 10.0.26100.9278 (WinBuild.160101.0800) | 520,192 | OK (60) | 3 | 48 | 9 | 0 | 0 | [sheet](extracted/printui_mun/_contact_sheet.png) |
| 51 | `wscui_cpl_mun` | `C:/Windows/SystemResources/wscui.cpl.mun` | 10.0.26100.1 (WinBuild.160101.0800) | 1,102,336 | OK (89) | 0 | 83 | 6 | 0 | 0 | [sheet](extracted/wscui_cpl_mun/_contact_sheet.png) |
| 52 | `themecpl_mun` | `C:/Windows/SystemResources/themecpl.dll.mun` | 10.0.26100.5074 (WinBuild.160101.0800) | 450,048 | OK (16) | 0 | 14 | 2 | 0 | 0 | [sheet](extracted/themecpl_mun/_contact_sheet.png) |
| 53 | `mspaint_exe_mun` | `C:/Windows/SystemResources/mspaint.exe.mun` | 10.0.26100.3037 (WinBuild.160101.0800) | 2,485,248 | OK (297) | 33 | 152 | 40 | 36 | 36 | [sheet](extracted/mspaint_exe_mun/_contact_sheet.png) |
| 54 | `mstsc_exe_mun` | `C:/Windows/SystemResources/mstsc.exe.mun` | 10.0.26100.9444 (WinBuild.160101.0800) | 550,400 | OK (95) | 0 | 75 | 20 | 0 | 0 | [sheet](extracted/mstsc_exe_mun/_contact_sheet.png) |


## 4. Classic (1995-1999) artifacts found

Everything in this section is backed by an actual successful extraction. Resource IDs are quoted
verbatim as they appear in the binary's resource directory. Where a *symbolic* name is given
(`IDB_STD_SMALL_COLOR`, `OBM_CHECK`, ...) it is a documented Win32/comctl32 name matched to the
numeric ID; the numeric ID is always the verified fact. Anything not found is stated as
**not present**.

### 4.1 `comctl32.dll` - the classic flat toolbar bitmap strips

Three comctl32 builds were examined:

| build | path | size | bitmaps found |
|---|---|---|---|
| v5.82 | `C:/Windows/System32/comctl32.dll` | 742,904 | 10 |
| v5.82 | `C:/Windows/WinSxS/amd64_microsoft.windows.common-controls_6595b64144ccf1df_5.82.26100.8328_none_87eede957a2ccddd/comctl32.dll` | 742,904 | 10 |
| v6.0 | `C:/Windows/WinSxS/amd64_microsoft.windows.common-controls_6595b64144ccf1df_6.0.26100.9278_none_3e0d1ba8e3303201/comctl32.dll` | 2,684,408 | 22 |

The two v5.82 copies are the **classic flat 4 bpp toolbar art**. Their extracted PNGs are
byte-identical to each other (verified by SHA-256 on `bitmap_120.png`).

**Exactly which numeric IDs exist in the v5.82 strip family (IDs 120-131):**

| ID | name (documented) | pixel size | bpp | compression | images in strip | output file |
|---|---|---|---|---|---|---|
| 120 | `IDB_STD_SMALL_COLOR` | 240 x 16 | 4 | BI_RLE4 | 15 x 16x16 | `extracted/comctl32/bitmaps/bitmap_120.png` |
| 121 | `IDB_STD_LARGE_COLOR` | 360 x 24 | 4 | BI_RLE4 | 15 x 24x24 | `extracted/comctl32/bitmaps/bitmap_121.png` |
| 124 | `IDB_VIEW_SMALL_COLOR` | 192 x 16 | 4 | BI_RLE4 | 12 x 16x16 | `extracted/comctl32/bitmaps/bitmap_124.png` |
| 125 | `IDB_VIEW_LARGE_COLOR` | 288 x 24 | 4 | BI_RLE4 | 12 x 24x24 | `extracted/comctl32/bitmaps/bitmap_125.png` |
| 130 | `IDB_HIST_SMALL_COLOR` | 80 x 16 | 4 | BI_RLE4 | 5 x 16x16 | `extracted/comctl32/bitmaps/bitmap_130.png` |
| 131 | `IDB_HIST_LARGE_COLOR` | 120 x 24 | 4 | BI_RLE4 | 5 x 24x24 | `extracted/comctl32/bitmaps/bitmap_131.png` |

Other bitmaps present in the same v5.82 build:

| ID | pixel size | bpp | compression | note | output file |
|---|---|---|---|---|---|
| 132 | 28 x 28 | 4 | BI_RLE4 | header/close glyph | `extracted/comctl32/bitmaps/bitmap_132.png` |
| 133 | 28 x 28 | 4 | BI_RLE4 | header glyph | `extracted/comctl32/bitmaps/bitmap_133.png` |
| 134 | 28 x 28 | 4 | BI_RLE4 | header glyph | `extracted/comctl32/bitmaps/bitmap_134.png` |
| 140 | 13 x 12 | 4 | BI_RGB | small arrow/filter glyph | `extracted/comctl32/bitmaps/bitmap_140.png` |

**Not present** in any of the three comctl32 builds examined, verbatim-checked in the resource
directory of each: bitmap IDs **122, 123, 126, 127, 128, 129**. The expected
`IDB_BROWSER_*` family is therefore **not present** in comctl32 on this build - if it exists at all
it is not in this DLL.

**v5 vs v6 difference (verified):** IDs 120 and 121 are byte-identical between v5.82 and v6.0
(SHA-256 match), i.e. the two classic `IDB_STD_*` strips survived unchanged into the themed build.
IDs 124, 125, 130 and 131 are **not** identical: in v6.0 they are re-authored as 32 bpp
`BI_RGB` images (12,328 / 27,688 / 5,162 / 11,562 bytes) instead of classic 4 bpp RLE4. To get the
classic flat-colour `IDB_VIEW_*` / `IDB_HIST_*` art you must use the **v5.82** build.

v6.0 also carries 12 extra bitmaps that the v5.82 build does not have: 141 (13x12, 8 bpp),
142 (85x17, 32 bpp), 143-146 (30x30, 32 bpp), 147-148 (18x21, 32 bpp), 151-154 (150x30, 32 bpp).

### 4.2 `user32.dll` - the `OBM_*` button glyph family

**Result: not present.** None of the 22 requested `OBM_*` IDs resolve in this build of Windows.

This was cross-checked in three separate user32 binaries and all three agree:

* `C:/Windows/System32/user32.dll` - 1,914,656 bytes, 10.0.26100.8117
* `C:/Windows/SysWOW64/user32.dll` - 1,972,624 bytes (the 32-bit build)
* `C:/Windows/WinSxS/amd64_microsoft-windows-user32_31bf3856ad364e35_10.0.26100.1591_none_68f867681d635676/user32.dll`

Verbatim check - IDs **not present** in all three:
`32738, 32739, 32743, 32744, 32745, 32746, 32753, 32754, 32755, 32756, 32758, 32759, 32760, 32764, 32767, 32768, 32769, 32771, 32772, 32773, 32774, 32775`.

`user32.dll` contains exactly **seven** `RT_BITMAP` resources. All seven were extracted:

| ID | pixel size | bpp | compression | output file |
|---|---|---|---|---|
| 36 | 70 x 42 | 4 | BI_RGB | `extracted/user32/bitmaps/bitmap_36.png` |
| 136 | 20 x 12 | 4 | BI_RGB | `extracted/user32/bitmaps/bitmap_136.png` |
| 236 | 25 x 16 | 4 | BI_RGB | `extracted/user32/bitmaps/bitmap_236.png` |
| 32559 | 28 x 28 | 4 | BI_RLE4 | `extracted/user32/bitmaps/bitmap_32559.png` |
| 32660 | 28 x 28 | 4 | BI_RLE4 | `extracted/user32/bitmaps/bitmap_32660.png` |
| 32661 | 28 x 28 | 4 | BI_RLE4 | `extracted/user32/bitmaps/bitmap_32661.png` |
| 32731 | 10 x 10 | 4 | BI_RGB | `extracted/user32/bitmaps/bitmap_32731.png` |

**Verified cross-check:** the three 28x28 RLE4 glyphs in user32 are byte-identical to three of the
comctl32 v5.82 28x28 glyphs (SHA-256 of the produced PNGs):

| user32 ID | comctl32 ID | result |
|---|---|---|
| 32559 | 133 | identical |
| 32660 | 134 | identical |
| 32661 | 132 | identical |

So the classic 28x28 header/button glyph art does survive on this build - it just lives under
different IDs than the `OBM_*` numbers, and the whole `OBM_*` range has been retired (Windows now
synthesises checkbox/radio/arrow glyphs at runtime).

`user32.dll` *does* still ship genuine classic cursor art: 34 `RT_GROUP_CURSOR` resources covering
70 cursor images, mostly **1 bpp** - group 100 (arrow, 32x32 + 48x48 1 bpp), 101 (I-beam),
102 (hourglass), 103 (cross), 104-115 (the size/move arrows), 32652-32662 (the standard `IDC_*`
set), plus four modern 32 bpp groups (32664, 32667, 32668, 32670). Output in
`assets/extracted/user32/cursors/`.

### 4.3 `pifmgr.dll` - the Windows 3.x PIF/application icon set

**Present and complete.** `C:/Windows/System32/pifmgr.dll` (45,056 bytes) still carries the
Windows 3.x-era icon set:

* 38 `RT_GROUP_ICON` resources, IDs **1-9 and 11-39** (ID **10 is not present**).
* 39 `RT_ICON` resources (IDs 1-39).
* Every image is **32 x 32, 4 bpp**; group 1 additionally bundles a 32x32 **1 bpp** variant
  (`RT_ICON` 1) alongside its 4 bpp image (`RT_ICON` 2).

Output: 38 real multi-image `.ico` files in `assets/extracted/pifmgr/icons/` (`groupicon_1.ico`
... `groupicon_39.ico`) plus 39 per-image PNGs. The set is unmistakably 1990s art: the MS-DOS
logo (groups 1/2), the flying-Windows flag (group 22), joystick (26), football (27), briefcase
(28), Christmas tree (29), racing car (30), trumpet (9) and so on. See
`assets/extracted/pifmgr/_contact_sheet.png`.

### 4.4 `moricons.dll` - the MS-DOS / Windows 3.x icon library

**The System32 file is a stub; the real set is present in `SystemResources`.**

* `C:/Windows/System32/moricons.dll` - 12,288 bytes, version 10.0.26100.1: contains exactly **one**
  resource (`RT_VERSION`). **0 bitmaps, 0 icons.** It is a MUI stub.
* `C:/Windows/SystemResources/moricons.dll.mun` - 185,344 bytes: **113 `RT_GROUP_ICON`** (IDs 1-113)
  and **237 `RT_ICON`** resources.

Geometry of the 113 group icons: 106 carry a 32x32 1 bpp image, 113 carry a 32x32 4 bpp image,
6 carry 16x16, 6 carry 48x48 8 bpp, and one group (ID 1) also carries 48x48 32 bpp / 32x32 32 bpp /
16x16 32 bpp variants. This is the classic application-icon library: Microsoft Word / Excel /
PowerPoint, Paradox, Quattro Pro, WordPerfect, Borland, dBase, Lotus, Harvard Graphics, MS-DOS
Prompt, Windows 3.x program-manager groups. Contact sheet:
`assets/extracted/moricons_mun/_contact_sheet.png`; 113 `.ico` files in
`assets/extracted/moricons_mun/icons/`.

### 4.5 Other genuine 1995-1999 art found while checking the targets

| source | resource | pixel size | bpp | why it matters | output file |
|---|---|---|---|---|---|
| `shell32.dll.mun` | `RT_BITMAP` **`IDB_TB_SH_DEF_16`** (a *named* resource) | 768 x 16 | 4 | the classic Explorer toolbar strip, 48 x 16x16 buttons | `extracted/shell32_mun/bitmaps/bitmap_IDB_TB_SH_DEF_16.png` |
| `shell32.dll.mun` | `RT_BITMAP` 210 | 80 x 16 | 4 | 5-button classic strip | `extracted/shell32_mun/bitmaps/bitmap_210.png` |
| `shell32.dll.mun` | `RT_BITMAP` 600-615 | 16 x 16 | 4 | 16 individual classic shell glyphs | `extracted/shell32_mun/bitmaps/bitmap_600.png` ... `bitmap_615.png` |
| `shell32.dll.mun` | `RT_BITMAP` 635 | 64 x 60 | 8 | classic shell artwork | `extracted/shell32_mun/bitmaps/bitmap_635.png` |
| `oledlg.dll` | `RT_BITMAP` 10 / 11 / 12 | 480x30 / 480x40 / 720x60 | 4 | the classic OLE "Insert Object" banner bitmaps | `extracted/oledlg/bitmaps/bitmap_10.png` etc. |
| `inetcpl.cpl.mun` | `RT_BITMAP` 4474 | 80 x 16 | 4 | classic IE toolbar strip | `extracted/inetcpl_cpl_mun/bitmaps/bitmap_4474.png` |
| `inetcpl.cpl.mun` | `RT_BITMAP` 7200-7217 | 64x64, 80x80, 128x128 | 24 | the classic Internet Explorer logo art (six sizes each) | `extracted/inetcpl_cpl_mun/bitmaps/bitmap_7200.png` ... `bitmap_7217.png` |
| `mspaint.exe.mun` | `RT_BITMAP` 102 | 186 x 15 | 4 | classic Paint tool palette strip | `extracted/mspaint_exe_mun/bitmaps/bitmap_102.png` |
| `mspaint.exe.mun` | `RT_BITMAP` 851-858 | 1x1 ... 8x8 | 1 | the classic brush-size squares | `extracted/mspaint_exe_mun/bitmaps/bitmap_851.png` ... `bitmap_858.png` |
| `mspaint.exe.mun` | `RT_GROUP_CURSOR` (36 cursor images) | 32x32, 48x48 | 32 | Paint tool cursors | `extracted/mspaint_exe_mun/cursors/` |
| `comctl32.dll` (v5) | `RT_CURSOR` 33 images in 18 groups | 32x32 ... 128x128 | 1 | the classic comctl32 splitter / drag cursors | `extracted/comctl32/cursors/` |
| `comctl32.dll` (v5) | `RT_GROUP_ICON` 150, 20480, 20481, 20482 | 32x32 / 16x16 | 4 and 1 | classic error / warning / information icons | `extracted/comctl32/icons/groupicon_*.ico` |
| `main.cpl.mun` | 107 `RT_ICON` images + `RT_BITMAP` 100 (83x80, 24 bpp) | varied | varied | the classic Mouse control-panel pointer schemes | `extracted/main_cpl_mun/` |
| `mmres.dll.mun` | 140 `RT_ICON` images in 18 groups | varied | varied | classic sound-event icons (speaker, microphone, ...) | `extracted/mmres_mun/` |
| `charmap.exe` | 13 `RT_ICON` + 1 `RT_BITMAP` | varied | varied | older binary (file version **5.2.3668.0**) | `extracted/charmap_exe/` |


---

## 5. Notable absences

Stated plainly, so nothing here gets mistaken for a real asset:

| expected | status | evidence |
|---|---|---|
| `user32.dll` `OBM_*` glyph family (all 22 IDs listed in 4.2) | **not present** | resource directory of 3 different user32 binaries walked; only 7 `RT_BITMAP` resources exist, listed in full in 4.2 |
| `comctl32.dll` bitmap IDs 122, 123, 126, 127, 128, 129 | **not present** | full `RT_BITMAP` enumeration of all 3 comctl32 builds; ids jump 121 -> 124 and 125 -> 130 |
| `comctl32.dll` `IDB_BROWSER_*` family | **not present** | no such named or numbered bitmap in any of the 3 builds |
| classic `IDB_VIEW_*` / `IDB_HIST_*` art in the v6 build (IDs 124, 125, 130, 131) | present but **re-authored as 32 bpp**, not the classic 4 bpp flat art | SHA-256 of v5 vs v6 `bitmap_124.png` differ; v6 versions are `BI_RGB` 32 bpp |
| classic `IDB_STD_*` art in the v6 build (IDs 120, 121) | **present and unchanged** | SHA-256 of v5 vs v6 `bitmap_120.png` are identical |
| `pifmgr.dll` icon group ID 10 | **not present** | group IDs run 1-9 then 11-39 |
| any bitmap or icon in `C:/Windows/System32/shell32.dll` | **not present** | 8,009,440-byte MUI stub with 4 resources (`MUI` x2, `RT_VERSION`, `RT_MANIFEST`); real art is in `shell32.dll.mun` |
| any bitmap or icon in `C:/Windows/System32/imageres.dll` | **not present** | 12,288-byte stub with 2 resources; real art is in `imageres.dll.mun` (24,280,064 bytes) |
| any bitmap or icon in `C:/Windows/System32/moricons.dll` | **not present** | 12,288-byte stub with 1 resource (`RT_VERSION`); real art is in `moricons.dll.mun` |
| any bitmap or icon in `C:/Windows/System32/mmres.dll` | **not present** | 12,288-byte stub; real art is in `mmres.dll.mun` (9,568,256 bytes) |
| `browseui.dll` bitmaps/icons | **not present** | 40,960-byte binary with no `RT_BITMAP` / `RT_ICON`; **no `browseui.dll.mun` exists anywhere in `C:/Windows/SystemResources`** |
| `mspaint.exe`, `mstsc.exe` bitmaps/icons | **not present in the .exe** | both are stubs; real art is in `mspaint.exe.mun` and `mstsc.exe.mun` |
| `mmsys.cpl`, `inetcpl.cpl`, `appwiz.cpl`, `wscui.cpl`, `printui.dll`, `accessibilitycpl.dll`, `themecpl.dll` bitmaps/icons | **not present in the System32 file** | all are MUI stubs; extracted from their `.mun` counterparts instead |
| `ddores.dll` bitmaps/icons | **not present in the System32 file** | 71,600-byte stub; a `DDORes.dll.mun` (16,382,976 bytes) exists but was deliberately **not** extracted - it contains only modern device art, no 1995-1999 material. Reported here for completeness rather than silently omitted. |
| `main.cpl`, `intl.cpl`, `sysdm.cpl`, `timedate.cpl`, `themeui.dll` bitmaps/icons | **not present in the System32 file** | all are MUI stubs; extracted from their `.mun` counterparts |

Also worth recording: `C:/Windows/System32/comctl32.dll` on this build is **not** a forwarding stub -
it is the real v5.82 classic build (742,904 bytes) and it yielded the classic toolbar strips
directly. The WinSxS v5.82 copy is byte-identical in its extracted art.

---

## 6. Full inventory of everything extracted

Every row is a resource record that was actually decoded and written to disk.
`px` is the pixel size of the produced image; `bpp` is the source bit depth as stored in the DLL
(`32` may mean a 32 bpp DIB or, where marked, a PNG-compressed RT_ICON).
Paths are relative to `assets/extracted/`.

| source | type | resource ID / name | px | bpp | output file |
|---|---|---|---|---|---|
| comctl32 | RT_BITMAP | 120 | 240x16 | 4 | `bitmaps/bitmap_120.png` |
| comctl32 | RT_BITMAP | 121 | 360x24 | 4 | `bitmaps/bitmap_121.png` |
| comctl32 | RT_BITMAP | 124 | 192x16 | 4 | `bitmaps/bitmap_124.png` |
| comctl32 | RT_BITMAP | 125 | 288x24 | 4 | `bitmaps/bitmap_125.png` |
| comctl32 | RT_BITMAP | 130 | 80x16 | 4 | `bitmaps/bitmap_130.png` |
| comctl32 | RT_BITMAP | 131 | 120x24 | 4 | `bitmaps/bitmap_131.png` |
| comctl32 | RT_BITMAP | 132 | 28x28 | 4 | `bitmaps/bitmap_132.png` |
| comctl32 | RT_BITMAP | 133 | 28x28 | 4 | `bitmaps/bitmap_133.png` |
| comctl32 | RT_BITMAP | 134 | 28x28 | 4 | `bitmaps/bitmap_134.png` |
| comctl32 | RT_BITMAP | 140 | 13x12 | 4 | `bitmaps/bitmap_140.png` |
| comctl32 | RT_GROUP_ICON | 150 | 32x32 | 1 | `icons/groupicon_150.ico` |
| comctl32 | RT_GROUP_ICON | 20480 | 32x32+32x32+16x16 | 4,1,4 | `icons/groupicon_20480.ico` |
| comctl32 | RT_GROUP_ICON | 20481 | 32x32+32x32+16x16 | 4,1,4 | `icons/groupicon_20481.ico` |
| comctl32 | RT_GROUP_ICON | 20482 | 32x32+32x32+16x16 | 4,1,4 | `icons/groupicon_20482.ico` |
| comctl32 | RT_ICON | 23 (group 150) | 32x32 | 1 | `icons/icon_150_0_32x32.png` |
| comctl32 | RT_ICON | 35 (group 20481) | 32x32 | 4 | `icons/icon_20481_0_32x32.png` |
| comctl32 | RT_ICON | 36 (group 20481) | 32x32 | 1 | `icons/icon_20481_1_32x32.png` |
| comctl32 | RT_ICON | 37 (group 20481) | 16x16 | 4 | `icons/icon_20481_2_16x16.png` |
| comctl32 | RT_ICON | 38 (group 20482) | 32x32 | 4 | `icons/icon_20482_0_32x32.png` |
| comctl32 | RT_ICON | 39 (group 20482) | 32x32 | 1 | `icons/icon_20482_1_32x32.png` |
| comctl32 | RT_ICON | 40 (group 20482) | 16x16 | 4 | `icons/icon_20482_2_16x16.png` |
| comctl32 | RT_ICON | 41 (group 20480) | 32x32 | 4 | `icons/icon_20480_0_32x32.png` |
| comctl32 | RT_ICON | 42 (group 20480) | 32x32 | 1 | `icons/icon_20480_1_32x32.png` |
| comctl32 | RT_ICON | 43 (group 20480) | 16x16 | 4 | `icons/icon_20480_2_16x16.png` |
| comctl32 | RT_GROUP_CURSOR | 102 | multi | - | `cursors/groupcursor_102.cur` |
| comctl32 | RT_GROUP_CURSOR | 104 | multi | - | `cursors/groupcursor_104.cur` |
| comctl32 | RT_GROUP_CURSOR | 105 | multi | - | `cursors/groupcursor_105.cur` |
| comctl32 | RT_GROUP_CURSOR | 106 | multi | - | `cursors/groupcursor_106.cur` |
| comctl32 | RT_GROUP_CURSOR | 107 | multi | - | `cursors/groupcursor_107.cur` |
| comctl32 | RT_GROUP_CURSOR | 108 | multi | - | `cursors/groupcursor_108.cur` |
| comctl32 | RT_GROUP_CURSOR | 109 | multi | - | `cursors/groupcursor_109.cur` |
| comctl32 | RT_GROUP_CURSOR | 110 | multi | - | `cursors/groupcursor_110.cur` |
| comctl32 | RT_GROUP_CURSOR | 111 | multi | - | `cursors/groupcursor_111.cur` |
| comctl32 | RT_GROUP_CURSOR | 112 | multi | - | `cursors/groupcursor_112.cur` |
| comctl32 | RT_GROUP_CURSOR | 113 | multi | - | `cursors/groupcursor_113.cur` |
| comctl32 | RT_GROUP_CURSOR | 114 | multi | - | `cursors/groupcursor_114.cur` |
| comctl32 | RT_GROUP_CURSOR | 115 | multi | - | `cursors/groupcursor_115.cur` |
| comctl32 | RT_GROUP_CURSOR | 116 | multi | - | `cursors/groupcursor_116.cur` |
| comctl32 | RT_GROUP_CURSOR | 117 | multi | - | `cursors/groupcursor_117.cur` |
| comctl32 | RT_GROUP_CURSOR | 118 | multi | - | `cursors/groupcursor_118.cur` |
| comctl32 | RT_GROUP_CURSOR | 119 | multi | - | `cursors/groupcursor_119.cur` |
| comctl32 | RT_GROUP_CURSOR | 135 | multi | - | `cursors/groupcursor_135.cur` |
| comctl32 | RT_CURSOR | 1 | 32x32 | 1 | `icons/cursor_102_1_32x32.png` |
| comctl32 | RT_CURSOR | 10 | 32x32 | 1 | `icons/cursor_107_10_32x32.png` |
| comctl32 | RT_CURSOR | 11 | 48x48 | 1 | `icons/cursor_107_11_48x48.png` |
| comctl32 | RT_CURSOR | 12 | 64x64 | 1 | `icons/cursor_107_12_64x64.png` |
| comctl32 | RT_CURSOR | 13 | 80x80 | 1 | `icons/cursor_107_13_80x80.png` |
| comctl32 | RT_CURSOR | 14 | 96x96 | 1 | `icons/cursor_107_14_96x96.png` |
| comctl32 | RT_CURSOR | 15 | 128x128 | 1 | `icons/cursor_107_15_128x128.png` |
| comctl32 | RT_CURSOR | 16 | 32x32 | 1 | `icons/cursor_135_16_32x32.png` |
| comctl32 | RT_CURSOR | 17 | 48x48 | 1 | `icons/cursor_135_17_48x48.png` |
| comctl32 | RT_CURSOR | 18 | 64x64 | 1 | `icons/cursor_135_18_64x64.png` |
| comctl32 | RT_CURSOR | 19 | 80x80 | 1 | `icons/cursor_135_19_80x80.png` |
| comctl32 | RT_CURSOR | 2 | 32x32 | 1 | `icons/cursor_104_2_32x32.png` |
| comctl32 | RT_CURSOR | 20 | 96x96 | 1 | `icons/cursor_135_20_96x96.png` |
| comctl32 | RT_CURSOR | 21 | 128x128 | 1 | `icons/cursor_135_21_128x128.png` |
| comctl32 | RT_CURSOR | 22 | 32x32 | 1 | `icons/cursor_108_22_32x32.png` |
| comctl32 | RT_CURSOR | 24 | 32x32 | 1 | `icons/cursor_109_24_32x32.png` |
| comctl32 | RT_CURSOR | 25 | 32x32 | 1 | `icons/cursor_110_25_32x32.png` |
| comctl32 | RT_CURSOR | 26 | 32x32 | 1 | `icons/cursor_111_26_32x32.png` |
| comctl32 | RT_CURSOR | 27 | 32x32 | 1 | `icons/cursor_112_27_32x32.png` |
| comctl32 | RT_CURSOR | 28 | 32x32 | 1 | `icons/cursor_113_28_32x32.png` |
| comctl32 | RT_CURSOR | 29 | 32x32 | 1 | `icons/cursor_114_29_32x32.png` |
| comctl32 | RT_CURSOR | 3 | 32x32 | 1 | `icons/cursor_105_3_32x32.png` |
| comctl32 | RT_CURSOR | 30 | 32x32 | 1 | `icons/cursor_115_30_32x32.png` |
| comctl32 | RT_CURSOR | 31 | 32x32 | 1 | `icons/cursor_116_31_32x32.png` |
| comctl32 | RT_CURSOR | 32 | 32x32 | 1 | `icons/cursor_117_32_32x32.png` |
| comctl32 | RT_CURSOR | 33 | 32x32 | 1 | `icons/cursor_118_33_32x32.png` |
| comctl32 | RT_CURSOR | 34 | 32x32 | 1 | `icons/cursor_119_34_32x32.png` |
| comctl32 | RT_CURSOR | 4 | 32x32 | 1 | `icons/cursor_106_4_32x32.png` |
| comctl32 | RT_CURSOR | 5 | 48x48 | 1 | `icons/cursor_106_5_48x48.png` |
| comctl32 | RT_CURSOR | 6 | 64x64 | 1 | `icons/cursor_106_6_64x64.png` |
| comctl32 | RT_CURSOR | 7 | 80x80 | 1 | `icons/cursor_106_7_80x80.png` |
| comctl32 | RT_CURSOR | 8 | 96x96 | 1 | `icons/cursor_106_8_96x96.png` |
| comctl32 | RT_CURSOR | 9 | 128x128 | 1 | `icons/cursor_106_9_128x128.png` |
| user32 | RT_BITMAP | 136 | 20x12 | 4 | `bitmaps/bitmap_136.png` |
| user32 | RT_BITMAP | 236 | 25x16 | 4 | `bitmaps/bitmap_236.png` |
| user32 | RT_BITMAP | 32559 | 28x28 | 4 | `bitmaps/bitmap_32559.png` |
| user32 | RT_BITMAP | 32660 | 28x28 | 4 | `bitmaps/bitmap_32660.png` |
| user32 | RT_BITMAP | 32661 | 28x28 | 4 | `bitmaps/bitmap_32661.png` |
| user32 | RT_BITMAP | 32731 | 10x10 | 4 | `bitmaps/bitmap_32731.png` |
| user32 | RT_BITMAP | 36 | 70x42 | 4 | `bitmaps/bitmap_36.png` |
| user32 | RT_GROUP_ICON | 100 | 64x64+32x32+16x16+64x64+48x48+32x32+16x16+64x64+48x48+32x32+16x16 | 4,4,4,8,8,8,8,32,32,32,32 | `icons/groupicon_100.ico` |
| user32 | RT_GROUP_ICON | 101 | 32x32+64x64+32x32+16x16+64x64+48x48+32x32+16x16+64x64+48x48+32x32+16x16 | 1,4,4,4,8,8,8,8,32,32,32,32 | `icons/groupicon_101.ico` |
| user32 | RT_GROUP_ICON | 102 | 32x32+64x64+32x32+64x64+48x48+32x32+16x16+64x64+48x48+32x32+16x16 | 1,4,4,8,8,8,8,32,32,32,32 | `icons/groupicon_102.ico` |
| user32 | RT_GROUP_ICON | 103 | 32x32+64x64+32x32+16x16+64x64+48x48+32x32+16x16+64x64+48x48+32x32+16x16 | 1,4,4,4,8,8,8,8,32,32,32,32 | `icons/groupicon_103.ico` |
| user32 | RT_GROUP_ICON | 104 | 32x32+64x64+32x32+16x16+64x64+48x48+32x32+16x16+64x64+48x48+32x32+16x16 | 1,4,4,4,8,8,8,8,32,32,32,32 | `icons/groupicon_104.ico` |
| user32 | RT_GROUP_ICON | 105 | 64x64+32x32+16x16+64x64+48x48+32x32+16x16+64x64+48x48+32x32+16x16 | 4,4,4,8,8,8,8,32,32,32,32 | `icons/groupicon_105.ico` |
| user32 | RT_GROUP_ICON | 106 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_106.ico` |
| user32 | RT_ICON | 1 (group 105) | 64x64 | 4 | `icons/icon_105_0_64x64.png` |
| user32 | RT_ICON | 10 (group 105) | 32x32 | 32 | `icons/icon_105_9_32x32.png` |
| user32 | RT_ICON | 11 (group 105) | 16x16 | 32 | `icons/icon_105_10_16x16.png` |
| user32 | RT_ICON | 12 (group 101) | 32x32 | 1 | `icons/icon_101_0_32x32.png` |
| user32 | RT_ICON | 13 (group 101) | 64x64 | 4 | `icons/icon_101_1_64x64.png` |
| user32 | RT_ICON | 14 (group 101) | 32x32 | 4 | `icons/icon_101_2_32x32.png` |
| user32 | RT_ICON | 15 (group 101) | 16x16 | 4 | `icons/icon_101_3_16x16.png` |
| user32 | RT_ICON | 16 (group 101) | 64x64 | 8 | `icons/icon_101_4_64x64.png` |
| user32 | RT_ICON | 17 (group 101) | 48x48 | 8 | `icons/icon_101_5_48x48.png` |
| user32 | RT_ICON | 18 (group 101) | 32x32 | 8 | `icons/icon_101_6_32x32.png` |
| user32 | RT_ICON | 19 (group 101) | 16x16 | 8 | `icons/icon_101_7_16x16.png` |
| user32 | RT_ICON | 2 (group 105) | 32x32 | 4 | `icons/icon_105_1_32x32.png` |
| user32 | RT_ICON | 20 (group 101) | 64x64 | 32 | `icons/icon_101_8_64x64.png` |
| user32 | RT_ICON | 21 (group 101) | 48x48 | 32 | `icons/icon_101_9_48x48.png` |
| user32 | RT_ICON | 22 (group 101) | 32x32 | 32 | `icons/icon_101_10_32x32.png` |
| user32 | RT_ICON | 23 (group 101) | 16x16 | 32 | `icons/icon_101_11_16x16.png` |
| user32 | RT_ICON | 24 (group 102) | 32x32 | 1 | `icons/icon_102_0_32x32.png` |
| user32 | RT_ICON | 25 (group 102) | 64x64 | 4 | `icons/icon_102_1_64x64.png` |
| user32 | RT_ICON | 26 (group 102) | 32x32 | 4 | `icons/icon_102_2_32x32.png` |
| user32 | RT_ICON | 27 (group 102) | 64x64 | 8 | `icons/icon_102_3_64x64.png` |
| user32 | RT_ICON | 28 (group 102) | 48x48 | 8 | `icons/icon_102_4_48x48.png` |
| user32 | RT_ICON | 29 (group 102) | 32x32 | 8 | `icons/icon_102_5_32x32.png` |
| user32 | RT_ICON | 3 (group 105) | 16x16 | 4 | `icons/icon_105_2_16x16.png` |
| user32 | RT_ICON | 30 (group 102) | 16x16 | 8 | `icons/icon_102_6_16x16.png` |
| user32 | RT_ICON | 31 (group 102) | 64x64 | 32 | `icons/icon_102_7_64x64.png` |
| user32 | RT_ICON | 32 (group 102) | 48x48 | 32 | `icons/icon_102_8_48x48.png` |
| user32 | RT_ICON | 33 (group 102) | 32x32 | 32 | `icons/icon_102_9_32x32.png` |
| user32 | RT_ICON | 34 (group 102) | 16x16 | 32 | `icons/icon_102_10_16x16.png` |
| user32 | RT_ICON | 35 (group 103) | 32x32 | 1 | `icons/icon_103_0_32x32.png` |
| user32 | RT_ICON | 36 (group 103) | 64x64 | 4 | `icons/icon_103_1_64x64.png` |
| user32 | RT_ICON | 37 (group 103) | 32x32 | 4 | `icons/icon_103_2_32x32.png` |
| user32 | RT_ICON | 38 (group 103) | 16x16 | 4 | `icons/icon_103_3_16x16.png` |
| user32 | RT_ICON | 39 (group 103) | 64x64 | 8 | `icons/icon_103_4_64x64.png` |
| user32 | RT_ICON | 4 (group 105) | 64x64 | 8 | `icons/icon_105_3_64x64.png` |
| user32 | RT_ICON | 40 (group 103) | 48x48 | 8 | `icons/icon_103_5_48x48.png` |
| user32 | RT_ICON | 41 (group 103) | 32x32 | 8 | `icons/icon_103_6_32x32.png` |
| user32 | RT_ICON | 42 (group 103) | 16x16 | 8 | `icons/icon_103_7_16x16.png` |
| user32 | RT_ICON | 43 (group 103) | 64x64 | 32 | `icons/icon_103_8_64x64.png` |
| user32 | RT_ICON | 44 (group 103) | 48x48 | 32 | `icons/icon_103_9_48x48.png` |
| user32 | RT_ICON | 45 (group 103) | 32x32 | 32 | `icons/icon_103_10_32x32.png` |
| user32 | RT_ICON | 46 (group 103) | 16x16 | 32 | `icons/icon_103_11_16x16.png` |
| user32 | RT_ICON | 47 (group 104) | 32x32 | 1 | `icons/icon_104_0_32x32.png` |
| user32 | RT_ICON | 48 (group 104) | 64x64 | 4 | `icons/icon_104_1_64x64.png` |
| user32 | RT_ICON | 49 (group 104) | 32x32 | 4 | `icons/icon_104_2_32x32.png` |
| user32 | RT_ICON | 5 (group 105) | 48x48 | 8 | `icons/icon_105_4_48x48.png` |
| user32 | RT_ICON | 50 (group 104) | 16x16 | 4 | `icons/icon_104_3_16x16.png` |
| user32 | RT_ICON | 51 (group 104) | 64x64 | 8 | `icons/icon_104_4_64x64.png` |
| user32 | RT_ICON | 52 (group 104) | 48x48 | 8 | `icons/icon_104_5_48x48.png` |
| user32 | RT_ICON | 53 (group 104) | 32x32 | 8 | `icons/icon_104_6_32x32.png` |
| user32 | RT_ICON | 54 (group 104) | 16x16 | 8 | `icons/icon_104_7_16x16.png` |
| user32 | RT_ICON | 55 (group 104) | 64x64 | 32 | `icons/icon_104_8_64x64.png` |
| user32 | RT_ICON | 56 (group 104) | 48x48 | 32 | `icons/icon_104_9_48x48.png` |
| user32 | RT_ICON | 57 (group 104) | 32x32 | 32 | `icons/icon_104_10_32x32.png` |
| user32 | RT_ICON | 58 (group 104) | 16x16 | 32 | `icons/icon_104_11_16x16.png` |
| user32 | RT_ICON | 59 (group 100) | 64x64 | 4 | `icons/icon_100_0_64x64.png` |
| user32 | RT_ICON | 6 (group 105) | 32x32 | 8 | `icons/icon_105_5_32x32.png` |
| user32 | RT_ICON | 60 (group 100) | 32x32 | 4 | `icons/icon_100_1_32x32.png` |
| user32 | RT_ICON | 61 (group 100) | 16x16 | 4 | `icons/icon_100_2_16x16.png` |
| user32 | RT_ICON | 62 (group 100) | 64x64 | 8 | `icons/icon_100_3_64x64.png` |
| user32 | RT_ICON | 63 (group 100) | 48x48 | 8 | `icons/icon_100_4_48x48.png` |
| user32 | RT_ICON | 64 (group 100) | 32x32 | 8 | `icons/icon_100_5_32x32.png` |
| user32 | RT_ICON | 65 (group 100) | 16x16 | 8 | `icons/icon_100_6_16x16.png` |
| user32 | RT_ICON | 66 (group 100) | 64x64 | 32 | `icons/icon_100_7_64x64.png` |
| user32 | RT_ICON | 67 (group 100) | 48x48 | 32 | `icons/icon_100_8_48x48.png` |
| user32 | RT_ICON | 68 (group 100) | 32x32 | 32 | `icons/icon_100_9_32x32.png` |
| user32 | RT_ICON | 69 (group 100) | 16x16 | 32 | `icons/icon_100_10_16x16.png` |
| user32 | RT_ICON | 7 (group 105) | 16x16 | 8 | `icons/icon_105_6_16x16.png` |
| user32 | RT_ICON | 70 (group 106) | 256x256 | 32 | `icons/icon_106_0_256x256.png` |
| user32 | RT_ICON | 71 (group 106) | 64x64 | 32 | `icons/icon_106_1_64x64.png` |
| user32 | RT_ICON | 72 (group 106) | 48x48 | 32 | `icons/icon_106_2_48x48.png` |
| user32 | RT_ICON | 73 (group 106) | 40x40 | 32 | `icons/icon_106_3_40x40.png` |
| user32 | RT_ICON | 74 (group 106) | 32x32 | 32 | `icons/icon_106_4_32x32.png` |
| user32 | RT_ICON | 75 (group 106) | 24x24 | 32 | `icons/icon_106_5_24x24.png` |
| user32 | RT_ICON | 76 (group 106) | 20x20 | 32 | `icons/icon_106_6_20x20.png` |
| user32 | RT_ICON | 77 (group 106) | 16x16 | 32 | `icons/icon_106_7_16x16.png` |
| user32 | RT_ICON | 8 (group 105) | 64x64 | 32 | `icons/icon_105_7_64x64.png` |
| user32 | RT_ICON | 9 (group 105) | 48x48 | 32 | `icons/icon_105_8_48x48.png` |
| user32 | RT_GROUP_CURSOR | 100 | multi | - | `cursors/groupcursor_100.cur` |
| user32 | RT_GROUP_CURSOR | 101 | multi | - | `cursors/groupcursor_101.cur` |
| user32 | RT_GROUP_CURSOR | 102 | multi | - | `cursors/groupcursor_102.cur` |
| user32 | RT_GROUP_CURSOR | 103 | multi | - | `cursors/groupcursor_103.cur` |
| user32 | RT_GROUP_CURSOR | 104 | multi | - | `cursors/groupcursor_104.cur` |
| user32 | RT_GROUP_CURSOR | 105 | multi | - | `cursors/groupcursor_105.cur` |
| user32 | RT_GROUP_CURSOR | 106 | multi | - | `cursors/groupcursor_106.cur` |
| user32 | RT_GROUP_CURSOR | 107 | multi | - | `cursors/groupcursor_107.cur` |
| user32 | RT_GROUP_CURSOR | 108 | multi | - | `cursors/groupcursor_108.cur` |
| user32 | RT_GROUP_CURSOR | 109 | multi | - | `cursors/groupcursor_109.cur` |
| user32 | RT_GROUP_CURSOR | 110 | multi | - | `cursors/groupcursor_110.cur` |
| user32 | RT_GROUP_CURSOR | 111 | multi | - | `cursors/groupcursor_111.cur` |
| user32 | RT_GROUP_CURSOR | 112 | multi | - | `cursors/groupcursor_112.cur` |
| user32 | RT_GROUP_CURSOR | 113 | multi | - | `cursors/groupcursor_113.cur` |
| user32 | RT_GROUP_CURSOR | 114 | multi | - | `cursors/groupcursor_114.cur` |
| user32 | RT_GROUP_CURSOR | 115 | multi | - | `cursors/groupcursor_115.cur` |
| user32 | RT_GROUP_CURSOR | 116 | multi | - | `cursors/groupcursor_116.cur` |
| user32 | RT_GROUP_CURSOR | 117 | multi | - | `cursors/groupcursor_117.cur` |
| user32 | RT_GROUP_CURSOR | 118 | multi | - | `cursors/groupcursor_118.cur` |
| user32 | RT_GROUP_CURSOR | 32652 | multi | - | `cursors/groupcursor_32652.cur` |
| user32 | RT_GROUP_CURSOR | 32653 | multi | - | `cursors/groupcursor_32653.cur` |
| user32 | RT_GROUP_CURSOR | 32654 | multi | - | `cursors/groupcursor_32654.cur` |
| user32 | RT_GROUP_CURSOR | 32655 | multi | - | `cursors/groupcursor_32655.cur` |
| user32 | RT_GROUP_CURSOR | 32656 | multi | - | `cursors/groupcursor_32656.cur` |
| user32 | RT_GROUP_CURSOR | 32657 | multi | - | `cursors/groupcursor_32657.cur` |
| user32 | RT_GROUP_CURSOR | 32658 | multi | - | `cursors/groupcursor_32658.cur` |
| user32 | RT_GROUP_CURSOR | 32659 | multi | - | `cursors/groupcursor_32659.cur` |
| user32 | RT_GROUP_CURSOR | 32660 | multi | - | `cursors/groupcursor_32660.cur` |
| user32 | RT_GROUP_CURSOR | 32661 | multi | - | `cursors/groupcursor_32661.cur` |
| user32 | RT_GROUP_CURSOR | 32662 | multi | - | `cursors/groupcursor_32662.cur` |
| user32 | RT_GROUP_CURSOR | 32664 | multi | - | `cursors/groupcursor_32664.cur` |
| user32 | RT_GROUP_CURSOR | 32667 | multi | - | `cursors/groupcursor_32667.cur` |
| user32 | RT_GROUP_CURSOR | 32668 | multi | - | `cursors/groupcursor_32668.cur` |
| user32 | RT_GROUP_CURSOR | 32670 | multi | - | `cursors/groupcursor_32670.cur` |
| user32 | RT_CURSOR | 100 | 48x48 | 1 | `icons/cursor_111_100_48x48.png` |
| user32 | RT_CURSOR | 101 | 32x32 | 1 | `icons/cursor_111_101_32x32.png` |
| user32 | RT_CURSOR | 102 | 48x48 | 1 | `icons/cursor_112_102_48x48.png` |
| user32 | RT_CURSOR | 103 | 32x32 | 1 | `icons/cursor_112_103_32x32.png` |
| user32 | RT_CURSOR | 104 | 48x48 | 1 | `icons/cursor_113_104_48x48.png` |
| user32 | RT_CURSOR | 105 | 32x32 | 1 | `icons/cursor_113_105_32x32.png` |
| user32 | RT_CURSOR | 106 | 48x48 | 1 | `icons/cursor_114_106_48x48.png` |
| user32 | RT_CURSOR | 107 | 32x32 | 1 | `icons/cursor_114_107_32x32.png` |
| user32 | RT_CURSOR | 108 | 48x48 | 1 | `icons/cursor_115_108_48x48.png` |
| user32 | RT_CURSOR | 109 | 32x32 | 1 | `icons/cursor_115_109_32x32.png` |
| user32 | RT_CURSOR | 110 | 48x48 | 4 | `icons/cursor_116_110_48x48.png` |
| user32 | RT_CURSOR | 111 | 32x32 | 4 | `icons/cursor_116_111_32x32.png` |
| user32 | RT_CURSOR | 112 | 32x32 | 32 | `icons/cursor_117_112_32x32.png` |
| user32 | RT_CURSOR | 113 | 48x48 | 32 | `icons/cursor_117_113_48x48.png` |
| user32 | RT_CURSOR | 114 | 64x64 | 32 | `icons/cursor_117_114_64x64.png` |
| user32 | RT_CURSOR | 115 | 96x96 | 32 | `icons/cursor_117_115_96x96.png` |
| user32 | RT_CURSOR | 116 | 128x128 | 32 | `icons/cursor_117_116_128x128.png` |
| user32 | RT_CURSOR | 117 | 32x32 | 32 | `icons/cursor_118_117_32x32.png` |
| user32 | RT_CURSOR | 118 | 48x48 | 32 | `icons/cursor_118_118_48x48.png` |
| user32 | RT_CURSOR | 119 | 64x64 | 32 | `icons/cursor_118_119_64x64.png` |
| user32 | RT_CURSOR | 120 | 96x96 | 32 | `icons/cursor_118_120_96x96.png` |
| user32 | RT_CURSOR | 121 | 128x128 | 32 | `icons/cursor_118_121_128x128.png` |
| user32 | RT_CURSOR | 122 | 48x48 | 1 | `icons/cursor_32652_122_48x48.png` |
| user32 | RT_CURSOR | 123 | 32x32 | 1 | `icons/cursor_32652_123_32x32.png` |
| user32 | RT_CURSOR | 124 | 48x48 | 1 | `icons/cursor_32653_124_48x48.png` |
| user32 | RT_CURSOR | 125 | 32x32 | 1 | `icons/cursor_32653_125_32x32.png` |
| user32 | RT_CURSOR | 126 | 48x48 | 1 | `icons/cursor_32654_126_48x48.png` |
| user32 | RT_CURSOR | 127 | 32x32 | 1 | `icons/cursor_32654_127_32x32.png` |
| user32 | RT_CURSOR | 128 | 48x48 | 1 | `icons/cursor_32655_128_48x48.png` |
| user32 | RT_CURSOR | 129 | 32x32 | 1 | `icons/cursor_32655_129_32x32.png` |
| user32 | RT_CURSOR | 130 | 48x48 | 1 | `icons/cursor_32656_130_48x48.png` |
| user32 | RT_CURSOR | 131 | 32x32 | 1 | `icons/cursor_32656_131_32x32.png` |
| user32 | RT_CURSOR | 132 | 48x48 | 1 | `icons/cursor_32657_132_48x48.png` |
| user32 | RT_CURSOR | 133 | 32x32 | 1 | `icons/cursor_32657_133_32x32.png` |
| user32 | RT_CURSOR | 134 | 48x48 | 1 | `icons/cursor_32658_134_48x48.png` |
| user32 | RT_CURSOR | 135 | 32x32 | 1 | `icons/cursor_32658_135_32x32.png` |
| user32 | RT_CURSOR | 136 | 48x48 | 1 | `icons/cursor_32659_136_48x48.png` |
| user32 | RT_CURSOR | 137 | 32x32 | 1 | `icons/cursor_32659_137_32x32.png` |
| user32 | RT_CURSOR | 138 | 48x48 | 1 | `icons/cursor_32660_138_48x48.png` |
| user32 | RT_CURSOR | 139 | 32x32 | 1 | `icons/cursor_32660_139_32x32.png` |
| user32 | RT_CURSOR | 140 | 48x48 | 1 | `icons/cursor_32661_140_48x48.png` |
| user32 | RT_CURSOR | 141 | 32x32 | 1 | `icons/cursor_32661_141_32x32.png` |
| user32 | RT_CURSOR | 142 | 48x48 | 1 | `icons/cursor_32662_142_48x48.png` |
| user32 | RT_CURSOR | 143 | 32x32 | 1 | `icons/cursor_32662_143_32x32.png` |
| user32 | RT_CURSOR | 144 | 32x32 | 32 | `icons/cursor_32664_144_32x32.png` |
| user32 | RT_CURSOR | 145 | 32x32 | 32 | `icons/cursor_32667_145_32x32.png` |
| user32 | RT_CURSOR | 146 | 32x32 | 32 | `icons/cursor_32668_146_32x32.png` |
| user32 | RT_CURSOR | 147 | 32x32 | 32 | `icons/cursor_32670_147_32x32.png` |
| user32 | RT_CURSOR | 78 | 48x48 | 1 | `icons/cursor_100_78_48x48.png` |
| user32 | RT_CURSOR | 79 | 32x32 | 1 | `icons/cursor_100_79_32x32.png` |
| user32 | RT_CURSOR | 80 | 32x32 | 1 | `icons/cursor_101_80_32x32.png` |
| user32 | RT_CURSOR | 81 | 48x48 | 4 | `icons/cursor_101_81_48x48.png` |
| user32 | RT_CURSOR | 82 | 48x48 | 1 | `icons/cursor_102_82_48x48.png` |
| user32 | RT_CURSOR | 83 | 32x32 | 1 | `icons/cursor_102_83_32x32.png` |
| user32 | RT_CURSOR | 84 | 32x32 | 1 | `icons/cursor_103_84_32x32.png` |
| user32 | RT_CURSOR | 85 | 48x48 | 1 | `icons/cursor_103_85_48x48.png` |
| user32 | RT_CURSOR | 86 | 48x48 | 1 | `icons/cursor_104_86_48x48.png` |
| user32 | RT_CURSOR | 87 | 32x32 | 1 | `icons/cursor_104_87_32x32.png` |
| user32 | RT_CURSOR | 88 | 48x48 | 1 | `icons/cursor_105_88_48x48.png` |
| user32 | RT_CURSOR | 89 | 32x32 | 1 | `icons/cursor_105_89_32x32.png` |
| user32 | RT_CURSOR | 90 | 48x48 | 1 | `icons/cursor_106_90_48x48.png` |
| user32 | RT_CURSOR | 91 | 32x32 | 1 | `icons/cursor_106_91_32x32.png` |
| user32 | RT_CURSOR | 92 | 48x48 | 1 | `icons/cursor_107_92_48x48.png` |
| user32 | RT_CURSOR | 93 | 32x32 | 1 | `icons/cursor_107_93_32x32.png` |
| user32 | RT_CURSOR | 94 | 48x48 | 1 | `icons/cursor_108_94_48x48.png` |
| user32 | RT_CURSOR | 95 | 32x32 | 1 | `icons/cursor_108_95_32x32.png` |
| user32 | RT_CURSOR | 96 | 48x48 | 1 | `icons/cursor_109_96_48x48.png` |
| user32 | RT_CURSOR | 97 | 32x32 | 1 | `icons/cursor_109_97_32x32.png` |
| user32 | RT_CURSOR | 98 | 48x48 | 1 | `icons/cursor_110_98_48x48.png` |
| user32 | RT_CURSOR | 99 | 32x32 | 1 | `icons/cursor_110_99_32x32.png` |
| pifmgr | RT_GROUP_ICON | 1 | 32x32+32x32 | 1,4 | `icons/groupicon_1.ico` |
| pifmgr | RT_GROUP_ICON | 11 | 32x32 | 4 | `icons/groupicon_11.ico` |
| pifmgr | RT_GROUP_ICON | 12 | 32x32 | 4 | `icons/groupicon_12.ico` |
| pifmgr | RT_GROUP_ICON | 13 | 32x32 | 4 | `icons/groupicon_13.ico` |
| pifmgr | RT_GROUP_ICON | 14 | 32x32 | 4 | `icons/groupicon_14.ico` |
| pifmgr | RT_GROUP_ICON | 15 | 32x32 | 4 | `icons/groupicon_15.ico` |
| pifmgr | RT_GROUP_ICON | 16 | 32x32 | 4 | `icons/groupicon_16.ico` |
| pifmgr | RT_GROUP_ICON | 17 | 32x32 | 4 | `icons/groupicon_17.ico` |
| pifmgr | RT_GROUP_ICON | 18 | 32x32 | 4 | `icons/groupicon_18.ico` |
| pifmgr | RT_GROUP_ICON | 19 | 32x32 | 4 | `icons/groupicon_19.ico` |
| pifmgr | RT_GROUP_ICON | 2 | 32x32 | 4 | `icons/groupicon_2.ico` |
| pifmgr | RT_GROUP_ICON | 20 | 32x32 | 4 | `icons/groupicon_20.ico` |
| pifmgr | RT_GROUP_ICON | 21 | 32x32 | 4 | `icons/groupicon_21.ico` |
| pifmgr | RT_GROUP_ICON | 22 | 32x32 | 4 | `icons/groupicon_22.ico` |
| pifmgr | RT_GROUP_ICON | 23 | 32x32 | 4 | `icons/groupicon_23.ico` |
| pifmgr | RT_GROUP_ICON | 24 | 32x32 | 4 | `icons/groupicon_24.ico` |
| pifmgr | RT_GROUP_ICON | 25 | 32x32 | 4 | `icons/groupicon_25.ico` |
| pifmgr | RT_GROUP_ICON | 26 | 32x32 | 4 | `icons/groupicon_26.ico` |
| pifmgr | RT_GROUP_ICON | 27 | 32x32 | 4 | `icons/groupicon_27.ico` |
| pifmgr | RT_GROUP_ICON | 28 | 32x32 | 4 | `icons/groupicon_28.ico` |
| pifmgr | RT_GROUP_ICON | 29 | 32x32 | 4 | `icons/groupicon_29.ico` |
| pifmgr | RT_GROUP_ICON | 3 | 32x32 | 4 | `icons/groupicon_3.ico` |
| pifmgr | RT_GROUP_ICON | 30 | 32x32 | 4 | `icons/groupicon_30.ico` |
| pifmgr | RT_GROUP_ICON | 31 | 32x32 | 4 | `icons/groupicon_31.ico` |
| pifmgr | RT_GROUP_ICON | 32 | 32x32 | 4 | `icons/groupicon_32.ico` |
| pifmgr | RT_GROUP_ICON | 33 | 32x32 | 4 | `icons/groupicon_33.ico` |
| pifmgr | RT_GROUP_ICON | 34 | 32x32 | 4 | `icons/groupicon_34.ico` |
| pifmgr | RT_GROUP_ICON | 35 | 32x32 | 4 | `icons/groupicon_35.ico` |
| pifmgr | RT_GROUP_ICON | 36 | 32x32 | 4 | `icons/groupicon_36.ico` |
| pifmgr | RT_GROUP_ICON | 37 | 32x32 | 4 | `icons/groupicon_37.ico` |
| pifmgr | RT_GROUP_ICON | 38 | 32x32 | 4 | `icons/groupicon_38.ico` |
| pifmgr | RT_GROUP_ICON | 39 | 32x32 | 4 | `icons/groupicon_39.ico` |
| pifmgr | RT_GROUP_ICON | 4 | 32x32 | 4 | `icons/groupicon_4.ico` |
| pifmgr | RT_GROUP_ICON | 5 | 32x32 | 4 | `icons/groupicon_5.ico` |
| pifmgr | RT_GROUP_ICON | 6 | 32x32 | 4 | `icons/groupicon_6.ico` |
| pifmgr | RT_GROUP_ICON | 7 | 32x32 | 4 | `icons/groupicon_7.ico` |
| pifmgr | RT_GROUP_ICON | 8 | 32x32 | 4 | `icons/groupicon_8.ico` |
| pifmgr | RT_GROUP_ICON | 9 | 32x32 | 4 | `icons/groupicon_9.ico` |
| pifmgr | RT_ICON | 1 (group 1) | 32x32 | 1 | `icons/icon_1_0_32x32.png` |
| pifmgr | RT_ICON | 10 (group 9) | 32x32 | 4 | `icons/icon_9_0_32x32.png` |
| pifmgr | RT_ICON | 11 (group 11) | 32x32 | 4 | `icons/icon_11_0_32x32.png` |
| pifmgr | RT_ICON | 12 (group 12) | 32x32 | 4 | `icons/icon_12_0_32x32.png` |
| pifmgr | RT_ICON | 13 (group 13) | 32x32 | 4 | `icons/icon_13_0_32x32.png` |
| pifmgr | RT_ICON | 14 (group 14) | 32x32 | 4 | `icons/icon_14_0_32x32.png` |
| pifmgr | RT_ICON | 15 (group 15) | 32x32 | 4 | `icons/icon_15_0_32x32.png` |
| pifmgr | RT_ICON | 16 (group 16) | 32x32 | 4 | `icons/icon_16_0_32x32.png` |
| pifmgr | RT_ICON | 17 (group 17) | 32x32 | 4 | `icons/icon_17_0_32x32.png` |
| pifmgr | RT_ICON | 18 (group 18) | 32x32 | 4 | `icons/icon_18_0_32x32.png` |
| pifmgr | RT_ICON | 19 (group 19) | 32x32 | 4 | `icons/icon_19_0_32x32.png` |
| pifmgr | RT_ICON | 2 (group 1) | 32x32 | 4 | `icons/icon_1_1_32x32.png` |
| pifmgr | RT_ICON | 20 (group 20) | 32x32 | 4 | `icons/icon_20_0_32x32.png` |
| pifmgr | RT_ICON | 21 (group 21) | 32x32 | 4 | `icons/icon_21_0_32x32.png` |
| pifmgr | RT_ICON | 22 (group 22) | 32x32 | 4 | `icons/icon_22_0_32x32.png` |
| pifmgr | RT_ICON | 23 (group 23) | 32x32 | 4 | `icons/icon_23_0_32x32.png` |
| pifmgr | RT_ICON | 24 (group 24) | 32x32 | 4 | `icons/icon_24_0_32x32.png` |
| pifmgr | RT_ICON | 25 (group 25) | 32x32 | 4 | `icons/icon_25_0_32x32.png` |
| pifmgr | RT_ICON | 26 (group 26) | 32x32 | 4 | `icons/icon_26_0_32x32.png` |
| pifmgr | RT_ICON | 27 (group 27) | 32x32 | 4 | `icons/icon_27_0_32x32.png` |
| pifmgr | RT_ICON | 28 (group 28) | 32x32 | 4 | `icons/icon_28_0_32x32.png` |
| pifmgr | RT_ICON | 29 (group 29) | 32x32 | 4 | `icons/icon_29_0_32x32.png` |
| pifmgr | RT_ICON | 3 (group 2) | 32x32 | 4 | `icons/icon_2_0_32x32.png` |
| pifmgr | RT_ICON | 30 (group 30) | 32x32 | 4 | `icons/icon_30_0_32x32.png` |
| pifmgr | RT_ICON | 31 (group 31) | 32x32 | 4 | `icons/icon_31_0_32x32.png` |
| pifmgr | RT_ICON | 32 (group 32) | 32x32 | 4 | `icons/icon_32_0_32x32.png` |
| pifmgr | RT_ICON | 33 (group 33) | 32x32 | 4 | `icons/icon_33_0_32x32.png` |
| pifmgr | RT_ICON | 34 (group 34) | 32x32 | 4 | `icons/icon_34_0_32x32.png` |
| pifmgr | RT_ICON | 35 (group 35) | 32x32 | 4 | `icons/icon_35_0_32x32.png` |
| pifmgr | RT_ICON | 36 (group 36) | 32x32 | 4 | `icons/icon_36_0_32x32.png` |
| pifmgr | RT_ICON | 37 (group 37) | 32x32 | 4 | `icons/icon_37_0_32x32.png` |
| pifmgr | RT_ICON | 38 (group 38) | 32x32 | 4 | `icons/icon_38_0_32x32.png` |
| pifmgr | RT_ICON | 39 (group 39) | 32x32 | 4 | `icons/icon_39_0_32x32.png` |
| pifmgr | RT_ICON | 4 (group 3) | 32x32 | 4 | `icons/icon_3_0_32x32.png` |
| pifmgr | RT_ICON | 5 (group 4) | 32x32 | 4 | `icons/icon_4_0_32x32.png` |
| pifmgr | RT_ICON | 6 (group 5) | 32x32 | 4 | `icons/icon_5_0_32x32.png` |
| pifmgr | RT_ICON | 7 (group 6) | 32x32 | 4 | `icons/icon_6_0_32x32.png` |
| pifmgr | RT_ICON | 8 (group 7) | 32x32 | 4 | `icons/icon_7_0_32x32.png` |
| pifmgr | RT_ICON | 9 (group 8) | 32x32 | 4 | `icons/icon_8_0_32x32.png` |
| joy_cpl | RT_GROUP_ICON | 102 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_102.ico` |
| joy_cpl | RT_ICON | 1 (group 102) | 256x256 | 32 | `icons/icon_102_0_256x256.png` |
| joy_cpl | RT_ICON | 2 (group 102) | 64x64 | 32 | `icons/icon_102_1_64x64.png` |
| joy_cpl | RT_ICON | 3 (group 102) | 48x48 | 32 | `icons/icon_102_2_48x48.png` |
| joy_cpl | RT_ICON | 4 (group 102) | 40x40 | 32 | `icons/icon_102_3_40x40.png` |
| joy_cpl | RT_ICON | 5 (group 102) | 32x32 | 32 | `icons/icon_102_4_32x32.png` |
| joy_cpl | RT_ICON | 6 (group 102) | 24x24 | 32 | `icons/icon_102_5_24x24.png` |
| joy_cpl | RT_ICON | 7 (group 102) | 20x20 | 32 | `icons/icon_102_6_20x20.png` |
| joy_cpl | RT_ICON | 8 (group 102) | 16x16 | 32 | `icons/icon_102_7_16x16.png` |
| desk_cpl | RT_GROUP_ICON | 100 | 48x48+32x32+16x16+48x48+32x32+16x16+256x256+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32,32 | `icons/groupicon_100.ico` |
| desk_cpl | RT_ICON | 1 (group 100) | 48x48 | 4 | `icons/icon_100_0_48x48.png` |
| desk_cpl | RT_ICON | 10 (group 100) | 16x16 | 32 | `icons/icon_100_9_16x16.png` |
| desk_cpl | RT_ICON | 2 (group 100) | 32x32 | 4 | `icons/icon_100_1_32x32.png` |
| desk_cpl | RT_ICON | 3 (group 100) | 16x16 | 4 | `icons/icon_100_2_16x16.png` |
| desk_cpl | RT_ICON | 4 (group 100) | 48x48 | 8 | `icons/icon_100_3_48x48.png` |
| desk_cpl | RT_ICON | 5 (group 100) | 32x32 | 8 | `icons/icon_100_4_32x32.png` |
| desk_cpl | RT_ICON | 6 (group 100) | 16x16 | 8 | `icons/icon_100_5_16x16.png` |
| desk_cpl | RT_ICON | 7 (group 100) | 256x256 | 32 | `icons/icon_100_6_256x256.png` |
| desk_cpl | RT_ICON | 8 (group 100) | 48x48 | 32 | `icons/icon_100_7_48x48.png` |
| desk_cpl | RT_ICON | 9 (group 100) | 32x32 | 32 | `icons/icon_100_8_32x32.png` |
| powercfg_cpl | RT_GROUP_ICON | 202 | 48x48+32x32+16x16+48x48+32x32+16x16+256x256+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32,32 | `icons/groupicon_202.ico` |
| powercfg_cpl | RT_ICON | 1 (group 202) | 48x48 | 4 | `icons/icon_202_0_48x48.png` |
| powercfg_cpl | RT_ICON | 10 (group 202) | 16x16 | 32 | `icons/icon_202_9_16x16.png` |
| powercfg_cpl | RT_ICON | 2 (group 202) | 32x32 | 4 | `icons/icon_202_1_32x32.png` |
| powercfg_cpl | RT_ICON | 3 (group 202) | 16x16 | 4 | `icons/icon_202_2_16x16.png` |
| powercfg_cpl | RT_ICON | 4 (group 202) | 48x48 | 8 | `icons/icon_202_3_48x48.png` |
| powercfg_cpl | RT_ICON | 5 (group 202) | 32x32 | 8 | `icons/icon_202_4_32x32.png` |
| powercfg_cpl | RT_ICON | 6 (group 202) | 16x16 | 8 | `icons/icon_202_5_16x16.png` |
| powercfg_cpl | RT_ICON | 7 (group 202) | 256x256 | 32 | `icons/icon_202_6_256x256.png` |
| powercfg_cpl | RT_ICON | 8 (group 202) | 48x48 | 32 | `icons/icon_202_7_48x48.png` |
| powercfg_cpl | RT_ICON | 9 (group 202) | 32x32 | 32 | `icons/icon_202_8_32x32.png` |
| oledlg | RT_BITMAP | 10 | 480x30 | 4 | `bitmaps/bitmap_10.png` |
| oledlg | RT_BITMAP | 11 | 480x40 | 4 | `bitmaps/bitmap_11.png` |
| oledlg | RT_BITMAP | 12 | 720x60 | 4 | `bitmaps/bitmap_12.png` |
| comctl32_winsxs_v5 | RT_BITMAP | 120 | 240x16 | 4 | `bitmaps/bitmap_120.png` |
| comctl32_winsxs_v5 | RT_BITMAP | 121 | 360x24 | 4 | `bitmaps/bitmap_121.png` |
| comctl32_winsxs_v5 | RT_BITMAP | 124 | 192x16 | 4 | `bitmaps/bitmap_124.png` |
| comctl32_winsxs_v5 | RT_BITMAP | 125 | 288x24 | 4 | `bitmaps/bitmap_125.png` |
| comctl32_winsxs_v5 | RT_BITMAP | 130 | 80x16 | 4 | `bitmaps/bitmap_130.png` |
| comctl32_winsxs_v5 | RT_BITMAP | 131 | 120x24 | 4 | `bitmaps/bitmap_131.png` |
| comctl32_winsxs_v5 | RT_BITMAP | 132 | 28x28 | 4 | `bitmaps/bitmap_132.png` |
| comctl32_winsxs_v5 | RT_BITMAP | 133 | 28x28 | 4 | `bitmaps/bitmap_133.png` |
| comctl32_winsxs_v5 | RT_BITMAP | 134 | 28x28 | 4 | `bitmaps/bitmap_134.png` |
| comctl32_winsxs_v5 | RT_BITMAP | 140 | 13x12 | 4 | `bitmaps/bitmap_140.png` |
| comctl32_winsxs_v5 | RT_GROUP_ICON | 150 | 32x32 | 1 | `icons/groupicon_150.ico` |
| comctl32_winsxs_v5 | RT_GROUP_ICON | 20480 | 32x32+32x32+16x16 | 4,1,4 | `icons/groupicon_20480.ico` |
| comctl32_winsxs_v5 | RT_GROUP_ICON | 20481 | 32x32+32x32+16x16 | 4,1,4 | `icons/groupicon_20481.ico` |
| comctl32_winsxs_v5 | RT_GROUP_ICON | 20482 | 32x32+32x32+16x16 | 4,1,4 | `icons/groupicon_20482.ico` |
| comctl32_winsxs_v5 | RT_ICON | 23 (group 150) | 32x32 | 1 | `icons/icon_150_0_32x32.png` |
| comctl32_winsxs_v5 | RT_ICON | 35 (group 20481) | 32x32 | 4 | `icons/icon_20481_0_32x32.png` |
| comctl32_winsxs_v5 | RT_ICON | 36 (group 20481) | 32x32 | 1 | `icons/icon_20481_1_32x32.png` |
| comctl32_winsxs_v5 | RT_ICON | 37 (group 20481) | 16x16 | 4 | `icons/icon_20481_2_16x16.png` |
| comctl32_winsxs_v5 | RT_ICON | 38 (group 20482) | 32x32 | 4 | `icons/icon_20482_0_32x32.png` |
| comctl32_winsxs_v5 | RT_ICON | 39 (group 20482) | 32x32 | 1 | `icons/icon_20482_1_32x32.png` |
| comctl32_winsxs_v5 | RT_ICON | 40 (group 20482) | 16x16 | 4 | `icons/icon_20482_2_16x16.png` |
| comctl32_winsxs_v5 | RT_ICON | 41 (group 20480) | 32x32 | 4 | `icons/icon_20480_0_32x32.png` |
| comctl32_winsxs_v5 | RT_ICON | 42 (group 20480) | 32x32 | 1 | `icons/icon_20480_1_32x32.png` |
| comctl32_winsxs_v5 | RT_ICON | 43 (group 20480) | 16x16 | 4 | `icons/icon_20480_2_16x16.png` |
| comctl32_winsxs_v5 | RT_GROUP_CURSOR | 102 | multi | - | `cursors/groupcursor_102.cur` |
| comctl32_winsxs_v5 | RT_GROUP_CURSOR | 104 | multi | - | `cursors/groupcursor_104.cur` |
| comctl32_winsxs_v5 | RT_GROUP_CURSOR | 105 | multi | - | `cursors/groupcursor_105.cur` |
| comctl32_winsxs_v5 | RT_GROUP_CURSOR | 106 | multi | - | `cursors/groupcursor_106.cur` |
| comctl32_winsxs_v5 | RT_GROUP_CURSOR | 107 | multi | - | `cursors/groupcursor_107.cur` |
| comctl32_winsxs_v5 | RT_GROUP_CURSOR | 108 | multi | - | `cursors/groupcursor_108.cur` |
| comctl32_winsxs_v5 | RT_GROUP_CURSOR | 109 | multi | - | `cursors/groupcursor_109.cur` |
| comctl32_winsxs_v5 | RT_GROUP_CURSOR | 110 | multi | - | `cursors/groupcursor_110.cur` |
| comctl32_winsxs_v5 | RT_GROUP_CURSOR | 111 | multi | - | `cursors/groupcursor_111.cur` |
| comctl32_winsxs_v5 | RT_GROUP_CURSOR | 112 | multi | - | `cursors/groupcursor_112.cur` |
| comctl32_winsxs_v5 | RT_GROUP_CURSOR | 113 | multi | - | `cursors/groupcursor_113.cur` |
| comctl32_winsxs_v5 | RT_GROUP_CURSOR | 114 | multi | - | `cursors/groupcursor_114.cur` |
| comctl32_winsxs_v5 | RT_GROUP_CURSOR | 115 | multi | - | `cursors/groupcursor_115.cur` |
| comctl32_winsxs_v5 | RT_GROUP_CURSOR | 116 | multi | - | `cursors/groupcursor_116.cur` |
| comctl32_winsxs_v5 | RT_GROUP_CURSOR | 117 | multi | - | `cursors/groupcursor_117.cur` |
| comctl32_winsxs_v5 | RT_GROUP_CURSOR | 118 | multi | - | `cursors/groupcursor_118.cur` |
| comctl32_winsxs_v5 | RT_GROUP_CURSOR | 119 | multi | - | `cursors/groupcursor_119.cur` |
| comctl32_winsxs_v5 | RT_GROUP_CURSOR | 135 | multi | - | `cursors/groupcursor_135.cur` |
| comctl32_winsxs_v5 | RT_CURSOR | 1 | 32x32 | 1 | `icons/cursor_102_1_32x32.png` |
| comctl32_winsxs_v5 | RT_CURSOR | 10 | 32x32 | 1 | `icons/cursor_107_10_32x32.png` |
| comctl32_winsxs_v5 | RT_CURSOR | 11 | 48x48 | 1 | `icons/cursor_107_11_48x48.png` |
| comctl32_winsxs_v5 | RT_CURSOR | 12 | 64x64 | 1 | `icons/cursor_107_12_64x64.png` |
| comctl32_winsxs_v5 | RT_CURSOR | 13 | 80x80 | 1 | `icons/cursor_107_13_80x80.png` |
| comctl32_winsxs_v5 | RT_CURSOR | 14 | 96x96 | 1 | `icons/cursor_107_14_96x96.png` |
| comctl32_winsxs_v5 | RT_CURSOR | 15 | 128x128 | 1 | `icons/cursor_107_15_128x128.png` |
| comctl32_winsxs_v5 | RT_CURSOR | 16 | 32x32 | 1 | `icons/cursor_135_16_32x32.png` |
| comctl32_winsxs_v5 | RT_CURSOR | 17 | 48x48 | 1 | `icons/cursor_135_17_48x48.png` |
| comctl32_winsxs_v5 | RT_CURSOR | 18 | 64x64 | 1 | `icons/cursor_135_18_64x64.png` |
| comctl32_winsxs_v5 | RT_CURSOR | 19 | 80x80 | 1 | `icons/cursor_135_19_80x80.png` |
| comctl32_winsxs_v5 | RT_CURSOR | 2 | 32x32 | 1 | `icons/cursor_104_2_32x32.png` |
| comctl32_winsxs_v5 | RT_CURSOR | 20 | 96x96 | 1 | `icons/cursor_135_20_96x96.png` |
| comctl32_winsxs_v5 | RT_CURSOR | 21 | 128x128 | 1 | `icons/cursor_135_21_128x128.png` |
| comctl32_winsxs_v5 | RT_CURSOR | 22 | 32x32 | 1 | `icons/cursor_108_22_32x32.png` |
| comctl32_winsxs_v5 | RT_CURSOR | 24 | 32x32 | 1 | `icons/cursor_109_24_32x32.png` |
| comctl32_winsxs_v5 | RT_CURSOR | 25 | 32x32 | 1 | `icons/cursor_110_25_32x32.png` |
| comctl32_winsxs_v5 | RT_CURSOR | 26 | 32x32 | 1 | `icons/cursor_111_26_32x32.png` |
| comctl32_winsxs_v5 | RT_CURSOR | 27 | 32x32 | 1 | `icons/cursor_112_27_32x32.png` |
| comctl32_winsxs_v5 | RT_CURSOR | 28 | 32x32 | 1 | `icons/cursor_113_28_32x32.png` |
| comctl32_winsxs_v5 | RT_CURSOR | 29 | 32x32 | 1 | `icons/cursor_114_29_32x32.png` |
| comctl32_winsxs_v5 | RT_CURSOR | 3 | 32x32 | 1 | `icons/cursor_105_3_32x32.png` |
| comctl32_winsxs_v5 | RT_CURSOR | 30 | 32x32 | 1 | `icons/cursor_115_30_32x32.png` |
| comctl32_winsxs_v5 | RT_CURSOR | 31 | 32x32 | 1 | `icons/cursor_116_31_32x32.png` |
| comctl32_winsxs_v5 | RT_CURSOR | 32 | 32x32 | 1 | `icons/cursor_117_32_32x32.png` |
| comctl32_winsxs_v5 | RT_CURSOR | 33 | 32x32 | 1 | `icons/cursor_118_33_32x32.png` |
| comctl32_winsxs_v5 | RT_CURSOR | 34 | 32x32 | 1 | `icons/cursor_119_34_32x32.png` |
| comctl32_winsxs_v5 | RT_CURSOR | 4 | 32x32 | 1 | `icons/cursor_106_4_32x32.png` |
| comctl32_winsxs_v5 | RT_CURSOR | 5 | 48x48 | 1 | `icons/cursor_106_5_48x48.png` |
| comctl32_winsxs_v5 | RT_CURSOR | 6 | 64x64 | 1 | `icons/cursor_106_6_64x64.png` |
| comctl32_winsxs_v5 | RT_CURSOR | 7 | 80x80 | 1 | `icons/cursor_106_7_80x80.png` |
| comctl32_winsxs_v5 | RT_CURSOR | 8 | 96x96 | 1 | `icons/cursor_106_8_96x96.png` |
| comctl32_winsxs_v5 | RT_CURSOR | 9 | 128x128 | 1 | `icons/cursor_106_9_128x128.png` |
| comctl32_winsxs_v6 | RT_BITMAP | 120 | 240x16 | 4 | `bitmaps/bitmap_120.png` |
| comctl32_winsxs_v6 | RT_BITMAP | 121 | 360x24 | 4 | `bitmaps/bitmap_121.png` |
| comctl32_winsxs_v6 | RT_BITMAP | 124 | 192x16 | 32 | `bitmaps/bitmap_124.png` |
| comctl32_winsxs_v6 | RT_BITMAP | 125 | 288x24 | 32 | `bitmaps/bitmap_125.png` |
| comctl32_winsxs_v6 | RT_BITMAP | 130 | 80x16 | 32 | `bitmaps/bitmap_130.png` |
| comctl32_winsxs_v6 | RT_BITMAP | 131 | 120x24 | 32 | `bitmaps/bitmap_131.png` |
| comctl32_winsxs_v6 | RT_BITMAP | 132 | 28x28 | 4 | `bitmaps/bitmap_132.png` |
| comctl32_winsxs_v6 | RT_BITMAP | 133 | 28x28 | 4 | `bitmaps/bitmap_133.png` |
| comctl32_winsxs_v6 | RT_BITMAP | 134 | 28x28 | 4 | `bitmaps/bitmap_134.png` |
| comctl32_winsxs_v6 | RT_BITMAP | 140 | 13x12 | 4 | `bitmaps/bitmap_140.png` |
| comctl32_winsxs_v6 | RT_BITMAP | 141 | 13x12 | 8 | `bitmaps/bitmap_141.png` |
| comctl32_winsxs_v6 | RT_BITMAP | 142 | 85x17 | 32 | `bitmaps/bitmap_142.png` |
| comctl32_winsxs_v6 | RT_BITMAP | 143 | 30x30 | 32 | `bitmaps/bitmap_143.png` |
| comctl32_winsxs_v6 | RT_BITMAP | 144 | 30x30 | 32 | `bitmaps/bitmap_144.png` |
| comctl32_winsxs_v6 | RT_BITMAP | 145 | 30x30 | 32 | `bitmaps/bitmap_145.png` |
| comctl32_winsxs_v6 | RT_BITMAP | 146 | 30x30 | 32 | `bitmaps/bitmap_146.png` |
| comctl32_winsxs_v6 | RT_BITMAP | 147 | 18x21 | 32 | `bitmaps/bitmap_147.png` |
| comctl32_winsxs_v6 | RT_BITMAP | 148 | 18x21 | 32 | `bitmaps/bitmap_148.png` |
| comctl32_winsxs_v6 | RT_BITMAP | 151 | 150x30 | 32 | `bitmaps/bitmap_151.png` |
| comctl32_winsxs_v6 | RT_BITMAP | 152 | 150x30 | 32 | `bitmaps/bitmap_152.png` |
| comctl32_winsxs_v6 | RT_BITMAP | 153 | 150x30 | 32 | `bitmaps/bitmap_153.png` |
| comctl32_winsxs_v6 | RT_BITMAP | 154 | 150x30 | 32 | `bitmaps/bitmap_154.png` |
| comctl32_winsxs_v6 | RT_GROUP_ICON | 150 | 32x32 | 1 | `icons/groupicon_150.ico` |
| comctl32_winsxs_v6 | RT_GROUP_ICON | 20480 | 32x32+32x32+16x16+48x48+32x32+16x16+48x48+32x32+16x16 | 4,1,4,8,8,8,32,32,32 | `icons/groupicon_20480.ico` |
| comctl32_winsxs_v6 | RT_GROUP_ICON | 20481 | 32x32+32x32+16x16+48x48+32x32+16x16+48x48+32x32+16x16 | 4,1,4,8,8,8,32,32,32 | `icons/groupicon_20481.ico` |
| comctl32_winsxs_v6 | RT_GROUP_ICON | 20482 | 32x32+32x32+16x16+48x48+32x32+16x16+48x48+32x32+16x16 | 4,1,4,8,8,8,32,32,32 | `icons/groupicon_20482.ico` |
| comctl32_winsxs_v6 | RT_ICON | 23 (group 150) | 32x32 | 1 | `icons/icon_150_0_32x32.png` |
| comctl32_winsxs_v6 | RT_ICON | 35 (group 20481) | 32x32 | 4 | `icons/icon_20481_0_32x32.png` |
| comctl32_winsxs_v6 | RT_ICON | 36 (group 20481) | 32x32 | 1 | `icons/icon_20481_1_32x32.png` |
| comctl32_winsxs_v6 | RT_ICON | 37 (group 20481) | 16x16 | 4 | `icons/icon_20481_2_16x16.png` |
| comctl32_winsxs_v6 | RT_ICON | 38 (group 20481) | 48x48 | 8 | `icons/icon_20481_3_48x48.png` |
| comctl32_winsxs_v6 | RT_ICON | 39 (group 20481) | 32x32 | 8 | `icons/icon_20481_4_32x32.png` |
| comctl32_winsxs_v6 | RT_ICON | 40 (group 20481) | 16x16 | 8 | `icons/icon_20481_5_16x16.png` |
| comctl32_winsxs_v6 | RT_ICON | 41 (group 20481) | 48x48 | 32 | `icons/icon_20481_6_48x48.png` |
| comctl32_winsxs_v6 | RT_ICON | 42 (group 20481) | 32x32 | 32 | `icons/icon_20481_7_32x32.png` |
| comctl32_winsxs_v6 | RT_ICON | 43 (group 20481) | 16x16 | 32 | `icons/icon_20481_8_16x16.png` |
| comctl32_winsxs_v6 | RT_ICON | 44 (group 20482) | 32x32 | 4 | `icons/icon_20482_0_32x32.png` |
| comctl32_winsxs_v6 | RT_ICON | 45 (group 20482) | 32x32 | 1 | `icons/icon_20482_1_32x32.png` |
| comctl32_winsxs_v6 | RT_ICON | 46 (group 20482) | 16x16 | 4 | `icons/icon_20482_2_16x16.png` |
| comctl32_winsxs_v6 | RT_ICON | 47 (group 20482) | 48x48 | 8 | `icons/icon_20482_3_48x48.png` |
| comctl32_winsxs_v6 | RT_ICON | 48 (group 20482) | 32x32 | 8 | `icons/icon_20482_4_32x32.png` |
| comctl32_winsxs_v6 | RT_ICON | 49 (group 20482) | 16x16 | 8 | `icons/icon_20482_5_16x16.png` |
| comctl32_winsxs_v6 | RT_ICON | 50 (group 20482) | 48x48 | 32 | `icons/icon_20482_6_48x48.png` |
| comctl32_winsxs_v6 | RT_ICON | 51 (group 20482) | 32x32 | 32 | `icons/icon_20482_7_32x32.png` |
| comctl32_winsxs_v6 | RT_ICON | 52 (group 20482) | 16x16 | 32 | `icons/icon_20482_8_16x16.png` |
| comctl32_winsxs_v6 | RT_ICON | 53 (group 20480) | 32x32 | 4 | `icons/icon_20480_0_32x32.png` |
| comctl32_winsxs_v6 | RT_ICON | 54 (group 20480) | 32x32 | 1 | `icons/icon_20480_1_32x32.png` |
| comctl32_winsxs_v6 | RT_ICON | 55 (group 20480) | 16x16 | 4 | `icons/icon_20480_2_16x16.png` |
| comctl32_winsxs_v6 | RT_ICON | 56 (group 20480) | 48x48 | 8 | `icons/icon_20480_3_48x48.png` |
| comctl32_winsxs_v6 | RT_ICON | 57 (group 20480) | 32x32 | 8 | `icons/icon_20480_4_32x32.png` |
| comctl32_winsxs_v6 | RT_ICON | 58 (group 20480) | 16x16 | 8 | `icons/icon_20480_5_16x16.png` |
| comctl32_winsxs_v6 | RT_ICON | 59 (group 20480) | 48x48 | 32 | `icons/icon_20480_6_48x48.png` |
| comctl32_winsxs_v6 | RT_ICON | 60 (group 20480) | 32x32 | 32 | `icons/icon_20480_7_32x32.png` |
| comctl32_winsxs_v6 | RT_ICON | 61 (group 20480) | 16x16 | 32 | `icons/icon_20480_8_16x16.png` |
| comctl32_winsxs_v6 | RT_GROUP_CURSOR | 102 | multi | - | `cursors/groupcursor_102.cur` |
| comctl32_winsxs_v6 | RT_GROUP_CURSOR | 104 | multi | - | `cursors/groupcursor_104.cur` |
| comctl32_winsxs_v6 | RT_GROUP_CURSOR | 105 | multi | - | `cursors/groupcursor_105.cur` |
| comctl32_winsxs_v6 | RT_GROUP_CURSOR | 106 | multi | - | `cursors/groupcursor_106.cur` |
| comctl32_winsxs_v6 | RT_GROUP_CURSOR | 107 | multi | - | `cursors/groupcursor_107.cur` |
| comctl32_winsxs_v6 | RT_GROUP_CURSOR | 108 | multi | - | `cursors/groupcursor_108.cur` |
| comctl32_winsxs_v6 | RT_GROUP_CURSOR | 109 | multi | - | `cursors/groupcursor_109.cur` |
| comctl32_winsxs_v6 | RT_GROUP_CURSOR | 110 | multi | - | `cursors/groupcursor_110.cur` |
| comctl32_winsxs_v6 | RT_GROUP_CURSOR | 111 | multi | - | `cursors/groupcursor_111.cur` |
| comctl32_winsxs_v6 | RT_GROUP_CURSOR | 112 | multi | - | `cursors/groupcursor_112.cur` |
| comctl32_winsxs_v6 | RT_GROUP_CURSOR | 113 | multi | - | `cursors/groupcursor_113.cur` |
| comctl32_winsxs_v6 | RT_GROUP_CURSOR | 114 | multi | - | `cursors/groupcursor_114.cur` |
| comctl32_winsxs_v6 | RT_GROUP_CURSOR | 115 | multi | - | `cursors/groupcursor_115.cur` |
| comctl32_winsxs_v6 | RT_GROUP_CURSOR | 116 | multi | - | `cursors/groupcursor_116.cur` |
| comctl32_winsxs_v6 | RT_GROUP_CURSOR | 117 | multi | - | `cursors/groupcursor_117.cur` |
| comctl32_winsxs_v6 | RT_GROUP_CURSOR | 118 | multi | - | `cursors/groupcursor_118.cur` |
| comctl32_winsxs_v6 | RT_GROUP_CURSOR | 119 | multi | - | `cursors/groupcursor_119.cur` |
| comctl32_winsxs_v6 | RT_GROUP_CURSOR | 135 | multi | - | `cursors/groupcursor_135.cur` |
| comctl32_winsxs_v6 | RT_CURSOR | 1 | 32x32 | 1 | `icons/cursor_102_1_32x32.png` |
| comctl32_winsxs_v6 | RT_CURSOR | 10 | 32x32 | 1 | `icons/cursor_107_10_32x32.png` |
| comctl32_winsxs_v6 | RT_CURSOR | 11 | 48x48 | 1 | `icons/cursor_107_11_48x48.png` |
| comctl32_winsxs_v6 | RT_CURSOR | 12 | 64x64 | 1 | `icons/cursor_107_12_64x64.png` |
| comctl32_winsxs_v6 | RT_CURSOR | 13 | 80x80 | 1 | `icons/cursor_107_13_80x80.png` |
| comctl32_winsxs_v6 | RT_CURSOR | 14 | 96x96 | 1 | `icons/cursor_107_14_96x96.png` |
| comctl32_winsxs_v6 | RT_CURSOR | 15 | 128x128 | 1 | `icons/cursor_107_15_128x128.png` |
| comctl32_winsxs_v6 | RT_CURSOR | 16 | 32x32 | 1 | `icons/cursor_135_16_32x32.png` |
| comctl32_winsxs_v6 | RT_CURSOR | 17 | 48x48 | 1 | `icons/cursor_135_17_48x48.png` |
| comctl32_winsxs_v6 | RT_CURSOR | 18 | 64x64 | 1 | `icons/cursor_135_18_64x64.png` |
| comctl32_winsxs_v6 | RT_CURSOR | 19 | 80x80 | 1 | `icons/cursor_135_19_80x80.png` |
| comctl32_winsxs_v6 | RT_CURSOR | 2 | 32x32 | 1 | `icons/cursor_104_2_32x32.png` |
| comctl32_winsxs_v6 | RT_CURSOR | 20 | 96x96 | 1 | `icons/cursor_135_20_96x96.png` |
| comctl32_winsxs_v6 | RT_CURSOR | 21 | 128x128 | 1 | `icons/cursor_135_21_128x128.png` |
| comctl32_winsxs_v6 | RT_CURSOR | 22 | 32x32 | 1 | `icons/cursor_108_22_32x32.png` |
| comctl32_winsxs_v6 | RT_CURSOR | 24 | 32x32 | 1 | `icons/cursor_109_24_32x32.png` |
| comctl32_winsxs_v6 | RT_CURSOR | 25 | 32x32 | 1 | `icons/cursor_110_25_32x32.png` |
| comctl32_winsxs_v6 | RT_CURSOR | 26 | 32x32 | 1 | `icons/cursor_111_26_32x32.png` |
| comctl32_winsxs_v6 | RT_CURSOR | 27 | 32x32 | 1 | `icons/cursor_112_27_32x32.png` |
| comctl32_winsxs_v6 | RT_CURSOR | 28 | 32x32 | 1 | `icons/cursor_113_28_32x32.png` |
| comctl32_winsxs_v6 | RT_CURSOR | 29 | 32x32 | 1 | `icons/cursor_114_29_32x32.png` |
| comctl32_winsxs_v6 | RT_CURSOR | 3 | 32x32 | 1 | `icons/cursor_105_3_32x32.png` |
| comctl32_winsxs_v6 | RT_CURSOR | 30 | 32x32 | 1 | `icons/cursor_115_30_32x32.png` |
| comctl32_winsxs_v6 | RT_CURSOR | 31 | 32x32 | 1 | `icons/cursor_116_31_32x32.png` |
| comctl32_winsxs_v6 | RT_CURSOR | 32 | 32x32 | 1 | `icons/cursor_117_32_32x32.png` |
| comctl32_winsxs_v6 | RT_CURSOR | 33 | 32x32 | 1 | `icons/cursor_118_33_32x32.png` |
| comctl32_winsxs_v6 | RT_CURSOR | 34 | 32x32 | 1 | `icons/cursor_119_34_32x32.png` |
| comctl32_winsxs_v6 | RT_CURSOR | 4 | 32x32 | 1 | `icons/cursor_106_4_32x32.png` |
| comctl32_winsxs_v6 | RT_CURSOR | 5 | 48x48 | 1 | `icons/cursor_106_5_48x48.png` |
| comctl32_winsxs_v6 | RT_CURSOR | 6 | 64x64 | 1 | `icons/cursor_106_6_64x64.png` |
| comctl32_winsxs_v6 | RT_CURSOR | 7 | 80x80 | 1 | `icons/cursor_106_7_80x80.png` |
| comctl32_winsxs_v6 | RT_CURSOR | 8 | 96x96 | 1 | `icons/cursor_106_8_96x96.png` |
| comctl32_winsxs_v6 | RT_CURSOR | 9 | 128x128 | 1 | `icons/cursor_106_9_128x128.png` |
| hdwwiz_cpl | RT_BITMAP | 106 | 164x628 | 24 | `bitmaps/bitmap_106.png` |
| hdwwiz_cpl | RT_BITMAP | 107 | 49x49 | 4 | `bitmaps/bitmap_107.png` |
| hdwwiz_cpl | RT_GROUP_ICON | 100 | 32x32+16x16 | 32,32 | `icons/groupicon_100.ico` |
| hdwwiz_cpl | RT_GROUP_ICON | 101 | 32x32+16x16 | 4,4 | `icons/groupicon_101.ico` |
| hdwwiz_cpl | RT_GROUP_ICON | 102 | 32x32+16x16 | 32,32 | `icons/groupicon_102.ico` |
| hdwwiz_cpl | RT_ICON | 1 (group 100) | 32x32 | 32 | `icons/icon_100_0_32x32.png` |
| hdwwiz_cpl | RT_ICON | 2 (group 100) | 16x16 | 32 | `icons/icon_100_1_16x16.png` |
| hdwwiz_cpl | RT_ICON | 3 (group 101) | 32x32 | 4 | `icons/icon_101_0_32x32.png` |
| hdwwiz_cpl | RT_ICON | 4 (group 101) | 16x16 | 4 | `icons/icon_101_1_16x16.png` |
| hdwwiz_cpl | RT_ICON | 5 (group 102) | 32x32 | 32 | `icons/icon_102_0_32x32.png` |
| hdwwiz_cpl | RT_ICON | 6 (group 102) | 16x16 | 32 | `icons/icon_102_1_16x16.png` |
| ncpa_cpl | RT_GROUP_ICON | 1001 | 48x48+32x32+16x16+48x48+32x32+16x16+256x256+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32,32 | `icons/groupicon_1001.ico` |
| ncpa_cpl | RT_ICON | 1 (group 1001) | 48x48 | 4 | `icons/icon_1001_0_48x48.png` |
| ncpa_cpl | RT_ICON | 10 (group 1001) | 16x16 | 32 | `icons/icon_1001_9_16x16.png` |
| ncpa_cpl | RT_ICON | 2 (group 1001) | 32x32 | 4 | `icons/icon_1001_1_32x32.png` |
| ncpa_cpl | RT_ICON | 3 (group 1001) | 16x16 | 4 | `icons/icon_1001_2_16x16.png` |
| ncpa_cpl | RT_ICON | 4 (group 1001) | 48x48 | 8 | `icons/icon_1001_3_48x48.png` |
| ncpa_cpl | RT_ICON | 5 (group 1001) | 32x32 | 8 | `icons/icon_1001_4_32x32.png` |
| ncpa_cpl | RT_ICON | 6 (group 1001) | 16x16 | 8 | `icons/icon_1001_5_16x16.png` |
| ncpa_cpl | RT_ICON | 7 (group 1001) | 256x256 | 32 | `icons/icon_1001_6_256x256.png` |
| ncpa_cpl | RT_ICON | 8 (group 1001) | 48x48 | 32 | `icons/icon_1001_7_48x48.png` |
| ncpa_cpl | RT_ICON | 9 (group 1001) | 32x32 | 32 | `icons/icon_1001_8_32x32.png` |
| telephon_cpl | RT_GROUP_ICON | 100 | 48x48+32x32+24x24+16x16+48x48+32x32+24x24+16x16+256x256+48x48+32x32+24x24+16x16 | 4,4,4,4,8,8,8,8,32,32,32,32,32 | `icons/groupicon_100.ico` |
| telephon_cpl | RT_ICON | 1 (group 100) | 48x48 | 4 | `icons/icon_100_0_48x48.png` |
| telephon_cpl | RT_ICON | 10 (group 100) | 48x48 | 32 | `icons/icon_100_9_48x48.png` |
| telephon_cpl | RT_ICON | 11 (group 100) | 32x32 | 32 | `icons/icon_100_10_32x32.png` |
| telephon_cpl | RT_ICON | 12 (group 100) | 24x24 | 32 | `icons/icon_100_11_24x24.png` |
| telephon_cpl | RT_ICON | 13 (group 100) | 16x16 | 32 | `icons/icon_100_12_16x16.png` |
| telephon_cpl | RT_ICON | 2 (group 100) | 32x32 | 4 | `icons/icon_100_1_32x32.png` |
| telephon_cpl | RT_ICON | 3 (group 100) | 24x24 | 4 | `icons/icon_100_2_24x24.png` |
| telephon_cpl | RT_ICON | 4 (group 100) | 16x16 | 4 | `icons/icon_100_3_16x16.png` |
| telephon_cpl | RT_ICON | 5 (group 100) | 48x48 | 8 | `icons/icon_100_4_48x48.png` |
| telephon_cpl | RT_ICON | 6 (group 100) | 32x32 | 8 | `icons/icon_100_5_32x32.png` |
| telephon_cpl | RT_ICON | 7 (group 100) | 24x24 | 8 | `icons/icon_100_6_24x24.png` |
| telephon_cpl | RT_ICON | 8 (group 100) | 16x16 | 8 | `icons/icon_100_7_16x16.png` |
| telephon_cpl | RT_ICON | 9 (group 100) | 256x256 | 32 | `icons/icon_100_8_256x256.png` |
| calc_exe | RT_GROUP_ICON | IDI_CALC_ICON | 16x16+256x256+32x32+48x48 | 32,32,32,32 | `icons/groupicon_IDI_CALC_ICON.ico` |
| calc_exe | RT_ICON | 1 (group IDI_CALC_ICON) | 16x16 | 32 | `icons/icon_IDI_CALC_ICON_0_16x16.png` |
| calc_exe | RT_ICON | 2 (group IDI_CALC_ICON) | 256x256 | 32 | `icons/icon_IDI_CALC_ICON_1_256x256.png` |
| calc_exe | RT_ICON | 3 (group IDI_CALC_ICON) | 32x32 | 32 | `icons/icon_IDI_CALC_ICON_2_32x32.png` |
| calc_exe | RT_ICON | 4 (group IDI_CALC_ICON) | 48x48 | 32 | `icons/icon_IDI_CALC_ICON_3_48x48.png` |
| charmap_exe | RT_BITMAP | 110 | 100x24 | 4 | `bitmaps/bitmap_110.png` |
| charmap_exe | RT_GROUP_ICON | 111 | 48x48+32x32+24x24+16x16+48x48+32x32+24x24+16x16+256x256+48x48+32x32+24x24+16x16 | 4,4,4,4,8,8,8,8,32,32,32,32,32 | `icons/groupicon_111.ico` |
| charmap_exe | RT_ICON | 1 (group 111) | 48x48 | 4 | `icons/icon_111_0_48x48.png` |
| charmap_exe | RT_ICON | 10 (group 111) | 48x48 | 32 | `icons/icon_111_9_48x48.png` |
| charmap_exe | RT_ICON | 11 (group 111) | 32x32 | 32 | `icons/icon_111_10_32x32.png` |
| charmap_exe | RT_ICON | 12 (group 111) | 24x24 | 32 | `icons/icon_111_11_24x24.png` |
| charmap_exe | RT_ICON | 13 (group 111) | 16x16 | 32 | `icons/icon_111_12_16x16.png` |
| charmap_exe | RT_ICON | 2 (group 111) | 32x32 | 4 | `icons/icon_111_1_32x32.png` |
| charmap_exe | RT_ICON | 3 (group 111) | 24x24 | 4 | `icons/icon_111_2_24x24.png` |
| charmap_exe | RT_ICON | 4 (group 111) | 16x16 | 4 | `icons/icon_111_3_16x16.png` |
| charmap_exe | RT_ICON | 5 (group 111) | 48x48 | 8 | `icons/icon_111_4_48x48.png` |
| charmap_exe | RT_ICON | 6 (group 111) | 32x32 | 8 | `icons/icon_111_5_32x32.png` |
| charmap_exe | RT_ICON | 7 (group 111) | 24x24 | 8 | `icons/icon_111_6_24x24.png` |
| charmap_exe | RT_ICON | 8 (group 111) | 16x16 | 8 | `icons/icon_111_7_16x16.png` |
| charmap_exe | RT_ICON | 9 (group 111) | 256x256 | 32 | `icons/icon_111_8_256x256.png` |
| notepad_exe | RT_GROUP_ICON | 2 | 48x48+32x32+24x24+16x16+48x48+32x32+24x24+16x16+256x256+48x48+32x32+24x24+16x16 | 4,4,4,4,8,8,8,8,32,32,32,32,32 | `icons/groupicon_2.ico` |
| notepad_exe | RT_GROUP_ICON | 300 | 26x24+21x20+17x16 | 32,32,32 | `icons/groupicon_300.ico` |
| notepad_exe | RT_GROUP_ICON | 301 | 26x24+21x20+17x16 | 32,32,32 | `icons/groupicon_301.ico` |
| notepad_exe | RT_GROUP_ICON | 302 | 26x24+21x20+17x16 | 32,32,32 | `icons/groupicon_302.ico` |
| notepad_exe | RT_ICON | 1 (group 2) | 48x48 | 4 | `icons/icon_2_0_48x48.png` |
| notepad_exe | RT_ICON | 10 (group 2) | 48x48 | 32 | `icons/icon_2_9_48x48.png` |
| notepad_exe | RT_ICON | 11 (group 2) | 32x32 | 32 | `icons/icon_2_10_32x32.png` |
| notepad_exe | RT_ICON | 12 (group 2) | 24x24 | 32 | `icons/icon_2_11_24x24.png` |
| notepad_exe | RT_ICON | 13 (group 2) | 16x16 | 32 | `icons/icon_2_12_16x16.png` |
| notepad_exe | RT_ICON | 14 (group 300) | 26x24 | 32 | `icons/icon_300_0_26x24.png` |
| notepad_exe | RT_ICON | 15 (group 300) | 21x20 | 32 | `icons/icon_300_1_21x20.png` |
| notepad_exe | RT_ICON | 16 (group 300) | 17x16 | 32 | `icons/icon_300_2_17x16.png` |
| notepad_exe | RT_ICON | 17 (group 301) | 26x24 | 32 | `icons/icon_301_0_26x24.png` |
| notepad_exe | RT_ICON | 18 (group 301) | 21x20 | 32 | `icons/icon_301_1_21x20.png` |
| notepad_exe | RT_ICON | 19 (group 301) | 17x16 | 32 | `icons/icon_301_2_17x16.png` |
| notepad_exe | RT_ICON | 2 (group 2) | 32x32 | 4 | `icons/icon_2_1_32x32.png` |
| notepad_exe | RT_ICON | 20 (group 302) | 26x24 | 32 | `icons/icon_302_0_26x24.png` |
| notepad_exe | RT_ICON | 21 (group 302) | 21x20 | 32 | `icons/icon_302_1_21x20.png` |
| notepad_exe | RT_ICON | 22 (group 302) | 17x16 | 32 | `icons/icon_302_2_17x16.png` |
| notepad_exe | RT_ICON | 3 (group 2) | 24x24 | 4 | `icons/icon_2_2_24x24.png` |
| notepad_exe | RT_ICON | 4 (group 2) | 16x16 | 4 | `icons/icon_2_3_16x16.png` |
| notepad_exe | RT_ICON | 5 (group 2) | 48x48 | 8 | `icons/icon_2_4_48x48.png` |
| notepad_exe | RT_ICON | 6 (group 2) | 32x32 | 8 | `icons/icon_2_5_32x32.png` |
| notepad_exe | RT_ICON | 7 (group 2) | 24x24 | 8 | `icons/icon_2_6_24x24.png` |
| notepad_exe | RT_ICON | 8 (group 2) | 16x16 | 8 | `icons/icon_2_7_16x16.png` |
| notepad_exe | RT_ICON | 9 (group 2) | 256x256 | 32 | `icons/icon_2_8_256x256.png` |
| sndvol_exe | RT_GROUP_ICON | 101 | 48x48+32x32+16x16+48x48+32x32+16x16+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_101.ico` |
| sndvol_exe | RT_GROUP_ICON | 110 | 32x32+24x24+20x20+16x16+32x32+24x24+20x20+16x16+32x32+24x24+20x20+16x16 | 4,4,4,4,8,8,8,8,32,32,32,32 | `icons/groupicon_110.ico` |
| sndvol_exe | RT_GROUP_ICON | 111 | 32x32+24x24+20x20+16x16+32x32+24x24+20x20+16x16+32x32+24x24+20x20+16x16 | 4,4,4,4,8,8,8,8,32,32,32,32 | `icons/groupicon_111.ico` |
| sndvol_exe | RT_ICON | 1 (group 101) | 48x48 | 4 | `icons/icon_101_0_48x48.png` |
| sndvol_exe | RT_ICON | 10 (group 110) | 32x32 | 4 | `icons/icon_110_0_32x32.png` |
| sndvol_exe | RT_ICON | 11 (group 110) | 24x24 | 4 | `icons/icon_110_1_24x24.png` |
| sndvol_exe | RT_ICON | 12 (group 110) | 20x20 | 4 | `icons/icon_110_2_20x20.png` |
| sndvol_exe | RT_ICON | 13 (group 110) | 16x16 | 4 | `icons/icon_110_3_16x16.png` |
| sndvol_exe | RT_ICON | 14 (group 110) | 32x32 | 8 | `icons/icon_110_4_32x32.png` |
| sndvol_exe | RT_ICON | 15 (group 110) | 24x24 | 8 | `icons/icon_110_5_24x24.png` |
| sndvol_exe | RT_ICON | 16 (group 110) | 20x20 | 8 | `icons/icon_110_6_20x20.png` |
| sndvol_exe | RT_ICON | 17 (group 110) | 16x16 | 8 | `icons/icon_110_7_16x16.png` |
| sndvol_exe | RT_ICON | 18 (group 110) | 32x32 | 32 | `icons/icon_110_8_32x32.png` |
| sndvol_exe | RT_ICON | 19 (group 110) | 24x24 | 32 | `icons/icon_110_9_24x24.png` |
| sndvol_exe | RT_ICON | 2 (group 101) | 32x32 | 4 | `icons/icon_101_1_32x32.png` |
| sndvol_exe | RT_ICON | 20 (group 110) | 20x20 | 32 | `icons/icon_110_10_20x20.png` |
| sndvol_exe | RT_ICON | 21 (group 110) | 16x16 | 32 | `icons/icon_110_11_16x16.png` |
| sndvol_exe | RT_ICON | 22 (group 111) | 32x32 | 4 | `icons/icon_111_0_32x32.png` |
| sndvol_exe | RT_ICON | 23 (group 111) | 24x24 | 4 | `icons/icon_111_1_24x24.png` |
| sndvol_exe | RT_ICON | 24 (group 111) | 20x20 | 4 | `icons/icon_111_2_20x20.png` |
| sndvol_exe | RT_ICON | 25 (group 111) | 16x16 | 4 | `icons/icon_111_3_16x16.png` |
| sndvol_exe | RT_ICON | 26 (group 111) | 32x32 | 8 | `icons/icon_111_4_32x32.png` |
| sndvol_exe | RT_ICON | 27 (group 111) | 24x24 | 8 | `icons/icon_111_5_24x24.png` |
| sndvol_exe | RT_ICON | 28 (group 111) | 20x20 | 8 | `icons/icon_111_6_20x20.png` |
| sndvol_exe | RT_ICON | 29 (group 111) | 16x16 | 8 | `icons/icon_111_7_16x16.png` |
| sndvol_exe | RT_ICON | 3 (group 101) | 16x16 | 4 | `icons/icon_101_2_16x16.png` |
| sndvol_exe | RT_ICON | 30 (group 111) | 32x32 | 32 | `icons/icon_111_8_32x32.png` |
| sndvol_exe | RT_ICON | 31 (group 111) | 24x24 | 32 | `icons/icon_111_9_24x24.png` |
| sndvol_exe | RT_ICON | 32 (group 111) | 20x20 | 32 | `icons/icon_111_10_20x20.png` |
| sndvol_exe | RT_ICON | 33 (group 111) | 16x16 | 32 | `icons/icon_111_11_16x16.png` |
| sndvol_exe | RT_ICON | 4 (group 101) | 48x48 | 8 | `icons/icon_101_3_48x48.png` |
| sndvol_exe | RT_ICON | 5 (group 101) | 32x32 | 8 | `icons/icon_101_4_32x32.png` |
| sndvol_exe | RT_ICON | 6 (group 101) | 16x16 | 8 | `icons/icon_101_5_16x16.png` |
| sndvol_exe | RT_ICON | 7 (group 101) | 48x48 | 32 | `icons/icon_101_6_48x48.png` |
| sndvol_exe | RT_ICON | 8 (group 101) | 32x32 | 32 | `icons/icon_101_7_32x32.png` |
| sndvol_exe | RT_ICON | 9 (group 101) | 16x16 | 32 | `icons/icon_101_8_16x16.png` |
| moricons_mun | RT_GROUP_ICON | 1 | 32x32+32x32 | 1,4 | `icons/groupicon_1.ico` |
| moricons_mun | RT_GROUP_ICON | 10 | 32x32+32x32 | 1,4 | `icons/groupicon_10.ico` |
| moricons_mun | RT_GROUP_ICON | 100 | 32x32+32x32 | 4,1 | `icons/groupicon_100.ico` |
| moricons_mun | RT_GROUP_ICON | 101 | 32x32+32x32 | 4,1 | `icons/groupicon_101.ico` |
| moricons_mun | RT_GROUP_ICON | 102 | 32x32+32x32 | 1,4 | `icons/groupicon_102.ico` |
| moricons_mun | RT_GROUP_ICON | 103 | 32x32+32x32 | 1,4 | `icons/groupicon_103.ico` |
| moricons_mun | RT_GROUP_ICON | 104 | 32x32+32x32 | 1,4 | `icons/groupicon_104.ico` |
| moricons_mun | RT_GROUP_ICON | 105 | 32x32+32x32 | 1,4 | `icons/groupicon_105.ico` |
| moricons_mun | RT_GROUP_ICON | 106 | 32x32+32x32 | 1,4 | `icons/groupicon_106.ico` |
| moricons_mun | RT_GROUP_ICON | 107 | 32x32+32x32 | 1,4 | `icons/groupicon_107.ico` |
| moricons_mun | RT_GROUP_ICON | 108 | 48x48+32x32+16x16+48x48+32x32+16x16+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_108.ico` |
| moricons_mun | RT_GROUP_ICON | 109 | 32x32+16x16+48x48 | 4,4,8 | `icons/groupicon_109.ico` |
| moricons_mun | RT_GROUP_ICON | 11 | 32x32+32x32 | 4,1 | `icons/groupicon_11.ico` |
| moricons_mun | RT_GROUP_ICON | 110 | 32x32+16x16+48x48 | 4,4,8 | `icons/groupicon_110.ico` |
| moricons_mun | RT_GROUP_ICON | 111 | 32x32+16x16+48x48 | 4,4,8 | `icons/groupicon_111.ico` |
| moricons_mun | RT_GROUP_ICON | 112 | 32x32+16x16+48x48 | 4,4,8 | `icons/groupicon_112.ico` |
| moricons_mun | RT_GROUP_ICON | 113 | 32x32+16x16+48x48 | 4,4,8 | `icons/groupicon_113.ico` |
| moricons_mun | RT_GROUP_ICON | 12 | 32x32+32x32 | 1,4 | `icons/groupicon_12.ico` |
| moricons_mun | RT_GROUP_ICON | 13 | 32x32+32x32 | 4,1 | `icons/groupicon_13.ico` |
| moricons_mun | RT_GROUP_ICON | 14 | 32x32+32x32 | 1,4 | `icons/groupicon_14.ico` |
| moricons_mun | RT_GROUP_ICON | 15 | 32x32+32x32 | 4,1 | `icons/groupicon_15.ico` |
| moricons_mun | RT_GROUP_ICON | 16 | 32x32+32x32 | 1,4 | `icons/groupicon_16.ico` |
| moricons_mun | RT_GROUP_ICON | 17 | 32x32+32x32 | 4,1 | `icons/groupicon_17.ico` |
| moricons_mun | RT_GROUP_ICON | 18 | 32x32+32x32 | 1,4 | `icons/groupicon_18.ico` |
| moricons_mun | RT_GROUP_ICON | 19 | 32x32+32x32 | 1,4 | `icons/groupicon_19.ico` |
| moricons_mun | RT_GROUP_ICON | 2 | 32x32+32x32 | 1,4 | `icons/groupicon_2.ico` |
| moricons_mun | RT_GROUP_ICON | 20 | 32x32+32x32 | 1,4 | `icons/groupicon_20.ico` |
| moricons_mun | RT_GROUP_ICON | 21 | 32x32+32x32 | 4,1 | `icons/groupicon_21.ico` |
| moricons_mun | RT_GROUP_ICON | 22 | 32x32+32x32 | 1,4 | `icons/groupicon_22.ico` |
| moricons_mun | RT_GROUP_ICON | 23 | 32x32+32x32 | 4,1 | `icons/groupicon_23.ico` |
| moricons_mun | RT_GROUP_ICON | 24 | 32x32+32x32 | 1,4 | `icons/groupicon_24.ico` |
| moricons_mun | RT_GROUP_ICON | 25 | 32x32+32x32 | 1,4 | `icons/groupicon_25.ico` |
| moricons_mun | RT_GROUP_ICON | 26 | 32x32+32x32 | 4,1 | `icons/groupicon_26.ico` |
| moricons_mun | RT_GROUP_ICON | 27 | 32x32+32x32 | 4,1 | `icons/groupicon_27.ico` |
| moricons_mun | RT_GROUP_ICON | 28 | 32x32+32x32 | 4,1 | `icons/groupicon_28.ico` |
| moricons_mun | RT_GROUP_ICON | 29 | 32x32+32x32 | 4,1 | `icons/groupicon_29.ico` |
| moricons_mun | RT_GROUP_ICON | 3 | 32x32+32x32 | 1,4 | `icons/groupicon_3.ico` |
| moricons_mun | RT_GROUP_ICON | 30 | 32x32+32x32 | 4,1 | `icons/groupicon_30.ico` |
| moricons_mun | RT_GROUP_ICON | 31 | 32x32+32x32 | 1,4 | `icons/groupicon_31.ico` |
| moricons_mun | RT_GROUP_ICON | 32 | 32x32+32x32 | 1,4 | `icons/groupicon_32.ico` |
| moricons_mun | RT_GROUP_ICON | 33 | 32x32+32x32 | 1,4 | `icons/groupicon_33.ico` |
| moricons_mun | RT_GROUP_ICON | 34 | 32x32+32x32 | 1,4 | `icons/groupicon_34.ico` |
| moricons_mun | RT_GROUP_ICON | 35 | 32x32+32x32 | 1,4 | `icons/groupicon_35.ico` |
| moricons_mun | RT_GROUP_ICON | 36 | 32x32+32x32 | 1,4 | `icons/groupicon_36.ico` |
| moricons_mun | RT_GROUP_ICON | 37 | 32x32+32x32 | 1,4 | `icons/groupicon_37.ico` |
| moricons_mun | RT_GROUP_ICON | 38 | 32x32+32x32 | 1,4 | `icons/groupicon_38.ico` |
| moricons_mun | RT_GROUP_ICON | 39 | 32x32+32x32 | 1,4 | `icons/groupicon_39.ico` |
| moricons_mun | RT_GROUP_ICON | 4 | 32x32+32x32 | 1,4 | `icons/groupicon_4.ico` |
| moricons_mun | RT_GROUP_ICON | 40 | 32x32+32x32 | 1,4 | `icons/groupicon_40.ico` |
| moricons_mun | RT_GROUP_ICON | 41 | 32x32+32x32 | 1,4 | `icons/groupicon_41.ico` |
| moricons_mun | RT_GROUP_ICON | 42 | 32x32+32x32 | 1,4 | `icons/groupicon_42.ico` |
| moricons_mun | RT_GROUP_ICON | 43 | 32x32+32x32 | 1,4 | `icons/groupicon_43.ico` |
| moricons_mun | RT_GROUP_ICON | 44 | 32x32+32x32 | 1,4 | `icons/groupicon_44.ico` |
| moricons_mun | RT_GROUP_ICON | 45 | 32x32+32x32 | 1,4 | `icons/groupicon_45.ico` |
| moricons_mun | RT_GROUP_ICON | 46 | 32x32+32x32 | 1,4 | `icons/groupicon_46.ico` |
| moricons_mun | RT_GROUP_ICON | 47 | 32x32+32x32 | 1,4 | `icons/groupicon_47.ico` |
| moricons_mun | RT_GROUP_ICON | 48 | 32x32+32x32 | 1,4 | `icons/groupicon_48.ico` |
| moricons_mun | RT_GROUP_ICON | 49 | 32x32+32x32 | 4,1 | `icons/groupicon_49.ico` |
| moricons_mun | RT_GROUP_ICON | 5 | 32x32+32x32 | 1,4 | `icons/groupicon_5.ico` |
| moricons_mun | RT_GROUP_ICON | 50 | 32x32+32x32 | 1,4 | `icons/groupicon_50.ico` |
| moricons_mun | RT_GROUP_ICON | 51 | 32x32+32x32 | 1,4 | `icons/groupicon_51.ico` |
| moricons_mun | RT_GROUP_ICON | 52 | 32x32+32x32 | 1,4 | `icons/groupicon_52.ico` |
| moricons_mun | RT_GROUP_ICON | 53 | 32x32+32x32 | 1,4 | `icons/groupicon_53.ico` |
| moricons_mun | RT_GROUP_ICON | 54 | 32x32+32x32 | 1,4 | `icons/groupicon_54.ico` |
| moricons_mun | RT_GROUP_ICON | 55 | 32x32+32x32 | 1,4 | `icons/groupicon_55.ico` |
| moricons_mun | RT_GROUP_ICON | 56 | 32x32+32x32 | 1,4 | `icons/groupicon_56.ico` |
| moricons_mun | RT_GROUP_ICON | 57 | 32x32+32x32 | 1,4 | `icons/groupicon_57.ico` |
| moricons_mun | RT_GROUP_ICON | 58 | 32x32+32x32 | 1,4 | `icons/groupicon_58.ico` |
| moricons_mun | RT_GROUP_ICON | 59 | 32x32+32x32 | 1,4 | `icons/groupicon_59.ico` |
| moricons_mun | RT_GROUP_ICON | 6 | 32x32+32x32 | 4,1 | `icons/groupicon_6.ico` |
| moricons_mun | RT_GROUP_ICON | 60 | 32x32+32x32 | 1,4 | `icons/groupicon_60.ico` |
| moricons_mun | RT_GROUP_ICON | 61 | 32x32+32x32 | 1,4 | `icons/groupicon_61.ico` |
| moricons_mun | RT_GROUP_ICON | 62 | 32x32+32x32 | 1,4 | `icons/groupicon_62.ico` |
| moricons_mun | RT_GROUP_ICON | 63 | 32x32+32x32 | 1,4 | `icons/groupicon_63.ico` |
| moricons_mun | RT_GROUP_ICON | 64 | 32x32+32x32 | 1,4 | `icons/groupicon_64.ico` |
| moricons_mun | RT_GROUP_ICON | 65 | 32x32+32x32 | 1,4 | `icons/groupicon_65.ico` |
| moricons_mun | RT_GROUP_ICON | 66 | 32x32+32x32 | 1,4 | `icons/groupicon_66.ico` |
| moricons_mun | RT_GROUP_ICON | 67 | 32x32+32x32 | 4,1 | `icons/groupicon_67.ico` |
| moricons_mun | RT_GROUP_ICON | 68 | 32x32+32x32 | 4,1 | `icons/groupicon_68.ico` |
| moricons_mun | RT_GROUP_ICON | 69 | 32x32+32x32 | 4,1 | `icons/groupicon_69.ico` |
| moricons_mun | RT_GROUP_ICON | 7 | 32x32+32x32 | 1,4 | `icons/groupicon_7.ico` |
| moricons_mun | RT_GROUP_ICON | 70 | 32x32+32x32 | 1,4 | `icons/groupicon_70.ico` |
| moricons_mun | RT_GROUP_ICON | 71 | 32x32+32x32 | 1,4 | `icons/groupicon_71.ico` |
| moricons_mun | RT_GROUP_ICON | 72 | 32x32+32x32 | 1,4 | `icons/groupicon_72.ico` |
| moricons_mun | RT_GROUP_ICON | 73 | 32x32+32x32 | 1,4 | `icons/groupicon_73.ico` |
| moricons_mun | RT_GROUP_ICON | 74 | 32x32+32x32 | 1,4 | `icons/groupicon_74.ico` |
| moricons_mun | RT_GROUP_ICON | 75 | 32x32+32x32 | 1,4 | `icons/groupicon_75.ico` |
| moricons_mun | RT_GROUP_ICON | 76 | 32x32+32x32 | 1,4 | `icons/groupicon_76.ico` |
| moricons_mun | RT_GROUP_ICON | 77 | 32x32+32x32 | 1,4 | `icons/groupicon_77.ico` |
| moricons_mun | RT_GROUP_ICON | 78 | 32x32+32x32 | 1,4 | `icons/groupicon_78.ico` |
| moricons_mun | RT_GROUP_ICON | 79 | 32x32+32x32 | 1,4 | `icons/groupicon_79.ico` |
| moricons_mun | RT_GROUP_ICON | 8 | 32x32+32x32 | 1,4 | `icons/groupicon_8.ico` |
| moricons_mun | RT_GROUP_ICON | 80 | 32x32+32x32 | 1,4 | `icons/groupicon_80.ico` |
| moricons_mun | RT_GROUP_ICON | 81 | 32x32+32x32 | 1,4 | `icons/groupicon_81.ico` |
| moricons_mun | RT_GROUP_ICON | 82 | 32x32+32x32 | 1,4 | `icons/groupicon_82.ico` |
| moricons_mun | RT_GROUP_ICON | 83 | 32x32+32x32 | 1,4 | `icons/groupicon_83.ico` |
| moricons_mun | RT_GROUP_ICON | 84 | 32x32+32x32 | 1,4 | `icons/groupicon_84.ico` |
| moricons_mun | RT_GROUP_ICON | 85 | 32x32+32x32 | 1,4 | `icons/groupicon_85.ico` |
| moricons_mun | RT_GROUP_ICON | 86 | 32x32+32x32 | 1,4 | `icons/groupicon_86.ico` |
| moricons_mun | RT_GROUP_ICON | 87 | 32x32+32x32 | 1,4 | `icons/groupicon_87.ico` |
| moricons_mun | RT_GROUP_ICON | 88 | 32x32+32x32 | 1,4 | `icons/groupicon_88.ico` |
| moricons_mun | RT_GROUP_ICON | 89 | 32x32+32x32 | 1,4 | `icons/groupicon_89.ico` |
| moricons_mun | RT_GROUP_ICON | 9 | 32x32+32x32 | 1,4 | `icons/groupicon_9.ico` |
| moricons_mun | RT_GROUP_ICON | 90 | 32x32+32x32 | 1,4 | `icons/groupicon_90.ico` |
| moricons_mun | RT_GROUP_ICON | 91 | 32x32+32x32 | 1,4 | `icons/groupicon_91.ico` |
| moricons_mun | RT_GROUP_ICON | 92 | 32x32+32x32 | 1,4 | `icons/groupicon_92.ico` |
| moricons_mun | RT_GROUP_ICON | 93 | 32x32+32x32 | 1,4 | `icons/groupicon_93.ico` |
| moricons_mun | RT_GROUP_ICON | 94 | 32x32+32x32 | 1,4 | `icons/groupicon_94.ico` |
| moricons_mun | RT_GROUP_ICON | 95 | 32x32+32x32 | 1,4 | `icons/groupicon_95.ico` |
| moricons_mun | RT_GROUP_ICON | 96 | 32x32 | 4 | `icons/groupicon_96.ico` |
| moricons_mun | RT_GROUP_ICON | 97 | 32x32+32x32 | 1,4 | `icons/groupicon_97.ico` |
| moricons_mun | RT_GROUP_ICON | 98 | 32x32+32x32 | 1,4 | `icons/groupicon_98.ico` |
| moricons_mun | RT_GROUP_ICON | 99 | 32x32+32x32 | 1,4 | `icons/groupicon_99.ico` |
| moricons_mun | RT_ICON | 1 (group 1) | 32x32 | 1 | `icons/icon_1_0_32x32.png` |
| moricons_mun | RT_ICON | 10 (group 5) | 32x32 | 4 | `icons/icon_5_1_32x32.png` |
| moricons_mun | RT_ICON | 100 (group 50) | 32x32 | 4 | `icons/icon_50_1_32x32.png` |
| moricons_mun | RT_ICON | 101 (group 51) | 32x32 | 1 | `icons/icon_51_0_32x32.png` |
| moricons_mun | RT_ICON | 102 (group 51) | 32x32 | 4 | `icons/icon_51_1_32x32.png` |
| moricons_mun | RT_ICON | 103 (group 52) | 32x32 | 1 | `icons/icon_52_0_32x32.png` |
| moricons_mun | RT_ICON | 104 (group 52) | 32x32 | 4 | `icons/icon_52_1_32x32.png` |
| moricons_mun | RT_ICON | 105 (group 53) | 32x32 | 1 | `icons/icon_53_0_32x32.png` |
| moricons_mun | RT_ICON | 106 (group 53) | 32x32 | 4 | `icons/icon_53_1_32x32.png` |
| moricons_mun | RT_ICON | 107 (group 54) | 32x32 | 1 | `icons/icon_54_0_32x32.png` |
| moricons_mun | RT_ICON | 108 (group 54) | 32x32 | 4 | `icons/icon_54_1_32x32.png` |
| moricons_mun | RT_ICON | 109 (group 55) | 32x32 | 1 | `icons/icon_55_0_32x32.png` |
| moricons_mun | RT_ICON | 11 (group 6) | 32x32 | 4 | `icons/icon_6_0_32x32.png` |
| moricons_mun | RT_ICON | 110 (group 55) | 32x32 | 4 | `icons/icon_55_1_32x32.png` |
| moricons_mun | RT_ICON | 111 (group 56) | 32x32 | 1 | `icons/icon_56_0_32x32.png` |
| moricons_mun | RT_ICON | 112 (group 56) | 32x32 | 4 | `icons/icon_56_1_32x32.png` |
| moricons_mun | RT_ICON | 113 (group 57) | 32x32 | 1 | `icons/icon_57_0_32x32.png` |
| moricons_mun | RT_ICON | 114 (group 57) | 32x32 | 4 | `icons/icon_57_1_32x32.png` |
| moricons_mun | RT_ICON | 115 (group 58) | 32x32 | 1 | `icons/icon_58_0_32x32.png` |
| moricons_mun | RT_ICON | 116 (group 58) | 32x32 | 4 | `icons/icon_58_1_32x32.png` |
| moricons_mun | RT_ICON | 117 (group 59) | 32x32 | 1 | `icons/icon_59_0_32x32.png` |
| moricons_mun | RT_ICON | 118 (group 59) | 32x32 | 4 | `icons/icon_59_1_32x32.png` |
| moricons_mun | RT_ICON | 119 (group 60) | 32x32 | 1 | `icons/icon_60_0_32x32.png` |
| moricons_mun | RT_ICON | 12 (group 6) | 32x32 | 1 | `icons/icon_6_1_32x32.png` |
| moricons_mun | RT_ICON | 120 (group 60) | 32x32 | 4 | `icons/icon_60_1_32x32.png` |
| moricons_mun | RT_ICON | 121 (group 61) | 32x32 | 1 | `icons/icon_61_0_32x32.png` |
| moricons_mun | RT_ICON | 122 (group 61) | 32x32 | 4 | `icons/icon_61_1_32x32.png` |
| moricons_mun | RT_ICON | 123 (group 62) | 32x32 | 1 | `icons/icon_62_0_32x32.png` |
| moricons_mun | RT_ICON | 124 (group 62) | 32x32 | 4 | `icons/icon_62_1_32x32.png` |
| moricons_mun | RT_ICON | 125 (group 63) | 32x32 | 1 | `icons/icon_63_0_32x32.png` |
| moricons_mun | RT_ICON | 126 (group 63) | 32x32 | 4 | `icons/icon_63_1_32x32.png` |
| moricons_mun | RT_ICON | 127 (group 64) | 32x32 | 1 | `icons/icon_64_0_32x32.png` |
| moricons_mun | RT_ICON | 128 (group 64) | 32x32 | 4 | `icons/icon_64_1_32x32.png` |
| moricons_mun | RT_ICON | 129 (group 65) | 32x32 | 1 | `icons/icon_65_0_32x32.png` |
| moricons_mun | RT_ICON | 13 (group 7) | 32x32 | 1 | `icons/icon_7_0_32x32.png` |
| moricons_mun | RT_ICON | 130 (group 65) | 32x32 | 4 | `icons/icon_65_1_32x32.png` |
| moricons_mun | RT_ICON | 131 (group 66) | 32x32 | 1 | `icons/icon_66_0_32x32.png` |
| moricons_mun | RT_ICON | 132 (group 66) | 32x32 | 4 | `icons/icon_66_1_32x32.png` |
| moricons_mun | RT_ICON | 133 (group 67) | 32x32 | 4 | `icons/icon_67_0_32x32.png` |
| moricons_mun | RT_ICON | 134 (group 67) | 32x32 | 1 | `icons/icon_67_1_32x32.png` |
| moricons_mun | RT_ICON | 135 (group 68) | 32x32 | 4 | `icons/icon_68_0_32x32.png` |
| moricons_mun | RT_ICON | 136 (group 68) | 32x32 | 1 | `icons/icon_68_1_32x32.png` |
| moricons_mun | RT_ICON | 137 (group 69) | 32x32 | 4 | `icons/icon_69_0_32x32.png` |
| moricons_mun | RT_ICON | 138 (group 69) | 32x32 | 1 | `icons/icon_69_1_32x32.png` |
| moricons_mun | RT_ICON | 139 (group 70) | 32x32 | 1 | `icons/icon_70_0_32x32.png` |
| moricons_mun | RT_ICON | 14 (group 7) | 32x32 | 4 | `icons/icon_7_1_32x32.png` |
| moricons_mun | RT_ICON | 140 (group 70) | 32x32 | 4 | `icons/icon_70_1_32x32.png` |
| moricons_mun | RT_ICON | 141 (group 71) | 32x32 | 1 | `icons/icon_71_0_32x32.png` |
| moricons_mun | RT_ICON | 142 (group 71) | 32x32 | 4 | `icons/icon_71_1_32x32.png` |
| moricons_mun | RT_ICON | 143 (group 72) | 32x32 | 1 | `icons/icon_72_0_32x32.png` |
| moricons_mun | RT_ICON | 144 (group 72) | 32x32 | 4 | `icons/icon_72_1_32x32.png` |
| moricons_mun | RT_ICON | 145 (group 73) | 32x32 | 1 | `icons/icon_73_0_32x32.png` |
| moricons_mun | RT_ICON | 146 (group 73) | 32x32 | 4 | `icons/icon_73_1_32x32.png` |
| moricons_mun | RT_ICON | 147 (group 74) | 32x32 | 1 | `icons/icon_74_0_32x32.png` |
| moricons_mun | RT_ICON | 148 (group 74) | 32x32 | 4 | `icons/icon_74_1_32x32.png` |
| moricons_mun | RT_ICON | 149 (group 75) | 32x32 | 1 | `icons/icon_75_0_32x32.png` |
| moricons_mun | RT_ICON | 15 (group 8) | 32x32 | 1 | `icons/icon_8_0_32x32.png` |
| moricons_mun | RT_ICON | 150 (group 75) | 32x32 | 4 | `icons/icon_75_1_32x32.png` |
| moricons_mun | RT_ICON | 151 (group 76) | 32x32 | 1 | `icons/icon_76_0_32x32.png` |
| moricons_mun | RT_ICON | 152 (group 76) | 32x32 | 4 | `icons/icon_76_1_32x32.png` |
| moricons_mun | RT_ICON | 153 (group 77) | 32x32 | 1 | `icons/icon_77_0_32x32.png` |
| moricons_mun | RT_ICON | 154 (group 77) | 32x32 | 4 | `icons/icon_77_1_32x32.png` |
| moricons_mun | RT_ICON | 155 (group 78) | 32x32 | 1 | `icons/icon_78_0_32x32.png` |
| moricons_mun | RT_ICON | 156 (group 78) | 32x32 | 4 | `icons/icon_78_1_32x32.png` |
| moricons_mun | RT_ICON | 157 (group 79) | 32x32 | 1 | `icons/icon_79_0_32x32.png` |
| moricons_mun | RT_ICON | 158 (group 79) | 32x32 | 4 | `icons/icon_79_1_32x32.png` |
| moricons_mun | RT_ICON | 159 (group 80) | 32x32 | 1 | `icons/icon_80_0_32x32.png` |
| moricons_mun | RT_ICON | 16 (group 8) | 32x32 | 4 | `icons/icon_8_1_32x32.png` |
| moricons_mun | RT_ICON | 160 (group 80) | 32x32 | 4 | `icons/icon_80_1_32x32.png` |
| moricons_mun | RT_ICON | 161 (group 81) | 32x32 | 1 | `icons/icon_81_0_32x32.png` |
| moricons_mun | RT_ICON | 162 (group 81) | 32x32 | 4 | `icons/icon_81_1_32x32.png` |
| moricons_mun | RT_ICON | 163 (group 82) | 32x32 | 1 | `icons/icon_82_0_32x32.png` |
| moricons_mun | RT_ICON | 164 (group 82) | 32x32 | 4 | `icons/icon_82_1_32x32.png` |
| moricons_mun | RT_ICON | 165 (group 83) | 32x32 | 1 | `icons/icon_83_0_32x32.png` |
| moricons_mun | RT_ICON | 166 (group 83) | 32x32 | 4 | `icons/icon_83_1_32x32.png` |
| moricons_mun | RT_ICON | 167 (group 84) | 32x32 | 1 | `icons/icon_84_0_32x32.png` |
| moricons_mun | RT_ICON | 168 (group 84) | 32x32 | 4 | `icons/icon_84_1_32x32.png` |
| moricons_mun | RT_ICON | 169 (group 85) | 32x32 | 1 | `icons/icon_85_0_32x32.png` |
| moricons_mun | RT_ICON | 17 (group 9) | 32x32 | 1 | `icons/icon_9_0_32x32.png` |
| moricons_mun | RT_ICON | 170 (group 85) | 32x32 | 4 | `icons/icon_85_1_32x32.png` |
| moricons_mun | RT_ICON | 171 (group 86) | 32x32 | 1 | `icons/icon_86_0_32x32.png` |
| moricons_mun | RT_ICON | 172 (group 86) | 32x32 | 4 | `icons/icon_86_1_32x32.png` |
| moricons_mun | RT_ICON | 173 (group 87) | 32x32 | 1 | `icons/icon_87_0_32x32.png` |
| moricons_mun | RT_ICON | 174 (group 87) | 32x32 | 4 | `icons/icon_87_1_32x32.png` |
| moricons_mun | RT_ICON | 175 (group 88) | 32x32 | 1 | `icons/icon_88_0_32x32.png` |
| moricons_mun | RT_ICON | 176 (group 88) | 32x32 | 4 | `icons/icon_88_1_32x32.png` |
| moricons_mun | RT_ICON | 177 (group 89) | 32x32 | 1 | `icons/icon_89_0_32x32.png` |
| moricons_mun | RT_ICON | 178 (group 89) | 32x32 | 4 | `icons/icon_89_1_32x32.png` |
| moricons_mun | RT_ICON | 179 (group 90) | 32x32 | 1 | `icons/icon_90_0_32x32.png` |
| moricons_mun | RT_ICON | 18 (group 9) | 32x32 | 4 | `icons/icon_9_1_32x32.png` |
| moricons_mun | RT_ICON | 180 (group 90) | 32x32 | 4 | `icons/icon_90_1_32x32.png` |
| moricons_mun | RT_ICON | 181 (group 91) | 32x32 | 1 | `icons/icon_91_0_32x32.png` |
| moricons_mun | RT_ICON | 182 (group 91) | 32x32 | 4 | `icons/icon_91_1_32x32.png` |
| moricons_mun | RT_ICON | 183 (group 92) | 32x32 | 1 | `icons/icon_92_0_32x32.png` |
| moricons_mun | RT_ICON | 184 (group 92) | 32x32 | 4 | `icons/icon_92_1_32x32.png` |
| moricons_mun | RT_ICON | 185 (group 93) | 32x32 | 1 | `icons/icon_93_0_32x32.png` |
| moricons_mun | RT_ICON | 186 (group 93) | 32x32 | 4 | `icons/icon_93_1_32x32.png` |
| moricons_mun | RT_ICON | 187 (group 94) | 32x32 | 1 | `icons/icon_94_0_32x32.png` |
| moricons_mun | RT_ICON | 188 (group 94) | 32x32 | 4 | `icons/icon_94_1_32x32.png` |
| moricons_mun | RT_ICON | 189 (group 95) | 32x32 | 1 | `icons/icon_95_0_32x32.png` |
| moricons_mun | RT_ICON | 19 (group 10) | 32x32 | 1 | `icons/icon_10_0_32x32.png` |
| moricons_mun | RT_ICON | 190 (group 95) | 32x32 | 4 | `icons/icon_95_1_32x32.png` |
| moricons_mun | RT_ICON | 191 (group 96) | 32x32 | 4 | `icons/icon_96_0_32x32.png` |
| moricons_mun | RT_ICON | 192 (group 97) | 32x32 | 1 | `icons/icon_97_0_32x32.png` |
| moricons_mun | RT_ICON | 193 (group 97) | 32x32 | 4 | `icons/icon_97_1_32x32.png` |
| moricons_mun | RT_ICON | 194 (group 98) | 32x32 | 1 | `icons/icon_98_0_32x32.png` |
| moricons_mun | RT_ICON | 195 (group 98) | 32x32 | 4 | `icons/icon_98_1_32x32.png` |
| moricons_mun | RT_ICON | 196 (group 99) | 32x32 | 1 | `icons/icon_99_0_32x32.png` |
| moricons_mun | RT_ICON | 197 (group 99) | 32x32 | 4 | `icons/icon_99_1_32x32.png` |
| moricons_mun | RT_ICON | 198 (group 100) | 32x32 | 4 | `icons/icon_100_0_32x32.png` |
| moricons_mun | RT_ICON | 199 (group 100) | 32x32 | 1 | `icons/icon_100_1_32x32.png` |
| moricons_mun | RT_ICON | 2 (group 1) | 32x32 | 4 | `icons/icon_1_1_32x32.png` |
| moricons_mun | RT_ICON | 20 (group 10) | 32x32 | 4 | `icons/icon_10_1_32x32.png` |
| moricons_mun | RT_ICON | 200 (group 101) | 32x32 | 4 | `icons/icon_101_0_32x32.png` |
| moricons_mun | RT_ICON | 201 (group 101) | 32x32 | 1 | `icons/icon_101_1_32x32.png` |
| moricons_mun | RT_ICON | 202 (group 102) | 32x32 | 1 | `icons/icon_102_0_32x32.png` |
| moricons_mun | RT_ICON | 203 (group 102) | 32x32 | 4 | `icons/icon_102_1_32x32.png` |
| moricons_mun | RT_ICON | 204 (group 103) | 32x32 | 1 | `icons/icon_103_0_32x32.png` |
| moricons_mun | RT_ICON | 205 (group 103) | 32x32 | 4 | `icons/icon_103_1_32x32.png` |
| moricons_mun | RT_ICON | 206 (group 104) | 32x32 | 1 | `icons/icon_104_0_32x32.png` |
| moricons_mun | RT_ICON | 207 (group 104) | 32x32 | 4 | `icons/icon_104_1_32x32.png` |
| moricons_mun | RT_ICON | 208 (group 105) | 32x32 | 1 | `icons/icon_105_0_32x32.png` |
| moricons_mun | RT_ICON | 209 (group 105) | 32x32 | 4 | `icons/icon_105_1_32x32.png` |
| moricons_mun | RT_ICON | 21 (group 11) | 32x32 | 4 | `icons/icon_11_0_32x32.png` |
| moricons_mun | RT_ICON | 210 (group 106) | 32x32 | 1 | `icons/icon_106_0_32x32.png` |
| moricons_mun | RT_ICON | 211 (group 106) | 32x32 | 4 | `icons/icon_106_1_32x32.png` |
| moricons_mun | RT_ICON | 212 (group 107) | 32x32 | 1 | `icons/icon_107_0_32x32.png` |
| moricons_mun | RT_ICON | 213 (group 107) | 32x32 | 4 | `icons/icon_107_1_32x32.png` |
| moricons_mun | RT_ICON | 214 (group 108) | 48x48 | 4 | `icons/icon_108_0_48x48.png` |
| moricons_mun | RT_ICON | 215 (group 108) | 32x32 | 4 | `icons/icon_108_1_32x32.png` |
| moricons_mun | RT_ICON | 216 (group 108) | 16x16 | 4 | `icons/icon_108_2_16x16.png` |
| moricons_mun | RT_ICON | 217 (group 108) | 48x48 | 8 | `icons/icon_108_3_48x48.png` |
| moricons_mun | RT_ICON | 218 (group 108) | 32x32 | 8 | `icons/icon_108_4_32x32.png` |
| moricons_mun | RT_ICON | 219 (group 108) | 16x16 | 8 | `icons/icon_108_5_16x16.png` |
| moricons_mun | RT_ICON | 22 (group 11) | 32x32 | 1 | `icons/icon_11_1_32x32.png` |
| moricons_mun | RT_ICON | 220 (group 108) | 48x48 | 32 | `icons/icon_108_6_48x48.png` |
| moricons_mun | RT_ICON | 221 (group 108) | 32x32 | 32 | `icons/icon_108_7_32x32.png` |
| moricons_mun | RT_ICON | 222 (group 108) | 16x16 | 32 | `icons/icon_108_8_16x16.png` |
| moricons_mun | RT_ICON | 223 (group 109) | 32x32 | 4 | `icons/icon_109_0_32x32.png` |
| moricons_mun | RT_ICON | 224 (group 109) | 16x16 | 4 | `icons/icon_109_1_16x16.png` |
| moricons_mun | RT_ICON | 225 (group 109) | 48x48 | 8 | `icons/icon_109_2_48x48.png` |
| moricons_mun | RT_ICON | 226 (group 110) | 32x32 | 4 | `icons/icon_110_0_32x32.png` |
| moricons_mun | RT_ICON | 227 (group 110) | 16x16 | 4 | `icons/icon_110_1_16x16.png` |
| moricons_mun | RT_ICON | 228 (group 110) | 48x48 | 8 | `icons/icon_110_2_48x48.png` |
| moricons_mun | RT_ICON | 229 (group 111) | 32x32 | 4 | `icons/icon_111_0_32x32.png` |
| moricons_mun | RT_ICON | 23 (group 12) | 32x32 | 1 | `icons/icon_12_0_32x32.png` |
| moricons_mun | RT_ICON | 230 (group 111) | 16x16 | 4 | `icons/icon_111_1_16x16.png` |
| moricons_mun | RT_ICON | 231 (group 111) | 48x48 | 8 | `icons/icon_111_2_48x48.png` |
| moricons_mun | RT_ICON | 232 (group 112) | 32x32 | 4 | `icons/icon_112_0_32x32.png` |
| moricons_mun | RT_ICON | 233 (group 112) | 16x16 | 4 | `icons/icon_112_1_16x16.png` |
| moricons_mun | RT_ICON | 234 (group 112) | 48x48 | 8 | `icons/icon_112_2_48x48.png` |
| moricons_mun | RT_ICON | 235 (group 113) | 32x32 | 4 | `icons/icon_113_0_32x32.png` |
| moricons_mun | RT_ICON | 236 (group 113) | 16x16 | 4 | `icons/icon_113_1_16x16.png` |
| moricons_mun | RT_ICON | 237 (group 113) | 48x48 | 8 | `icons/icon_113_2_48x48.png` |
| moricons_mun | RT_ICON | 24 (group 12) | 32x32 | 4 | `icons/icon_12_1_32x32.png` |
| moricons_mun | RT_ICON | 25 (group 13) | 32x32 | 4 | `icons/icon_13_0_32x32.png` |
| moricons_mun | RT_ICON | 26 (group 13) | 32x32 | 1 | `icons/icon_13_1_32x32.png` |
| moricons_mun | RT_ICON | 27 (group 14) | 32x32 | 1 | `icons/icon_14_0_32x32.png` |
| moricons_mun | RT_ICON | 28 (group 14) | 32x32 | 4 | `icons/icon_14_1_32x32.png` |
| moricons_mun | RT_ICON | 29 (group 15) | 32x32 | 4 | `icons/icon_15_0_32x32.png` |
| moricons_mun | RT_ICON | 3 (group 2) | 32x32 | 1 | `icons/icon_2_0_32x32.png` |
| moricons_mun | RT_ICON | 30 (group 15) | 32x32 | 1 | `icons/icon_15_1_32x32.png` |
| moricons_mun | RT_ICON | 31 (group 16) | 32x32 | 1 | `icons/icon_16_0_32x32.png` |
| moricons_mun | RT_ICON | 32 (group 16) | 32x32 | 4 | `icons/icon_16_1_32x32.png` |
| moricons_mun | RT_ICON | 33 (group 17) | 32x32 | 4 | `icons/icon_17_0_32x32.png` |
| moricons_mun | RT_ICON | 34 (group 17) | 32x32 | 1 | `icons/icon_17_1_32x32.png` |
| moricons_mun | RT_ICON | 35 (group 18) | 32x32 | 1 | `icons/icon_18_0_32x32.png` |
| moricons_mun | RT_ICON | 36 (group 18) | 32x32 | 4 | `icons/icon_18_1_32x32.png` |
| moricons_mun | RT_ICON | 37 (group 19) | 32x32 | 1 | `icons/icon_19_0_32x32.png` |
| moricons_mun | RT_ICON | 38 (group 19) | 32x32 | 4 | `icons/icon_19_1_32x32.png` |
| moricons_mun | RT_ICON | 39 (group 20) | 32x32 | 1 | `icons/icon_20_0_32x32.png` |
| moricons_mun | RT_ICON | 4 (group 2) | 32x32 | 4 | `icons/icon_2_1_32x32.png` |
| moricons_mun | RT_ICON | 40 (group 20) | 32x32 | 4 | `icons/icon_20_1_32x32.png` |
| moricons_mun | RT_ICON | 41 (group 21) | 32x32 | 4 | `icons/icon_21_0_32x32.png` |
| moricons_mun | RT_ICON | 42 (group 21) | 32x32 | 1 | `icons/icon_21_1_32x32.png` |
| moricons_mun | RT_ICON | 43 (group 22) | 32x32 | 1 | `icons/icon_22_0_32x32.png` |
| moricons_mun | RT_ICON | 44 (group 22) | 32x32 | 4 | `icons/icon_22_1_32x32.png` |
| moricons_mun | RT_ICON | 45 (group 23) | 32x32 | 4 | `icons/icon_23_0_32x32.png` |
| moricons_mun | RT_ICON | 46 (group 23) | 32x32 | 1 | `icons/icon_23_1_32x32.png` |
| moricons_mun | RT_ICON | 47 (group 24) | 32x32 | 1 | `icons/icon_24_0_32x32.png` |
| moricons_mun | RT_ICON | 48 (group 24) | 32x32 | 4 | `icons/icon_24_1_32x32.png` |
| moricons_mun | RT_ICON | 49 (group 25) | 32x32 | 1 | `icons/icon_25_0_32x32.png` |
| moricons_mun | RT_ICON | 5 (group 3) | 32x32 | 1 | `icons/icon_3_0_32x32.png` |
| moricons_mun | RT_ICON | 50 (group 25) | 32x32 | 4 | `icons/icon_25_1_32x32.png` |
| moricons_mun | RT_ICON | 51 (group 26) | 32x32 | 4 | `icons/icon_26_0_32x32.png` |
| moricons_mun | RT_ICON | 52 (group 26) | 32x32 | 1 | `icons/icon_26_1_32x32.png` |
| moricons_mun | RT_ICON | 53 (group 27) | 32x32 | 4 | `icons/icon_27_0_32x32.png` |
| moricons_mun | RT_ICON | 54 (group 27) | 32x32 | 1 | `icons/icon_27_1_32x32.png` |
| moricons_mun | RT_ICON | 55 (group 28) | 32x32 | 4 | `icons/icon_28_0_32x32.png` |
| moricons_mun | RT_ICON | 56 (group 28) | 32x32 | 1 | `icons/icon_28_1_32x32.png` |
| moricons_mun | RT_ICON | 57 (group 29) | 32x32 | 4 | `icons/icon_29_0_32x32.png` |
| moricons_mun | RT_ICON | 58 (group 29) | 32x32 | 1 | `icons/icon_29_1_32x32.png` |
| moricons_mun | RT_ICON | 59 (group 30) | 32x32 | 4 | `icons/icon_30_0_32x32.png` |
| moricons_mun | RT_ICON | 6 (group 3) | 32x32 | 4 | `icons/icon_3_1_32x32.png` |
| moricons_mun | RT_ICON | 60 (group 30) | 32x32 | 1 | `icons/icon_30_1_32x32.png` |
| moricons_mun | RT_ICON | 61 (group 31) | 32x32 | 1 | `icons/icon_31_0_32x32.png` |
| moricons_mun | RT_ICON | 62 (group 31) | 32x32 | 4 | `icons/icon_31_1_32x32.png` |
| moricons_mun | RT_ICON | 63 (group 32) | 32x32 | 1 | `icons/icon_32_0_32x32.png` |
| moricons_mun | RT_ICON | 64 (group 32) | 32x32 | 4 | `icons/icon_32_1_32x32.png` |
| moricons_mun | RT_ICON | 65 (group 33) | 32x32 | 1 | `icons/icon_33_0_32x32.png` |
| moricons_mun | RT_ICON | 66 (group 33) | 32x32 | 4 | `icons/icon_33_1_32x32.png` |
| moricons_mun | RT_ICON | 67 (group 34) | 32x32 | 1 | `icons/icon_34_0_32x32.png` |
| moricons_mun | RT_ICON | 68 (group 34) | 32x32 | 4 | `icons/icon_34_1_32x32.png` |
| moricons_mun | RT_ICON | 69 (group 35) | 32x32 | 1 | `icons/icon_35_0_32x32.png` |
| moricons_mun | RT_ICON | 7 (group 4) | 32x32 | 1 | `icons/icon_4_0_32x32.png` |
| moricons_mun | RT_ICON | 70 (group 35) | 32x32 | 4 | `icons/icon_35_1_32x32.png` |
| moricons_mun | RT_ICON | 71 (group 36) | 32x32 | 1 | `icons/icon_36_0_32x32.png` |
| moricons_mun | RT_ICON | 72 (group 36) | 32x32 | 4 | `icons/icon_36_1_32x32.png` |
| moricons_mun | RT_ICON | 73 (group 37) | 32x32 | 1 | `icons/icon_37_0_32x32.png` |
| moricons_mun | RT_ICON | 74 (group 37) | 32x32 | 4 | `icons/icon_37_1_32x32.png` |
| moricons_mun | RT_ICON | 75 (group 38) | 32x32 | 1 | `icons/icon_38_0_32x32.png` |
| moricons_mun | RT_ICON | 76 (group 38) | 32x32 | 4 | `icons/icon_38_1_32x32.png` |
| moricons_mun | RT_ICON | 77 (group 39) | 32x32 | 1 | `icons/icon_39_0_32x32.png` |
| moricons_mun | RT_ICON | 78 (group 39) | 32x32 | 4 | `icons/icon_39_1_32x32.png` |
| moricons_mun | RT_ICON | 79 (group 40) | 32x32 | 1 | `icons/icon_40_0_32x32.png` |
| moricons_mun | RT_ICON | 8 (group 4) | 32x32 | 4 | `icons/icon_4_1_32x32.png` |
| moricons_mun | RT_ICON | 80 (group 40) | 32x32 | 4 | `icons/icon_40_1_32x32.png` |
| moricons_mun | RT_ICON | 81 (group 41) | 32x32 | 1 | `icons/icon_41_0_32x32.png` |
| moricons_mun | RT_ICON | 82 (group 41) | 32x32 | 4 | `icons/icon_41_1_32x32.png` |
| moricons_mun | RT_ICON | 83 (group 42) | 32x32 | 1 | `icons/icon_42_0_32x32.png` |
| moricons_mun | RT_ICON | 84 (group 42) | 32x32 | 4 | `icons/icon_42_1_32x32.png` |
| moricons_mun | RT_ICON | 85 (group 43) | 32x32 | 1 | `icons/icon_43_0_32x32.png` |
| moricons_mun | RT_ICON | 86 (group 43) | 32x32 | 4 | `icons/icon_43_1_32x32.png` |
| moricons_mun | RT_ICON | 87 (group 44) | 32x32 | 1 | `icons/icon_44_0_32x32.png` |
| moricons_mun | RT_ICON | 88 (group 44) | 32x32 | 4 | `icons/icon_44_1_32x32.png` |
| moricons_mun | RT_ICON | 89 (group 45) | 32x32 | 1 | `icons/icon_45_0_32x32.png` |
| moricons_mun | RT_ICON | 9 (group 5) | 32x32 | 1 | `icons/icon_5_0_32x32.png` |
| moricons_mun | RT_ICON | 90 (group 45) | 32x32 | 4 | `icons/icon_45_1_32x32.png` |
| moricons_mun | RT_ICON | 91 (group 46) | 32x32 | 1 | `icons/icon_46_0_32x32.png` |
| moricons_mun | RT_ICON | 92 (group 46) | 32x32 | 4 | `icons/icon_46_1_32x32.png` |
| moricons_mun | RT_ICON | 93 (group 47) | 32x32 | 1 | `icons/icon_47_0_32x32.png` |
| moricons_mun | RT_ICON | 94 (group 47) | 32x32 | 4 | `icons/icon_47_1_32x32.png` |
| moricons_mun | RT_ICON | 95 (group 48) | 32x32 | 1 | `icons/icon_48_0_32x32.png` |
| moricons_mun | RT_ICON | 96 (group 48) | 32x32 | 4 | `icons/icon_48_1_32x32.png` |
| moricons_mun | RT_ICON | 97 (group 49) | 32x32 | 4 | `icons/icon_49_0_32x32.png` |
| moricons_mun | RT_ICON | 98 (group 49) | 32x32 | 1 | `icons/icon_49_1_32x32.png` |
| moricons_mun | RT_ICON | 99 (group 50) | 32x32 | 1 | `icons/icon_50_0_32x32.png` |
| shell32_mun | RT_BITMAP | 133 | 14x14 | 4 | `bitmaps/bitmap_133.png` |
| shell32_mun | RT_BITMAP | 134 | 14x14 | 4 | `bitmaps/bitmap_134.png` |
| shell32_mun | RT_BITMAP | 135 | 36x16 | 4 | `bitmaps/bitmap_135.png` |
| shell32_mun | RT_BITMAP | 136 | 36x16 | 4 | `bitmaps/bitmap_136.png` |
| shell32_mun | RT_BITMAP | 142 | 11x22 | 4 | `bitmaps/bitmap_142.png` |
| shell32_mun | RT_BITMAP | 143 | 11x22 | 4 | `bitmaps/bitmap_143.png` |
| shell32_mun | RT_BITMAP | 145 | 20x24 | 4 | `bitmaps/bitmap_145.png` |
| shell32_mun | RT_BITMAP | 210 | 80x16 | 4 | `bitmaps/bitmap_210.png` |
| shell32_mun | RT_BITMAP | 211 | 16x16 | 4 | `bitmaps/bitmap_211.png` |
| shell32_mun | RT_BITMAP | 311 | 32x128 | 32 | `bitmaps/bitmap_311.png` |
| shell32_mun | RT_BITMAP | 312 | 200x63 | 24 | `bitmaps/bitmap_312.png` |
| shell32_mun | RT_BITMAP | 420 | 132x31 | 24 | `bitmaps/bitmap_420.png` |
| shell32_mun | RT_BITMAP | 500 | 750x3 | 24 | `bitmaps/bitmap_500.png` |
| shell32_mun | RT_BITMAP | 600 | 16x16 | 4 | `bitmaps/bitmap_600.png` |
| shell32_mun | RT_BITMAP | 601 | 16x16 | 4 | `bitmaps/bitmap_601.png` |
| shell32_mun | RT_BITMAP | 602 | 16x16 | 4 | `bitmaps/bitmap_602.png` |
| shell32_mun | RT_BITMAP | 603 | 16x16 | 4 | `bitmaps/bitmap_603.png` |
| shell32_mun | RT_BITMAP | 604 | 16x16 | 4 | `bitmaps/bitmap_604.png` |
| shell32_mun | RT_BITMAP | 605 | 16x16 | 4 | `bitmaps/bitmap_605.png` |
| shell32_mun | RT_BITMAP | 606 | 16x16 | 4 | `bitmaps/bitmap_606.png` |
| shell32_mun | RT_BITMAP | 607 | 16x16 | 4 | `bitmaps/bitmap_607.png` |
| shell32_mun | RT_BITMAP | 608 | 16x16 | 4 | `bitmaps/bitmap_608.png` |
| shell32_mun | RT_BITMAP | 609 | 16x16 | 4 | `bitmaps/bitmap_609.png` |
| shell32_mun | RT_BITMAP | 610 | 16x16 | 4 | `bitmaps/bitmap_610.png` |
| shell32_mun | RT_BITMAP | 611 | 16x16 | 4 | `bitmaps/bitmap_611.png` |
| shell32_mun | RT_BITMAP | 612 | 16x16 | 4 | `bitmaps/bitmap_612.png` |
| shell32_mun | RT_BITMAP | 613 | 16x16 | 4 | `bitmaps/bitmap_613.png` |
| shell32_mun | RT_BITMAP | 614 | 16x16 | 4 | `bitmaps/bitmap_614.png` |
| shell32_mun | RT_BITMAP | 615 | 16x16 | 4 | `bitmaps/bitmap_615.png` |
| shell32_mun | RT_BITMAP | 633 | 1x1 | 24 | `bitmaps/bitmap_633.png` |
| shell32_mun | RT_BITMAP | 635 | 64x60 | 8 | `bitmaps/bitmap_635.png` |
| shell32_mun | RT_BITMAP | 650 | 13x13 | 32 | `bitmaps/bitmap_650.png` |
| shell32_mun | RT_BITMAP | 700 | 7x6 | 24 | `bitmaps/bitmap_700.png` |
| shell32_mun | RT_BITMAP | 701 | 200x48 | 32 | `bitmaps/bitmap_701.png` |
| shell32_mun | RT_BITMAP | IDB_TB_SH_DEF_16 | 768x16 | 4 | `bitmaps/bitmap_IDB_TB_SH_DEF_16.png` |
| shell32_mun | RT_GROUP_ICON | 1 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1.ico` |
| shell32_mun | RT_GROUP_ICON | 10 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_10.ico` |
| shell32_mun | RT_GROUP_ICON | 1001 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1001.ico` |
| shell32_mun | RT_GROUP_ICON | 1002 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1002.ico` |
| shell32_mun | RT_GROUP_ICON | 1003 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1003.ico` |
| shell32_mun | RT_GROUP_ICON | 1004 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1004.ico` |
| shell32_mun | RT_GROUP_ICON | 1005 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1005.ico` |
| shell32_mun | RT_GROUP_ICON | 1006 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1006.ico` |
| shell32_mun | RT_GROUP_ICON | 1007 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1007.ico` |
| shell32_mun | RT_GROUP_ICON | 1008 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1008.ico` |
| shell32_mun | RT_GROUP_ICON | 1009 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1009.ico` |
| shell32_mun | RT_GROUP_ICON | 1010 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1010.ico` |
| shell32_mun | RT_GROUP_ICON | 1011 | 16x16+16x16+16x16+32x32+32x32+32x32 | 4,8,32,4,8,32 | `icons/groupicon_1011.ico` |
| shell32_mun | RT_GROUP_ICON | 11 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_11.ico` |
| shell32_mun | RT_GROUP_ICON | 12 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_12.ico` |
| shell32_mun | RT_GROUP_ICON | 13 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_13.ico` |
| shell32_mun | RT_GROUP_ICON | 133 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_133.ico` |
| shell32_mun | RT_GROUP_ICON | 134 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_134.ico` |
| shell32_mun | RT_GROUP_ICON | 135 | 16x16+16x16 | 8,32 | `icons/groupicon_135.ico` |
| shell32_mun | RT_GROUP_ICON | 137 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_137.ico` |
| shell32_mun | RT_GROUP_ICON | 138 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_138.ico` |
| shell32_mun | RT_GROUP_ICON | 139 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_139.ico` |
| shell32_mun | RT_GROUP_ICON | 14 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_14.ico` |
| shell32_mun | RT_GROUP_ICON | 140 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_140.ico` |
| shell32_mun | RT_GROUP_ICON | 141 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_141.ico` |
| shell32_mun | RT_GROUP_ICON | 142 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_142.ico` |
| shell32_mun | RT_GROUP_ICON | 143 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_143.ico` |
| shell32_mun | RT_GROUP_ICON | 144 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_144.ico` |
| shell32_mun | RT_GROUP_ICON | 145 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_145.ico` |
| shell32_mun | RT_GROUP_ICON | 146 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_146.ico` |
| shell32_mun | RT_GROUP_ICON | 147 | 32x32+16x16+48x48+16x16+32x32+48x48 | 8,4,8,8,4,4 | `icons/groupicon_147.ico` |
| shell32_mun | RT_GROUP_ICON | 148 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_148.ico` |
| shell32_mun | RT_GROUP_ICON | 15 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_15.ico` |
| shell32_mun | RT_GROUP_ICON | 151 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_151.ico` |
| shell32_mun | RT_GROUP_ICON | 152 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_152.ico` |
| shell32_mun | RT_GROUP_ICON | 153 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_153.ico` |
| shell32_mun | RT_GROUP_ICON | 154 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_154.ico` |
| shell32_mun | RT_GROUP_ICON | 155 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_155.ico` |
| shell32_mun | RT_GROUP_ICON | 156 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_156.ico` |
| shell32_mun | RT_GROUP_ICON | 157 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_157.ico` |
| shell32_mun | RT_GROUP_ICON | 16 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16.ico` |
| shell32_mun | RT_GROUP_ICON | 160 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_160.ico` |
| shell32_mun | RT_GROUP_ICON | 161 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_161.ico` |
| shell32_mun | RT_GROUP_ICON | 165 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_165.ico` |
| shell32_mun | RT_GROUP_ICON | 166 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_166.ico` |
| shell32_mun | RT_GROUP_ICON | 167 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_167.ico` |
| shell32_mun | RT_GROUP_ICON | 16710 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16710.ico` |
| shell32_mun | RT_GROUP_ICON | 16715 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16715.ico` |
| shell32_mun | RT_GROUP_ICON | 16717 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16717.ico` |
| shell32_mun | RT_GROUP_ICON | 16718 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16718.ico` |
| shell32_mun | RT_GROUP_ICON | 16721 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16721.ico` |
| shell32_mun | RT_GROUP_ICON | 16739 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16739.ico` |
| shell32_mun | RT_GROUP_ICON | 16741 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16741.ico` |
| shell32_mun | RT_GROUP_ICON | 16742 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16742.ico` |
| shell32_mun | RT_GROUP_ICON | 16743 | 32x32 | 8 | `icons/groupicon_16743.ico` |
| shell32_mun | RT_GROUP_ICON | 16744 | 48x48+32x32+16x16+48x48+32x32+16x16+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_16744.ico` |
| shell32_mun | RT_GROUP_ICON | 16746 | 48x48+32x32+16x16+48x48+32x32+16x16 | 8,8,8,32,32,32 | `icons/groupicon_16746.ico` |
| shell32_mun | RT_GROUP_ICON | 16747 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16747.ico` |
| shell32_mun | RT_GROUP_ICON | 16748 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16748.ico` |
| shell32_mun | RT_GROUP_ICON | 16749 | 32x32+24x24+20x20+16x16+32x32+24x24+20x20+16x16+32x32+24x24+20x20+16x16 | 4,4,4,4,8,8,8,8,32,32,32,32 | `icons/groupicon_16749.ico` |
| shell32_mun | RT_GROUP_ICON | 16750 | 32x32+24x24+20x20+16x16+32x32+24x24+20x20+16x16+32x32+24x24+20x20+16x16 | 4,4,4,4,8,8,8,8,32,32,32,32 | `icons/groupicon_16750.ico` |
| shell32_mun | RT_GROUP_ICON | 16751 | 16x16 | 32 | `icons/groupicon_16751.ico` |
| shell32_mun | RT_GROUP_ICON | 16752 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16752.ico` |
| shell32_mun | RT_GROUP_ICON | 16753 | 16x16+16x16 | 4,32 | `icons/groupicon_16753.ico` |
| shell32_mun | RT_GROUP_ICON | 16754 | 16x16 | 32 | `icons/groupicon_16754.ico` |
| shell32_mun | RT_GROUP_ICON | 16755 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16755.ico` |
| shell32_mun | RT_GROUP_ICON | 16756 | 16x16 | 32 | `icons/groupicon_16756.ico` |
| shell32_mun | RT_GROUP_ICON | 16757 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16757.ico` |
| shell32_mun | RT_GROUP_ICON | 16758 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16758.ico` |
| shell32_mun | RT_GROUP_ICON | 16759 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16759.ico` |
| shell32_mun | RT_GROUP_ICON | 16760 | 16x16 | 32 | `icons/groupicon_16760.ico` |
| shell32_mun | RT_GROUP_ICON | 16761 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16761.ico` |
| shell32_mun | RT_GROUP_ICON | 16762 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16762.ico` |
| shell32_mun | RT_GROUP_ICON | 16763 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16763.ico` |
| shell32_mun | RT_GROUP_ICON | 16767 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16767.ico` |
| shell32_mun | RT_GROUP_ICON | 16768 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16768.ico` |
| shell32_mun | RT_GROUP_ICON | 16769 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16769.ico` |
| shell32_mun | RT_GROUP_ICON | 16770 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16770.ico` |
| shell32_mun | RT_GROUP_ICON | 16771 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16771.ico` |
| shell32_mun | RT_GROUP_ICON | 16772 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16772.ico` |
| shell32_mun | RT_GROUP_ICON | 16773 | 16x16 | 32 | `icons/groupicon_16773.ico` |
| shell32_mun | RT_GROUP_ICON | 16774 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16774.ico` |
| shell32_mun | RT_GROUP_ICON | 16775 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16775.ico` |
| shell32_mun | RT_GROUP_ICON | 16776 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16776.ico` |
| shell32_mun | RT_GROUP_ICON | 16777 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16777.ico` |
| shell32_mun | RT_GROUP_ICON | 16778 | 64x64+48x48+40x40+32x32+24x24+20x20+16x16+64x64+48x48+40x40+32x32+24x24+20x20+16x16+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 4,4,4,4,4,4,4,8,8,8,8,8,8,8,32,32,32,32,32,32,32 | `icons/groupicon_16778.ico` |
| shell32_mun | RT_GROUP_ICON | 16779 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16779.ico` |
| shell32_mun | RT_GROUP_ICON | 16780 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16780.ico` |
| shell32_mun | RT_GROUP_ICON | 16781 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16781.ico` |
| shell32_mun | RT_GROUP_ICON | 16782 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16782.ico` |
| shell32_mun | RT_GROUP_ICON | 16783 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16783.ico` |
| shell32_mun | RT_GROUP_ICON | 16784 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16784.ico` |
| shell32_mun | RT_GROUP_ICON | 16787 | 16x16+16x16+16x16 | 4,8,32 | `icons/groupicon_16787.ico` |
| shell32_mun | RT_GROUP_ICON | 16788 | 16x16+16x16+16x16 | 4,8,32 | `icons/groupicon_16788.ico` |
| shell32_mun | RT_GROUP_ICON | 16789 | 16x16+16x16+16x16 | 4,8,32 | `icons/groupicon_16789.ico` |
| shell32_mun | RT_GROUP_ICON | 16790 | 16x16+16x16+16x16 | 4,8,32 | `icons/groupicon_16790.ico` |
| shell32_mun | RT_GROUP_ICON | 16791 | 16x16+16x16+16x16 | 4,8,32 | `icons/groupicon_16791.ico` |
| shell32_mun | RT_GROUP_ICON | 16792 | 16x16+16x16+16x16 | 4,8,32 | `icons/groupicon_16792.ico` |
| shell32_mun | RT_GROUP_ICON | 16793 | 16x16+16x16+16x16 | 4,8,32 | `icons/groupicon_16793.ico` |
| shell32_mun | RT_GROUP_ICON | 16794 | 16x16+16x16+16x16 | 4,8,32 | `icons/groupicon_16794.ico` |
| shell32_mun | RT_GROUP_ICON | 16795 | 16x16+16x16+16x16 | 4,8,32 | `icons/groupicon_16795.ico` |
| shell32_mun | RT_GROUP_ICON | 16796 | 16x16+16x16+16x16 | 4,8,32 | `icons/groupicon_16796.ico` |
| shell32_mun | RT_GROUP_ICON | 16797 | 16x16+16x16+16x16 | 4,8,32 | `icons/groupicon_16797.ico` |
| shell32_mun | RT_GROUP_ICON | 16798 | 16x16+16x16+16x16 | 4,8,32 | `icons/groupicon_16798.ico` |
| shell32_mun | RT_GROUP_ICON | 16799 | 32x32+16x16+32x32+16x16+32x32+16x16 | 4,4,8,8,32,32 | `icons/groupicon_16799.ico` |
| shell32_mun | RT_GROUP_ICON | 168 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_168.ico` |
| shell32_mun | RT_GROUP_ICON | 16800 | 32x32+16x16+32x32+16x16+32x32+16x16 | 4,4,8,8,32,32 | `icons/groupicon_16800.ico` |
| shell32_mun | RT_GROUP_ICON | 16801 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16801.ico` |
| shell32_mun | RT_GROUP_ICON | 16802 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16802.ico` |
| shell32_mun | RT_GROUP_ICON | 16803 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16803.ico` |
| shell32_mun | RT_GROUP_ICON | 16804 | 48x48+32x32+24x24+16x16+48x48+32x32+24x24+16x16+256x256+64x64+48x48+32x32+24x24+16x16 | 4,4,4,4,8,8,8,8,32,32,32,32,32,32 | `icons/groupicon_16804.ico` |
| shell32_mun | RT_GROUP_ICON | 16805 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16805.ico` |
| shell32_mun | RT_GROUP_ICON | 16806 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16806.ico` |
| shell32_mun | RT_GROUP_ICON | 16807 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16807.ico` |
| shell32_mun | RT_GROUP_ICON | 16810 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16810.ico` |
| shell32_mun | RT_GROUP_ICON | 16811 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16811.ico` |
| shell32_mun | RT_GROUP_ICON | 16812 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16812.ico` |
| shell32_mun | RT_GROUP_ICON | 16813 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16813.ico` |
| shell32_mun | RT_GROUP_ICON | 16814 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16814.ico` |
| shell32_mun | RT_GROUP_ICON | 16817 | 80x80+64x64+48x48+42x41+34x33+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16817.ico` |
| shell32_mun | RT_GROUP_ICON | 16818 | 80x80+64x64+48x48+42x41+34x33+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16818.ico` |
| shell32_mun | RT_GROUP_ICON | 16819 | 256x256+48x48+32x32+24x24+16x16 | 32,32,32,32,32 | `icons/groupicon_16819.ico` |
| shell32_mun | RT_GROUP_ICON | 16820 | 58x58+45x45+32x32 | 32,32,32 | `icons/groupicon_16820.ico` |
| shell32_mun | RT_GROUP_ICON | 16821 | 58x58+45x45+32x32 | 32,32,32 | `icons/groupicon_16821.ico` |
| shell32_mun | RT_GROUP_ICON | 16822 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16822.ico` |
| shell32_mun | RT_GROUP_ICON | 16823 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16823.ico` |
| shell32_mun | RT_GROUP_ICON | 16824 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16824.ico` |
| shell32_mun | RT_GROUP_ICON | 16825 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16825.ico` |
| shell32_mun | RT_GROUP_ICON | 16826 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16826.ico` |
| shell32_mun | RT_GROUP_ICON | 16827 | 80x80+64x64+48x48+42x41+34x33+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_16827.ico` |
| shell32_mun | RT_GROUP_ICON | 169 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_169.ico` |
| shell32_mun | RT_GROUP_ICON | 17 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_17.ico` |
| shell32_mun | RT_GROUP_ICON | 170 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_170.ico` |
| shell32_mun | RT_GROUP_ICON | 171 | 32x32+48x48+16x16 | 4,8,4 | `icons/groupicon_171.ico` |
| shell32_mun | RT_GROUP_ICON | 172 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_172.ico` |
| shell32_mun | RT_GROUP_ICON | 173 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_173.ico` |
| shell32_mun | RT_GROUP_ICON | 174 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_174.ico` |
| shell32_mun | RT_GROUP_ICON | 175 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_175.ico` |
| shell32_mun | RT_GROUP_ICON | 176 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_176.ico` |
| shell32_mun | RT_GROUP_ICON | 177 | 32x32+16x16 | 4,4 | `icons/groupicon_177.ico` |
| shell32_mun | RT_GROUP_ICON | 178 | 48x48+32x32+16x16+48x48+32x32+16x16+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_178.ico` |
| shell32_mun | RT_GROUP_ICON | 179 | 32x32+16x16 | 4,4 | `icons/groupicon_179.ico` |
| shell32_mun | RT_GROUP_ICON | 18 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_18.ico` |
| shell32_mun | RT_GROUP_ICON | 180 | 32x32 | 8 | `icons/groupicon_180.ico` |
| shell32_mun | RT_GROUP_ICON | 181 | 32x32 | 8 | `icons/groupicon_181.ico` |
| shell32_mun | RT_GROUP_ICON | 182 | 32x32+32x32+32x32 | 4,8,32 | `icons/groupicon_182.ico` |
| shell32_mun | RT_GROUP_ICON | 183 | 32x32+32x32+32x32 | 4,8,32 | `icons/groupicon_183.ico` |
| shell32_mun | RT_GROUP_ICON | 184 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_184.ico` |
| shell32_mun | RT_GROUP_ICON | 185 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_185.ico` |
| shell32_mun | RT_GROUP_ICON | 186 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_186.ico` |
| shell32_mun | RT_GROUP_ICON | 187 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_187.ico` |
| shell32_mun | RT_GROUP_ICON | 19 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_19.ico` |
| shell32_mun | RT_GROUP_ICON | 191 | 32x32+16x16+32x32+16x16+48x48+48x48 | 4,4,8,8,8,4 | `icons/groupicon_191.ico` |
| shell32_mun | RT_GROUP_ICON | 192 | 32x32+16x16+48x48+32x32+48x48+16x16 | 4,4,4,8,8,8 | `icons/groupicon_192.ico` |
| shell32_mun | RT_GROUP_ICON | 193 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_193.ico` |
| shell32_mun | RT_GROUP_ICON | 194 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_194.ico` |
| shell32_mun | RT_GROUP_ICON | 196 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_196.ico` |
| shell32_mun | RT_GROUP_ICON | 197 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_197.ico` |
| shell32_mun | RT_GROUP_ICON | 198 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_198.ico` |
| shell32_mun | RT_GROUP_ICON | 199 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_199.ico` |
| shell32_mun | RT_GROUP_ICON | 2 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_2.ico` |
| shell32_mun | RT_GROUP_ICON | 20 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_20.ico` |
| shell32_mun | RT_GROUP_ICON | 200 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_200.ico` |
| shell32_mun | RT_GROUP_ICON | 21 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_21.ico` |
| shell32_mun | RT_GROUP_ICON | 210 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_210.ico` |
| shell32_mun | RT_GROUP_ICON | 22 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_22.ico` |
| shell32_mun | RT_GROUP_ICON | 220 | 32x32+16x16+48x48+32x32+48x48+16x16 | 4,4,4,8,8,8 | `icons/groupicon_220.ico` |
| shell32_mun | RT_GROUP_ICON | 221 | 32x32+16x16+48x48+32x32+16x16+48x48 | 4,4,4,8,8,8 | `icons/groupicon_221.ico` |
| shell32_mun | RT_GROUP_ICON | 222 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_222.ico` |
| shell32_mun | RT_GROUP_ICON | 223 | 32x32+32x32 | 1,4 | `icons/groupicon_223.ico` |
| shell32_mun | RT_GROUP_ICON | 224 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_224.ico` |
| shell32_mun | RT_GROUP_ICON | 225 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_225.ico` |
| shell32_mun | RT_GROUP_ICON | 226 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_226.ico` |
| shell32_mun | RT_GROUP_ICON | 227 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_227.ico` |
| shell32_mun | RT_GROUP_ICON | 228 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_228.ico` |
| shell32_mun | RT_GROUP_ICON | 229 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_229.ico` |
| shell32_mun | RT_GROUP_ICON | 23 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_23.ico` |
| shell32_mun | RT_GROUP_ICON | 230 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_230.ico` |
| shell32_mun | RT_GROUP_ICON | 231 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_231.ico` |
| shell32_mun | RT_GROUP_ICON | 232 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_232.ico` |
| shell32_mun | RT_GROUP_ICON | 233 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_233.ico` |
| shell32_mun | RT_GROUP_ICON | 234 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_234.ico` |
| shell32_mun | RT_GROUP_ICON | 235 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_235.ico` |
| shell32_mun | RT_GROUP_ICON | 236 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_236.ico` |
| shell32_mun | RT_GROUP_ICON | 237 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_237.ico` |
| shell32_mun | RT_GROUP_ICON | 238 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_238.ico` |
| shell32_mun | RT_GROUP_ICON | 239 | 48x48+32x32+16x16+48x48+32x32+16x16+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_239.ico` |
| shell32_mun | RT_GROUP_ICON | 24 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_24.ico` |
| shell32_mun | RT_GROUP_ICON | 240 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_240.ico` |
| shell32_mun | RT_GROUP_ICON | 241 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_241.ico` |
| shell32_mun | RT_GROUP_ICON | 242 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_242.ico` |
| shell32_mun | RT_GROUP_ICON | 243 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_243.ico` |
| shell32_mun | RT_GROUP_ICON | 244 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_244.ico` |
| shell32_mun | RT_GROUP_ICON | 245 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_245.ico` |
| shell32_mun | RT_GROUP_ICON | 246 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_246.ico` |
| shell32_mun | RT_GROUP_ICON | 247 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_247.ico` |
| shell32_mun | RT_GROUP_ICON | 248 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_248.ico` |
| shell32_mun | RT_GROUP_ICON | 249 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_249.ico` |
| shell32_mun | RT_GROUP_ICON | 25 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_25.ico` |
| shell32_mun | RT_GROUP_ICON | 250 | 48x48+32x32+16x16+48x48+32x32+16x16+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_250.ico` |
| shell32_mun | RT_GROUP_ICON | 251 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_251.ico` |
| shell32_mun | RT_GROUP_ICON | 252 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_252.ico` |
| shell32_mun | RT_GROUP_ICON | 253 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_253.ico` |
| shell32_mun | RT_GROUP_ICON | 254 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_254.ico` |
| shell32_mun | RT_GROUP_ICON | 255 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_255.ico` |
| shell32_mun | RT_GROUP_ICON | 256 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_256.ico` |
| shell32_mun | RT_GROUP_ICON | 257 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_257.ico` |
| shell32_mun | RT_GROUP_ICON | 258 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_258.ico` |
| shell32_mun | RT_GROUP_ICON | 259 | 48x48+32x32+16x16+48x48+32x32+16x16 | 8,8,8,32,32,32 | `icons/groupicon_259.ico` |
| shell32_mun | RT_GROUP_ICON | 26 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_26.ico` |
| shell32_mun | RT_GROUP_ICON | 260 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_260.ico` |
| shell32_mun | RT_GROUP_ICON | 261 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_261.ico` |
| shell32_mun | RT_GROUP_ICON | 262 | 64x64+48x48+40x40+32x32+24x24+20x20+16x16+64x64+48x48+40x40+32x32+24x24+20x20+16x16+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 4,4,4,4,4,4,4,8,8,8,8,8,8,8,32,32,32,32,32,32,32 | `icons/groupicon_262.ico` |
| shell32_mun | RT_GROUP_ICON | 263 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_263.ico` |
| shell32_mun | RT_GROUP_ICON | 264 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_264.ico` |
| shell32_mun | RT_GROUP_ICON | 265 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_265.ico` |
| shell32_mun | RT_GROUP_ICON | 266 | 32x32+24x24+16x16+32x32+24x24+16x16+32x32+24x24+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_266.ico` |
| shell32_mun | RT_GROUP_ICON | 267 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_267.ico` |
| shell32_mun | RT_GROUP_ICON | 268 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_268.ico` |
| shell32_mun | RT_GROUP_ICON | 269 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_269.ico` |
| shell32_mun | RT_GROUP_ICON | 27 | 48x48+32x32+16x16+48x48+32x32+16x16+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_27.ico` |
| shell32_mun | RT_GROUP_ICON | 270 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_270.ico` |
| shell32_mun | RT_GROUP_ICON | 271 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_271.ico` |
| shell32_mun | RT_GROUP_ICON | 272 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_272.ico` |
| shell32_mun | RT_GROUP_ICON | 273 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_273.ico` |
| shell32_mun | RT_GROUP_ICON | 274 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_274.ico` |
| shell32_mun | RT_GROUP_ICON | 275 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_275.ico` |
| shell32_mun | RT_GROUP_ICON | 276 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_276.ico` |
| shell32_mun | RT_GROUP_ICON | 277 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_277.ico` |
| shell32_mun | RT_GROUP_ICON | 278 | 48x48+32x32+16x16+48x48+32x32+16x16+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_278.ico` |
| shell32_mun | RT_GROUP_ICON | 279 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_279.ico` |
| shell32_mun | RT_GROUP_ICON | 28 | 48x48+32x32+24x24+16x16+48x48+32x32+24x24+16x16+48x48+32x32+24x24+16x16 | 4,4,4,4,8,8,8,8,32,32,32,32 | `icons/groupicon_28.ico` |
| shell32_mun | RT_GROUP_ICON | 281 | 32x32+16x16+48x48+32x32+16x16+48x48+32x32+16x16 | 4,4,8,8,8,32,32,32 | `icons/groupicon_281.ico` |
| shell32_mun | RT_GROUP_ICON | 282 | 32x32+16x16+48x48+31x31+16x16+48x48+31x31+16x16 | 4,4,8,8,8,32,32,32 | `icons/groupicon_282.ico` |
| shell32_mun | RT_GROUP_ICON | 283 | 48x48+32x32+16x16+48x48+32x32+16x16+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_283.ico` |
| shell32_mun | RT_GROUP_ICON | 284 | 48x48+32x32+16x16+48x48+32x32+16x16+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_284.ico` |
| shell32_mun | RT_GROUP_ICON | 289 | 48x48+32x32+16x16+39x48+32x32+16x16 | 8,8,8,32,32,32 | `icons/groupicon_289.ico` |
| shell32_mun | RT_GROUP_ICON | 29 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_29.ico` |
| shell32_mun | RT_GROUP_ICON | 290 | 16x16 | 32 | `icons/groupicon_290.ico` |
| shell32_mun | RT_GROUP_ICON | 291 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_291.ico` |
| shell32_mun | RT_GROUP_ICON | 292 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_292.ico` |
| shell32_mun | RT_GROUP_ICON | 293 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_293.ico` |
| shell32_mun | RT_GROUP_ICON | 294 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_294.ico` |
| shell32_mun | RT_GROUP_ICON | 295 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_295.ico` |
| shell32_mun | RT_GROUP_ICON | 296 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_296.ico` |
| shell32_mun | RT_GROUP_ICON | 297 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_297.ico` |
| shell32_mun | RT_GROUP_ICON | 298 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_298.ico` |
| shell32_mun | RT_GROUP_ICON | 299 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_299.ico` |
| shell32_mun | RT_GROUP_ICON | 3 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_3.ico` |
| shell32_mun | RT_GROUP_ICON | 30 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_30.ico` |
| shell32_mun | RT_GROUP_ICON | 300 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_300.ico` |
| shell32_mun | RT_GROUP_ICON | 301 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_301.ico` |
| shell32_mun | RT_GROUP_ICON | 302 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_302.ico` |
| shell32_mun | RT_GROUP_ICON | 303 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_303.ico` |
| shell32_mun | RT_GROUP_ICON | 304 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_304.ico` |
| shell32_mun | RT_GROUP_ICON | 305 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_305.ico` |
| shell32_mun | RT_GROUP_ICON | 306 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_306.ico` |
| shell32_mun | RT_GROUP_ICON | 307 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_307.ico` |
| shell32_mun | RT_GROUP_ICON | 308 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_308.ico` |
| shell32_mun | RT_GROUP_ICON | 309 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_309.ico` |
| shell32_mun | RT_GROUP_ICON | 31 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_31.ico` |
| shell32_mun | RT_GROUP_ICON | 310 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_310.ico` |
| shell32_mun | RT_GROUP_ICON | 311 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_311.ico` |
| shell32_mun | RT_GROUP_ICON | 312 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_312.ico` |
| shell32_mun | RT_GROUP_ICON | 313 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_313.ico` |
| shell32_mun | RT_GROUP_ICON | 314 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_314.ico` |
| shell32_mun | RT_GROUP_ICON | 315 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_315.ico` |
| shell32_mun | RT_GROUP_ICON | 316 | 48x48+32x32+16x16+48x48+32x32+16x16+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_316.ico` |
| shell32_mun | RT_GROUP_ICON | 317 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_317.ico` |
| shell32_mun | RT_GROUP_ICON | 318 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_318.ico` |
| shell32_mun | RT_GROUP_ICON | 319 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_319.ico` |
| shell32_mun | RT_GROUP_ICON | 32 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_32.ico` |
| shell32_mun | RT_GROUP_ICON | 320 | 32x32+24x24+16x16+32x32+24x24+16x16+32x32+24x24+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_320.ico` |
| shell32_mun | RT_GROUP_ICON | 321 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_321.ico` |
| shell32_mun | RT_GROUP_ICON | 322 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_322.ico` |
| shell32_mun | RT_GROUP_ICON | 323 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_323.ico` |
| shell32_mun | RT_GROUP_ICON | 324 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_324.ico` |
| shell32_mun | RT_GROUP_ICON | 325 | 48x48+32x32+16x16+48x48+32x32+16x16+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_325.ico` |
| shell32_mun | RT_GROUP_ICON | 326 | 48x48+32x32+16x16+48x48+32x32+16x16+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_326.ico` |
| shell32_mun | RT_GROUP_ICON | 327 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_327.ico` |
| shell32_mun | RT_GROUP_ICON | 328 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_328.ico` |
| shell32_mun | RT_GROUP_ICON | 329 | 48x48+32x32+16x16+48x48+32x32+16x16+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_329.ico` |
| shell32_mun | RT_GROUP_ICON | 33 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_33.ico` |
| shell32_mun | RT_GROUP_ICON | 330 | 48x48+32x32+16x16+48x48+32x32+16x16+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_330.ico` |
| shell32_mun | RT_GROUP_ICON | 331 | 48x48+32x32+16x16+48x48+32x32+16x16+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_331.ico` |
| shell32_mun | RT_GROUP_ICON | 337 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_337.ico` |
| shell32_mun | RT_GROUP_ICON | 338 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_338.ico` |
| shell32_mun | RT_GROUP_ICON | 34 | 32x32+32x32+16x16+48x48+48x48+16x16 | 4,8,4,8,4,8 | `icons/groupicon_34.ico` |
| shell32_mun | RT_GROUP_ICON | 35 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_35.ico` |
| shell32_mun | RT_GROUP_ICON | 36 | 32x32+16x16 | 4,4 | `icons/groupicon_36.ico` |
| shell32_mun | RT_GROUP_ICON | 37 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_37.ico` |
| shell32_mun | RT_GROUP_ICON | 37219 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_37219.ico` |
| shell32_mun | RT_GROUP_ICON | 38 | 32x32+16x16 | 4,4 | `icons/groupicon_38.ico` |
| shell32_mun | RT_GROUP_ICON | 39 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_39.ico` |
| shell32_mun | RT_GROUP_ICON | 4 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_4.ico` |
| shell32_mun | RT_GROUP_ICON | 40 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_40.ico` |
| shell32_mun | RT_GROUP_ICON | 41 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_41.ico` |
| shell32_mun | RT_GROUP_ICON | 42 | 32x32+16x16 | 4,4 | `icons/groupicon_42.ico` |
| shell32_mun | RT_GROUP_ICON | 43 | 32x32+16x16+32x32+16x16 | 4,4,8,8 | `icons/groupicon_43.ico` |
| shell32_mun | RT_GROUP_ICON | 44 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_44.ico` |
| shell32_mun | RT_GROUP_ICON | 45 | 48x48+32x32+24x24+16x16+48x48+32x32+24x24+16x16+48x48+32x32+24x24+16x16 | 4,4,4,4,8,8,8,8,32,32,32,32 | `icons/groupicon_45.ico` |
| shell32_mun | RT_GROUP_ICON | 46 | 256x256+64x64+48x48+40x40+32x32+24x25+20x21+17x17 | 32,32,32,32,32,32,32,32 | `icons/groupicon_46.ico` |
| shell32_mun | RT_GROUP_ICON | 47 | 48x48+32x32+24x24+16x16+64x64+48x48+32x32+24x24+16x16+256x256+64x64+48x48+32x32+25x24+16x16 | 4,4,4,4,8,8,8,8,8,32,32,32,32,32,32 | `icons/groupicon_47.ico` |
| shell32_mun | RT_GROUP_ICON | 48 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_48.ico` |
| shell32_mun | RT_GROUP_ICON | 49 | 32x32+16x16 | 4,4 | `icons/groupicon_49.ico` |
| shell32_mun | RT_GROUP_ICON | 5 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5.ico` |
| shell32_mun | RT_GROUP_ICON | 50 | 16x16 | 4 | `icons/groupicon_50.ico` |
| shell32_mun | RT_GROUP_ICON | 51 | 16x16 | 4 | `icons/groupicon_51.ico` |
| shell32_mun | RT_GROUP_ICON | 512 | 48x48+32x32+16x16+48x48+32x32+16x16+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_512.ico` |
| shell32_mun | RT_GROUP_ICON | 51380 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_51380.ico` |
| shell32_mun | RT_GROUP_ICON | 51586 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_51586.ico` |
| shell32_mun | RT_GROUP_ICON | 51680 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_51680.ico` |
| shell32_mun | RT_GROUP_ICON | 51681 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_51681.ico` |
| shell32_mun | RT_GROUP_ICON | 51682 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_51682.ico` |
| shell32_mun | RT_GROUP_ICON | 51683 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_51683.ico` |
| shell32_mun | RT_GROUP_ICON | 51684 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_51684.ico` |
| shell32_mun | RT_GROUP_ICON | 51685 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_51685.ico` |
| shell32_mun | RT_GROUP_ICON | 51686 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_51686.ico` |
| shell32_mun | RT_GROUP_ICON | 51687 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_51687.ico` |
| shell32_mun | RT_GROUP_ICON | 52 | 16x16 | 4 | `icons/groupicon_52.ico` |
| shell32_mun | RT_GROUP_ICON | 53 | 16x16 | 4 | `icons/groupicon_53.ico` |
| shell32_mun | RT_GROUP_ICON | 54 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_54.ico` |
| shell32_mun | RT_GROUP_ICON | 6 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_6.ico` |
| shell32_mun | RT_GROUP_ICON | 62998 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_62998.ico` |
| shell32_mun | RT_GROUP_ICON | 62999 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_62999.ico` |
| shell32_mun | RT_GROUP_ICON | 63000 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_63000.ico` |
| shell32_mun | RT_GROUP_ICON | 63001 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_63001.ico` |
| shell32_mun | RT_GROUP_ICON | 63008 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_63008.ico` |
| shell32_mun | RT_GROUP_ICON | 63009 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_63009.ico` |
| shell32_mun | RT_GROUP_ICON | 63010 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_63010.ico` |
| shell32_mun | RT_GROUP_ICON | 63011 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_63011.ico` |
| shell32_mun | RT_GROUP_ICON | 7 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_7.ico` |
| shell32_mun | RT_GROUP_ICON | 8 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_8.ico` |
| shell32_mun | RT_GROUP_ICON | 8240 | 32x32+16x16 | 4,4 | `icons/groupicon_8240.ico` |
| shell32_mun | RT_GROUP_ICON | 9 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_9.ico` |
| shell32_mun | RT_ICON | 1 (group 62998) | 256x256 | 32 | `icons/icon_62998_0_256x256.png` |
| shell32_mun | RT_ICON | 10 (group 62999) | 64x64 | 32 | `icons/icon_62999_1_64x64.png` |
| shell32_mun | RT_ICON | 100 (group 4) | 20x20 | 32 | `icons/icon_4_6_20x20.png` |
| shell32_mun | RT_ICON | 1000 (group 16810) | 32x32 | 32 | `icons/icon_16810_4_32x32.png` |
| shell32_mun | RT_ICON | 1001 (group 16810) | 24x24 | 32 | `icons/icon_16810_5_24x24.png` |
| shell32_mun | RT_ICON | 1002 (group 16810) | 20x20 | 32 | `icons/icon_16810_6_20x20.png` |
| shell32_mun | RT_ICON | 1003 (group 16810) | 16x16 | 32 | `icons/icon_16810_7_16x16.png` |
| shell32_mun | RT_ICON | 1004 (group 16811) | 256x256 | 32 | `icons/icon_16811_0_256x256.png` |
| shell32_mun | RT_ICON | 1005 (group 16811) | 64x64 | 32 | `icons/icon_16811_1_64x64.png` |
| shell32_mun | RT_ICON | 1006 (group 16811) | 48x48 | 32 | `icons/icon_16811_2_48x48.png` |
| shell32_mun | RT_ICON | 1007 (group 16811) | 40x40 | 32 | `icons/icon_16811_3_40x40.png` |
| shell32_mun | RT_ICON | 1008 (group 16811) | 32x32 | 32 | `icons/icon_16811_4_32x32.png` |
| shell32_mun | RT_ICON | 1009 (group 16811) | 24x24 | 32 | `icons/icon_16811_5_24x24.png` |
| shell32_mun | RT_ICON | 101 (group 4) | 16x16 | 32 | `icons/icon_4_7_16x16.png` |
| shell32_mun | RT_ICON | 1010 (group 16811) | 20x20 | 32 | `icons/icon_16811_6_20x20.png` |
| shell32_mun | RT_ICON | 1011 (group 16811) | 16x16 | 32 | `icons/icon_16811_7_16x16.png` |
| shell32_mun | RT_ICON | 1012 (group 16812) | 256x256 | 32 | `icons/icon_16812_0_256x256.png` |
| shell32_mun | RT_ICON | 1013 (group 16812) | 64x64 | 32 | `icons/icon_16812_1_64x64.png` |
| shell32_mun | RT_ICON | 1014 (group 16812) | 48x48 | 32 | `icons/icon_16812_2_48x48.png` |
| shell32_mun | RT_ICON | 1015 (group 16812) | 40x40 | 32 | `icons/icon_16812_3_40x40.png` |
| shell32_mun | RT_ICON | 1016 (group 16812) | 32x32 | 32 | `icons/icon_16812_4_32x32.png` |
| shell32_mun | RT_ICON | 1017 (group 16812) | 24x24 | 32 | `icons/icon_16812_5_24x24.png` |
| shell32_mun | RT_ICON | 1018 (group 16812) | 20x20 | 32 | `icons/icon_16812_6_20x20.png` |
| shell32_mun | RT_ICON | 1019 (group 16812) | 16x16 | 32 | `icons/icon_16812_7_16x16.png` |
| shell32_mun | RT_ICON | 102 (group 5) | 256x256 | 32 | `icons/icon_5_0_256x256.png` |
| shell32_mun | RT_ICON | 1020 (group 233) | 256x256 | 32 | `icons/icon_233_0_256x256.png` |
| shell32_mun | RT_ICON | 1021 (group 233) | 64x64 | 32 | `icons/icon_233_1_64x64.png` |
| shell32_mun | RT_ICON | 1022 (group 233) | 48x48 | 32 | `icons/icon_233_2_48x48.png` |
| shell32_mun | RT_ICON | 1023 (group 233) | 40x40 | 32 | `icons/icon_233_3_40x40.png` |
| shell32_mun | RT_ICON | 1024 (group 233) | 32x32 | 32 | `icons/icon_233_4_32x32.png` |
| shell32_mun | RT_ICON | 1025 (group 233) | 24x24 | 32 | `icons/icon_233_5_24x24.png` |
| shell32_mun | RT_ICON | 1026 (group 233) | 20x20 | 32 | `icons/icon_233_6_20x20.png` |
| shell32_mun | RT_ICON | 1027 (group 233) | 16x16 | 32 | `icons/icon_233_7_16x16.png` |
| shell32_mun | RT_ICON | 1028 (group 252) | 256x256 | 32 | `icons/icon_252_0_256x256.png` |
| shell32_mun | RT_ICON | 1029 (group 252) | 64x64 | 32 | `icons/icon_252_1_64x64.png` |
| shell32_mun | RT_ICON | 103 (group 5) | 64x64 | 32 | `icons/icon_5_1_64x64.png` |
| shell32_mun | RT_ICON | 1030 (group 252) | 48x48 | 32 | `icons/icon_252_2_48x48.png` |
| shell32_mun | RT_ICON | 1031 (group 252) | 40x40 | 32 | `icons/icon_252_3_40x40.png` |
| shell32_mun | RT_ICON | 1032 (group 252) | 32x32 | 32 | `icons/icon_252_4_32x32.png` |
| shell32_mun | RT_ICON | 1033 (group 252) | 24x24 | 32 | `icons/icon_252_5_24x24.png` |
| shell32_mun | RT_ICON | 1034 (group 252) | 20x20 | 32 | `icons/icon_252_6_20x20.png` |
| shell32_mun | RT_ICON | 1035 (group 252) | 16x16 | 32 | `icons/icon_252_7_16x16.png` |
| shell32_mun | RT_ICON | 1036 (group 278) | 48x48 | 4 | `icons/icon_278_0_48x48.png` |
| shell32_mun | RT_ICON | 1037 (group 278) | 32x32 | 4 | `icons/icon_278_1_32x32.png` |
| shell32_mun | RT_ICON | 1038 (group 278) | 16x16 | 4 | `icons/icon_278_2_16x16.png` |
| shell32_mun | RT_ICON | 1039 (group 278) | 48x48 | 8 | `icons/icon_278_3_48x48.png` |
| shell32_mun | RT_ICON | 104 (group 5) | 48x48 | 32 | `icons/icon_5_2_48x48.png` |
| shell32_mun | RT_ICON | 1040 (group 278) | 32x32 | 8 | `icons/icon_278_4_32x32.png` |
| shell32_mun | RT_ICON | 1041 (group 278) | 16x16 | 8 | `icons/icon_278_5_16x16.png` |
| shell32_mun | RT_ICON | 1042 (group 278) | 48x48 | 32 | `icons/icon_278_6_48x48.png` |
| shell32_mun | RT_ICON | 1043 (group 278) | 32x32 | 32 | `icons/icon_278_7_32x32.png` |
| shell32_mun | RT_ICON | 1044 (group 278) | 16x16 | 32 | `icons/icon_278_8_16x16.png` |
| shell32_mun | RT_ICON | 1045 (group 279) | 256x256 | 32 | `icons/icon_279_0_256x256.png` |
| shell32_mun | RT_ICON | 1046 (group 279) | 64x64 | 32 | `icons/icon_279_1_64x64.png` |
| shell32_mun | RT_ICON | 1047 (group 279) | 48x48 | 32 | `icons/icon_279_2_48x48.png` |
| shell32_mun | RT_ICON | 1048 (group 279) | 40x40 | 32 | `icons/icon_279_3_40x40.png` |
| shell32_mun | RT_ICON | 1049 (group 279) | 32x32 | 32 | `icons/icon_279_4_32x32.png` |
| shell32_mun | RT_ICON | 105 (group 5) | 40x40 | 32 | `icons/icon_5_3_40x40.png` |
| shell32_mun | RT_ICON | 1050 (group 279) | 24x24 | 32 | `icons/icon_279_5_24x24.png` |
| shell32_mun | RT_ICON | 1051 (group 279) | 20x20 | 32 | `icons/icon_279_6_20x20.png` |
| shell32_mun | RT_ICON | 1052 (group 279) | 16x16 | 32 | `icons/icon_279_7_16x16.png` |
| shell32_mun | RT_ICON | 1053 (group 281) | 32x32 | 4 | `icons/icon_281_0_32x32.png` |
| shell32_mun | RT_ICON | 1054 (group 281) | 16x16 | 4 | `icons/icon_281_1_16x16.png` |
| shell32_mun | RT_ICON | 1055 (group 281) | 48x48 | 8 | `icons/icon_281_2_48x48.png` |
| shell32_mun | RT_ICON | 1056 (group 281) | 32x32 | 8 | `icons/icon_281_3_32x32.png` |
| shell32_mun | RT_ICON | 1057 (group 281) | 16x16 | 8 | `icons/icon_281_4_16x16.png` |
| shell32_mun | RT_ICON | 1058 (group 281) | 48x48 | 32 | `icons/icon_281_5_48x48.png` |
| shell32_mun | RT_ICON | 1059 (group 281) | 32x32 | 32 | `icons/icon_281_6_32x32.png` |
| shell32_mun | RT_ICON | 106 (group 5) | 32x32 | 32 | `icons/icon_5_4_32x32.png` |
| shell32_mun | RT_ICON | 1060 (group 281) | 16x16 | 32 | `icons/icon_281_7_16x16.png` |
| shell32_mun | RT_ICON | 1061 (group 282) | 32x32 | 4 | `icons/icon_282_0_32x32.png` |
| shell32_mun | RT_ICON | 1062 (group 282) | 16x16 | 4 | `icons/icon_282_1_16x16.png` |
| shell32_mun | RT_ICON | 1063 (group 282) | 48x48 | 8 | `icons/icon_282_2_48x48.png` |
| shell32_mun | RT_ICON | 1064 (group 282) | 31x31 | 8 | `icons/icon_282_3_31x31.png` |
| shell32_mun | RT_ICON | 1065 (group 282) | 16x16 | 8 | `icons/icon_282_4_16x16.png` |
| shell32_mun | RT_ICON | 1066 (group 282) | 48x48 | 32 | `icons/icon_282_5_48x48.png` |
| shell32_mun | RT_ICON | 1067 (group 282) | 31x31 | 32 | `icons/icon_282_6_31x31.png` |
| shell32_mun | RT_ICON | 1068 (group 282) | 16x16 | 32 | `icons/icon_282_7_16x16.png` |
| shell32_mun | RT_ICON | 1069 (group 283) | 48x48 | 4 | `icons/icon_283_0_48x48.png` |
| shell32_mun | RT_ICON | 107 (group 5) | 24x24 | 32 | `icons/icon_5_5_24x24.png` |
| shell32_mun | RT_ICON | 1070 (group 283) | 32x32 | 4 | `icons/icon_283_1_32x32.png` |
| shell32_mun | RT_ICON | 1071 (group 283) | 16x16 | 4 | `icons/icon_283_2_16x16.png` |
| shell32_mun | RT_ICON | 1072 (group 283) | 48x48 | 8 | `icons/icon_283_3_48x48.png` |
| shell32_mun | RT_ICON | 1073 (group 283) | 32x32 | 8 | `icons/icon_283_4_32x32.png` |
| shell32_mun | RT_ICON | 1074 (group 283) | 16x16 | 8 | `icons/icon_283_5_16x16.png` |
| shell32_mun | RT_ICON | 1075 (group 283) | 48x48 | 32 | `icons/icon_283_6_48x48.png` |
| shell32_mun | RT_ICON | 1076 (group 283) | 32x32 | 32 | `icons/icon_283_7_32x32.png` |
| shell32_mun | RT_ICON | 1077 (group 283) | 16x16 | 32 | `icons/icon_283_8_16x16.png` |
| shell32_mun | RT_ICON | 1078 (group 284) | 48x48 | 4 | `icons/icon_284_0_48x48.png` |
| shell32_mun | RT_ICON | 1079 (group 284) | 32x32 | 4 | `icons/icon_284_1_32x32.png` |
| shell32_mun | RT_ICON | 108 (group 5) | 20x20 | 32 | `icons/icon_5_6_20x20.png` |
| shell32_mun | RT_ICON | 1080 (group 284) | 16x16 | 4 | `icons/icon_284_2_16x16.png` |
| shell32_mun | RT_ICON | 1081 (group 284) | 48x48 | 8 | `icons/icon_284_3_48x48.png` |
| shell32_mun | RT_ICON | 1082 (group 284) | 32x32 | 8 | `icons/icon_284_4_32x32.png` |
| shell32_mun | RT_ICON | 1083 (group 284) | 16x16 | 8 | `icons/icon_284_5_16x16.png` |
| shell32_mun | RT_ICON | 1084 (group 284) | 48x48 | 32 | `icons/icon_284_6_48x48.png` |
| shell32_mun | RT_ICON | 1085 (group 284) | 32x32 | 32 | `icons/icon_284_7_32x32.png` |
| shell32_mun | RT_ICON | 1086 (group 284) | 16x16 | 32 | `icons/icon_284_8_16x16.png` |
| shell32_mun | RT_ICON | 1087 (group 289) | 48x48 | 8 | `icons/icon_289_0_48x48.png` |
| shell32_mun | RT_ICON | 1088 (group 289) | 32x32 | 8 | `icons/icon_289_1_32x32.png` |
| shell32_mun | RT_ICON | 1089 (group 289) | 16x16 | 8 | `icons/icon_289_2_16x16.png` |
| shell32_mun | RT_ICON | 109 (group 5) | 16x16 | 32 | `icons/icon_5_7_16x16.png` |
| shell32_mun | RT_ICON | 1090 (group 289) | 39x48 | 32 | `icons/icon_289_3_39x48.png` |
| shell32_mun | RT_ICON | 1091 (group 289) | 32x32 | 32 | `icons/icon_289_4_32x32.png` |
| shell32_mun | RT_ICON | 1092 (group 289) | 16x16 | 32 | `icons/icon_289_5_16x16.png` |
| shell32_mun | RT_ICON | 1093 (group 290) | 16x16 | 32 | `icons/icon_290_0_16x16.png` |
| shell32_mun | RT_ICON | 1094 (group 305) | 256x256 | 32 | `icons/icon_305_0_256x256.png` |
| shell32_mun | RT_ICON | 1095 (group 305) | 64x64 | 32 | `icons/icon_305_1_64x64.png` |
| shell32_mun | RT_ICON | 1096 (group 305) | 48x48 | 32 | `icons/icon_305_2_48x48.png` |
| shell32_mun | RT_ICON | 1097 (group 305) | 40x40 | 32 | `icons/icon_305_3_40x40.png` |
| shell32_mun | RT_ICON | 1098 (group 305) | 32x32 | 32 | `icons/icon_305_4_32x32.png` |
| shell32_mun | RT_ICON | 1099 (group 305) | 24x24 | 32 | `icons/icon_305_5_24x24.png` |
| shell32_mun | RT_ICON | 11 (group 62999) | 48x48 | 32 | `icons/icon_62999_2_48x48.png` |
| shell32_mun | RT_ICON | 110 (group 6) | 256x256 | 32 | `icons/icon_6_0_256x256.png` |
| shell32_mun | RT_ICON | 1100 (group 305) | 20x20 | 32 | `icons/icon_305_6_20x20.png` |
| shell32_mun | RT_ICON | 1101 (group 305) | 16x16 | 32 | `icons/icon_305_7_16x16.png` |
| shell32_mun | RT_ICON | 1102 (group 512) | 48x48 | 4 | `icons/icon_512_0_48x48.png` |
| shell32_mun | RT_ICON | 1103 (group 512) | 32x32 | 4 | `icons/icon_512_1_32x32.png` |
| shell32_mun | RT_ICON | 1104 (group 512) | 16x16 | 4 | `icons/icon_512_2_16x16.png` |
| shell32_mun | RT_ICON | 1105 (group 512) | 48x48 | 8 | `icons/icon_512_3_48x48.png` |
| shell32_mun | RT_ICON | 1106 (group 512) | 32x32 | 8 | `icons/icon_512_4_32x32.png` |
| shell32_mun | RT_ICON | 1107 (group 512) | 16x16 | 8 | `icons/icon_512_5_16x16.png` |
| shell32_mun | RT_ICON | 1108 (group 512) | 48x48 | 32 | `icons/icon_512_6_48x48.png` |
| shell32_mun | RT_ICON | 1109 (group 512) | 32x32 | 32 | `icons/icon_512_7_32x32.png` |
| shell32_mun | RT_ICON | 111 (group 6) | 64x64 | 32 | `icons/icon_6_1_64x64.png` |
| shell32_mun | RT_ICON | 1110 (group 512) | 16x16 | 32 | `icons/icon_512_8_16x16.png` |
| shell32_mun | RT_ICON | 1111 (group 1004) | 256x256 | 32 | `icons/icon_1004_0_256x256.png` |
| shell32_mun | RT_ICON | 1112 (group 1004) | 64x64 | 32 | `icons/icon_1004_1_64x64.png` |
| shell32_mun | RT_ICON | 1113 (group 1004) | 48x48 | 32 | `icons/icon_1004_2_48x48.png` |
| shell32_mun | RT_ICON | 1114 (group 1004) | 40x40 | 32 | `icons/icon_1004_3_40x40.png` |
| shell32_mun | RT_ICON | 1115 (group 1004) | 32x32 | 32 | `icons/icon_1004_4_32x32.png` |
| shell32_mun | RT_ICON | 1116 (group 1004) | 24x24 | 32 | `icons/icon_1004_5_24x24.png` |
| shell32_mun | RT_ICON | 1117 (group 1004) | 20x20 | 32 | `icons/icon_1004_6_20x20.png` |
| shell32_mun | RT_ICON | 1118 (group 1004) | 16x16 | 32 | `icons/icon_1004_7_16x16.png` |
| shell32_mun | RT_ICON | 1119 (group 1005) | 256x256 | 32 | `icons/icon_1005_0_256x256.png` |
| shell32_mun | RT_ICON | 112 (group 6) | 48x48 | 32 | `icons/icon_6_2_48x48.png` |
| shell32_mun | RT_ICON | 1120 (group 1005) | 64x64 | 32 | `icons/icon_1005_1_64x64.png` |
| shell32_mun | RT_ICON | 1121 (group 1005) | 48x48 | 32 | `icons/icon_1005_2_48x48.png` |
| shell32_mun | RT_ICON | 1122 (group 1005) | 40x40 | 32 | `icons/icon_1005_3_40x40.png` |
| shell32_mun | RT_ICON | 1123 (group 1005) | 32x32 | 32 | `icons/icon_1005_4_32x32.png` |
| shell32_mun | RT_ICON | 1124 (group 1005) | 24x24 | 32 | `icons/icon_1005_5_24x24.png` |
| shell32_mun | RT_ICON | 1125 (group 1005) | 20x20 | 32 | `icons/icon_1005_6_20x20.png` |
| shell32_mun | RT_ICON | 1126 (group 1005) | 16x16 | 32 | `icons/icon_1005_7_16x16.png` |
| shell32_mun | RT_ICON | 1127 (group 1011) | 16x16 | 4 | `icons/icon_1011_0_16x16.png` |
| shell32_mun | RT_ICON | 1128 (group 1011) | 16x16 | 8 | `icons/icon_1011_1_16x16.png` |
| shell32_mun | RT_ICON | 1129 (group 1011) | 16x16 | 32 | `icons/icon_1011_2_16x16.png` |
| shell32_mun | RT_ICON | 113 (group 6) | 40x40 | 32 | `icons/icon_6_3_40x40.png` |
| shell32_mun | RT_ICON | 1130 (group 1011) | 32x32 | 4 | `icons/icon_1011_3_32x32.png` |
| shell32_mun | RT_ICON | 1131 (group 1011) | 32x32 | 8 | `icons/icon_1011_4_32x32.png` |
| shell32_mun | RT_ICON | 1132 (group 1011) | 32x32 | 32 | `icons/icon_1011_5_32x32.png` |
| shell32_mun | RT_ICON | 1133 (group 8240) | 32x32 | 4 | `icons/icon_8240_0_32x32.png` |
| shell32_mun | RT_ICON | 1134 (group 8240) | 16x16 | 4 | `icons/icon_8240_1_16x16.png` |
| shell32_mun | RT_ICON | 1135 (group 222) | 256x256 | 32 | `icons/icon_222_0_256x256.png` |
| shell32_mun | RT_ICON | 1136 (group 222) | 64x64 | 32 | `icons/icon_222_1_64x64.png` |
| shell32_mun | RT_ICON | 1137 (group 222) | 48x48 | 32 | `icons/icon_222_2_48x48.png` |
| shell32_mun | RT_ICON | 1138 (group 222) | 40x40 | 32 | `icons/icon_222_3_40x40.png` |
| shell32_mun | RT_ICON | 1139 (group 222) | 32x32 | 32 | `icons/icon_222_4_32x32.png` |
| shell32_mun | RT_ICON | 114 (group 6) | 32x32 | 32 | `icons/icon_6_4_32x32.png` |
| shell32_mun | RT_ICON | 1140 (group 222) | 24x24 | 32 | `icons/icon_222_5_24x24.png` |
| shell32_mun | RT_ICON | 1141 (group 222) | 20x20 | 32 | `icons/icon_222_6_20x20.png` |
| shell32_mun | RT_ICON | 1142 (group 222) | 16x16 | 32 | `icons/icon_222_7_16x16.png` |
| shell32_mun | RT_ICON | 1143 (group 223) | 32x32 | 1 | `icons/icon_223_0_32x32.png` |
| shell32_mun | RT_ICON | 1144 (group 223) | 32x32 | 4 | `icons/icon_223_1_32x32.png` |
| shell32_mun | RT_ICON | 1145 (group 224) | 256x256 | 32 | `icons/icon_224_0_256x256.png` |
| shell32_mun | RT_ICON | 1146 (group 224) | 64x64 | 32 | `icons/icon_224_1_64x64.png` |
| shell32_mun | RT_ICON | 1147 (group 224) | 48x48 | 32 | `icons/icon_224_2_48x48.png` |
| shell32_mun | RT_ICON | 1148 (group 224) | 40x40 | 32 | `icons/icon_224_3_40x40.png` |
| shell32_mun | RT_ICON | 1149 (group 224) | 32x32 | 32 | `icons/icon_224_4_32x32.png` |
| shell32_mun | RT_ICON | 115 (group 6) | 24x24 | 32 | `icons/icon_6_5_24x24.png` |
| shell32_mun | RT_ICON | 1150 (group 224) | 24x24 | 32 | `icons/icon_224_5_24x24.png` |
| shell32_mun | RT_ICON | 1151 (group 224) | 20x20 | 32 | `icons/icon_224_6_20x20.png` |
| shell32_mun | RT_ICON | 1152 (group 224) | 16x16 | 32 | `icons/icon_224_7_16x16.png` |
| shell32_mun | RT_ICON | 1153 (group 225) | 256x256 | 32 | `icons/icon_225_0_256x256.png` |
| shell32_mun | RT_ICON | 1154 (group 225) | 64x64 | 32 | `icons/icon_225_1_64x64.png` |
| shell32_mun | RT_ICON | 1155 (group 225) | 48x48 | 32 | `icons/icon_225_2_48x48.png` |
| shell32_mun | RT_ICON | 1156 (group 225) | 40x40 | 32 | `icons/icon_225_3_40x40.png` |
| shell32_mun | RT_ICON | 1157 (group 225) | 32x32 | 32 | `icons/icon_225_4_32x32.png` |
| shell32_mun | RT_ICON | 1158 (group 225) | 24x24 | 32 | `icons/icon_225_5_24x24.png` |
| shell32_mun | RT_ICON | 1159 (group 225) | 20x20 | 32 | `icons/icon_225_6_20x20.png` |
| shell32_mun | RT_ICON | 116 (group 6) | 20x20 | 32 | `icons/icon_6_6_20x20.png` |
| shell32_mun | RT_ICON | 1160 (group 225) | 16x16 | 32 | `icons/icon_225_7_16x16.png` |
| shell32_mun | RT_ICON | 1161 (group 226) | 256x256 | 32 | `icons/icon_226_0_256x256.png` |
| shell32_mun | RT_ICON | 1162 (group 226) | 64x64 | 32 | `icons/icon_226_1_64x64.png` |
| shell32_mun | RT_ICON | 1163 (group 226) | 48x48 | 32 | `icons/icon_226_2_48x48.png` |
| shell32_mun | RT_ICON | 1164 (group 226) | 40x40 | 32 | `icons/icon_226_3_40x40.png` |
| shell32_mun | RT_ICON | 1165 (group 226) | 32x32 | 32 | `icons/icon_226_4_32x32.png` |
| shell32_mun | RT_ICON | 1166 (group 226) | 24x24 | 32 | `icons/icon_226_5_24x24.png` |
| shell32_mun | RT_ICON | 1167 (group 226) | 20x20 | 32 | `icons/icon_226_6_20x20.png` |
| shell32_mun | RT_ICON | 1168 (group 226) | 16x16 | 32 | `icons/icon_226_7_16x16.png` |
| shell32_mun | RT_ICON | 1169 (group 227) | 256x256 | 32 | `icons/icon_227_0_256x256.png` |
| shell32_mun | RT_ICON | 117 (group 6) | 16x16 | 32 | `icons/icon_6_7_16x16.png` |
| shell32_mun | RT_ICON | 1170 (group 227) | 64x64 | 32 | `icons/icon_227_1_64x64.png` |
| shell32_mun | RT_ICON | 1171 (group 227) | 48x48 | 32 | `icons/icon_227_2_48x48.png` |
| shell32_mun | RT_ICON | 1172 (group 227) | 40x40 | 32 | `icons/icon_227_3_40x40.png` |
| shell32_mun | RT_ICON | 1173 (group 227) | 32x32 | 32 | `icons/icon_227_4_32x32.png` |
| shell32_mun | RT_ICON | 1174 (group 227) | 24x24 | 32 | `icons/icon_227_5_24x24.png` |
| shell32_mun | RT_ICON | 1175 (group 227) | 20x20 | 32 | `icons/icon_227_6_20x20.png` |
| shell32_mun | RT_ICON | 1176 (group 227) | 16x16 | 32 | `icons/icon_227_7_16x16.png` |
| shell32_mun | RT_ICON | 1177 (group 228) | 256x256 | 32 | `icons/icon_228_0_256x256.png` |
| shell32_mun | RT_ICON | 1178 (group 228) | 64x64 | 32 | `icons/icon_228_1_64x64.png` |
| shell32_mun | RT_ICON | 1179 (group 228) | 48x48 | 32 | `icons/icon_228_2_48x48.png` |
| shell32_mun | RT_ICON | 118 (group 7) | 256x256 | 32 | `icons/icon_7_0_256x256.png` |
| shell32_mun | RT_ICON | 1180 (group 228) | 40x40 | 32 | `icons/icon_228_3_40x40.png` |
| shell32_mun | RT_ICON | 1181 (group 228) | 32x32 | 32 | `icons/icon_228_4_32x32.png` |
| shell32_mun | RT_ICON | 1182 (group 228) | 24x24 | 32 | `icons/icon_228_5_24x24.png` |
| shell32_mun | RT_ICON | 1183 (group 228) | 20x20 | 32 | `icons/icon_228_6_20x20.png` |
| shell32_mun | RT_ICON | 1184 (group 228) | 16x16 | 32 | `icons/icon_228_7_16x16.png` |
| shell32_mun | RT_ICON | 1185 (group 229) | 256x256 | 32 | `icons/icon_229_0_256x256.png` |
| shell32_mun | RT_ICON | 1186 (group 229) | 64x64 | 32 | `icons/icon_229_1_64x64.png` |
| shell32_mun | RT_ICON | 1187 (group 229) | 48x48 | 32 | `icons/icon_229_2_48x48.png` |
| shell32_mun | RT_ICON | 1188 (group 229) | 40x40 | 32 | `icons/icon_229_3_40x40.png` |
| shell32_mun | RT_ICON | 1189 (group 229) | 32x32 | 32 | `icons/icon_229_4_32x32.png` |
| shell32_mun | RT_ICON | 119 (group 7) | 64x64 | 32 | `icons/icon_7_1_64x64.png` |
| shell32_mun | RT_ICON | 1190 (group 229) | 24x24 | 32 | `icons/icon_229_5_24x24.png` |
| shell32_mun | RT_ICON | 1191 (group 229) | 20x20 | 32 | `icons/icon_229_6_20x20.png` |
| shell32_mun | RT_ICON | 1192 (group 229) | 16x16 | 32 | `icons/icon_229_7_16x16.png` |
| shell32_mun | RT_ICON | 1193 (group 230) | 256x256 | 32 | `icons/icon_230_0_256x256.png` |
| shell32_mun | RT_ICON | 1194 (group 230) | 64x64 | 32 | `icons/icon_230_1_64x64.png` |
| shell32_mun | RT_ICON | 1195 (group 230) | 48x48 | 32 | `icons/icon_230_2_48x48.png` |
| shell32_mun | RT_ICON | 1196 (group 230) | 40x40 | 32 | `icons/icon_230_3_40x40.png` |
| shell32_mun | RT_ICON | 1197 (group 230) | 32x32 | 32 | `icons/icon_230_4_32x32.png` |
| shell32_mun | RT_ICON | 1198 (group 230) | 24x24 | 32 | `icons/icon_230_5_24x24.png` |
| shell32_mun | RT_ICON | 1199 (group 230) | 20x20 | 32 | `icons/icon_230_6_20x20.png` |
| shell32_mun | RT_ICON | 12 (group 62999) | 40x40 | 32 | `icons/icon_62999_3_40x40.png` |
| shell32_mun | RT_ICON | 120 (group 7) | 48x48 | 32 | `icons/icon_7_2_48x48.png` |
| shell32_mun | RT_ICON | 1200 (group 230) | 16x16 | 32 | `icons/icon_230_7_16x16.png` |
| shell32_mun | RT_ICON | 1201 (group 234) | 256x256 | 32 | `icons/icon_234_0_256x256.png` |
| shell32_mun | RT_ICON | 1202 (group 234) | 64x64 | 32 | `icons/icon_234_1_64x64.png` |
| shell32_mun | RT_ICON | 1203 (group 234) | 48x48 | 32 | `icons/icon_234_2_48x48.png` |
| shell32_mun | RT_ICON | 1204 (group 234) | 40x40 | 32 | `icons/icon_234_3_40x40.png` |
| shell32_mun | RT_ICON | 1205 (group 234) | 32x32 | 32 | `icons/icon_234_4_32x32.png` |
| shell32_mun | RT_ICON | 1206 (group 234) | 24x24 | 32 | `icons/icon_234_5_24x24.png` |
| shell32_mun | RT_ICON | 1207 (group 234) | 20x20 | 32 | `icons/icon_234_6_20x20.png` |
| shell32_mun | RT_ICON | 1208 (group 234) | 16x16 | 32 | `icons/icon_234_7_16x16.png` |
| shell32_mun | RT_ICON | 1209 (group 16739) | 256x256 | 32 | `icons/icon_16739_0_256x256.png` |
| shell32_mun | RT_ICON | 121 (group 7) | 40x40 | 32 | `icons/icon_7_3_40x40.png` |
| shell32_mun | RT_ICON | 1210 (group 16739) | 64x64 | 32 | `icons/icon_16739_1_64x64.png` |
| shell32_mun | RT_ICON | 1211 (group 16739) | 48x48 | 32 | `icons/icon_16739_2_48x48.png` |
| shell32_mun | RT_ICON | 1212 (group 16739) | 40x40 | 32 | `icons/icon_16739_3_40x40.png` |
| shell32_mun | RT_ICON | 1213 (group 16739) | 32x32 | 32 | `icons/icon_16739_4_32x32.png` |
| shell32_mun | RT_ICON | 1214 (group 16739) | 24x24 | 32 | `icons/icon_16739_5_24x24.png` |
| shell32_mun | RT_ICON | 1215 (group 16739) | 20x20 | 32 | `icons/icon_16739_6_20x20.png` |
| shell32_mun | RT_ICON | 1216 (group 16739) | 16x16 | 32 | `icons/icon_16739_7_16x16.png` |
| shell32_mun | RT_ICON | 1217 (group 235) | 256x256 | 32 | `icons/icon_235_0_256x256.png` |
| shell32_mun | RT_ICON | 1218 (group 235) | 64x64 | 32 | `icons/icon_235_1_64x64.png` |
| shell32_mun | RT_ICON | 1219 (group 235) | 48x48 | 32 | `icons/icon_235_2_48x48.png` |
| shell32_mun | RT_ICON | 122 (group 7) | 32x32 | 32 | `icons/icon_7_4_32x32.png` |
| shell32_mun | RT_ICON | 1220 (group 235) | 40x40 | 32 | `icons/icon_235_3_40x40.png` |
| shell32_mun | RT_ICON | 1221 (group 235) | 32x32 | 32 | `icons/icon_235_4_32x32.png` |
| shell32_mun | RT_ICON | 1222 (group 235) | 24x24 | 32 | `icons/icon_235_5_24x24.png` |
| shell32_mun | RT_ICON | 1223 (group 235) | 20x20 | 32 | `icons/icon_235_6_20x20.png` |
| shell32_mun | RT_ICON | 1224 (group 235) | 16x16 | 32 | `icons/icon_235_7_16x16.png` |
| shell32_mun | RT_ICON | 1225 (group 236) | 256x256 | 32 | `icons/icon_236_0_256x256.png` |
| shell32_mun | RT_ICON | 1226 (group 236) | 64x64 | 32 | `icons/icon_236_1_64x64.png` |
| shell32_mun | RT_ICON | 1227 (group 236) | 48x48 | 32 | `icons/icon_236_2_48x48.png` |
| shell32_mun | RT_ICON | 1228 (group 236) | 40x40 | 32 | `icons/icon_236_3_40x40.png` |
| shell32_mun | RT_ICON | 1229 (group 236) | 32x32 | 32 | `icons/icon_236_4_32x32.png` |
| shell32_mun | RT_ICON | 123 (group 7) | 24x24 | 32 | `icons/icon_7_5_24x24.png` |
| shell32_mun | RT_ICON | 1230 (group 236) | 24x24 | 32 | `icons/icon_236_5_24x24.png` |
| shell32_mun | RT_ICON | 1231 (group 236) | 20x20 | 32 | `icons/icon_236_6_20x20.png` |
| shell32_mun | RT_ICON | 1232 (group 236) | 16x16 | 32 | `icons/icon_236_7_16x16.png` |
| shell32_mun | RT_ICON | 1233 (group 237) | 256x256 | 32 | `icons/icon_237_0_256x256.png` |
| shell32_mun | RT_ICON | 1234 (group 237) | 64x64 | 32 | `icons/icon_237_1_64x64.png` |
| shell32_mun | RT_ICON | 1235 (group 237) | 48x48 | 32 | `icons/icon_237_2_48x48.png` |
| shell32_mun | RT_ICON | 1236 (group 237) | 40x40 | 32 | `icons/icon_237_3_40x40.png` |
| shell32_mun | RT_ICON | 1237 (group 237) | 32x32 | 32 | `icons/icon_237_4_32x32.png` |
| shell32_mun | RT_ICON | 1238 (group 237) | 24x24 | 32 | `icons/icon_237_5_24x24.png` |
| shell32_mun | RT_ICON | 1239 (group 237) | 20x20 | 32 | `icons/icon_237_6_20x20.png` |
| shell32_mun | RT_ICON | 124 (group 7) | 20x20 | 32 | `icons/icon_7_6_20x20.png` |
| shell32_mun | RT_ICON | 1240 (group 237) | 16x16 | 32 | `icons/icon_237_7_16x16.png` |
| shell32_mun | RT_ICON | 1241 (group 238) | 256x256 | 32 | `icons/icon_238_0_256x256.png` |
| shell32_mun | RT_ICON | 1242 (group 238) | 64x64 | 32 | `icons/icon_238_1_64x64.png` |
| shell32_mun | RT_ICON | 1243 (group 238) | 48x48 | 32 | `icons/icon_238_2_48x48.png` |
| shell32_mun | RT_ICON | 1244 (group 238) | 40x40 | 32 | `icons/icon_238_3_40x40.png` |
| shell32_mun | RT_ICON | 1245 (group 238) | 32x32 | 32 | `icons/icon_238_4_32x32.png` |
| shell32_mun | RT_ICON | 1246 (group 238) | 24x24 | 32 | `icons/icon_238_5_24x24.png` |
| shell32_mun | RT_ICON | 1247 (group 238) | 20x20 | 32 | `icons/icon_238_6_20x20.png` |
| shell32_mun | RT_ICON | 1248 (group 238) | 16x16 | 32 | `icons/icon_238_7_16x16.png` |
| shell32_mun | RT_ICON | 1249 (group 239) | 48x48 | 4 | `icons/icon_239_0_48x48.png` |
| shell32_mun | RT_ICON | 125 (group 7) | 16x16 | 32 | `icons/icon_7_7_16x16.png` |
| shell32_mun | RT_ICON | 1250 (group 239) | 32x32 | 4 | `icons/icon_239_1_32x32.png` |
| shell32_mun | RT_ICON | 1251 (group 239) | 16x16 | 4 | `icons/icon_239_2_16x16.png` |
| shell32_mun | RT_ICON | 1252 (group 239) | 48x48 | 8 | `icons/icon_239_3_48x48.png` |
| shell32_mun | RT_ICON | 1253 (group 239) | 32x32 | 8 | `icons/icon_239_4_32x32.png` |
| shell32_mun | RT_ICON | 1254 (group 239) | 16x16 | 8 | `icons/icon_239_5_16x16.png` |
| shell32_mun | RT_ICON | 1255 (group 239) | 48x48 | 32 | `icons/icon_239_6_48x48.png` |
| shell32_mun | RT_ICON | 1256 (group 239) | 32x32 | 32 | `icons/icon_239_7_32x32.png` |
| shell32_mun | RT_ICON | 1257 (group 239) | 16x16 | 32 | `icons/icon_239_8_16x16.png` |
| shell32_mun | RT_ICON | 1258 (group 16743) | 32x32 | 8 | `icons/icon_16743_0_32x32.png` |
| shell32_mun | RT_ICON | 1259 (group 16753) | 16x16 | 4 | `icons/icon_16753_0_16x16.png` |
| shell32_mun | RT_ICON | 126 (group 8) | 256x256 | 32 | `icons/icon_8_0_256x256.png` |
| shell32_mun | RT_ICON | 1260 (group 16753) | 16x16 | 32 | `icons/icon_16753_1_16x16.png` |
| shell32_mun | RT_ICON | 1261 (group 240) | 256x256 | 32 | `icons/icon_240_0_256x256.png` |
| shell32_mun | RT_ICON | 1262 (group 240) | 64x64 | 32 | `icons/icon_240_1_64x64.png` |
| shell32_mun | RT_ICON | 1263 (group 240) | 48x48 | 32 | `icons/icon_240_2_48x48.png` |
| shell32_mun | RT_ICON | 1264 (group 240) | 40x40 | 32 | `icons/icon_240_3_40x40.png` |
| shell32_mun | RT_ICON | 1265 (group 240) | 32x32 | 32 | `icons/icon_240_4_32x32.png` |
| shell32_mun | RT_ICON | 1266 (group 240) | 24x24 | 32 | `icons/icon_240_5_24x24.png` |
| shell32_mun | RT_ICON | 1267 (group 240) | 20x20 | 32 | `icons/icon_240_6_20x20.png` |
| shell32_mun | RT_ICON | 1268 (group 240) | 16x16 | 32 | `icons/icon_240_7_16x16.png` |
| shell32_mun | RT_ICON | 1269 (group 242) | 256x256 | 32 | `icons/icon_242_0_256x256.png` |
| shell32_mun | RT_ICON | 127 (group 8) | 64x64 | 32 | `icons/icon_8_1_64x64.png` |
| shell32_mun | RT_ICON | 1270 (group 242) | 64x64 | 32 | `icons/icon_242_1_64x64.png` |
| shell32_mun | RT_ICON | 1271 (group 242) | 48x48 | 32 | `icons/icon_242_2_48x48.png` |
| shell32_mun | RT_ICON | 1272 (group 242) | 40x40 | 32 | `icons/icon_242_3_40x40.png` |
| shell32_mun | RT_ICON | 1273 (group 242) | 32x32 | 32 | `icons/icon_242_4_32x32.png` |
| shell32_mun | RT_ICON | 1274 (group 242) | 24x24 | 32 | `icons/icon_242_5_24x24.png` |
| shell32_mun | RT_ICON | 1275 (group 242) | 20x20 | 32 | `icons/icon_242_6_20x20.png` |
| shell32_mun | RT_ICON | 1276 (group 242) | 16x16 | 32 | `icons/icon_242_7_16x16.png` |
| shell32_mun | RT_ICON | 1277 (group 241) | 256x256 | 32 | `icons/icon_241_0_256x256.png` |
| shell32_mun | RT_ICON | 1278 (group 241) | 64x64 | 32 | `icons/icon_241_1_64x64.png` |
| shell32_mun | RT_ICON | 1279 (group 241) | 48x48 | 32 | `icons/icon_241_2_48x48.png` |
| shell32_mun | RT_ICON | 128 (group 8) | 48x48 | 32 | `icons/icon_8_2_48x48.png` |
| shell32_mun | RT_ICON | 1280 (group 241) | 40x40 | 32 | `icons/icon_241_3_40x40.png` |
| shell32_mun | RT_ICON | 1281 (group 241) | 32x32 | 32 | `icons/icon_241_4_32x32.png` |
| shell32_mun | RT_ICON | 1282 (group 241) | 24x24 | 32 | `icons/icon_241_5_24x24.png` |
| shell32_mun | RT_ICON | 1283 (group 241) | 20x20 | 32 | `icons/icon_241_6_20x20.png` |
| shell32_mun | RT_ICON | 1284 (group 241) | 16x16 | 32 | `icons/icon_241_7_16x16.png` |
| shell32_mun | RT_ICON | 1285 (group 243) | 256x256 | 32 | `icons/icon_243_0_256x256.png` |
| shell32_mun | RT_ICON | 1286 (group 243) | 64x64 | 32 | `icons/icon_243_1_64x64.png` |
| shell32_mun | RT_ICON | 1287 (group 243) | 48x48 | 32 | `icons/icon_243_2_48x48.png` |
| shell32_mun | RT_ICON | 1288 (group 243) | 40x40 | 32 | `icons/icon_243_3_40x40.png` |
| shell32_mun | RT_ICON | 1289 (group 243) | 32x32 | 32 | `icons/icon_243_4_32x32.png` |
| shell32_mun | RT_ICON | 129 (group 8) | 40x40 | 32 | `icons/icon_8_3_40x40.png` |
| shell32_mun | RT_ICON | 1290 (group 243) | 24x24 | 32 | `icons/icon_243_5_24x24.png` |
| shell32_mun | RT_ICON | 1291 (group 243) | 20x20 | 32 | `icons/icon_243_6_20x20.png` |
| shell32_mun | RT_ICON | 1292 (group 243) | 16x16 | 32 | `icons/icon_243_7_16x16.png` |
| shell32_mun | RT_ICON | 1293 (group 16762) | 256x256 | 32 | `icons/icon_16762_0_256x256.png` |
| shell32_mun | RT_ICON | 1294 (group 16762) | 64x64 | 32 | `icons/icon_16762_1_64x64.png` |
| shell32_mun | RT_ICON | 1295 (group 16762) | 48x48 | 32 | `icons/icon_16762_2_48x48.png` |
| shell32_mun | RT_ICON | 1296 (group 16762) | 40x40 | 32 | `icons/icon_16762_3_40x40.png` |
| shell32_mun | RT_ICON | 1297 (group 16762) | 32x32 | 32 | `icons/icon_16762_4_32x32.png` |
| shell32_mun | RT_ICON | 1298 (group 16762) | 24x24 | 32 | `icons/icon_16762_5_24x24.png` |
| shell32_mun | RT_ICON | 1299 (group 16762) | 20x20 | 32 | `icons/icon_16762_6_20x20.png` |
| shell32_mun | RT_ICON | 13 (group 62999) | 32x32 | 32 | `icons/icon_62999_4_32x32.png` |
| shell32_mun | RT_ICON | 130 (group 8) | 32x32 | 32 | `icons/icon_8_4_32x32.png` |
| shell32_mun | RT_ICON | 1300 (group 16762) | 16x16 | 32 | `icons/icon_16762_7_16x16.png` |
| shell32_mun | RT_ICON | 1301 (group 16763) | 256x256 | 32 | `icons/icon_16763_0_256x256.png` |
| shell32_mun | RT_ICON | 1302 (group 16763) | 64x64 | 32 | `icons/icon_16763_1_64x64.png` |
| shell32_mun | RT_ICON | 1303 (group 16763) | 48x48 | 32 | `icons/icon_16763_2_48x48.png` |
| shell32_mun | RT_ICON | 1304 (group 16763) | 40x40 | 32 | `icons/icon_16763_3_40x40.png` |
| shell32_mun | RT_ICON | 1305 (group 16763) | 32x32 | 32 | `icons/icon_16763_4_32x32.png` |
| shell32_mun | RT_ICON | 1306 (group 16763) | 24x24 | 32 | `icons/icon_16763_5_24x24.png` |
| shell32_mun | RT_ICON | 1307 (group 16763) | 20x20 | 32 | `icons/icon_16763_6_20x20.png` |
| shell32_mun | RT_ICON | 1308 (group 16763) | 16x16 | 32 | `icons/icon_16763_7_16x16.png` |
| shell32_mun | RT_ICON | 1309 (group 244) | 256x256 | 32 | `icons/icon_244_0_256x256.png` |
| shell32_mun | RT_ICON | 131 (group 8) | 24x24 | 32 | `icons/icon_8_5_24x24.png` |
| shell32_mun | RT_ICON | 1310 (group 244) | 64x64 | 32 | `icons/icon_244_1_64x64.png` |
| shell32_mun | RT_ICON | 1311 (group 244) | 48x48 | 32 | `icons/icon_244_2_48x48.png` |
| shell32_mun | RT_ICON | 1312 (group 244) | 40x40 | 32 | `icons/icon_244_3_40x40.png` |
| shell32_mun | RT_ICON | 1313 (group 244) | 32x32 | 32 | `icons/icon_244_4_32x32.png` |
| shell32_mun | RT_ICON | 1314 (group 244) | 24x24 | 32 | `icons/icon_244_5_24x24.png` |
| shell32_mun | RT_ICON | 1315 (group 244) | 20x20 | 32 | `icons/icon_244_6_20x20.png` |
| shell32_mun | RT_ICON | 1316 (group 244) | 16x16 | 32 | `icons/icon_244_7_16x16.png` |
| shell32_mun | RT_ICON | 1317 (group 245) | 256x256 | 32 | `icons/icon_245_0_256x256.png` |
| shell32_mun | RT_ICON | 1318 (group 245) | 64x64 | 32 | `icons/icon_245_1_64x64.png` |
| shell32_mun | RT_ICON | 1319 (group 245) | 48x48 | 32 | `icons/icon_245_2_48x48.png` |
| shell32_mun | RT_ICON | 132 (group 8) | 20x20 | 32 | `icons/icon_8_6_20x20.png` |
| shell32_mun | RT_ICON | 1320 (group 245) | 40x40 | 32 | `icons/icon_245_3_40x40.png` |
| shell32_mun | RT_ICON | 1321 (group 245) | 32x32 | 32 | `icons/icon_245_4_32x32.png` |
| shell32_mun | RT_ICON | 1322 (group 245) | 24x24 | 32 | `icons/icon_245_5_24x24.png` |
| shell32_mun | RT_ICON | 1323 (group 245) | 20x20 | 32 | `icons/icon_245_6_20x20.png` |
| shell32_mun | RT_ICON | 1324 (group 245) | 16x16 | 32 | `icons/icon_245_7_16x16.png` |
| shell32_mun | RT_ICON | 1325 (group 246) | 256x256 | 32 | `icons/icon_246_0_256x256.png` |
| shell32_mun | RT_ICON | 1326 (group 246) | 64x64 | 32 | `icons/icon_246_1_64x64.png` |
| shell32_mun | RT_ICON | 1327 (group 246) | 48x48 | 32 | `icons/icon_246_2_48x48.png` |
| shell32_mun | RT_ICON | 1328 (group 246) | 40x40 | 32 | `icons/icon_246_3_40x40.png` |
| shell32_mun | RT_ICON | 1329 (group 246) | 32x32 | 32 | `icons/icon_246_4_32x32.png` |
| shell32_mun | RT_ICON | 133 (group 8) | 16x16 | 32 | `icons/icon_8_7_16x16.png` |
| shell32_mun | RT_ICON | 1330 (group 246) | 24x24 | 32 | `icons/icon_246_5_24x24.png` |
| shell32_mun | RT_ICON | 1331 (group 246) | 20x20 | 32 | `icons/icon_246_6_20x20.png` |
| shell32_mun | RT_ICON | 1332 (group 246) | 16x16 | 32 | `icons/icon_246_7_16x16.png` |
| shell32_mun | RT_ICON | 1333 (group 247) | 256x256 | 32 | `icons/icon_247_0_256x256.png` |
| shell32_mun | RT_ICON | 1334 (group 247) | 64x64 | 32 | `icons/icon_247_1_64x64.png` |
| shell32_mun | RT_ICON | 1335 (group 247) | 48x48 | 32 | `icons/icon_247_2_48x48.png` |
| shell32_mun | RT_ICON | 1336 (group 247) | 40x40 | 32 | `icons/icon_247_3_40x40.png` |
| shell32_mun | RT_ICON | 1337 (group 247) | 32x32 | 32 | `icons/icon_247_4_32x32.png` |
| shell32_mun | RT_ICON | 1338 (group 247) | 24x24 | 32 | `icons/icon_247_5_24x24.png` |
| shell32_mun | RT_ICON | 1339 (group 247) | 20x20 | 32 | `icons/icon_247_6_20x20.png` |
| shell32_mun | RT_ICON | 134 (group 9) | 256x256 | 32 | `icons/icon_9_0_256x256.png` |
| shell32_mun | RT_ICON | 1340 (group 247) | 16x16 | 32 | `icons/icon_247_7_16x16.png` |
| shell32_mun | RT_ICON | 1341 (group 248) | 256x256 | 32 | `icons/icon_248_0_256x256.png` |
| shell32_mun | RT_ICON | 1342 (group 248) | 64x64 | 32 | `icons/icon_248_1_64x64.png` |
| shell32_mun | RT_ICON | 1343 (group 248) | 48x48 | 32 | `icons/icon_248_2_48x48.png` |
| shell32_mun | RT_ICON | 1344 (group 248) | 40x40 | 32 | `icons/icon_248_3_40x40.png` |
| shell32_mun | RT_ICON | 1345 (group 248) | 32x32 | 32 | `icons/icon_248_4_32x32.png` |
| shell32_mun | RT_ICON | 1346 (group 248) | 24x24 | 32 | `icons/icon_248_5_24x24.png` |
| shell32_mun | RT_ICON | 1347 (group 248) | 20x20 | 32 | `icons/icon_248_6_20x20.png` |
| shell32_mun | RT_ICON | 1348 (group 248) | 16x16 | 32 | `icons/icon_248_7_16x16.png` |
| shell32_mun | RT_ICON | 1349 (group 249) | 256x256 | 32 | `icons/icon_249_0_256x256.png` |
| shell32_mun | RT_ICON | 135 (group 9) | 64x64 | 32 | `icons/icon_9_1_64x64.png` |
| shell32_mun | RT_ICON | 1350 (group 249) | 64x64 | 32 | `icons/icon_249_1_64x64.png` |
| shell32_mun | RT_ICON | 1351 (group 249) | 48x48 | 32 | `icons/icon_249_2_48x48.png` |
| shell32_mun | RT_ICON | 1352 (group 249) | 40x40 | 32 | `icons/icon_249_3_40x40.png` |
| shell32_mun | RT_ICON | 1353 (group 249) | 32x32 | 32 | `icons/icon_249_4_32x32.png` |
| shell32_mun | RT_ICON | 1354 (group 249) | 24x24 | 32 | `icons/icon_249_5_24x24.png` |
| shell32_mun | RT_ICON | 1355 (group 249) | 20x20 | 32 | `icons/icon_249_6_20x20.png` |
| shell32_mun | RT_ICON | 1356 (group 249) | 16x16 | 32 | `icons/icon_249_7_16x16.png` |
| shell32_mun | RT_ICON | 1357 (group 250) | 48x48 | 4 | `icons/icon_250_0_48x48.png` |
| shell32_mun | RT_ICON | 1358 (group 250) | 32x32 | 4 | `icons/icon_250_1_32x32.png` |
| shell32_mun | RT_ICON | 1359 (group 250) | 16x16 | 4 | `icons/icon_250_2_16x16.png` |
| shell32_mun | RT_ICON | 136 (group 9) | 48x48 | 32 | `icons/icon_9_2_48x48.png` |
| shell32_mun | RT_ICON | 1360 (group 250) | 48x48 | 8 | `icons/icon_250_3_48x48.png` |
| shell32_mun | RT_ICON | 1361 (group 250) | 32x32 | 8 | `icons/icon_250_4_32x32.png` |
| shell32_mun | RT_ICON | 1362 (group 250) | 16x16 | 8 | `icons/icon_250_5_16x16.png` |
| shell32_mun | RT_ICON | 1363 (group 250) | 48x48 | 32 | `icons/icon_250_6_48x48.png` |
| shell32_mun | RT_ICON | 1364 (group 250) | 32x32 | 32 | `icons/icon_250_7_32x32.png` |
| shell32_mun | RT_ICON | 1365 (group 250) | 16x16 | 32 | `icons/icon_250_8_16x16.png` |
| shell32_mun | RT_ICON | 1366 (group 251) | 256x256 | 32 | `icons/icon_251_0_256x256.png` |
| shell32_mun | RT_ICON | 1367 (group 251) | 64x64 | 32 | `icons/icon_251_1_64x64.png` |
| shell32_mun | RT_ICON | 1368 (group 251) | 48x48 | 32 | `icons/icon_251_2_48x48.png` |
| shell32_mun | RT_ICON | 1369 (group 251) | 40x40 | 32 | `icons/icon_251_3_40x40.png` |
| shell32_mun | RT_ICON | 137 (group 9) | 40x40 | 32 | `icons/icon_9_3_40x40.png` |
| shell32_mun | RT_ICON | 1370 (group 251) | 32x32 | 32 | `icons/icon_251_4_32x32.png` |
| shell32_mun | RT_ICON | 1371 (group 251) | 24x24 | 32 | `icons/icon_251_5_24x24.png` |
| shell32_mun | RT_ICON | 1372 (group 251) | 20x20 | 32 | `icons/icon_251_6_20x20.png` |
| shell32_mun | RT_ICON | 1373 (group 251) | 16x16 | 32 | `icons/icon_251_7_16x16.png` |
| shell32_mun | RT_ICON | 1374 (group 16767) | 256x256 | 32 | `icons/icon_16767_0_256x256.png` |
| shell32_mun | RT_ICON | 1375 (group 16767) | 64x64 | 32 | `icons/icon_16767_1_64x64.png` |
| shell32_mun | RT_ICON | 1376 (group 16767) | 48x48 | 32 | `icons/icon_16767_2_48x48.png` |
| shell32_mun | RT_ICON | 1377 (group 16767) | 40x40 | 32 | `icons/icon_16767_3_40x40.png` |
| shell32_mun | RT_ICON | 1378 (group 16767) | 32x32 | 32 | `icons/icon_16767_4_32x32.png` |
| shell32_mun | RT_ICON | 1379 (group 16767) | 24x24 | 32 | `icons/icon_16767_5_24x24.png` |
| shell32_mun | RT_ICON | 138 (group 9) | 32x32 | 32 | `icons/icon_9_4_32x32.png` |
| shell32_mun | RT_ICON | 1380 (group 16767) | 20x20 | 32 | `icons/icon_16767_6_20x20.png` |
| shell32_mun | RT_ICON | 1381 (group 16767) | 16x16 | 32 | `icons/icon_16767_7_16x16.png` |
| shell32_mun | RT_ICON | 1382 (group 16768) | 256x256 | 32 | `icons/icon_16768_0_256x256.png` |
| shell32_mun | RT_ICON | 1383 (group 16768) | 64x64 | 32 | `icons/icon_16768_1_64x64.png` |
| shell32_mun | RT_ICON | 1384 (group 16768) | 48x48 | 32 | `icons/icon_16768_2_48x48.png` |
| shell32_mun | RT_ICON | 1385 (group 16768) | 40x40 | 32 | `icons/icon_16768_3_40x40.png` |
| shell32_mun | RT_ICON | 1386 (group 16768) | 32x32 | 32 | `icons/icon_16768_4_32x32.png` |
| shell32_mun | RT_ICON | 1387 (group 16768) | 24x24 | 32 | `icons/icon_16768_5_24x24.png` |
| shell32_mun | RT_ICON | 1388 (group 16768) | 20x20 | 32 | `icons/icon_16768_6_20x20.png` |
| shell32_mun | RT_ICON | 1389 (group 16768) | 16x16 | 32 | `icons/icon_16768_7_16x16.png` |
| shell32_mun | RT_ICON | 139 (group 9) | 24x24 | 32 | `icons/icon_9_5_24x24.png` |
| shell32_mun | RT_ICON | 1390 (group 253) | 256x256 | 32 | `icons/icon_253_0_256x256.png` |
| shell32_mun | RT_ICON | 1391 (group 253) | 64x64 | 32 | `icons/icon_253_1_64x64.png` |
| shell32_mun | RT_ICON | 1392 (group 253) | 48x48 | 32 | `icons/icon_253_2_48x48.png` |
| shell32_mun | RT_ICON | 1393 (group 253) | 40x40 | 32 | `icons/icon_253_3_40x40.png` |
| shell32_mun | RT_ICON | 1394 (group 253) | 32x32 | 32 | `icons/icon_253_4_32x32.png` |
| shell32_mun | RT_ICON | 1395 (group 253) | 24x24 | 32 | `icons/icon_253_5_24x24.png` |
| shell32_mun | RT_ICON | 1396 (group 253) | 20x20 | 32 | `icons/icon_253_6_20x20.png` |
| shell32_mun | RT_ICON | 1397 (group 253) | 16x16 | 32 | `icons/icon_253_7_16x16.png` |
| shell32_mun | RT_ICON | 1398 (group 254) | 256x256 | 32 | `icons/icon_254_0_256x256.png` |
| shell32_mun | RT_ICON | 1399 (group 254) | 64x64 | 32 | `icons/icon_254_1_64x64.png` |
| shell32_mun | RT_ICON | 14 (group 62999) | 24x24 | 32 | `icons/icon_62999_5_24x24.png` |
| shell32_mun | RT_ICON | 140 (group 9) | 20x20 | 32 | `icons/icon_9_6_20x20.png` |
| shell32_mun | RT_ICON | 1400 (group 254) | 48x48 | 32 | `icons/icon_254_2_48x48.png` |
| shell32_mun | RT_ICON | 1401 (group 254) | 40x40 | 32 | `icons/icon_254_3_40x40.png` |
| shell32_mun | RT_ICON | 1402 (group 254) | 32x32 | 32 | `icons/icon_254_4_32x32.png` |
| shell32_mun | RT_ICON | 1403 (group 254) | 24x24 | 32 | `icons/icon_254_5_24x24.png` |
| shell32_mun | RT_ICON | 1404 (group 254) | 20x20 | 32 | `icons/icon_254_6_20x20.png` |
| shell32_mun | RT_ICON | 1405 (group 254) | 16x16 | 32 | `icons/icon_254_7_16x16.png` |
| shell32_mun | RT_ICON | 1406 (group 255) | 256x256 | 32 | `icons/icon_255_0_256x256.png` |
| shell32_mun | RT_ICON | 1407 (group 255) | 64x64 | 32 | `icons/icon_255_1_64x64.png` |
| shell32_mun | RT_ICON | 1408 (group 255) | 48x48 | 32 | `icons/icon_255_2_48x48.png` |
| shell32_mun | RT_ICON | 1409 (group 255) | 40x40 | 32 | `icons/icon_255_3_40x40.png` |
| shell32_mun | RT_ICON | 141 (group 9) | 16x16 | 32 | `icons/icon_9_7_16x16.png` |
| shell32_mun | RT_ICON | 1410 (group 255) | 32x32 | 32 | `icons/icon_255_4_32x32.png` |
| shell32_mun | RT_ICON | 1411 (group 255) | 24x24 | 32 | `icons/icon_255_5_24x24.png` |
| shell32_mun | RT_ICON | 1412 (group 255) | 20x20 | 32 | `icons/icon_255_6_20x20.png` |
| shell32_mun | RT_ICON | 1413 (group 255) | 16x16 | 32 | `icons/icon_255_7_16x16.png` |
| shell32_mun | RT_ICON | 1414 (group 256) | 256x256 | 32 | `icons/icon_256_0_256x256.png` |
| shell32_mun | RT_ICON | 1415 (group 256) | 64x64 | 32 | `icons/icon_256_1_64x64.png` |
| shell32_mun | RT_ICON | 1416 (group 256) | 48x48 | 32 | `icons/icon_256_2_48x48.png` |
| shell32_mun | RT_ICON | 1417 (group 256) | 40x40 | 32 | `icons/icon_256_3_40x40.png` |
| shell32_mun | RT_ICON | 1418 (group 256) | 32x32 | 32 | `icons/icon_256_4_32x32.png` |
| shell32_mun | RT_ICON | 1419 (group 256) | 24x24 | 32 | `icons/icon_256_5_24x24.png` |
| shell32_mun | RT_ICON | 142 (group 10) | 256x256 | 32 | `icons/icon_10_0_256x256.png` |
| shell32_mun | RT_ICON | 1420 (group 256) | 20x20 | 32 | `icons/icon_256_6_20x20.png` |
| shell32_mun | RT_ICON | 1421 (group 256) | 16x16 | 32 | `icons/icon_256_7_16x16.png` |
| shell32_mun | RT_ICON | 1422 (group 257) | 256x256 | 32 | `icons/icon_257_0_256x256.png` |
| shell32_mun | RT_ICON | 1423 (group 257) | 64x64 | 32 | `icons/icon_257_1_64x64.png` |
| shell32_mun | RT_ICON | 1424 (group 257) | 48x48 | 32 | `icons/icon_257_2_48x48.png` |
| shell32_mun | RT_ICON | 1425 (group 257) | 40x40 | 32 | `icons/icon_257_3_40x40.png` |
| shell32_mun | RT_ICON | 1426 (group 257) | 32x32 | 32 | `icons/icon_257_4_32x32.png` |
| shell32_mun | RT_ICON | 1427 (group 257) | 24x24 | 32 | `icons/icon_257_5_24x24.png` |
| shell32_mun | RT_ICON | 1428 (group 257) | 20x20 | 32 | `icons/icon_257_6_20x20.png` |
| shell32_mun | RT_ICON | 1429 (group 257) | 16x16 | 32 | `icons/icon_257_7_16x16.png` |
| shell32_mun | RT_ICON | 143 (group 10) | 64x64 | 32 | `icons/icon_10_1_64x64.png` |
| shell32_mun | RT_ICON | 1430 (group 258) | 256x256 | 32 | `icons/icon_258_0_256x256.png` |
| shell32_mun | RT_ICON | 1431 (group 258) | 64x64 | 32 | `icons/icon_258_1_64x64.png` |
| shell32_mun | RT_ICON | 1432 (group 258) | 48x48 | 32 | `icons/icon_258_2_48x48.png` |
| shell32_mun | RT_ICON | 1433 (group 258) | 40x40 | 32 | `icons/icon_258_3_40x40.png` |
| shell32_mun | RT_ICON | 1434 (group 258) | 32x32 | 32 | `icons/icon_258_4_32x32.png` |
| shell32_mun | RT_ICON | 1435 (group 258) | 24x24 | 32 | `icons/icon_258_5_24x24.png` |
| shell32_mun | RT_ICON | 1436 (group 258) | 20x20 | 32 | `icons/icon_258_6_20x20.png` |
| shell32_mun | RT_ICON | 1437 (group 258) | 16x16 | 32 | `icons/icon_258_7_16x16.png` |
| shell32_mun | RT_ICON | 1438 (group 259) | 48x48 | 8 | `icons/icon_259_0_48x48.png` |
| shell32_mun | RT_ICON | 1439 (group 259) | 32x32 | 8 | `icons/icon_259_1_32x32.png` |
| shell32_mun | RT_ICON | 144 (group 10) | 48x48 | 32 | `icons/icon_10_2_48x48.png` |
| shell32_mun | RT_ICON | 1440 (group 259) | 16x16 | 8 | `icons/icon_259_2_16x16.png` |
| shell32_mun | RT_ICON | 1441 (group 259) | 48x48 | 32 | `icons/icon_259_3_48x48.png` |
| shell32_mun | RT_ICON | 1442 (group 259) | 32x32 | 32 | `icons/icon_259_4_32x32.png` |
| shell32_mun | RT_ICON | 1443 (group 259) | 16x16 | 32 | `icons/icon_259_5_16x16.png` |
| shell32_mun | RT_ICON | 1444 (group 16782) | 256x256 | 32 | `icons/icon_16782_0_256x256.png` |
| shell32_mun | RT_ICON | 1445 (group 16782) | 64x64 | 32 | `icons/icon_16782_1_64x64.png` |
| shell32_mun | RT_ICON | 1446 (group 16782) | 48x48 | 32 | `icons/icon_16782_2_48x48.png` |
| shell32_mun | RT_ICON | 1447 (group 16782) | 40x40 | 32 | `icons/icon_16782_3_40x40.png` |
| shell32_mun | RT_ICON | 1448 (group 16782) | 32x32 | 32 | `icons/icon_16782_4_32x32.png` |
| shell32_mun | RT_ICON | 1449 (group 16782) | 24x24 | 32 | `icons/icon_16782_5_24x24.png` |
| shell32_mun | RT_ICON | 145 (group 10) | 40x40 | 32 | `icons/icon_10_3_40x40.png` |
| shell32_mun | RT_ICON | 1450 (group 16782) | 20x20 | 32 | `icons/icon_16782_6_20x20.png` |
| shell32_mun | RT_ICON | 1451 (group 16782) | 16x16 | 32 | `icons/icon_16782_7_16x16.png` |
| shell32_mun | RT_ICON | 1452 (group 260) | 256x256 | 32 | `icons/icon_260_0_256x256.png` |
| shell32_mun | RT_ICON | 1453 (group 260) | 64x64 | 32 | `icons/icon_260_1_64x64.png` |
| shell32_mun | RT_ICON | 1454 (group 260) | 48x48 | 32 | `icons/icon_260_2_48x48.png` |
| shell32_mun | RT_ICON | 1455 (group 260) | 40x40 | 32 | `icons/icon_260_3_40x40.png` |
| shell32_mun | RT_ICON | 1456 (group 260) | 32x32 | 32 | `icons/icon_260_4_32x32.png` |
| shell32_mun | RT_ICON | 1457 (group 260) | 24x24 | 32 | `icons/icon_260_5_24x24.png` |
| shell32_mun | RT_ICON | 1458 (group 260) | 20x20 | 32 | `icons/icon_260_6_20x20.png` |
| shell32_mun | RT_ICON | 1459 (group 260) | 16x16 | 32 | `icons/icon_260_7_16x16.png` |
| shell32_mun | RT_ICON | 146 (group 10) | 32x32 | 32 | `icons/icon_10_4_32x32.png` |
| shell32_mun | RT_ICON | 1460 (group 261) | 256x256 | 32 | `icons/icon_261_0_256x256.png` |
| shell32_mun | RT_ICON | 1461 (group 261) | 64x64 | 32 | `icons/icon_261_1_64x64.png` |
| shell32_mun | RT_ICON | 1462 (group 261) | 48x48 | 32 | `icons/icon_261_2_48x48.png` |
| shell32_mun | RT_ICON | 1463 (group 261) | 40x40 | 32 | `icons/icon_261_3_40x40.png` |
| shell32_mun | RT_ICON | 1464 (group 261) | 32x32 | 32 | `icons/icon_261_4_32x32.png` |
| shell32_mun | RT_ICON | 1465 (group 261) | 24x24 | 32 | `icons/icon_261_5_24x24.png` |
| shell32_mun | RT_ICON | 1466 (group 261) | 20x20 | 32 | `icons/icon_261_6_20x20.png` |
| shell32_mun | RT_ICON | 1467 (group 261) | 16x16 | 32 | `icons/icon_261_7_16x16.png` |
| shell32_mun | RT_ICON | 1468 (group 262) | 64x64 | 4 | `icons/icon_262_0_64x64.png` |
| shell32_mun | RT_ICON | 1469 (group 262) | 48x48 | 4 | `icons/icon_262_1_48x48.png` |
| shell32_mun | RT_ICON | 147 (group 10) | 24x24 | 32 | `icons/icon_10_5_24x24.png` |
| shell32_mun | RT_ICON | 1470 (group 262) | 40x40 | 4 | `icons/icon_262_2_40x40.png` |
| shell32_mun | RT_ICON | 1471 (group 262) | 32x32 | 4 | `icons/icon_262_3_32x32.png` |
| shell32_mun | RT_ICON | 1472 (group 262) | 24x24 | 4 | `icons/icon_262_4_24x24.png` |
| shell32_mun | RT_ICON | 1473 (group 262) | 20x20 | 4 | `icons/icon_262_5_20x20.png` |
| shell32_mun | RT_ICON | 1474 (group 262) | 16x16 | 4 | `icons/icon_262_6_16x16.png` |
| shell32_mun | RT_ICON | 1475 (group 262) | 64x64 | 8 | `icons/icon_262_7_64x64.png` |
| shell32_mun | RT_ICON | 1476 (group 262) | 48x48 | 8 | `icons/icon_262_8_48x48.png` |
| shell32_mun | RT_ICON | 1477 (group 262) | 40x40 | 8 | `icons/icon_262_9_40x40.png` |
| shell32_mun | RT_ICON | 1478 (group 262) | 32x32 | 8 | `icons/icon_262_10_32x32.png` |
| shell32_mun | RT_ICON | 1479 (group 262) | 24x24 | 8 | `icons/icon_262_11_24x24.png` |
| shell32_mun | RT_ICON | 148 (group 10) | 20x20 | 32 | `icons/icon_10_6_20x20.png` |
| shell32_mun | RT_ICON | 1480 (group 262) | 20x20 | 8 | `icons/icon_262_12_20x20.png` |
| shell32_mun | RT_ICON | 1481 (group 262) | 16x16 | 8 | `icons/icon_262_13_16x16.png` |
| shell32_mun | RT_ICON | 1482 (group 262) | 64x64 | 32 | `icons/icon_262_14_64x64.png` |
| shell32_mun | RT_ICON | 1483 (group 262) | 48x48 | 32 | `icons/icon_262_15_48x48.png` |
| shell32_mun | RT_ICON | 1484 (group 262) | 40x40 | 32 | `icons/icon_262_16_40x40.png` |
| shell32_mun | RT_ICON | 1485 (group 262) | 32x32 | 32 | `icons/icon_262_17_32x32.png` |
| shell32_mun | RT_ICON | 1486 (group 262) | 24x24 | 32 | `icons/icon_262_18_24x24.png` |
| shell32_mun | RT_ICON | 1487 (group 262) | 20x20 | 32 | `icons/icon_262_19_20x20.png` |
| shell32_mun | RT_ICON | 1488 (group 262) | 16x16 | 32 | `icons/icon_262_20_16x16.png` |
| shell32_mun | RT_ICON | 1489 (group 263) | 256x256 | 32 | `icons/icon_263_0_256x256.png` |
| shell32_mun | RT_ICON | 149 (group 10) | 16x16 | 32 | `icons/icon_10_7_16x16.png` |
| shell32_mun | RT_ICON | 1490 (group 263) | 64x64 | 32 | `icons/icon_263_1_64x64.png` |
| shell32_mun | RT_ICON | 1491 (group 263) | 48x48 | 32 | `icons/icon_263_2_48x48.png` |
| shell32_mun | RT_ICON | 1492 (group 263) | 40x40 | 32 | `icons/icon_263_3_40x40.png` |
| shell32_mun | RT_ICON | 1493 (group 263) | 32x32 | 32 | `icons/icon_263_4_32x32.png` |
| shell32_mun | RT_ICON | 1494 (group 263) | 24x24 | 32 | `icons/icon_263_5_24x24.png` |
| shell32_mun | RT_ICON | 1495 (group 263) | 20x20 | 32 | `icons/icon_263_6_20x20.png` |
| shell32_mun | RT_ICON | 1496 (group 263) | 16x16 | 32 | `icons/icon_263_7_16x16.png` |
| shell32_mun | RT_ICON | 1497 (group 264) | 256x256 | 32 | `icons/icon_264_0_256x256.png` |
| shell32_mun | RT_ICON | 1498 (group 264) | 64x64 | 32 | `icons/icon_264_1_64x64.png` |
| shell32_mun | RT_ICON | 1499 (group 264) | 48x48 | 32 | `icons/icon_264_2_48x48.png` |
| shell32_mun | RT_ICON | 15 (group 62999) | 20x20 | 32 | `icons/icon_62999_6_20x20.png` |
| shell32_mun | RT_ICON | 150 (group 11) | 256x256 | 32 | `icons/icon_11_0_256x256.png` |
| shell32_mun | RT_ICON | 1500 (group 264) | 40x40 | 32 | `icons/icon_264_3_40x40.png` |
| shell32_mun | RT_ICON | 1501 (group 264) | 32x32 | 32 | `icons/icon_264_4_32x32.png` |
| shell32_mun | RT_ICON | 1502 (group 264) | 24x24 | 32 | `icons/icon_264_5_24x24.png` |
| shell32_mun | RT_ICON | 1503 (group 264) | 20x20 | 32 | `icons/icon_264_6_20x20.png` |
| shell32_mun | RT_ICON | 1504 (group 264) | 16x16 | 32 | `icons/icon_264_7_16x16.png` |
| shell32_mun | RT_ICON | 1505 (group 1001) | 256x256 | 32 | `icons/icon_1001_0_256x256.png` |
| shell32_mun | RT_ICON | 1506 (group 1001) | 64x64 | 32 | `icons/icon_1001_1_64x64.png` |
| shell32_mun | RT_ICON | 1507 (group 1001) | 48x48 | 32 | `icons/icon_1001_2_48x48.png` |
| shell32_mun | RT_ICON | 1508 (group 1001) | 40x40 | 32 | `icons/icon_1001_3_40x40.png` |
| shell32_mun | RT_ICON | 1509 (group 1001) | 32x32 | 32 | `icons/icon_1001_4_32x32.png` |
| shell32_mun | RT_ICON | 151 (group 11) | 64x64 | 32 | `icons/icon_11_1_64x64.png` |
| shell32_mun | RT_ICON | 1510 (group 1001) | 24x24 | 32 | `icons/icon_1001_5_24x24.png` |
| shell32_mun | RT_ICON | 1511 (group 1001) | 20x20 | 32 | `icons/icon_1001_6_20x20.png` |
| shell32_mun | RT_ICON | 1512 (group 1001) | 16x16 | 32 | `icons/icon_1001_7_16x16.png` |
| shell32_mun | RT_ICON | 1513 (group 265) | 256x256 | 32 | `icons/icon_265_0_256x256.png` |
| shell32_mun | RT_ICON | 1514 (group 265) | 64x64 | 32 | `icons/icon_265_1_64x64.png` |
| shell32_mun | RT_ICON | 1515 (group 265) | 48x48 | 32 | `icons/icon_265_2_48x48.png` |
| shell32_mun | RT_ICON | 1516 (group 265) | 40x40 | 32 | `icons/icon_265_3_40x40.png` |
| shell32_mun | RT_ICON | 1517 (group 265) | 32x32 | 32 | `icons/icon_265_4_32x32.png` |
| shell32_mun | RT_ICON | 1518 (group 265) | 24x24 | 32 | `icons/icon_265_5_24x24.png` |
| shell32_mun | RT_ICON | 1519 (group 265) | 20x20 | 32 | `icons/icon_265_6_20x20.png` |
| shell32_mun | RT_ICON | 152 (group 11) | 48x48 | 32 | `icons/icon_11_2_48x48.png` |
| shell32_mun | RT_ICON | 1520 (group 265) | 16x16 | 32 | `icons/icon_265_7_16x16.png` |
| shell32_mun | RT_ICON | 1521 (group 266) | 32x32 | 4 | `icons/icon_266_0_32x32.png` |
| shell32_mun | RT_ICON | 1522 (group 266) | 24x24 | 4 | `icons/icon_266_1_24x24.png` |
| shell32_mun | RT_ICON | 1523 (group 266) | 16x16 | 4 | `icons/icon_266_2_16x16.png` |
| shell32_mun | RT_ICON | 1524 (group 266) | 32x32 | 8 | `icons/icon_266_3_32x32.png` |
| shell32_mun | RT_ICON | 1525 (group 266) | 24x24 | 8 | `icons/icon_266_4_24x24.png` |
| shell32_mun | RT_ICON | 1526 (group 266) | 16x16 | 8 | `icons/icon_266_5_16x16.png` |
| shell32_mun | RT_ICON | 1527 (group 266) | 32x32 | 32 | `icons/icon_266_6_32x32.png` |
| shell32_mun | RT_ICON | 1528 (group 266) | 24x24 | 32 | `icons/icon_266_7_24x24.png` |
| shell32_mun | RT_ICON | 1529 (group 266) | 16x16 | 32 | `icons/icon_266_8_16x16.png` |
| shell32_mun | RT_ICON | 153 (group 11) | 40x40 | 32 | `icons/icon_11_3_40x40.png` |
| shell32_mun | RT_ICON | 1530 (group 320) | 32x32 | 4 | `icons/icon_320_0_32x32.png` |
| shell32_mun | RT_ICON | 1531 (group 320) | 24x24 | 4 | `icons/icon_320_1_24x24.png` |
| shell32_mun | RT_ICON | 1532 (group 320) | 16x16 | 4 | `icons/icon_320_2_16x16.png` |
| shell32_mun | RT_ICON | 1533 (group 320) | 32x32 | 8 | `icons/icon_320_3_32x32.png` |
| shell32_mun | RT_ICON | 1534 (group 320) | 24x24 | 8 | `icons/icon_320_4_24x24.png` |
| shell32_mun | RT_ICON | 1535 (group 320) | 16x16 | 8 | `icons/icon_320_5_16x16.png` |
| shell32_mun | RT_ICON | 1536 (group 320) | 32x32 | 32 | `icons/icon_320_6_32x32.png` |
| shell32_mun | RT_ICON | 1537 (group 320) | 24x24 | 32 | `icons/icon_320_7_24x24.png` |
| shell32_mun | RT_ICON | 1538 (group 320) | 16x16 | 32 | `icons/icon_320_8_16x16.png` |
| shell32_mun | RT_ICON | 1539 (group 321) | 256x256 | 32 | `icons/icon_321_0_256x256.png` |
| shell32_mun | RT_ICON | 154 (group 11) | 32x32 | 32 | `icons/icon_11_4_32x32.png` |
| shell32_mun | RT_ICON | 1540 (group 321) | 64x64 | 32 | `icons/icon_321_1_64x64.png` |
| shell32_mun | RT_ICON | 1541 (group 321) | 48x48 | 32 | `icons/icon_321_2_48x48.png` |
| shell32_mun | RT_ICON | 1542 (group 321) | 40x40 | 32 | `icons/icon_321_3_40x40.png` |
| shell32_mun | RT_ICON | 1543 (group 321) | 32x32 | 32 | `icons/icon_321_4_32x32.png` |
| shell32_mun | RT_ICON | 1544 (group 321) | 24x24 | 32 | `icons/icon_321_5_24x24.png` |
| shell32_mun | RT_ICON | 1545 (group 321) | 20x20 | 32 | `icons/icon_321_6_20x20.png` |
| shell32_mun | RT_ICON | 1546 (group 321) | 16x16 | 32 | `icons/icon_321_7_16x16.png` |
| shell32_mun | RT_ICON | 1547 (group 267) | 256x256 | 32 | `icons/icon_267_0_256x256.png` |
| shell32_mun | RT_ICON | 1548 (group 267) | 64x64 | 32 | `icons/icon_267_1_64x64.png` |
| shell32_mun | RT_ICON | 1549 (group 267) | 48x48 | 32 | `icons/icon_267_2_48x48.png` |
| shell32_mun | RT_ICON | 155 (group 11) | 24x24 | 32 | `icons/icon_11_5_24x24.png` |
| shell32_mun | RT_ICON | 1550 (group 267) | 40x40 | 32 | `icons/icon_267_3_40x40.png` |
| shell32_mun | RT_ICON | 1551 (group 267) | 32x32 | 32 | `icons/icon_267_4_32x32.png` |
| shell32_mun | RT_ICON | 1552 (group 267) | 24x24 | 32 | `icons/icon_267_5_24x24.png` |
| shell32_mun | RT_ICON | 1553 (group 267) | 20x20 | 32 | `icons/icon_267_6_20x20.png` |
| shell32_mun | RT_ICON | 1554 (group 267) | 16x16 | 32 | `icons/icon_267_7_16x16.png` |
| shell32_mun | RT_ICON | 1555 (group 16742) | 256x256 | 32 | `icons/icon_16742_0_256x256.png` |
| shell32_mun | RT_ICON | 1556 (group 16742) | 64x64 | 32 | `icons/icon_16742_1_64x64.png` |
| shell32_mun | RT_ICON | 1557 (group 16742) | 48x48 | 32 | `icons/icon_16742_2_48x48.png` |
| shell32_mun | RT_ICON | 1558 (group 16742) | 40x40 | 32 | `icons/icon_16742_3_40x40.png` |
| shell32_mun | RT_ICON | 1559 (group 16742) | 32x32 | 32 | `icons/icon_16742_4_32x32.png` |
| shell32_mun | RT_ICON | 156 (group 11) | 20x20 | 32 | `icons/icon_11_6_20x20.png` |
| shell32_mun | RT_ICON | 1560 (group 16742) | 24x24 | 32 | `icons/icon_16742_5_24x24.png` |
| shell32_mun | RT_ICON | 1561 (group 16742) | 20x20 | 32 | `icons/icon_16742_6_20x20.png` |
| shell32_mun | RT_ICON | 1562 (group 16742) | 16x16 | 32 | `icons/icon_16742_7_16x16.png` |
| shell32_mun | RT_ICON | 1563 (group 16754) | 16x16 | 32 | `icons/icon_16754_0_16x16.png` |
| shell32_mun | RT_ICON | 1564 (group 16755) | 256x256 | 32 | `icons/icon_16755_0_256x256.png` |
| shell32_mun | RT_ICON | 1565 (group 16755) | 64x64 | 32 | `icons/icon_16755_1_64x64.png` |
| shell32_mun | RT_ICON | 1566 (group 16755) | 48x48 | 32 | `icons/icon_16755_2_48x48.png` |
| shell32_mun | RT_ICON | 1567 (group 16755) | 40x40 | 32 | `icons/icon_16755_3_40x40.png` |
| shell32_mun | RT_ICON | 1568 (group 16755) | 32x32 | 32 | `icons/icon_16755_4_32x32.png` |
| shell32_mun | RT_ICON | 1569 (group 16755) | 24x24 | 32 | `icons/icon_16755_5_24x24.png` |
| shell32_mun | RT_ICON | 157 (group 11) | 16x16 | 32 | `icons/icon_11_7_16x16.png` |
| shell32_mun | RT_ICON | 1570 (group 16755) | 20x20 | 32 | `icons/icon_16755_6_20x20.png` |
| shell32_mun | RT_ICON | 1571 (group 16755) | 16x16 | 32 | `icons/icon_16755_7_16x16.png` |
| shell32_mun | RT_ICON | 1572 (group 16756) | 16x16 | 32 | `icons/icon_16756_0_16x16.png` |
| shell32_mun | RT_ICON | 1573 (group 16757) | 256x256 | 32 | `icons/icon_16757_0_256x256.png` |
| shell32_mun | RT_ICON | 1574 (group 16757) | 64x64 | 32 | `icons/icon_16757_1_64x64.png` |
| shell32_mun | RT_ICON | 1575 (group 16757) | 48x48 | 32 | `icons/icon_16757_2_48x48.png` |
| shell32_mun | RT_ICON | 1576 (group 16757) | 40x40 | 32 | `icons/icon_16757_3_40x40.png` |
| shell32_mun | RT_ICON | 1577 (group 16757) | 32x32 | 32 | `icons/icon_16757_4_32x32.png` |
| shell32_mun | RT_ICON | 1578 (group 16757) | 24x24 | 32 | `icons/icon_16757_5_24x24.png` |
| shell32_mun | RT_ICON | 1579 (group 16757) | 20x20 | 32 | `icons/icon_16757_6_20x20.png` |
| shell32_mun | RT_ICON | 158 (group 12) | 256x256 | 32 | `icons/icon_12_0_256x256.png` |
| shell32_mun | RT_ICON | 1580 (group 16757) | 16x16 | 32 | `icons/icon_16757_7_16x16.png` |
| shell32_mun | RT_ICON | 1581 (group 16758) | 256x256 | 32 | `icons/icon_16758_0_256x256.png` |
| shell32_mun | RT_ICON | 1582 (group 16758) | 64x64 | 32 | `icons/icon_16758_1_64x64.png` |
| shell32_mun | RT_ICON | 1583 (group 16758) | 48x48 | 32 | `icons/icon_16758_2_48x48.png` |
| shell32_mun | RT_ICON | 1584 (group 16758) | 40x40 | 32 | `icons/icon_16758_3_40x40.png` |
| shell32_mun | RT_ICON | 1585 (group 16758) | 32x32 | 32 | `icons/icon_16758_4_32x32.png` |
| shell32_mun | RT_ICON | 1586 (group 16758) | 24x24 | 32 | `icons/icon_16758_5_24x24.png` |
| shell32_mun | RT_ICON | 1587 (group 16758) | 20x20 | 32 | `icons/icon_16758_6_20x20.png` |
| shell32_mun | RT_ICON | 1588 (group 16758) | 16x16 | 32 | `icons/icon_16758_7_16x16.png` |
| shell32_mun | RT_ICON | 1589 (group 16759) | 256x256 | 32 | `icons/icon_16759_0_256x256.png` |
| shell32_mun | RT_ICON | 159 (group 12) | 64x64 | 32 | `icons/icon_12_1_64x64.png` |
| shell32_mun | RT_ICON | 1590 (group 16759) | 64x64 | 32 | `icons/icon_16759_1_64x64.png` |
| shell32_mun | RT_ICON | 1591 (group 16759) | 48x48 | 32 | `icons/icon_16759_2_48x48.png` |
| shell32_mun | RT_ICON | 1592 (group 16759) | 40x40 | 32 | `icons/icon_16759_3_40x40.png` |
| shell32_mun | RT_ICON | 1593 (group 16759) | 32x32 | 32 | `icons/icon_16759_4_32x32.png` |
| shell32_mun | RT_ICON | 1594 (group 16759) | 24x24 | 32 | `icons/icon_16759_5_24x24.png` |
| shell32_mun | RT_ICON | 1595 (group 16759) | 20x20 | 32 | `icons/icon_16759_6_20x20.png` |
| shell32_mun | RT_ICON | 1596 (group 16759) | 16x16 | 32 | `icons/icon_16759_7_16x16.png` |
| shell32_mun | RT_ICON | 1597 (group 16760) | 16x16 | 32 | `icons/icon_16760_0_16x16.png` |
| shell32_mun | RT_ICON | 1598 (group 16814) | 256x256 | 32 | `icons/icon_16814_0_256x256.png` |
| shell32_mun | RT_ICON | 1599 (group 16814) | 64x64 | 32 | `icons/icon_16814_1_64x64.png` |
| shell32_mun | RT_ICON | 16 (group 62999) | 16x16 | 32 | `icons/icon_62999_7_16x16.png` |
| shell32_mun | RT_ICON | 160 (group 12) | 48x48 | 32 | `icons/icon_12_2_48x48.png` |
| shell32_mun | RT_ICON | 1600 (group 16814) | 48x48 | 32 | `icons/icon_16814_2_48x48.png` |
| shell32_mun | RT_ICON | 1601 (group 16814) | 40x40 | 32 | `icons/icon_16814_3_40x40.png` |
| shell32_mun | RT_ICON | 1602 (group 16814) | 32x32 | 32 | `icons/icon_16814_4_32x32.png` |
| shell32_mun | RT_ICON | 1603 (group 16814) | 24x24 | 32 | `icons/icon_16814_5_24x24.png` |
| shell32_mun | RT_ICON | 1604 (group 16814) | 20x20 | 32 | `icons/icon_16814_6_20x20.png` |
| shell32_mun | RT_ICON | 1605 (group 16814) | 16x16 | 32 | `icons/icon_16814_7_16x16.png` |
| shell32_mun | RT_ICON | 1606 (group 16813) | 256x256 | 32 | `icons/icon_16813_0_256x256.png` |
| shell32_mun | RT_ICON | 1607 (group 16813) | 64x64 | 32 | `icons/icon_16813_1_64x64.png` |
| shell32_mun | RT_ICON | 1608 (group 16813) | 48x48 | 32 | `icons/icon_16813_2_48x48.png` |
| shell32_mun | RT_ICON | 1609 (group 16813) | 40x40 | 32 | `icons/icon_16813_3_40x40.png` |
| shell32_mun | RT_ICON | 161 (group 12) | 40x40 | 32 | `icons/icon_12_3_40x40.png` |
| shell32_mun | RT_ICON | 1610 (group 16813) | 32x32 | 32 | `icons/icon_16813_4_32x32.png` |
| shell32_mun | RT_ICON | 1611 (group 16813) | 24x24 | 32 | `icons/icon_16813_5_24x24.png` |
| shell32_mun | RT_ICON | 1612 (group 16813) | 20x20 | 32 | `icons/icon_16813_6_20x20.png` |
| shell32_mun | RT_ICON | 1613 (group 16813) | 16x16 | 32 | `icons/icon_16813_7_16x16.png` |
| shell32_mun | RT_ICON | 1614 (group 16773) | 16x16 | 32 | `icons/icon_16773_0_16x16.png` |
| shell32_mun | RT_ICON | 1615 (group 16772) | 256x256 | 32 | `icons/icon_16772_0_256x256.png` |
| shell32_mun | RT_ICON | 1616 (group 16772) | 64x64 | 32 | `icons/icon_16772_1_64x64.png` |
| shell32_mun | RT_ICON | 1617 (group 16772) | 48x48 | 32 | `icons/icon_16772_2_48x48.png` |
| shell32_mun | RT_ICON | 1618 (group 16772) | 40x40 | 32 | `icons/icon_16772_3_40x40.png` |
| shell32_mun | RT_ICON | 1619 (group 16772) | 32x32 | 32 | `icons/icon_16772_4_32x32.png` |
| shell32_mun | RT_ICON | 162 (group 12) | 32x32 | 32 | `icons/icon_12_4_32x32.png` |
| shell32_mun | RT_ICON | 1620 (group 16772) | 24x24 | 32 | `icons/icon_16772_5_24x24.png` |
| shell32_mun | RT_ICON | 1621 (group 16772) | 20x20 | 32 | `icons/icon_16772_6_20x20.png` |
| shell32_mun | RT_ICON | 1622 (group 16772) | 16x16 | 32 | `icons/icon_16772_7_16x16.png` |
| shell32_mun | RT_ICON | 1623 (group 16774) | 256x256 | 32 | `icons/icon_16774_0_256x256.png` |
| shell32_mun | RT_ICON | 1624 (group 16774) | 64x64 | 32 | `icons/icon_16774_1_64x64.png` |
| shell32_mun | RT_ICON | 1625 (group 16774) | 48x48 | 32 | `icons/icon_16774_2_48x48.png` |
| shell32_mun | RT_ICON | 1626 (group 16774) | 40x40 | 32 | `icons/icon_16774_3_40x40.png` |
| shell32_mun | RT_ICON | 1627 (group 16774) | 32x32 | 32 | `icons/icon_16774_4_32x32.png` |
| shell32_mun | RT_ICON | 1628 (group 16774) | 24x24 | 32 | `icons/icon_16774_5_24x24.png` |
| shell32_mun | RT_ICON | 1629 (group 16774) | 20x20 | 32 | `icons/icon_16774_6_20x20.png` |
| shell32_mun | RT_ICON | 163 (group 12) | 24x24 | 32 | `icons/icon_12_5_24x24.png` |
| shell32_mun | RT_ICON | 1630 (group 16774) | 16x16 | 32 | `icons/icon_16774_7_16x16.png` |
| shell32_mun | RT_ICON | 1631 (group 16775) | 256x256 | 32 | `icons/icon_16775_0_256x256.png` |
| shell32_mun | RT_ICON | 1632 (group 16775) | 64x64 | 32 | `icons/icon_16775_1_64x64.png` |
| shell32_mun | RT_ICON | 1633 (group 16775) | 48x48 | 32 | `icons/icon_16775_2_48x48.png` |
| shell32_mun | RT_ICON | 1634 (group 16775) | 40x40 | 32 | `icons/icon_16775_3_40x40.png` |
| shell32_mun | RT_ICON | 1635 (group 16775) | 32x32 | 32 | `icons/icon_16775_4_32x32.png` |
| shell32_mun | RT_ICON | 1636 (group 16775) | 24x24 | 32 | `icons/icon_16775_5_24x24.png` |
| shell32_mun | RT_ICON | 1637 (group 16775) | 20x20 | 32 | `icons/icon_16775_6_20x20.png` |
| shell32_mun | RT_ICON | 1638 (group 16775) | 16x16 | 32 | `icons/icon_16775_7_16x16.png` |
| shell32_mun | RT_ICON | 1639 (group 16776) | 256x256 | 32 | `icons/icon_16776_0_256x256.png` |
| shell32_mun | RT_ICON | 164 (group 12) | 20x20 | 32 | `icons/icon_12_6_20x20.png` |
| shell32_mun | RT_ICON | 1640 (group 16776) | 64x64 | 32 | `icons/icon_16776_1_64x64.png` |
| shell32_mun | RT_ICON | 1641 (group 16776) | 48x48 | 32 | `icons/icon_16776_2_48x48.png` |
| shell32_mun | RT_ICON | 1642 (group 16776) | 40x40 | 32 | `icons/icon_16776_3_40x40.png` |
| shell32_mun | RT_ICON | 1643 (group 16776) | 32x32 | 32 | `icons/icon_16776_4_32x32.png` |
| shell32_mun | RT_ICON | 1644 (group 16776) | 24x24 | 32 | `icons/icon_16776_5_24x24.png` |
| shell32_mun | RT_ICON | 1645 (group 16776) | 20x20 | 32 | `icons/icon_16776_6_20x20.png` |
| shell32_mun | RT_ICON | 1646 (group 16776) | 16x16 | 32 | `icons/icon_16776_7_16x16.png` |
| shell32_mun | RT_ICON | 1647 (group 16778) | 64x64 | 4 | `icons/icon_16778_0_64x64.png` |
| shell32_mun | RT_ICON | 1648 (group 16778) | 48x48 | 4 | `icons/icon_16778_1_48x48.png` |
| shell32_mun | RT_ICON | 1649 (group 16778) | 40x40 | 4 | `icons/icon_16778_2_40x40.png` |
| shell32_mun | RT_ICON | 165 (group 12) | 16x16 | 32 | `icons/icon_12_7_16x16.png` |
| shell32_mun | RT_ICON | 1650 (group 16778) | 32x32 | 4 | `icons/icon_16778_3_32x32.png` |
| shell32_mun | RT_ICON | 1651 (group 16778) | 24x24 | 4 | `icons/icon_16778_4_24x24.png` |
| shell32_mun | RT_ICON | 1652 (group 16778) | 20x20 | 4 | `icons/icon_16778_5_20x20.png` |
| shell32_mun | RT_ICON | 1653 (group 16778) | 16x16 | 4 | `icons/icon_16778_6_16x16.png` |
| shell32_mun | RT_ICON | 1654 (group 16778) | 64x64 | 8 | `icons/icon_16778_7_64x64.png` |
| shell32_mun | RT_ICON | 1655 (group 16778) | 48x48 | 8 | `icons/icon_16778_8_48x48.png` |
| shell32_mun | RT_ICON | 1656 (group 16778) | 40x40 | 8 | `icons/icon_16778_9_40x40.png` |
| shell32_mun | RT_ICON | 1657 (group 16778) | 32x32 | 8 | `icons/icon_16778_10_32x32.png` |
| shell32_mun | RT_ICON | 1658 (group 16778) | 24x24 | 8 | `icons/icon_16778_11_24x24.png` |
| shell32_mun | RT_ICON | 1659 (group 16778) | 20x20 | 8 | `icons/icon_16778_12_20x20.png` |
| shell32_mun | RT_ICON | 166 (group 13) | 256x256 | 32 | `icons/icon_13_0_256x256.png` |
| shell32_mun | RT_ICON | 1660 (group 16778) | 16x16 | 8 | `icons/icon_16778_13_16x16.png` |
| shell32_mun | RT_ICON | 1661 (group 16778) | 64x64 | 32 | `icons/icon_16778_14_64x64.png` |
| shell32_mun | RT_ICON | 1662 (group 16778) | 48x48 | 32 | `icons/icon_16778_15_48x48.png` |
| shell32_mun | RT_ICON | 1663 (group 16778) | 40x40 | 32 | `icons/icon_16778_16_40x40.png` |
| shell32_mun | RT_ICON | 1664 (group 16778) | 32x32 | 32 | `icons/icon_16778_17_32x32.png` |
| shell32_mun | RT_ICON | 1665 (group 16778) | 24x24 | 32 | `icons/icon_16778_18_24x24.png` |
| shell32_mun | RT_ICON | 1666 (group 16778) | 20x20 | 32 | `icons/icon_16778_19_20x20.png` |
| shell32_mun | RT_ICON | 1667 (group 16778) | 16x16 | 32 | `icons/icon_16778_20_16x16.png` |
| shell32_mun | RT_ICON | 1668 (group 16779) | 256x256 | 32 | `icons/icon_16779_0_256x256.png` |
| shell32_mun | RT_ICON | 1669 (group 16779) | 64x64 | 32 | `icons/icon_16779_1_64x64.png` |
| shell32_mun | RT_ICON | 167 (group 13) | 64x64 | 32 | `icons/icon_13_1_64x64.png` |
| shell32_mun | RT_ICON | 1670 (group 16779) | 48x48 | 32 | `icons/icon_16779_2_48x48.png` |
| shell32_mun | RT_ICON | 1671 (group 16779) | 40x40 | 32 | `icons/icon_16779_3_40x40.png` |
| shell32_mun | RT_ICON | 1672 (group 16779) | 32x32 | 32 | `icons/icon_16779_4_32x32.png` |
| shell32_mun | RT_ICON | 1673 (group 16779) | 24x24 | 32 | `icons/icon_16779_5_24x24.png` |
| shell32_mun | RT_ICON | 1674 (group 16779) | 20x20 | 32 | `icons/icon_16779_6_20x20.png` |
| shell32_mun | RT_ICON | 1675 (group 16779) | 16x16 | 32 | `icons/icon_16779_7_16x16.png` |
| shell32_mun | RT_ICON | 1676 (group 16804) | 48x48 | 4 | `icons/icon_16804_0_48x48.png` |
| shell32_mun | RT_ICON | 1677 (group 16804) | 32x32 | 4 | `icons/icon_16804_1_32x32.png` |
| shell32_mun | RT_ICON | 1678 (group 16804) | 24x24 | 4 | `icons/icon_16804_2_24x24.png` |
| shell32_mun | RT_ICON | 1679 (group 16804) | 16x16 | 4 | `icons/icon_16804_3_16x16.png` |
| shell32_mun | RT_ICON | 168 (group 13) | 48x48 | 32 | `icons/icon_13_2_48x48.png` |
| shell32_mun | RT_ICON | 1680 (group 16804) | 48x48 | 8 | `icons/icon_16804_4_48x48.png` |
| shell32_mun | RT_ICON | 1681 (group 16804) | 32x32 | 8 | `icons/icon_16804_5_32x32.png` |
| shell32_mun | RT_ICON | 1682 (group 16804) | 24x24 | 8 | `icons/icon_16804_6_24x24.png` |
| shell32_mun | RT_ICON | 1683 (group 16804) | 16x16 | 8 | `icons/icon_16804_7_16x16.png` |
| shell32_mun | RT_ICON | 1684 (group 16804) | 256x256 | 32 | `icons/icon_16804_8_256x256.png` |
| shell32_mun | RT_ICON | 1685 (group 16804) | 64x64 | 32 | `icons/icon_16804_9_64x64.png` |
| shell32_mun | RT_ICON | 1686 (group 16804) | 48x48 | 32 | `icons/icon_16804_10_48x48.png` |
| shell32_mun | RT_ICON | 1687 (group 16804) | 32x32 | 32 | `icons/icon_16804_11_32x32.png` |
| shell32_mun | RT_ICON | 1688 (group 16804) | 24x24 | 32 | `icons/icon_16804_12_24x24.png` |
| shell32_mun | RT_ICON | 1689 (group 16804) | 16x16 | 32 | `icons/icon_16804_13_16x16.png` |
| shell32_mun | RT_ICON | 169 (group 13) | 40x40 | 32 | `icons/icon_13_3_40x40.png` |
| shell32_mun | RT_ICON | 1690 (group 268) | 256x256 | 32 | `icons/icon_268_0_256x256.png` |
| shell32_mun | RT_ICON | 1691 (group 268) | 64x64 | 32 | `icons/icon_268_1_64x64.png` |
| shell32_mun | RT_ICON | 1692 (group 268) | 48x48 | 32 | `icons/icon_268_2_48x48.png` |
| shell32_mun | RT_ICON | 1693 (group 268) | 40x40 | 32 | `icons/icon_268_3_40x40.png` |
| shell32_mun | RT_ICON | 1694 (group 268) | 32x32 | 32 | `icons/icon_268_4_32x32.png` |
| shell32_mun | RT_ICON | 1695 (group 268) | 24x24 | 32 | `icons/icon_268_5_24x24.png` |
| shell32_mun | RT_ICON | 1696 (group 268) | 20x20 | 32 | `icons/icon_268_6_20x20.png` |
| shell32_mun | RT_ICON | 1697 (group 268) | 16x16 | 32 | `icons/icon_268_7_16x16.png` |
| shell32_mun | RT_ICON | 1698 (group 269) | 256x256 | 32 | `icons/icon_269_0_256x256.png` |
| shell32_mun | RT_ICON | 1699 (group 269) | 64x64 | 32 | `icons/icon_269_1_64x64.png` |
| shell32_mun | RT_ICON | 17 (group 63000) | 256x256 | 32 | `icons/icon_63000_0_256x256.png` |
| shell32_mun | RT_ICON | 170 (group 13) | 32x32 | 32 | `icons/icon_13_4_32x32.png` |
| shell32_mun | RT_ICON | 1700 (group 269) | 48x48 | 32 | `icons/icon_269_2_48x48.png` |
| shell32_mun | RT_ICON | 1701 (group 269) | 40x40 | 32 | `icons/icon_269_3_40x40.png` |
| shell32_mun | RT_ICON | 1702 (group 269) | 32x32 | 32 | `icons/icon_269_4_32x32.png` |
| shell32_mun | RT_ICON | 1703 (group 269) | 24x24 | 32 | `icons/icon_269_5_24x24.png` |
| shell32_mun | RT_ICON | 1704 (group 269) | 20x20 | 32 | `icons/icon_269_6_20x20.png` |
| shell32_mun | RT_ICON | 1705 (group 269) | 16x16 | 32 | `icons/icon_269_7_16x16.png` |
| shell32_mun | RT_ICON | 1706 (group 270) | 256x256 | 32 | `icons/icon_270_0_256x256.png` |
| shell32_mun | RT_ICON | 1707 (group 270) | 64x64 | 32 | `icons/icon_270_1_64x64.png` |
| shell32_mun | RT_ICON | 1708 (group 270) | 48x48 | 32 | `icons/icon_270_2_48x48.png` |
| shell32_mun | RT_ICON | 1709 (group 270) | 40x40 | 32 | `icons/icon_270_3_40x40.png` |
| shell32_mun | RT_ICON | 171 (group 13) | 24x24 | 32 | `icons/icon_13_5_24x24.png` |
| shell32_mun | RT_ICON | 1710 (group 270) | 32x32 | 32 | `icons/icon_270_4_32x32.png` |
| shell32_mun | RT_ICON | 1711 (group 270) | 24x24 | 32 | `icons/icon_270_5_24x24.png` |
| shell32_mun | RT_ICON | 1712 (group 270) | 20x20 | 32 | `icons/icon_270_6_20x20.png` |
| shell32_mun | RT_ICON | 1713 (group 270) | 16x16 | 32 | `icons/icon_270_7_16x16.png` |
| shell32_mun | RT_ICON | 1714 (group 271) | 256x256 | 32 | `icons/icon_271_0_256x256.png` |
| shell32_mun | RT_ICON | 1715 (group 271) | 64x64 | 32 | `icons/icon_271_1_64x64.png` |
| shell32_mun | RT_ICON | 1716 (group 271) | 48x48 | 32 | `icons/icon_271_2_48x48.png` |
| shell32_mun | RT_ICON | 1717 (group 271) | 40x40 | 32 | `icons/icon_271_3_40x40.png` |
| shell32_mun | RT_ICON | 1718 (group 271) | 32x32 | 32 | `icons/icon_271_4_32x32.png` |
| shell32_mun | RT_ICON | 1719 (group 271) | 24x24 | 32 | `icons/icon_271_5_24x24.png` |
| shell32_mun | RT_ICON | 172 (group 13) | 20x20 | 32 | `icons/icon_13_6_20x20.png` |
| shell32_mun | RT_ICON | 1720 (group 271) | 20x20 | 32 | `icons/icon_271_6_20x20.png` |
| shell32_mun | RT_ICON | 1721 (group 271) | 16x16 | 32 | `icons/icon_271_7_16x16.png` |
| shell32_mun | RT_ICON | 1722 (group 272) | 256x256 | 32 | `icons/icon_272_0_256x256.png` |
| shell32_mun | RT_ICON | 1723 (group 272) | 64x64 | 32 | `icons/icon_272_1_64x64.png` |
| shell32_mun | RT_ICON | 1724 (group 272) | 48x48 | 32 | `icons/icon_272_2_48x48.png` |
| shell32_mun | RT_ICON | 1725 (group 272) | 40x40 | 32 | `icons/icon_272_3_40x40.png` |
| shell32_mun | RT_ICON | 1726 (group 272) | 32x32 | 32 | `icons/icon_272_4_32x32.png` |
| shell32_mun | RT_ICON | 1727 (group 272) | 24x24 | 32 | `icons/icon_272_5_24x24.png` |
| shell32_mun | RT_ICON | 1728 (group 272) | 20x20 | 32 | `icons/icon_272_6_20x20.png` |
| shell32_mun | RT_ICON | 1729 (group 272) | 16x16 | 32 | `icons/icon_272_7_16x16.png` |
| shell32_mun | RT_ICON | 173 (group 13) | 16x16 | 32 | `icons/icon_13_7_16x16.png` |
| shell32_mun | RT_ICON | 1730 (group 273) | 256x256 | 32 | `icons/icon_273_0_256x256.png` |
| shell32_mun | RT_ICON | 1731 (group 273) | 64x64 | 32 | `icons/icon_273_1_64x64.png` |
| shell32_mun | RT_ICON | 1732 (group 273) | 48x48 | 32 | `icons/icon_273_2_48x48.png` |
| shell32_mun | RT_ICON | 1733 (group 273) | 40x40 | 32 | `icons/icon_273_3_40x40.png` |
| shell32_mun | RT_ICON | 1734 (group 273) | 32x32 | 32 | `icons/icon_273_4_32x32.png` |
| shell32_mun | RT_ICON | 1735 (group 273) | 24x24 | 32 | `icons/icon_273_5_24x24.png` |
| shell32_mun | RT_ICON | 1736 (group 273) | 20x20 | 32 | `icons/icon_273_6_20x20.png` |
| shell32_mun | RT_ICON | 1737 (group 273) | 16x16 | 32 | `icons/icon_273_7_16x16.png` |
| shell32_mun | RT_ICON | 1738 (group 274) | 256x256 | 32 | `icons/icon_274_0_256x256.png` |
| shell32_mun | RT_ICON | 1739 (group 274) | 64x64 | 32 | `icons/icon_274_1_64x64.png` |
| shell32_mun | RT_ICON | 174 (group 14) | 256x256 | 32 | `icons/icon_14_0_256x256.png` |
| shell32_mun | RT_ICON | 1740 (group 274) | 48x48 | 32 | `icons/icon_274_2_48x48.png` |
| shell32_mun | RT_ICON | 1741 (group 274) | 40x40 | 32 | `icons/icon_274_3_40x40.png` |
| shell32_mun | RT_ICON | 1742 (group 274) | 32x32 | 32 | `icons/icon_274_4_32x32.png` |
| shell32_mun | RT_ICON | 1743 (group 274) | 24x24 | 32 | `icons/icon_274_5_24x24.png` |
| shell32_mun | RT_ICON | 1744 (group 274) | 20x20 | 32 | `icons/icon_274_6_20x20.png` |
| shell32_mun | RT_ICON | 1745 (group 274) | 16x16 | 32 | `icons/icon_274_7_16x16.png` |
| shell32_mun | RT_ICON | 1746 (group 275) | 256x256 | 32 | `icons/icon_275_0_256x256.png` |
| shell32_mun | RT_ICON | 1747 (group 275) | 64x64 | 32 | `icons/icon_275_1_64x64.png` |
| shell32_mun | RT_ICON | 1748 (group 275) | 48x48 | 32 | `icons/icon_275_2_48x48.png` |
| shell32_mun | RT_ICON | 1749 (group 275) | 40x40 | 32 | `icons/icon_275_3_40x40.png` |
| shell32_mun | RT_ICON | 175 (group 14) | 64x64 | 32 | `icons/icon_14_1_64x64.png` |
| shell32_mun | RT_ICON | 1750 (group 275) | 32x32 | 32 | `icons/icon_275_4_32x32.png` |
| shell32_mun | RT_ICON | 1751 (group 275) | 24x24 | 32 | `icons/icon_275_5_24x24.png` |
| shell32_mun | RT_ICON | 1752 (group 275) | 20x20 | 32 | `icons/icon_275_6_20x20.png` |
| shell32_mun | RT_ICON | 1753 (group 275) | 16x16 | 32 | `icons/icon_275_7_16x16.png` |
| shell32_mun | RT_ICON | 1754 (group 276) | 256x256 | 32 | `icons/icon_276_0_256x256.png` |
| shell32_mun | RT_ICON | 1755 (group 276) | 64x64 | 32 | `icons/icon_276_1_64x64.png` |
| shell32_mun | RT_ICON | 1756 (group 276) | 48x48 | 32 | `icons/icon_276_2_48x48.png` |
| shell32_mun | RT_ICON | 1757 (group 276) | 40x40 | 32 | `icons/icon_276_3_40x40.png` |
| shell32_mun | RT_ICON | 1758 (group 276) | 32x32 | 32 | `icons/icon_276_4_32x32.png` |
| shell32_mun | RT_ICON | 1759 (group 276) | 24x24 | 32 | `icons/icon_276_5_24x24.png` |
| shell32_mun | RT_ICON | 176 (group 14) | 48x48 | 32 | `icons/icon_14_2_48x48.png` |
| shell32_mun | RT_ICON | 1760 (group 276) | 20x20 | 32 | `icons/icon_276_6_20x20.png` |
| shell32_mun | RT_ICON | 1761 (group 276) | 16x16 | 32 | `icons/icon_276_7_16x16.png` |
| shell32_mun | RT_ICON | 1762 (group 277) | 256x256 | 32 | `icons/icon_277_0_256x256.png` |
| shell32_mun | RT_ICON | 1763 (group 277) | 64x64 | 32 | `icons/icon_277_1_64x64.png` |
| shell32_mun | RT_ICON | 1764 (group 277) | 48x48 | 32 | `icons/icon_277_2_48x48.png` |
| shell32_mun | RT_ICON | 1765 (group 277) | 40x40 | 32 | `icons/icon_277_3_40x40.png` |
| shell32_mun | RT_ICON | 1766 (group 277) | 32x32 | 32 | `icons/icon_277_4_32x32.png` |
| shell32_mun | RT_ICON | 1767 (group 277) | 24x24 | 32 | `icons/icon_277_5_24x24.png` |
| shell32_mun | RT_ICON | 1768 (group 277) | 20x20 | 32 | `icons/icon_277_6_20x20.png` |
| shell32_mun | RT_ICON | 1769 (group 277) | 16x16 | 32 | `icons/icon_277_7_16x16.png` |
| shell32_mun | RT_ICON | 177 (group 14) | 40x40 | 32 | `icons/icon_14_3_40x40.png` |
| shell32_mun | RT_ICON | 1770 (group 16799) | 32x32 | 4 | `icons/icon_16799_0_32x32.png` |
| shell32_mun | RT_ICON | 1771 (group 16799) | 16x16 | 4 | `icons/icon_16799_1_16x16.png` |
| shell32_mun | RT_ICON | 1772 (group 16799) | 32x32 | 8 | `icons/icon_16799_2_32x32.png` |
| shell32_mun | RT_ICON | 1773 (group 16799) | 16x16 | 8 | `icons/icon_16799_3_16x16.png` |
| shell32_mun | RT_ICON | 1774 (group 16799) | 32x32 | 32 | `icons/icon_16799_4_32x32.png` |
| shell32_mun | RT_ICON | 1775 (group 16799) | 16x16 | 32 | `icons/icon_16799_5_16x16.png` |
| shell32_mun | RT_ICON | 1776 (group 16800) | 32x32 | 4 | `icons/icon_16800_0_32x32.png` |
| shell32_mun | RT_ICON | 1777 (group 16800) | 16x16 | 4 | `icons/icon_16800_1_16x16.png` |
| shell32_mun | RT_ICON | 1778 (group 16800) | 32x32 | 8 | `icons/icon_16800_2_32x32.png` |
| shell32_mun | RT_ICON | 1779 (group 16800) | 16x16 | 8 | `icons/icon_16800_3_16x16.png` |
| shell32_mun | RT_ICON | 178 (group 14) | 32x32 | 32 | `icons/icon_14_4_32x32.png` |
| shell32_mun | RT_ICON | 1780 (group 16800) | 32x32 | 32 | `icons/icon_16800_4_32x32.png` |
| shell32_mun | RT_ICON | 1781 (group 16800) | 16x16 | 32 | `icons/icon_16800_5_16x16.png` |
| shell32_mun | RT_ICON | 1782 (group 291) | 256x256 | 32 | `icons/icon_291_0_256x256.png` |
| shell32_mun | RT_ICON | 1783 (group 291) | 64x64 | 32 | `icons/icon_291_1_64x64.png` |
| shell32_mun | RT_ICON | 1784 (group 291) | 48x48 | 32 | `icons/icon_291_2_48x48.png` |
| shell32_mun | RT_ICON | 1785 (group 291) | 40x40 | 32 | `icons/icon_291_3_40x40.png` |
| shell32_mun | RT_ICON | 1786 (group 291) | 32x32 | 32 | `icons/icon_291_4_32x32.png` |
| shell32_mun | RT_ICON | 1787 (group 291) | 24x24 | 32 | `icons/icon_291_5_24x24.png` |
| shell32_mun | RT_ICON | 1788 (group 291) | 20x20 | 32 | `icons/icon_291_6_20x20.png` |
| shell32_mun | RT_ICON | 1789 (group 291) | 16x16 | 32 | `icons/icon_291_7_16x16.png` |
| shell32_mun | RT_ICON | 179 (group 14) | 24x24 | 32 | `icons/icon_14_5_24x24.png` |
| shell32_mun | RT_ICON | 1790 (group 292) | 256x256 | 32 | `icons/icon_292_0_256x256.png` |
| shell32_mun | RT_ICON | 1791 (group 292) | 64x64 | 32 | `icons/icon_292_1_64x64.png` |
| shell32_mun | RT_ICON | 1792 (group 292) | 48x48 | 32 | `icons/icon_292_2_48x48.png` |
| shell32_mun | RT_ICON | 1793 (group 292) | 40x40 | 32 | `icons/icon_292_3_40x40.png` |
| shell32_mun | RT_ICON | 1794 (group 292) | 32x32 | 32 | `icons/icon_292_4_32x32.png` |
| shell32_mun | RT_ICON | 1795 (group 292) | 24x24 | 32 | `icons/icon_292_5_24x24.png` |
| shell32_mun | RT_ICON | 1796 (group 292) | 20x20 | 32 | `icons/icon_292_6_20x20.png` |
| shell32_mun | RT_ICON | 1797 (group 292) | 16x16 | 32 | `icons/icon_292_7_16x16.png` |
| shell32_mun | RT_ICON | 1798 (group 293) | 256x256 | 32 | `icons/icon_293_0_256x256.png` |
| shell32_mun | RT_ICON | 1799 (group 293) | 64x64 | 32 | `icons/icon_293_1_64x64.png` |
| shell32_mun | RT_ICON | 18 (group 63000) | 64x64 | 32 | `icons/icon_63000_1_64x64.png` |
| shell32_mun | RT_ICON | 180 (group 14) | 20x20 | 32 | `icons/icon_14_6_20x20.png` |
| shell32_mun | RT_ICON | 1800 (group 293) | 48x48 | 32 | `icons/icon_293_2_48x48.png` |
| shell32_mun | RT_ICON | 1801 (group 293) | 40x40 | 32 | `icons/icon_293_3_40x40.png` |
| shell32_mun | RT_ICON | 1802 (group 293) | 32x32 | 32 | `icons/icon_293_4_32x32.png` |
| shell32_mun | RT_ICON | 1803 (group 293) | 24x24 | 32 | `icons/icon_293_5_24x24.png` |
| shell32_mun | RT_ICON | 1804 (group 293) | 20x20 | 32 | `icons/icon_293_6_20x20.png` |
| shell32_mun | RT_ICON | 1805 (group 293) | 16x16 | 32 | `icons/icon_293_7_16x16.png` |
| shell32_mun | RT_ICON | 1806 (group 294) | 256x256 | 32 | `icons/icon_294_0_256x256.png` |
| shell32_mun | RT_ICON | 1807 (group 294) | 64x64 | 32 | `icons/icon_294_1_64x64.png` |
| shell32_mun | RT_ICON | 1808 (group 294) | 48x48 | 32 | `icons/icon_294_2_48x48.png` |
| shell32_mun | RT_ICON | 1809 (group 294) | 40x40 | 32 | `icons/icon_294_3_40x40.png` |
| shell32_mun | RT_ICON | 181 (group 14) | 16x16 | 32 | `icons/icon_14_7_16x16.png` |
| shell32_mun | RT_ICON | 1810 (group 294) | 32x32 | 32 | `icons/icon_294_4_32x32.png` |
| shell32_mun | RT_ICON | 1811 (group 294) | 24x24 | 32 | `icons/icon_294_5_24x24.png` |
| shell32_mun | RT_ICON | 1812 (group 294) | 20x20 | 32 | `icons/icon_294_6_20x20.png` |
| shell32_mun | RT_ICON | 1813 (group 294) | 16x16 | 32 | `icons/icon_294_7_16x16.png` |
| shell32_mun | RT_ICON | 1814 (group 295) | 256x256 | 32 | `icons/icon_295_0_256x256.png` |
| shell32_mun | RT_ICON | 1815 (group 295) | 64x64 | 32 | `icons/icon_295_1_64x64.png` |
| shell32_mun | RT_ICON | 1816 (group 295) | 48x48 | 32 | `icons/icon_295_2_48x48.png` |
| shell32_mun | RT_ICON | 1817 (group 295) | 40x40 | 32 | `icons/icon_295_3_40x40.png` |
| shell32_mun | RT_ICON | 1818 (group 295) | 32x32 | 32 | `icons/icon_295_4_32x32.png` |
| shell32_mun | RT_ICON | 1819 (group 295) | 24x24 | 32 | `icons/icon_295_5_24x24.png` |
| shell32_mun | RT_ICON | 182 (group 15) | 256x256 | 32 | `icons/icon_15_0_256x256.png` |
| shell32_mun | RT_ICON | 1820 (group 295) | 20x20 | 32 | `icons/icon_295_6_20x20.png` |
| shell32_mun | RT_ICON | 1821 (group 295) | 16x16 | 32 | `icons/icon_295_7_16x16.png` |
| shell32_mun | RT_ICON | 1822 (group 296) | 256x256 | 32 | `icons/icon_296_0_256x256.png` |
| shell32_mun | RT_ICON | 1823 (group 296) | 64x64 | 32 | `icons/icon_296_1_64x64.png` |
| shell32_mun | RT_ICON | 1824 (group 296) | 48x48 | 32 | `icons/icon_296_2_48x48.png` |
| shell32_mun | RT_ICON | 1825 (group 296) | 40x40 | 32 | `icons/icon_296_3_40x40.png` |
| shell32_mun | RT_ICON | 1826 (group 296) | 32x32 | 32 | `icons/icon_296_4_32x32.png` |
| shell32_mun | RT_ICON | 1827 (group 296) | 24x24 | 32 | `icons/icon_296_5_24x24.png` |
| shell32_mun | RT_ICON | 1828 (group 296) | 20x20 | 32 | `icons/icon_296_6_20x20.png` |
| shell32_mun | RT_ICON | 1829 (group 296) | 16x16 | 32 | `icons/icon_296_7_16x16.png` |
| shell32_mun | RT_ICON | 183 (group 15) | 64x64 | 32 | `icons/icon_15_1_64x64.png` |
| shell32_mun | RT_ICON | 1830 (group 297) | 256x256 | 32 | `icons/icon_297_0_256x256.png` |
| shell32_mun | RT_ICON | 1831 (group 297) | 64x64 | 32 | `icons/icon_297_1_64x64.png` |
| shell32_mun | RT_ICON | 1832 (group 297) | 48x48 | 32 | `icons/icon_297_2_48x48.png` |
| shell32_mun | RT_ICON | 1833 (group 297) | 40x40 | 32 | `icons/icon_297_3_40x40.png` |
| shell32_mun | RT_ICON | 1834 (group 297) | 32x32 | 32 | `icons/icon_297_4_32x32.png` |
| shell32_mun | RT_ICON | 1835 (group 297) | 24x24 | 32 | `icons/icon_297_5_24x24.png` |
| shell32_mun | RT_ICON | 1836 (group 297) | 20x20 | 32 | `icons/icon_297_6_20x20.png` |
| shell32_mun | RT_ICON | 1837 (group 297) | 16x16 | 32 | `icons/icon_297_7_16x16.png` |
| shell32_mun | RT_ICON | 1838 (group 298) | 256x256 | 32 | `icons/icon_298_0_256x256.png` |
| shell32_mun | RT_ICON | 1839 (group 298) | 64x64 | 32 | `icons/icon_298_1_64x64.png` |
| shell32_mun | RT_ICON | 184 (group 15) | 48x48 | 32 | `icons/icon_15_2_48x48.png` |
| shell32_mun | RT_ICON | 1840 (group 298) | 48x48 | 32 | `icons/icon_298_2_48x48.png` |
| shell32_mun | RT_ICON | 1841 (group 298) | 40x40 | 32 | `icons/icon_298_3_40x40.png` |
| shell32_mun | RT_ICON | 1842 (group 298) | 32x32 | 32 | `icons/icon_298_4_32x32.png` |
| shell32_mun | RT_ICON | 1843 (group 298) | 24x24 | 32 | `icons/icon_298_5_24x24.png` |
| shell32_mun | RT_ICON | 1844 (group 298) | 20x20 | 32 | `icons/icon_298_6_20x20.png` |
| shell32_mun | RT_ICON | 1845 (group 298) | 16x16 | 32 | `icons/icon_298_7_16x16.png` |
| shell32_mun | RT_ICON | 1846 (group 318) | 256x256 | 32 | `icons/icon_318_0_256x256.png` |
| shell32_mun | RT_ICON | 1847 (group 318) | 64x64 | 32 | `icons/icon_318_1_64x64.png` |
| shell32_mun | RT_ICON | 1848 (group 318) | 48x48 | 32 | `icons/icon_318_2_48x48.png` |
| shell32_mun | RT_ICON | 1849 (group 318) | 40x40 | 32 | `icons/icon_318_3_40x40.png` |
| shell32_mun | RT_ICON | 185 (group 15) | 40x40 | 32 | `icons/icon_15_3_40x40.png` |
| shell32_mun | RT_ICON | 1850 (group 318) | 32x32 | 32 | `icons/icon_318_4_32x32.png` |
| shell32_mun | RT_ICON | 1851 (group 318) | 24x24 | 32 | `icons/icon_318_5_24x24.png` |
| shell32_mun | RT_ICON | 1852 (group 318) | 20x20 | 32 | `icons/icon_318_6_20x20.png` |
| shell32_mun | RT_ICON | 1853 (group 318) | 16x16 | 32 | `icons/icon_318_7_16x16.png` |
| shell32_mun | RT_ICON | 1854 (group 299) | 256x256 | 32 | `icons/icon_299_0_256x256.png` |
| shell32_mun | RT_ICON | 1855 (group 299) | 64x64 | 32 | `icons/icon_299_1_64x64.png` |
| shell32_mun | RT_ICON | 1856 (group 299) | 48x48 | 32 | `icons/icon_299_2_48x48.png` |
| shell32_mun | RT_ICON | 1857 (group 299) | 40x40 | 32 | `icons/icon_299_3_40x40.png` |
| shell32_mun | RT_ICON | 1858 (group 299) | 32x32 | 32 | `icons/icon_299_4_32x32.png` |
| shell32_mun | RT_ICON | 1859 (group 299) | 24x24 | 32 | `icons/icon_299_5_24x24.png` |
| shell32_mun | RT_ICON | 186 (group 15) | 32x32 | 32 | `icons/icon_15_4_32x32.png` |
| shell32_mun | RT_ICON | 1860 (group 299) | 20x20 | 32 | `icons/icon_299_6_20x20.png` |
| shell32_mun | RT_ICON | 1861 (group 299) | 16x16 | 32 | `icons/icon_299_7_16x16.png` |
| shell32_mun | RT_ICON | 1862 (group 300) | 256x256 | 32 | `icons/icon_300_0_256x256.png` |
| shell32_mun | RT_ICON | 1863 (group 300) | 64x64 | 32 | `icons/icon_300_1_64x64.png` |
| shell32_mun | RT_ICON | 1864 (group 300) | 48x48 | 32 | `icons/icon_300_2_48x48.png` |
| shell32_mun | RT_ICON | 1865 (group 300) | 40x40 | 32 | `icons/icon_300_3_40x40.png` |
| shell32_mun | RT_ICON | 1866 (group 300) | 32x32 | 32 | `icons/icon_300_4_32x32.png` |
| shell32_mun | RT_ICON | 1867 (group 300) | 24x24 | 32 | `icons/icon_300_5_24x24.png` |
| shell32_mun | RT_ICON | 1868 (group 300) | 20x20 | 32 | `icons/icon_300_6_20x20.png` |
| shell32_mun | RT_ICON | 1869 (group 300) | 16x16 | 32 | `icons/icon_300_7_16x16.png` |
| shell32_mun | RT_ICON | 187 (group 15) | 24x24 | 32 | `icons/icon_15_5_24x24.png` |
| shell32_mun | RT_ICON | 1870 (group 301) | 256x256 | 32 | `icons/icon_301_0_256x256.png` |
| shell32_mun | RT_ICON | 1871 (group 301) | 64x64 | 32 | `icons/icon_301_1_64x64.png` |
| shell32_mun | RT_ICON | 1872 (group 301) | 48x48 | 32 | `icons/icon_301_2_48x48.png` |
| shell32_mun | RT_ICON | 1873 (group 301) | 40x40 | 32 | `icons/icon_301_3_40x40.png` |
| shell32_mun | RT_ICON | 1874 (group 301) | 32x32 | 32 | `icons/icon_301_4_32x32.png` |
| shell32_mun | RT_ICON | 1875 (group 301) | 24x24 | 32 | `icons/icon_301_5_24x24.png` |
| shell32_mun | RT_ICON | 1876 (group 301) | 20x20 | 32 | `icons/icon_301_6_20x20.png` |
| shell32_mun | RT_ICON | 1877 (group 301) | 16x16 | 32 | `icons/icon_301_7_16x16.png` |
| shell32_mun | RT_ICON | 1878 (group 302) | 256x256 | 32 | `icons/icon_302_0_256x256.png` |
| shell32_mun | RT_ICON | 1879 (group 302) | 64x64 | 32 | `icons/icon_302_1_64x64.png` |
| shell32_mun | RT_ICON | 188 (group 15) | 20x20 | 32 | `icons/icon_15_6_20x20.png` |
| shell32_mun | RT_ICON | 1880 (group 302) | 48x48 | 32 | `icons/icon_302_2_48x48.png` |
| shell32_mun | RT_ICON | 1881 (group 302) | 40x40 | 32 | `icons/icon_302_3_40x40.png` |
| shell32_mun | RT_ICON | 1882 (group 302) | 32x32 | 32 | `icons/icon_302_4_32x32.png` |
| shell32_mun | RT_ICON | 1883 (group 302) | 24x24 | 32 | `icons/icon_302_5_24x24.png` |
| shell32_mun | RT_ICON | 1884 (group 302) | 20x20 | 32 | `icons/icon_302_6_20x20.png` |
| shell32_mun | RT_ICON | 1885 (group 302) | 16x16 | 32 | `icons/icon_302_7_16x16.png` |
| shell32_mun | RT_ICON | 1886 (group 303) | 256x256 | 32 | `icons/icon_303_0_256x256.png` |
| shell32_mun | RT_ICON | 1887 (group 303) | 64x64 | 32 | `icons/icon_303_1_64x64.png` |
| shell32_mun | RT_ICON | 1888 (group 303) | 48x48 | 32 | `icons/icon_303_2_48x48.png` |
| shell32_mun | RT_ICON | 1889 (group 303) | 40x40 | 32 | `icons/icon_303_3_40x40.png` |
| shell32_mun | RT_ICON | 189 (group 15) | 16x16 | 32 | `icons/icon_15_7_16x16.png` |
| shell32_mun | RT_ICON | 1890 (group 303) | 32x32 | 32 | `icons/icon_303_4_32x32.png` |
| shell32_mun | RT_ICON | 1891 (group 303) | 24x24 | 32 | `icons/icon_303_5_24x24.png` |
| shell32_mun | RT_ICON | 1892 (group 303) | 20x20 | 32 | `icons/icon_303_6_20x20.png` |
| shell32_mun | RT_ICON | 1893 (group 303) | 16x16 | 32 | `icons/icon_303_7_16x16.png` |
| shell32_mun | RT_ICON | 1894 (group 304) | 256x256 | 32 | `icons/icon_304_0_256x256.png` |
| shell32_mun | RT_ICON | 1895 (group 304) | 64x64 | 32 | `icons/icon_304_1_64x64.png` |
| shell32_mun | RT_ICON | 1896 (group 304) | 48x48 | 32 | `icons/icon_304_2_48x48.png` |
| shell32_mun | RT_ICON | 1897 (group 304) | 40x40 | 32 | `icons/icon_304_3_40x40.png` |
| shell32_mun | RT_ICON | 1898 (group 304) | 32x32 | 32 | `icons/icon_304_4_32x32.png` |
| shell32_mun | RT_ICON | 1899 (group 304) | 24x24 | 32 | `icons/icon_304_5_24x24.png` |
| shell32_mun | RT_ICON | 19 (group 63000) | 48x48 | 32 | `icons/icon_63000_2_48x48.png` |
| shell32_mun | RT_ICON | 190 (group 16) | 256x256 | 32 | `icons/icon_16_0_256x256.png` |
| shell32_mun | RT_ICON | 1900 (group 304) | 20x20 | 32 | `icons/icon_304_6_20x20.png` |
| shell32_mun | RT_ICON | 1901 (group 304) | 16x16 | 32 | `icons/icon_304_7_16x16.png` |
| shell32_mun | RT_ICON | 1902 (group 306) | 256x256 | 32 | `icons/icon_306_0_256x256.png` |
| shell32_mun | RT_ICON | 1903 (group 306) | 64x64 | 32 | `icons/icon_306_1_64x64.png` |
| shell32_mun | RT_ICON | 1904 (group 306) | 48x48 | 32 | `icons/icon_306_2_48x48.png` |
| shell32_mun | RT_ICON | 1905 (group 306) | 40x40 | 32 | `icons/icon_306_3_40x40.png` |
| shell32_mun | RT_ICON | 1906 (group 306) | 32x32 | 32 | `icons/icon_306_4_32x32.png` |
| shell32_mun | RT_ICON | 1907 (group 306) | 24x24 | 32 | `icons/icon_306_5_24x24.png` |
| shell32_mun | RT_ICON | 1908 (group 306) | 20x20 | 32 | `icons/icon_306_6_20x20.png` |
| shell32_mun | RT_ICON | 1909 (group 306) | 16x16 | 32 | `icons/icon_306_7_16x16.png` |
| shell32_mun | RT_ICON | 191 (group 16) | 64x64 | 32 | `icons/icon_16_1_64x64.png` |
| shell32_mun | RT_ICON | 1910 (group 307) | 256x256 | 32 | `icons/icon_307_0_256x256.png` |
| shell32_mun | RT_ICON | 1911 (group 307) | 64x64 | 32 | `icons/icon_307_1_64x64.png` |
| shell32_mun | RT_ICON | 1912 (group 307) | 48x48 | 32 | `icons/icon_307_2_48x48.png` |
| shell32_mun | RT_ICON | 1913 (group 307) | 40x40 | 32 | `icons/icon_307_3_40x40.png` |
| shell32_mun | RT_ICON | 1914 (group 307) | 32x32 | 32 | `icons/icon_307_4_32x32.png` |
| shell32_mun | RT_ICON | 1915 (group 307) | 24x24 | 32 | `icons/icon_307_5_24x24.png` |
| shell32_mun | RT_ICON | 1916 (group 307) | 20x20 | 32 | `icons/icon_307_6_20x20.png` |
| shell32_mun | RT_ICON | 1917 (group 307) | 16x16 | 32 | `icons/icon_307_7_16x16.png` |
| shell32_mun | RT_ICON | 1918 (group 308) | 256x256 | 32 | `icons/icon_308_0_256x256.png` |
| shell32_mun | RT_ICON | 1919 (group 308) | 64x64 | 32 | `icons/icon_308_1_64x64.png` |
| shell32_mun | RT_ICON | 192 (group 16) | 48x48 | 32 | `icons/icon_16_2_48x48.png` |
| shell32_mun | RT_ICON | 1920 (group 308) | 48x48 | 32 | `icons/icon_308_2_48x48.png` |
| shell32_mun | RT_ICON | 1921 (group 308) | 40x40 | 32 | `icons/icon_308_3_40x40.png` |
| shell32_mun | RT_ICON | 1922 (group 308) | 32x32 | 32 | `icons/icon_308_4_32x32.png` |
| shell32_mun | RT_ICON | 1923 (group 308) | 24x24 | 32 | `icons/icon_308_5_24x24.png` |
| shell32_mun | RT_ICON | 1924 (group 308) | 20x20 | 32 | `icons/icon_308_6_20x20.png` |
| shell32_mun | RT_ICON | 1925 (group 308) | 16x16 | 32 | `icons/icon_308_7_16x16.png` |
| shell32_mun | RT_ICON | 1926 (group 309) | 256x256 | 32 | `icons/icon_309_0_256x256.png` |
| shell32_mun | RT_ICON | 1927 (group 309) | 64x64 | 32 | `icons/icon_309_1_64x64.png` |
| shell32_mun | RT_ICON | 1928 (group 309) | 48x48 | 32 | `icons/icon_309_2_48x48.png` |
| shell32_mun | RT_ICON | 1929 (group 309) | 40x40 | 32 | `icons/icon_309_3_40x40.png` |
| shell32_mun | RT_ICON | 193 (group 16) | 40x40 | 32 | `icons/icon_16_3_40x40.png` |
| shell32_mun | RT_ICON | 1930 (group 309) | 32x32 | 32 | `icons/icon_309_4_32x32.png` |
| shell32_mun | RT_ICON | 1931 (group 309) | 24x24 | 32 | `icons/icon_309_5_24x24.png` |
| shell32_mun | RT_ICON | 1932 (group 309) | 20x20 | 32 | `icons/icon_309_6_20x20.png` |
| shell32_mun | RT_ICON | 1933 (group 309) | 16x16 | 32 | `icons/icon_309_7_16x16.png` |
| shell32_mun | RT_ICON | 1934 (group 310) | 256x256 | 32 | `icons/icon_310_0_256x256.png` |
| shell32_mun | RT_ICON | 1935 (group 310) | 64x64 | 32 | `icons/icon_310_1_64x64.png` |
| shell32_mun | RT_ICON | 1936 (group 310) | 48x48 | 32 | `icons/icon_310_2_48x48.png` |
| shell32_mun | RT_ICON | 1937 (group 310) | 40x40 | 32 | `icons/icon_310_3_40x40.png` |
| shell32_mun | RT_ICON | 1938 (group 310) | 32x32 | 32 | `icons/icon_310_4_32x32.png` |
| shell32_mun | RT_ICON | 1939 (group 310) | 24x24 | 32 | `icons/icon_310_5_24x24.png` |
| shell32_mun | RT_ICON | 194 (group 16) | 32x32 | 32 | `icons/icon_16_4_32x32.png` |
| shell32_mun | RT_ICON | 1940 (group 310) | 20x20 | 32 | `icons/icon_310_6_20x20.png` |
| shell32_mun | RT_ICON | 1941 (group 310) | 16x16 | 32 | `icons/icon_310_7_16x16.png` |
| shell32_mun | RT_ICON | 1942 (group 311) | 256x256 | 32 | `icons/icon_311_0_256x256.png` |
| shell32_mun | RT_ICON | 1943 (group 311) | 64x64 | 32 | `icons/icon_311_1_64x64.png` |
| shell32_mun | RT_ICON | 1944 (group 311) | 48x48 | 32 | `icons/icon_311_2_48x48.png` |
| shell32_mun | RT_ICON | 1945 (group 311) | 40x40 | 32 | `icons/icon_311_3_40x40.png` |
| shell32_mun | RT_ICON | 1946 (group 311) | 32x32 | 32 | `icons/icon_311_4_32x32.png` |
| shell32_mun | RT_ICON | 1947 (group 311) | 24x24 | 32 | `icons/icon_311_5_24x24.png` |
| shell32_mun | RT_ICON | 1948 (group 311) | 20x20 | 32 | `icons/icon_311_6_20x20.png` |
| shell32_mun | RT_ICON | 1949 (group 311) | 16x16 | 32 | `icons/icon_311_7_16x16.png` |
| shell32_mun | RT_ICON | 195 (group 16) | 24x24 | 32 | `icons/icon_16_5_24x24.png` |
| shell32_mun | RT_ICON | 1950 (group 312) | 256x256 | 32 | `icons/icon_312_0_256x256.png` |
| shell32_mun | RT_ICON | 1951 (group 312) | 64x64 | 32 | `icons/icon_312_1_64x64.png` |
| shell32_mun | RT_ICON | 1952 (group 312) | 48x48 | 32 | `icons/icon_312_2_48x48.png` |
| shell32_mun | RT_ICON | 1953 (group 312) | 40x40 | 32 | `icons/icon_312_3_40x40.png` |
| shell32_mun | RT_ICON | 1954 (group 312) | 32x32 | 32 | `icons/icon_312_4_32x32.png` |
| shell32_mun | RT_ICON | 1955 (group 312) | 24x24 | 32 | `icons/icon_312_5_24x24.png` |
| shell32_mun | RT_ICON | 1956 (group 312) | 20x20 | 32 | `icons/icon_312_6_20x20.png` |
| shell32_mun | RT_ICON | 1957 (group 312) | 16x16 | 32 | `icons/icon_312_7_16x16.png` |
| shell32_mun | RT_ICON | 1958 (group 313) | 256x256 | 32 | `icons/icon_313_0_256x256.png` |
| shell32_mun | RT_ICON | 1959 (group 313) | 64x64 | 32 | `icons/icon_313_1_64x64.png` |
| shell32_mun | RT_ICON | 196 (group 16) | 20x20 | 32 | `icons/icon_16_6_20x20.png` |
| shell32_mun | RT_ICON | 1960 (group 313) | 48x48 | 32 | `icons/icon_313_2_48x48.png` |
| shell32_mun | RT_ICON | 1961 (group 313) | 40x40 | 32 | `icons/icon_313_3_40x40.png` |
| shell32_mun | RT_ICON | 1962 (group 313) | 32x32 | 32 | `icons/icon_313_4_32x32.png` |
| shell32_mun | RT_ICON | 1963 (group 313) | 24x24 | 32 | `icons/icon_313_5_24x24.png` |
| shell32_mun | RT_ICON | 1964 (group 313) | 20x20 | 32 | `icons/icon_313_6_20x20.png` |
| shell32_mun | RT_ICON | 1965 (group 313) | 16x16 | 32 | `icons/icon_313_7_16x16.png` |
| shell32_mun | RT_ICON | 1966 (group 314) | 256x256 | 32 | `icons/icon_314_0_256x256.png` |
| shell32_mun | RT_ICON | 1967 (group 314) | 64x64 | 32 | `icons/icon_314_1_64x64.png` |
| shell32_mun | RT_ICON | 1968 (group 314) | 48x48 | 32 | `icons/icon_314_2_48x48.png` |
| shell32_mun | RT_ICON | 1969 (group 314) | 40x40 | 32 | `icons/icon_314_3_40x40.png` |
| shell32_mun | RT_ICON | 197 (group 16) | 16x16 | 32 | `icons/icon_16_7_16x16.png` |
| shell32_mun | RT_ICON | 1970 (group 314) | 32x32 | 32 | `icons/icon_314_4_32x32.png` |
| shell32_mun | RT_ICON | 1971 (group 314) | 24x24 | 32 | `icons/icon_314_5_24x24.png` |
| shell32_mun | RT_ICON | 1972 (group 314) | 20x20 | 32 | `icons/icon_314_6_20x20.png` |
| shell32_mun | RT_ICON | 1973 (group 314) | 16x16 | 32 | `icons/icon_314_7_16x16.png` |
| shell32_mun | RT_ICON | 1974 (group 315) | 256x256 | 32 | `icons/icon_315_0_256x256.png` |
| shell32_mun | RT_ICON | 1975 (group 315) | 64x64 | 32 | `icons/icon_315_1_64x64.png` |
| shell32_mun | RT_ICON | 1976 (group 315) | 48x48 | 32 | `icons/icon_315_2_48x48.png` |
| shell32_mun | RT_ICON | 1977 (group 315) | 40x40 | 32 | `icons/icon_315_3_40x40.png` |
| shell32_mun | RT_ICON | 1978 (group 315) | 32x32 | 32 | `icons/icon_315_4_32x32.png` |
| shell32_mun | RT_ICON | 1979 (group 315) | 24x24 | 32 | `icons/icon_315_5_24x24.png` |
| shell32_mun | RT_ICON | 198 (group 17) | 256x256 | 32 | `icons/icon_17_0_256x256.png` |
| shell32_mun | RT_ICON | 1980 (group 315) | 20x20 | 32 | `icons/icon_315_6_20x20.png` |
| shell32_mun | RT_ICON | 1981 (group 315) | 16x16 | 32 | `icons/icon_315_7_16x16.png` |
| shell32_mun | RT_ICON | 1982 (group 316) | 48x48 | 4 | `icons/icon_316_0_48x48.png` |
| shell32_mun | RT_ICON | 1983 (group 316) | 32x32 | 4 | `icons/icon_316_1_32x32.png` |
| shell32_mun | RT_ICON | 1984 (group 316) | 16x16 | 4 | `icons/icon_316_2_16x16.png` |
| shell32_mun | RT_ICON | 1985 (group 316) | 48x48 | 8 | `icons/icon_316_3_48x48.png` |
| shell32_mun | RT_ICON | 1986 (group 316) | 32x32 | 8 | `icons/icon_316_4_32x32.png` |
| shell32_mun | RT_ICON | 1987 (group 316) | 16x16 | 8 | `icons/icon_316_5_16x16.png` |
| shell32_mun | RT_ICON | 1988 (group 316) | 48x48 | 32 | `icons/icon_316_6_48x48.png` |
| shell32_mun | RT_ICON | 1989 (group 316) | 32x32 | 32 | `icons/icon_316_7_32x32.png` |
| shell32_mun | RT_ICON | 199 (group 17) | 64x64 | 32 | `icons/icon_17_1_64x64.png` |
| shell32_mun | RT_ICON | 1990 (group 316) | 16x16 | 32 | `icons/icon_316_8_16x16.png` |
| shell32_mun | RT_ICON | 1991 (group 317) | 256x256 | 32 | `icons/icon_317_0_256x256.png` |
| shell32_mun | RT_ICON | 1992 (group 317) | 64x64 | 32 | `icons/icon_317_1_64x64.png` |
| shell32_mun | RT_ICON | 1993 (group 317) | 48x48 | 32 | `icons/icon_317_2_48x48.png` |
| shell32_mun | RT_ICON | 1994 (group 317) | 40x40 | 32 | `icons/icon_317_3_40x40.png` |
| shell32_mun | RT_ICON | 1995 (group 317) | 32x32 | 32 | `icons/icon_317_4_32x32.png` |
| shell32_mun | RT_ICON | 1996 (group 317) | 24x24 | 32 | `icons/icon_317_5_24x24.png` |
| shell32_mun | RT_ICON | 1997 (group 317) | 20x20 | 32 | `icons/icon_317_6_20x20.png` |
| shell32_mun | RT_ICON | 1998 (group 317) | 16x16 | 32 | `icons/icon_317_7_16x16.png` |
| shell32_mun | RT_ICON | 1999 (group 319) | 256x256 | 32 | `icons/icon_319_0_256x256.png` |
| shell32_mun | RT_ICON | 2 (group 62998) | 64x64 | 32 | `icons/icon_62998_1_64x64.png` |
| shell32_mun | RT_ICON | 20 (group 63000) | 40x40 | 32 | `icons/icon_63000_3_40x40.png` |
| shell32_mun | RT_ICON | 200 (group 17) | 48x48 | 32 | `icons/icon_17_2_48x48.png` |
| shell32_mun | RT_ICON | 2000 (group 319) | 64x64 | 32 | `icons/icon_319_1_64x64.png` |
| shell32_mun | RT_ICON | 2001 (group 319) | 48x48 | 32 | `icons/icon_319_2_48x48.png` |
| shell32_mun | RT_ICON | 2002 (group 319) | 40x40 | 32 | `icons/icon_319_3_40x40.png` |
| shell32_mun | RT_ICON | 2003 (group 319) | 32x32 | 32 | `icons/icon_319_4_32x32.png` |
| shell32_mun | RT_ICON | 2004 (group 319) | 24x24 | 32 | `icons/icon_319_5_24x24.png` |
| shell32_mun | RT_ICON | 2005 (group 319) | 20x20 | 32 | `icons/icon_319_6_20x20.png` |
| shell32_mun | RT_ICON | 2006 (group 319) | 16x16 | 32 | `icons/icon_319_7_16x16.png` |
| shell32_mun | RT_ICON | 2007 (group 16761) | 256x256 | 32 | `icons/icon_16761_0_256x256.png` |
| shell32_mun | RT_ICON | 2008 (group 16761) | 64x64 | 32 | `icons/icon_16761_1_64x64.png` |
| shell32_mun | RT_ICON | 2009 (group 16761) | 48x48 | 32 | `icons/icon_16761_2_48x48.png` |
| shell32_mun | RT_ICON | 201 (group 17) | 40x40 | 32 | `icons/icon_17_3_40x40.png` |
| shell32_mun | RT_ICON | 2010 (group 16761) | 40x40 | 32 | `icons/icon_16761_3_40x40.png` |
| shell32_mun | RT_ICON | 2011 (group 16761) | 32x32 | 32 | `icons/icon_16761_4_32x32.png` |
| shell32_mun | RT_ICON | 2012 (group 16761) | 24x24 | 32 | `icons/icon_16761_5_24x24.png` |
| shell32_mun | RT_ICON | 2013 (group 16761) | 20x20 | 32 | `icons/icon_16761_6_20x20.png` |
| shell32_mun | RT_ICON | 2014 (group 16761) | 16x16 | 32 | `icons/icon_16761_7_16x16.png` |
| shell32_mun | RT_ICON | 2015 (group 1002) | 256x256 | 32 | `icons/icon_1002_0_256x256.png` |
| shell32_mun | RT_ICON | 2016 (group 1002) | 64x64 | 32 | `icons/icon_1002_1_64x64.png` |
| shell32_mun | RT_ICON | 2017 (group 1002) | 48x48 | 32 | `icons/icon_1002_2_48x48.png` |
| shell32_mun | RT_ICON | 2018 (group 1002) | 40x40 | 32 | `icons/icon_1002_3_40x40.png` |
| shell32_mun | RT_ICON | 2019 (group 1002) | 32x32 | 32 | `icons/icon_1002_4_32x32.png` |
| shell32_mun | RT_ICON | 202 (group 17) | 32x32 | 32 | `icons/icon_17_4_32x32.png` |
| shell32_mun | RT_ICON | 2020 (group 1002) | 24x24 | 32 | `icons/icon_1002_5_24x24.png` |
| shell32_mun | RT_ICON | 2021 (group 1002) | 20x20 | 32 | `icons/icon_1002_6_20x20.png` |
| shell32_mun | RT_ICON | 2022 (group 1002) | 16x16 | 32 | `icons/icon_1002_7_16x16.png` |
| shell32_mun | RT_ICON | 2023 (group 1003) | 256x256 | 32 | `icons/icon_1003_0_256x256.png` |
| shell32_mun | RT_ICON | 2024 (group 1003) | 64x64 | 32 | `icons/icon_1003_1_64x64.png` |
| shell32_mun | RT_ICON | 2025 (group 1003) | 48x48 | 32 | `icons/icon_1003_2_48x48.png` |
| shell32_mun | RT_ICON | 2026 (group 1003) | 40x40 | 32 | `icons/icon_1003_3_40x40.png` |
| shell32_mun | RT_ICON | 2027 (group 1003) | 32x32 | 32 | `icons/icon_1003_4_32x32.png` |
| shell32_mun | RT_ICON | 2028 (group 1003) | 24x24 | 32 | `icons/icon_1003_5_24x24.png` |
| shell32_mun | RT_ICON | 2029 (group 1003) | 20x20 | 32 | `icons/icon_1003_6_20x20.png` |
| shell32_mun | RT_ICON | 203 (group 17) | 24x24 | 32 | `icons/icon_17_5_24x24.png` |
| shell32_mun | RT_ICON | 2030 (group 1003) | 16x16 | 32 | `icons/icon_1003_7_16x16.png` |
| shell32_mun | RT_ICON | 2031 (group 1006) | 256x256 | 32 | `icons/icon_1006_0_256x256.png` |
| shell32_mun | RT_ICON | 2032 (group 1006) | 64x64 | 32 | `icons/icon_1006_1_64x64.png` |
| shell32_mun | RT_ICON | 2033 (group 1006) | 48x48 | 32 | `icons/icon_1006_2_48x48.png` |
| shell32_mun | RT_ICON | 2034 (group 1006) | 40x40 | 32 | `icons/icon_1006_3_40x40.png` |
| shell32_mun | RT_ICON | 2035 (group 1006) | 32x32 | 32 | `icons/icon_1006_4_32x32.png` |
| shell32_mun | RT_ICON | 2036 (group 1006) | 24x24 | 32 | `icons/icon_1006_5_24x24.png` |
| shell32_mun | RT_ICON | 2037 (group 1006) | 20x20 | 32 | `icons/icon_1006_6_20x20.png` |
| shell32_mun | RT_ICON | 2038 (group 1006) | 16x16 | 32 | `icons/icon_1006_7_16x16.png` |
| shell32_mun | RT_ICON | 2039 (group 1007) | 256x256 | 32 | `icons/icon_1007_0_256x256.png` |
| shell32_mun | RT_ICON | 204 (group 17) | 20x20 | 32 | `icons/icon_17_6_20x20.png` |
| shell32_mun | RT_ICON | 2040 (group 1007) | 64x64 | 32 | `icons/icon_1007_1_64x64.png` |
| shell32_mun | RT_ICON | 2041 (group 1007) | 48x48 | 32 | `icons/icon_1007_2_48x48.png` |
| shell32_mun | RT_ICON | 2042 (group 1007) | 40x40 | 32 | `icons/icon_1007_3_40x40.png` |
| shell32_mun | RT_ICON | 2043 (group 1007) | 32x32 | 32 | `icons/icon_1007_4_32x32.png` |
| shell32_mun | RT_ICON | 2044 (group 1007) | 24x24 | 32 | `icons/icon_1007_5_24x24.png` |
| shell32_mun | RT_ICON | 2045 (group 1007) | 20x20 | 32 | `icons/icon_1007_6_20x20.png` |
| shell32_mun | RT_ICON | 2046 (group 1007) | 16x16 | 32 | `icons/icon_1007_7_16x16.png` |
| shell32_mun | RT_ICON | 2047 (group 1008) | 256x256 | 32 | `icons/icon_1008_0_256x256.png` |
| shell32_mun | RT_ICON | 2048 (group 1008) | 64x64 | 32 | `icons/icon_1008_1_64x64.png` |
| shell32_mun | RT_ICON | 2049 (group 1008) | 48x48 | 32 | `icons/icon_1008_2_48x48.png` |
| shell32_mun | RT_ICON | 205 (group 17) | 16x16 | 32 | `icons/icon_17_7_16x16.png` |
| shell32_mun | RT_ICON | 2050 (group 1008) | 40x40 | 32 | `icons/icon_1008_3_40x40.png` |
| shell32_mun | RT_ICON | 2051 (group 1008) | 32x32 | 32 | `icons/icon_1008_4_32x32.png` |
| shell32_mun | RT_ICON | 2052 (group 1008) | 24x24 | 32 | `icons/icon_1008_5_24x24.png` |
| shell32_mun | RT_ICON | 2053 (group 1008) | 20x20 | 32 | `icons/icon_1008_6_20x20.png` |
| shell32_mun | RT_ICON | 2054 (group 1008) | 16x16 | 32 | `icons/icon_1008_7_16x16.png` |
| shell32_mun | RT_ICON | 2055 (group 1009) | 256x256 | 32 | `icons/icon_1009_0_256x256.png` |
| shell32_mun | RT_ICON | 2056 (group 1009) | 64x64 | 32 | `icons/icon_1009_1_64x64.png` |
| shell32_mun | RT_ICON | 2057 (group 1009) | 48x48 | 32 | `icons/icon_1009_2_48x48.png` |
| shell32_mun | RT_ICON | 2058 (group 1009) | 40x40 | 32 | `icons/icon_1009_3_40x40.png` |
| shell32_mun | RT_ICON | 2059 (group 1009) | 32x32 | 32 | `icons/icon_1009_4_32x32.png` |
| shell32_mun | RT_ICON | 206 (group 18) | 256x256 | 32 | `icons/icon_18_0_256x256.png` |
| shell32_mun | RT_ICON | 2060 (group 1009) | 24x24 | 32 | `icons/icon_1009_5_24x24.png` |
| shell32_mun | RT_ICON | 2061 (group 1009) | 20x20 | 32 | `icons/icon_1009_6_20x20.png` |
| shell32_mun | RT_ICON | 2062 (group 1009) | 16x16 | 32 | `icons/icon_1009_7_16x16.png` |
| shell32_mun | RT_ICON | 2063 (group 1010) | 256x256 | 32 | `icons/icon_1010_0_256x256.png` |
| shell32_mun | RT_ICON | 2064 (group 1010) | 64x64 | 32 | `icons/icon_1010_1_64x64.png` |
| shell32_mun | RT_ICON | 2065 (group 1010) | 48x48 | 32 | `icons/icon_1010_2_48x48.png` |
| shell32_mun | RT_ICON | 2066 (group 1010) | 40x40 | 32 | `icons/icon_1010_3_40x40.png` |
| shell32_mun | RT_ICON | 2067 (group 1010) | 32x32 | 32 | `icons/icon_1010_4_32x32.png` |
| shell32_mun | RT_ICON | 2068 (group 1010) | 24x24 | 32 | `icons/icon_1010_5_24x24.png` |
| shell32_mun | RT_ICON | 2069 (group 1010) | 20x20 | 32 | `icons/icon_1010_6_20x20.png` |
| shell32_mun | RT_ICON | 207 (group 18) | 64x64 | 32 | `icons/icon_18_1_64x64.png` |
| shell32_mun | RT_ICON | 2070 (group 1010) | 16x16 | 32 | `icons/icon_1010_7_16x16.png` |
| shell32_mun | RT_ICON | 2071 (group 322) | 256x256 | 32 | `icons/icon_322_0_256x256.png` |
| shell32_mun | RT_ICON | 2072 (group 322) | 64x64 | 32 | `icons/icon_322_1_64x64.png` |
| shell32_mun | RT_ICON | 2073 (group 322) | 48x48 | 32 | `icons/icon_322_2_48x48.png` |
| shell32_mun | RT_ICON | 2074 (group 322) | 40x40 | 32 | `icons/icon_322_3_40x40.png` |
| shell32_mun | RT_ICON | 2075 (group 322) | 32x32 | 32 | `icons/icon_322_4_32x32.png` |
| shell32_mun | RT_ICON | 2076 (group 322) | 24x24 | 32 | `icons/icon_322_5_24x24.png` |
| shell32_mun | RT_ICON | 2077 (group 322) | 20x20 | 32 | `icons/icon_322_6_20x20.png` |
| shell32_mun | RT_ICON | 2078 (group 322) | 16x16 | 32 | `icons/icon_322_7_16x16.png` |
| shell32_mun | RT_ICON | 2079 (group 323) | 256x256 | 32 | `icons/icon_323_0_256x256.png` |
| shell32_mun | RT_ICON | 208 (group 18) | 48x48 | 32 | `icons/icon_18_2_48x48.png` |
| shell32_mun | RT_ICON | 2080 (group 323) | 64x64 | 32 | `icons/icon_323_1_64x64.png` |
| shell32_mun | RT_ICON | 2081 (group 323) | 48x48 | 32 | `icons/icon_323_2_48x48.png` |
| shell32_mun | RT_ICON | 2082 (group 323) | 40x40 | 32 | `icons/icon_323_3_40x40.png` |
| shell32_mun | RT_ICON | 2083 (group 323) | 32x32 | 32 | `icons/icon_323_4_32x32.png` |
| shell32_mun | RT_ICON | 2084 (group 323) | 24x24 | 32 | `icons/icon_323_5_24x24.png` |
| shell32_mun | RT_ICON | 2085 (group 323) | 20x20 | 32 | `icons/icon_323_6_20x20.png` |
| shell32_mun | RT_ICON | 2086 (group 323) | 16x16 | 32 | `icons/icon_323_7_16x16.png` |
| shell32_mun | RT_ICON | 2087 (group 324) | 256x256 | 32 | `icons/icon_324_0_256x256.png` |
| shell32_mun | RT_ICON | 2088 (group 324) | 64x64 | 32 | `icons/icon_324_1_64x64.png` |
| shell32_mun | RT_ICON | 2089 (group 324) | 48x48 | 32 | `icons/icon_324_2_48x48.png` |
| shell32_mun | RT_ICON | 209 (group 18) | 40x40 | 32 | `icons/icon_18_3_40x40.png` |
| shell32_mun | RT_ICON | 2090 (group 324) | 40x40 | 32 | `icons/icon_324_3_40x40.png` |
| shell32_mun | RT_ICON | 2091 (group 324) | 32x32 | 32 | `icons/icon_324_4_32x32.png` |
| shell32_mun | RT_ICON | 2092 (group 324) | 24x24 | 32 | `icons/icon_324_5_24x24.png` |
| shell32_mun | RT_ICON | 2093 (group 324) | 20x20 | 32 | `icons/icon_324_6_20x20.png` |
| shell32_mun | RT_ICON | 2094 (group 324) | 16x16 | 32 | `icons/icon_324_7_16x16.png` |
| shell32_mun | RT_ICON | 2095 (group 325) | 48x48 | 4 | `icons/icon_325_0_48x48.png` |
| shell32_mun | RT_ICON | 2096 (group 325) | 32x32 | 4 | `icons/icon_325_1_32x32.png` |
| shell32_mun | RT_ICON | 2097 (group 325) | 16x16 | 4 | `icons/icon_325_2_16x16.png` |
| shell32_mun | RT_ICON | 2098 (group 325) | 48x48 | 8 | `icons/icon_325_3_48x48.png` |
| shell32_mun | RT_ICON | 2099 (group 325) | 32x32 | 8 | `icons/icon_325_4_32x32.png` |
| shell32_mun | RT_ICON | 21 (group 63000) | 32x32 | 32 | `icons/icon_63000_4_32x32.png` |
| shell32_mun | RT_ICON | 210 (group 18) | 32x32 | 32 | `icons/icon_18_4_32x32.png` |
| shell32_mun | RT_ICON | 2100 (group 325) | 16x16 | 8 | `icons/icon_325_5_16x16.png` |
| shell32_mun | RT_ICON | 2101 (group 325) | 48x48 | 32 | `icons/icon_325_6_48x48.png` |
| shell32_mun | RT_ICON | 2102 (group 325) | 32x32 | 32 | `icons/icon_325_7_32x32.png` |
| shell32_mun | RT_ICON | 2103 (group 325) | 16x16 | 32 | `icons/icon_325_8_16x16.png` |
| shell32_mun | RT_ICON | 2104 (group 326) | 48x48 | 4 | `icons/icon_326_0_48x48.png` |
| shell32_mun | RT_ICON | 2105 (group 326) | 32x32 | 4 | `icons/icon_326_1_32x32.png` |
| shell32_mun | RT_ICON | 2106 (group 326) | 16x16 | 4 | `icons/icon_326_2_16x16.png` |
| shell32_mun | RT_ICON | 2107 (group 326) | 48x48 | 8 | `icons/icon_326_3_48x48.png` |
| shell32_mun | RT_ICON | 2108 (group 326) | 32x32 | 8 | `icons/icon_326_4_32x32.png` |
| shell32_mun | RT_ICON | 2109 (group 326) | 16x16 | 8 | `icons/icon_326_5_16x16.png` |
| shell32_mun | RT_ICON | 211 (group 18) | 24x24 | 32 | `icons/icon_18_5_24x24.png` |
| shell32_mun | RT_ICON | 2110 (group 326) | 48x48 | 32 | `icons/icon_326_6_48x48.png` |
| shell32_mun | RT_ICON | 2111 (group 326) | 32x32 | 32 | `icons/icon_326_7_32x32.png` |
| shell32_mun | RT_ICON | 2112 (group 326) | 16x16 | 32 | `icons/icon_326_8_16x16.png` |
| shell32_mun | RT_ICON | 2113 (group 327) | 256x256 | 32 | `icons/icon_327_0_256x256.png` |
| shell32_mun | RT_ICON | 2114 (group 327) | 64x64 | 32 | `icons/icon_327_1_64x64.png` |
| shell32_mun | RT_ICON | 2115 (group 327) | 48x48 | 32 | `icons/icon_327_2_48x48.png` |
| shell32_mun | RT_ICON | 2116 (group 327) | 40x40 | 32 | `icons/icon_327_3_40x40.png` |
| shell32_mun | RT_ICON | 2117 (group 327) | 32x32 | 32 | `icons/icon_327_4_32x32.png` |
| shell32_mun | RT_ICON | 2118 (group 327) | 24x24 | 32 | `icons/icon_327_5_24x24.png` |
| shell32_mun | RT_ICON | 2119 (group 327) | 20x20 | 32 | `icons/icon_327_6_20x20.png` |
| shell32_mun | RT_ICON | 212 (group 18) | 20x20 | 32 | `icons/icon_18_6_20x20.png` |
| shell32_mun | RT_ICON | 2120 (group 327) | 16x16 | 32 | `icons/icon_327_7_16x16.png` |
| shell32_mun | RT_ICON | 2121 (group 328) | 256x256 | 32 | `icons/icon_328_0_256x256.png` |
| shell32_mun | RT_ICON | 2122 (group 328) | 64x64 | 32 | `icons/icon_328_1_64x64.png` |
| shell32_mun | RT_ICON | 2123 (group 328) | 48x48 | 32 | `icons/icon_328_2_48x48.png` |
| shell32_mun | RT_ICON | 2124 (group 328) | 40x40 | 32 | `icons/icon_328_3_40x40.png` |
| shell32_mun | RT_ICON | 2125 (group 328) | 32x32 | 32 | `icons/icon_328_4_32x32.png` |
| shell32_mun | RT_ICON | 2126 (group 328) | 24x24 | 32 | `icons/icon_328_5_24x24.png` |
| shell32_mun | RT_ICON | 2127 (group 328) | 20x20 | 32 | `icons/icon_328_6_20x20.png` |
| shell32_mun | RT_ICON | 2128 (group 328) | 16x16 | 32 | `icons/icon_328_7_16x16.png` |
| shell32_mun | RT_ICON | 2129 (group 329) | 48x48 | 4 | `icons/icon_329_0_48x48.png` |
| shell32_mun | RT_ICON | 213 (group 18) | 16x16 | 32 | `icons/icon_18_7_16x16.png` |
| shell32_mun | RT_ICON | 2130 (group 329) | 32x32 | 4 | `icons/icon_329_1_32x32.png` |
| shell32_mun | RT_ICON | 2131 (group 329) | 16x16 | 4 | `icons/icon_329_2_16x16.png` |
| shell32_mun | RT_ICON | 2132 (group 329) | 48x48 | 8 | `icons/icon_329_3_48x48.png` |
| shell32_mun | RT_ICON | 2133 (group 329) | 32x32 | 8 | `icons/icon_329_4_32x32.png` |
| shell32_mun | RT_ICON | 2134 (group 329) | 16x16 | 8 | `icons/icon_329_5_16x16.png` |
| shell32_mun | RT_ICON | 2135 (group 329) | 48x48 | 32 | `icons/icon_329_6_48x48.png` |
| shell32_mun | RT_ICON | 2136 (group 329) | 32x32 | 32 | `icons/icon_329_7_32x32.png` |
| shell32_mun | RT_ICON | 2137 (group 329) | 16x16 | 32 | `icons/icon_329_8_16x16.png` |
| shell32_mun | RT_ICON | 2138 (group 330) | 48x48 | 4 | `icons/icon_330_0_48x48.png` |
| shell32_mun | RT_ICON | 2139 (group 330) | 32x32 | 4 | `icons/icon_330_1_32x32.png` |
| shell32_mun | RT_ICON | 214 (group 19) | 256x256 | 32 | `icons/icon_19_0_256x256.png` |
| shell32_mun | RT_ICON | 2140 (group 330) | 16x16 | 4 | `icons/icon_330_2_16x16.png` |
| shell32_mun | RT_ICON | 2141 (group 330) | 48x48 | 8 | `icons/icon_330_3_48x48.png` |
| shell32_mun | RT_ICON | 2142 (group 330) | 32x32 | 8 | `icons/icon_330_4_32x32.png` |
| shell32_mun | RT_ICON | 2143 (group 330) | 16x16 | 8 | `icons/icon_330_5_16x16.png` |
| shell32_mun | RT_ICON | 2144 (group 330) | 48x48 | 32 | `icons/icon_330_6_48x48.png` |
| shell32_mun | RT_ICON | 2145 (group 330) | 32x32 | 32 | `icons/icon_330_7_32x32.png` |
| shell32_mun | RT_ICON | 2146 (group 330) | 16x16 | 32 | `icons/icon_330_8_16x16.png` |
| shell32_mun | RT_ICON | 2147 (group 331) | 48x48 | 4 | `icons/icon_331_0_48x48.png` |
| shell32_mun | RT_ICON | 2148 (group 331) | 32x32 | 4 | `icons/icon_331_1_32x32.png` |
| shell32_mun | RT_ICON | 2149 (group 331) | 16x16 | 4 | `icons/icon_331_2_16x16.png` |
| shell32_mun | RT_ICON | 215 (group 19) | 64x64 | 32 | `icons/icon_19_1_64x64.png` |
| shell32_mun | RT_ICON | 2150 (group 331) | 48x48 | 8 | `icons/icon_331_3_48x48.png` |
| shell32_mun | RT_ICON | 2151 (group 331) | 32x32 | 8 | `icons/icon_331_4_32x32.png` |
| shell32_mun | RT_ICON | 2152 (group 331) | 16x16 | 8 | `icons/icon_331_5_16x16.png` |
| shell32_mun | RT_ICON | 2153 (group 331) | 48x48 | 32 | `icons/icon_331_6_48x48.png` |
| shell32_mun | RT_ICON | 2154 (group 331) | 32x32 | 32 | `icons/icon_331_7_32x32.png` |
| shell32_mun | RT_ICON | 2155 (group 331) | 16x16 | 32 | `icons/icon_331_8_16x16.png` |
| shell32_mun | RT_ICON | 2156 (group 337) | 256x256 | 32 | `icons/icon_337_0_256x256.png` |
| shell32_mun | RT_ICON | 2157 (group 337) | 64x64 | 32 | `icons/icon_337_1_64x64.png` |
| shell32_mun | RT_ICON | 2158 (group 337) | 48x48 | 32 | `icons/icon_337_2_48x48.png` |
| shell32_mun | RT_ICON | 2159 (group 337) | 40x40 | 32 | `icons/icon_337_3_40x40.png` |
| shell32_mun | RT_ICON | 216 (group 19) | 48x48 | 32 | `icons/icon_19_2_48x48.png` |
| shell32_mun | RT_ICON | 2160 (group 337) | 32x32 | 32 | `icons/icon_337_4_32x32.png` |
| shell32_mun | RT_ICON | 2161 (group 337) | 24x24 | 32 | `icons/icon_337_5_24x24.png` |
| shell32_mun | RT_ICON | 2162 (group 337) | 20x20 | 32 | `icons/icon_337_6_20x20.png` |
| shell32_mun | RT_ICON | 2163 (group 337) | 16x16 | 32 | `icons/icon_337_7_16x16.png` |
| shell32_mun | RT_ICON | 2164 (group 338) | 256x256 | 32 | `icons/icon_338_0_256x256.png` |
| shell32_mun | RT_ICON | 2165 (group 338) | 64x64 | 32 | `icons/icon_338_1_64x64.png` |
| shell32_mun | RT_ICON | 2166 (group 338) | 48x48 | 32 | `icons/icon_338_2_48x48.png` |
| shell32_mun | RT_ICON | 2167 (group 338) | 40x40 | 32 | `icons/icon_338_3_40x40.png` |
| shell32_mun | RT_ICON | 2168 (group 338) | 32x32 | 32 | `icons/icon_338_4_32x32.png` |
| shell32_mun | RT_ICON | 2169 (group 338) | 24x24 | 32 | `icons/icon_338_5_24x24.png` |
| shell32_mun | RT_ICON | 217 (group 19) | 40x40 | 32 | `icons/icon_19_3_40x40.png` |
| shell32_mun | RT_ICON | 2170 (group 338) | 20x20 | 32 | `icons/icon_338_6_20x20.png` |
| shell32_mun | RT_ICON | 2171 (group 338) | 16x16 | 32 | `icons/icon_338_7_16x16.png` |
| shell32_mun | RT_ICON | 2172 (group 16741) | 256x256 | 32 | `icons/icon_16741_0_256x256.png` |
| shell32_mun | RT_ICON | 2173 (group 16741) | 64x64 | 32 | `icons/icon_16741_1_64x64.png` |
| shell32_mun | RT_ICON | 2174 (group 16741) | 48x48 | 32 | `icons/icon_16741_2_48x48.png` |
| shell32_mun | RT_ICON | 2175 (group 16741) | 40x40 | 32 | `icons/icon_16741_3_40x40.png` |
| shell32_mun | RT_ICON | 2176 (group 16741) | 32x32 | 32 | `icons/icon_16741_4_32x32.png` |
| shell32_mun | RT_ICON | 2177 (group 16741) | 24x24 | 32 | `icons/icon_16741_5_24x24.png` |
| shell32_mun | RT_ICON | 2178 (group 16741) | 20x20 | 32 | `icons/icon_16741_6_20x20.png` |
| shell32_mun | RT_ICON | 2179 (group 16741) | 16x16 | 32 | `icons/icon_16741_7_16x16.png` |
| shell32_mun | RT_ICON | 218 (group 19) | 32x32 | 32 | `icons/icon_19_4_32x32.png` |
| shell32_mun | RT_ICON | 2180 (group 16807) | 256x256 | 32 | `icons/icon_16807_0_256x256.png` |
| shell32_mun | RT_ICON | 2181 (group 16807) | 64x64 | 32 | `icons/icon_16807_1_64x64.png` |
| shell32_mun | RT_ICON | 2182 (group 16807) | 48x48 | 32 | `icons/icon_16807_2_48x48.png` |
| shell32_mun | RT_ICON | 2183 (group 16807) | 40x40 | 32 | `icons/icon_16807_3_40x40.png` |
| shell32_mun | RT_ICON | 2184 (group 16807) | 32x32 | 32 | `icons/icon_16807_4_32x32.png` |
| shell32_mun | RT_ICON | 2185 (group 16807) | 24x24 | 32 | `icons/icon_16807_5_24x24.png` |
| shell32_mun | RT_ICON | 2186 (group 16807) | 20x20 | 32 | `icons/icon_16807_6_20x20.png` |
| shell32_mun | RT_ICON | 2187 (group 16807) | 16x16 | 32 | `icons/icon_16807_7_16x16.png` |
| shell32_mun | RT_ICON | 2188 (group 16747) | 256x256 | 32 | `icons/icon_16747_0_256x256.png` |
| shell32_mun | RT_ICON | 2189 (group 16747) | 64x64 | 32 | `icons/icon_16747_1_64x64.png` |
| shell32_mun | RT_ICON | 219 (group 19) | 24x24 | 32 | `icons/icon_19_5_24x24.png` |
| shell32_mun | RT_ICON | 2190 (group 16747) | 48x48 | 32 | `icons/icon_16747_2_48x48.png` |
| shell32_mun | RT_ICON | 2191 (group 16747) | 40x40 | 32 | `icons/icon_16747_3_40x40.png` |
| shell32_mun | RT_ICON | 2192 (group 16747) | 32x32 | 32 | `icons/icon_16747_4_32x32.png` |
| shell32_mun | RT_ICON | 2193 (group 16747) | 24x24 | 32 | `icons/icon_16747_5_24x24.png` |
| shell32_mun | RT_ICON | 2194 (group 16747) | 20x20 | 32 | `icons/icon_16747_6_20x20.png` |
| shell32_mun | RT_ICON | 2195 (group 16747) | 16x16 | 32 | `icons/icon_16747_7_16x16.png` |
| shell32_mun | RT_ICON | 2196 (group 16783) | 256x256 | 32 | `icons/icon_16783_0_256x256.png` |
| shell32_mun | RT_ICON | 2197 (group 16783) | 64x64 | 32 | `icons/icon_16783_1_64x64.png` |
| shell32_mun | RT_ICON | 2198 (group 16783) | 48x48 | 32 | `icons/icon_16783_2_48x48.png` |
| shell32_mun | RT_ICON | 2199 (group 16783) | 40x40 | 32 | `icons/icon_16783_3_40x40.png` |
| shell32_mun | RT_ICON | 22 (group 63000) | 24x24 | 32 | `icons/icon_63000_5_24x24.png` |
| shell32_mun | RT_ICON | 220 (group 19) | 20x20 | 32 | `icons/icon_19_6_20x20.png` |
| shell32_mun | RT_ICON | 2200 (group 16783) | 32x32 | 32 | `icons/icon_16783_4_32x32.png` |
| shell32_mun | RT_ICON | 2201 (group 16783) | 24x24 | 32 | `icons/icon_16783_5_24x24.png` |
| shell32_mun | RT_ICON | 2202 (group 16783) | 20x20 | 32 | `icons/icon_16783_6_20x20.png` |
| shell32_mun | RT_ICON | 2203 (group 16783) | 16x16 | 32 | `icons/icon_16783_7_16x16.png` |
| shell32_mun | RT_ICON | 2204 (group 16748) | 256x256 | 32 | `icons/icon_16748_0_256x256.png` |
| shell32_mun | RT_ICON | 2205 (group 16748) | 64x64 | 32 | `icons/icon_16748_1_64x64.png` |
| shell32_mun | RT_ICON | 2206 (group 16748) | 48x48 | 32 | `icons/icon_16748_2_48x48.png` |
| shell32_mun | RT_ICON | 2207 (group 16748) | 40x40 | 32 | `icons/icon_16748_3_40x40.png` |
| shell32_mun | RT_ICON | 2208 (group 16748) | 32x32 | 32 | `icons/icon_16748_4_32x32.png` |
| shell32_mun | RT_ICON | 2209 (group 16748) | 24x24 | 32 | `icons/icon_16748_5_24x24.png` |
| shell32_mun | RT_ICON | 221 (group 19) | 16x16 | 32 | `icons/icon_19_7_16x16.png` |
| shell32_mun | RT_ICON | 2210 (group 16748) | 20x20 | 32 | `icons/icon_16748_6_20x20.png` |
| shell32_mun | RT_ICON | 2211 (group 16748) | 16x16 | 32 | `icons/icon_16748_7_16x16.png` |
| shell32_mun | RT_ICON | 2212 (group 16749) | 32x32 | 4 | `icons/icon_16749_0_32x32.png` |
| shell32_mun | RT_ICON | 2213 (group 16749) | 24x24 | 4 | `icons/icon_16749_1_24x24.png` |
| shell32_mun | RT_ICON | 2214 (group 16749) | 20x20 | 4 | `icons/icon_16749_2_20x20.png` |
| shell32_mun | RT_ICON | 2215 (group 16749) | 16x16 | 4 | `icons/icon_16749_3_16x16.png` |
| shell32_mun | RT_ICON | 2216 (group 16749) | 32x32 | 8 | `icons/icon_16749_4_32x32.png` |
| shell32_mun | RT_ICON | 2217 (group 16749) | 24x24 | 8 | `icons/icon_16749_5_24x24.png` |
| shell32_mun | RT_ICON | 2218 (group 16749) | 20x20 | 8 | `icons/icon_16749_6_20x20.png` |
| shell32_mun | RT_ICON | 2219 (group 16749) | 16x16 | 8 | `icons/icon_16749_7_16x16.png` |
| shell32_mun | RT_ICON | 222 (group 20) | 256x256 | 32 | `icons/icon_20_0_256x256.png` |
| shell32_mun | RT_ICON | 2220 (group 16749) | 32x32 | 32 | `icons/icon_16749_8_32x32.png` |
| shell32_mun | RT_ICON | 2221 (group 16749) | 24x24 | 32 | `icons/icon_16749_9_24x24.png` |
| shell32_mun | RT_ICON | 2222 (group 16749) | 20x20 | 32 | `icons/icon_16749_10_20x20.png` |
| shell32_mun | RT_ICON | 2223 (group 16749) | 16x16 | 32 | `icons/icon_16749_11_16x16.png` |
| shell32_mun | RT_ICON | 2224 (group 16750) | 32x32 | 4 | `icons/icon_16750_0_32x32.png` |
| shell32_mun | RT_ICON | 2225 (group 16750) | 24x24 | 4 | `icons/icon_16750_1_24x24.png` |
| shell32_mun | RT_ICON | 2226 (group 16750) | 20x20 | 4 | `icons/icon_16750_2_20x20.png` |
| shell32_mun | RT_ICON | 2227 (group 16750) | 16x16 | 4 | `icons/icon_16750_3_16x16.png` |
| shell32_mun | RT_ICON | 2228 (group 16750) | 32x32 | 8 | `icons/icon_16750_4_32x32.png` |
| shell32_mun | RT_ICON | 2229 (group 16750) | 24x24 | 8 | `icons/icon_16750_5_24x24.png` |
| shell32_mun | RT_ICON | 223 (group 20) | 64x64 | 32 | `icons/icon_20_1_64x64.png` |
| shell32_mun | RT_ICON | 2230 (group 16750) | 20x20 | 8 | `icons/icon_16750_6_20x20.png` |
| shell32_mun | RT_ICON | 2231 (group 16750) | 16x16 | 8 | `icons/icon_16750_7_16x16.png` |
| shell32_mun | RT_ICON | 2232 (group 16750) | 32x32 | 32 | `icons/icon_16750_8_32x32.png` |
| shell32_mun | RT_ICON | 2233 (group 16750) | 24x24 | 32 | `icons/icon_16750_9_24x24.png` |
| shell32_mun | RT_ICON | 2234 (group 16750) | 20x20 | 32 | `icons/icon_16750_10_20x20.png` |
| shell32_mun | RT_ICON | 2235 (group 16750) | 16x16 | 32 | `icons/icon_16750_11_16x16.png` |
| shell32_mun | RT_ICON | 2236 (group 16751) | 16x16 | 32 | `icons/icon_16751_0_16x16.png` |
| shell32_mun | RT_ICON | 2237 (group 16746) | 48x48 | 8 | `icons/icon_16746_0_48x48.png` |
| shell32_mun | RT_ICON | 2238 (group 16746) | 32x32 | 8 | `icons/icon_16746_1_32x32.png` |
| shell32_mun | RT_ICON | 2239 (group 16746) | 16x16 | 8 | `icons/icon_16746_2_16x16.png` |
| shell32_mun | RT_ICON | 224 (group 20) | 48x48 | 32 | `icons/icon_20_2_48x48.png` |
| shell32_mun | RT_ICON | 2240 (group 16746) | 48x48 | 32 | `icons/icon_16746_3_48x48.png` |
| shell32_mun | RT_ICON | 2241 (group 16746) | 32x32 | 32 | `icons/icon_16746_4_32x32.png` |
| shell32_mun | RT_ICON | 2242 (group 16746) | 16x16 | 32 | `icons/icon_16746_5_16x16.png` |
| shell32_mun | RT_ICON | 2243 (group 16752) | 256x256 | 32 | `icons/icon_16752_0_256x256.png` |
| shell32_mun | RT_ICON | 2244 (group 16752) | 64x64 | 32 | `icons/icon_16752_1_64x64.png` |
| shell32_mun | RT_ICON | 2245 (group 16752) | 48x48 | 32 | `icons/icon_16752_2_48x48.png` |
| shell32_mun | RT_ICON | 2246 (group 16752) | 40x40 | 32 | `icons/icon_16752_3_40x40.png` |
| shell32_mun | RT_ICON | 2247 (group 16752) | 32x32 | 32 | `icons/icon_16752_4_32x32.png` |
| shell32_mun | RT_ICON | 2248 (group 16752) | 24x24 | 32 | `icons/icon_16752_5_24x24.png` |
| shell32_mun | RT_ICON | 2249 (group 16752) | 20x20 | 32 | `icons/icon_16752_6_20x20.png` |
| shell32_mun | RT_ICON | 225 (group 20) | 40x40 | 32 | `icons/icon_20_3_40x40.png` |
| shell32_mun | RT_ICON | 2250 (group 16752) | 16x16 | 32 | `icons/icon_16752_7_16x16.png` |
| shell32_mun | RT_ICON | 2251 (group 16784) | 256x256 | 32 | `icons/icon_16784_0_256x256.png` |
| shell32_mun | RT_ICON | 2252 (group 16784) | 64x64 | 32 | `icons/icon_16784_1_64x64.png` |
| shell32_mun | RT_ICON | 2253 (group 16784) | 48x48 | 32 | `icons/icon_16784_2_48x48.png` |
| shell32_mun | RT_ICON | 2254 (group 16784) | 40x40 | 32 | `icons/icon_16784_3_40x40.png` |
| shell32_mun | RT_ICON | 2255 (group 16784) | 32x32 | 32 | `icons/icon_16784_4_32x32.png` |
| shell32_mun | RT_ICON | 2256 (group 16784) | 24x24 | 32 | `icons/icon_16784_5_24x24.png` |
| shell32_mun | RT_ICON | 2257 (group 16784) | 20x20 | 32 | `icons/icon_16784_6_20x20.png` |
| shell32_mun | RT_ICON | 2258 (group 16784) | 16x16 | 32 | `icons/icon_16784_7_16x16.png` |
| shell32_mun | RT_ICON | 2259 (group 16797) | 16x16 | 4 | `icons/icon_16797_0_16x16.png` |
| shell32_mun | RT_ICON | 226 (group 20) | 32x32 | 32 | `icons/icon_20_4_32x32.png` |
| shell32_mun | RT_ICON | 2260 (group 16797) | 16x16 | 8 | `icons/icon_16797_1_16x16.png` |
| shell32_mun | RT_ICON | 2261 (group 16797) | 16x16 | 32 | `icons/icon_16797_2_16x16.png` |
| shell32_mun | RT_ICON | 2262 (group 16798) | 16x16 | 4 | `icons/icon_16798_0_16x16.png` |
| shell32_mun | RT_ICON | 2263 (group 16798) | 16x16 | 8 | `icons/icon_16798_1_16x16.png` |
| shell32_mun | RT_ICON | 2264 (group 16798) | 16x16 | 32 | `icons/icon_16798_2_16x16.png` |
| shell32_mun | RT_ICON | 2265 (group 16787) | 16x16 | 4 | `icons/icon_16787_0_16x16.png` |
| shell32_mun | RT_ICON | 2266 (group 16787) | 16x16 | 8 | `icons/icon_16787_1_16x16.png` |
| shell32_mun | RT_ICON | 2267 (group 16787) | 16x16 | 32 | `icons/icon_16787_2_16x16.png` |
| shell32_mun | RT_ICON | 2268 (group 16788) | 16x16 | 4 | `icons/icon_16788_0_16x16.png` |
| shell32_mun | RT_ICON | 2269 (group 16788) | 16x16 | 8 | `icons/icon_16788_1_16x16.png` |
| shell32_mun | RT_ICON | 227 (group 20) | 24x24 | 32 | `icons/icon_20_5_24x24.png` |
| shell32_mun | RT_ICON | 2270 (group 16788) | 16x16 | 32 | `icons/icon_16788_2_16x16.png` |
| shell32_mun | RT_ICON | 2271 (group 16789) | 16x16 | 4 | `icons/icon_16789_0_16x16.png` |
| shell32_mun | RT_ICON | 2272 (group 16789) | 16x16 | 8 | `icons/icon_16789_1_16x16.png` |
| shell32_mun | RT_ICON | 2273 (group 16789) | 16x16 | 32 | `icons/icon_16789_2_16x16.png` |
| shell32_mun | RT_ICON | 2274 (group 16790) | 16x16 | 4 | `icons/icon_16790_0_16x16.png` |
| shell32_mun | RT_ICON | 2275 (group 16790) | 16x16 | 8 | `icons/icon_16790_1_16x16.png` |
| shell32_mun | RT_ICON | 2276 (group 16790) | 16x16 | 32 | `icons/icon_16790_2_16x16.png` |
| shell32_mun | RT_ICON | 2277 (group 16791) | 16x16 | 4 | `icons/icon_16791_0_16x16.png` |
| shell32_mun | RT_ICON | 2278 (group 16791) | 16x16 | 8 | `icons/icon_16791_1_16x16.png` |
| shell32_mun | RT_ICON | 2279 (group 16791) | 16x16 | 32 | `icons/icon_16791_2_16x16.png` |
| shell32_mun | RT_ICON | 228 (group 20) | 20x20 | 32 | `icons/icon_20_6_20x20.png` |
| shell32_mun | RT_ICON | 2280 (group 16792) | 16x16 | 4 | `icons/icon_16792_0_16x16.png` |
| shell32_mun | RT_ICON | 2281 (group 16792) | 16x16 | 8 | `icons/icon_16792_1_16x16.png` |
| shell32_mun | RT_ICON | 2282 (group 16792) | 16x16 | 32 | `icons/icon_16792_2_16x16.png` |
| shell32_mun | RT_ICON | 2283 (group 16793) | 16x16 | 4 | `icons/icon_16793_0_16x16.png` |
| shell32_mun | RT_ICON | 2284 (group 16793) | 16x16 | 8 | `icons/icon_16793_1_16x16.png` |
| shell32_mun | RT_ICON | 2285 (group 16793) | 16x16 | 32 | `icons/icon_16793_2_16x16.png` |
| shell32_mun | RT_ICON | 2286 (group 16794) | 16x16 | 4 | `icons/icon_16794_0_16x16.png` |
| shell32_mun | RT_ICON | 2287 (group 16794) | 16x16 | 8 | `icons/icon_16794_1_16x16.png` |
| shell32_mun | RT_ICON | 2288 (group 16794) | 16x16 | 32 | `icons/icon_16794_2_16x16.png` |
| shell32_mun | RT_ICON | 2289 (group 16795) | 16x16 | 4 | `icons/icon_16795_0_16x16.png` |
| shell32_mun | RT_ICON | 229 (group 20) | 16x16 | 32 | `icons/icon_20_7_16x16.png` |
| shell32_mun | RT_ICON | 2290 (group 16795) | 16x16 | 8 | `icons/icon_16795_1_16x16.png` |
| shell32_mun | RT_ICON | 2291 (group 16795) | 16x16 | 32 | `icons/icon_16795_2_16x16.png` |
| shell32_mun | RT_ICON | 2292 (group 16796) | 16x16 | 4 | `icons/icon_16796_0_16x16.png` |
| shell32_mun | RT_ICON | 2293 (group 16796) | 16x16 | 8 | `icons/icon_16796_1_16x16.png` |
| shell32_mun | RT_ICON | 2294 (group 16796) | 16x16 | 32 | `icons/icon_16796_2_16x16.png` |
| shell32_mun | RT_ICON | 2295 (group 16801) | 256x256 | 32 | `icons/icon_16801_0_256x256.png` |
| shell32_mun | RT_ICON | 2296 (group 16801) | 64x64 | 32 | `icons/icon_16801_1_64x64.png` |
| shell32_mun | RT_ICON | 2297 (group 16801) | 48x48 | 32 | `icons/icon_16801_2_48x48.png` |
| shell32_mun | RT_ICON | 2298 (group 16801) | 40x40 | 32 | `icons/icon_16801_3_40x40.png` |
| shell32_mun | RT_ICON | 2299 (group 16801) | 32x32 | 32 | `icons/icon_16801_4_32x32.png` |
| shell32_mun | RT_ICON | 23 (group 63000) | 20x20 | 32 | `icons/icon_63000_6_20x20.png` |
| shell32_mun | RT_ICON | 230 (group 21) | 256x256 | 32 | `icons/icon_21_0_256x256.png` |
| shell32_mun | RT_ICON | 2300 (group 16801) | 24x24 | 32 | `icons/icon_16801_5_24x24.png` |
| shell32_mun | RT_ICON | 2301 (group 16801) | 20x20 | 32 | `icons/icon_16801_6_20x20.png` |
| shell32_mun | RT_ICON | 2302 (group 16801) | 16x16 | 32 | `icons/icon_16801_7_16x16.png` |
| shell32_mun | RT_ICON | 2303 (group 16802) | 256x256 | 32 | `icons/icon_16802_0_256x256.png` |
| shell32_mun | RT_ICON | 2304 (group 16802) | 64x64 | 32 | `icons/icon_16802_1_64x64.png` |
| shell32_mun | RT_ICON | 2305 (group 16802) | 48x48 | 32 | `icons/icon_16802_2_48x48.png` |
| shell32_mun | RT_ICON | 2306 (group 16802) | 40x40 | 32 | `icons/icon_16802_3_40x40.png` |
| shell32_mun | RT_ICON | 2307 (group 16802) | 32x32 | 32 | `icons/icon_16802_4_32x32.png` |
| shell32_mun | RT_ICON | 2308 (group 16802) | 24x24 | 32 | `icons/icon_16802_5_24x24.png` |
| shell32_mun | RT_ICON | 2309 (group 16802) | 20x20 | 32 | `icons/icon_16802_6_20x20.png` |
| shell32_mun | RT_ICON | 231 (group 21) | 64x64 | 32 | `icons/icon_21_1_64x64.png` |
| shell32_mun | RT_ICON | 2310 (group 16802) | 16x16 | 32 | `icons/icon_16802_7_16x16.png` |
| shell32_mun | RT_ICON | 2311 (group 16803) | 256x256 | 32 | `icons/icon_16803_0_256x256.png` |
| shell32_mun | RT_ICON | 2312 (group 16803) | 64x64 | 32 | `icons/icon_16803_1_64x64.png` |
| shell32_mun | RT_ICON | 2313 (group 16803) | 48x48 | 32 | `icons/icon_16803_2_48x48.png` |
| shell32_mun | RT_ICON | 2314 (group 16803) | 40x40 | 32 | `icons/icon_16803_3_40x40.png` |
| shell32_mun | RT_ICON | 2315 (group 16803) | 32x32 | 32 | `icons/icon_16803_4_32x32.png` |
| shell32_mun | RT_ICON | 2316 (group 16803) | 24x24 | 32 | `icons/icon_16803_5_24x24.png` |
| shell32_mun | RT_ICON | 2317 (group 16803) | 20x20 | 32 | `icons/icon_16803_6_20x20.png` |
| shell32_mun | RT_ICON | 2318 (group 16803) | 16x16 | 32 | `icons/icon_16803_7_16x16.png` |
| shell32_mun | RT_ICON | 2319 (group 16805) | 256x256 | 32 | `icons/icon_16805_0_256x256.png` |
| shell32_mun | RT_ICON | 232 (group 21) | 48x48 | 32 | `icons/icon_21_2_48x48.png` |
| shell32_mun | RT_ICON | 2320 (group 16805) | 64x64 | 32 | `icons/icon_16805_1_64x64.png` |
| shell32_mun | RT_ICON | 2321 (group 16805) | 48x48 | 32 | `icons/icon_16805_2_48x48.png` |
| shell32_mun | RT_ICON | 2322 (group 16805) | 40x40 | 32 | `icons/icon_16805_3_40x40.png` |
| shell32_mun | RT_ICON | 2323 (group 16805) | 32x32 | 32 | `icons/icon_16805_4_32x32.png` |
| shell32_mun | RT_ICON | 2324 (group 16805) | 24x24 | 32 | `icons/icon_16805_5_24x24.png` |
| shell32_mun | RT_ICON | 2325 (group 16805) | 20x20 | 32 | `icons/icon_16805_6_20x20.png` |
| shell32_mun | RT_ICON | 2326 (group 16805) | 16x16 | 32 | `icons/icon_16805_7_16x16.png` |
| shell32_mun | RT_ICON | 2327 (group 16817) | 80x80 | 32 | `icons/icon_16817_0_80x80.png` |
| shell32_mun | RT_ICON | 2328 (group 16817) | 64x64 | 32 | `icons/icon_16817_1_64x64.png` |
| shell32_mun | RT_ICON | 2329 (group 16817) | 48x48 | 32 | `icons/icon_16817_2_48x48.png` |
| shell32_mun | RT_ICON | 233 (group 21) | 40x40 | 32 | `icons/icon_21_3_40x40.png` |
| shell32_mun | RT_ICON | 2330 (group 16817) | 42x41 | 32 | `icons/icon_16817_3_42x41.png` |
| shell32_mun | RT_ICON | 2331 (group 16817) | 34x33 | 32 | `icons/icon_16817_4_34x33.png` |
| shell32_mun | RT_ICON | 2332 (group 16817) | 24x24 | 32 | `icons/icon_16817_5_24x24.png` |
| shell32_mun | RT_ICON | 2333 (group 16817) | 20x20 | 32 | `icons/icon_16817_6_20x20.png` |
| shell32_mun | RT_ICON | 2334 (group 16817) | 16x16 | 32 | `icons/icon_16817_7_16x16.png` |
| shell32_mun | RT_ICON | 2335 (group 16818) | 80x80 | 32 | `icons/icon_16818_0_80x80.png` |
| shell32_mun | RT_ICON | 2336 (group 16818) | 64x64 | 32 | `icons/icon_16818_1_64x64.png` |
| shell32_mun | RT_ICON | 2337 (group 16818) | 48x48 | 32 | `icons/icon_16818_2_48x48.png` |
| shell32_mun | RT_ICON | 2338 (group 16818) | 42x41 | 32 | `icons/icon_16818_3_42x41.png` |
| shell32_mun | RT_ICON | 2339 (group 16818) | 34x33 | 32 | `icons/icon_16818_4_34x33.png` |
| shell32_mun | RT_ICON | 234 (group 21) | 32x32 | 32 | `icons/icon_21_4_32x32.png` |
| shell32_mun | RT_ICON | 2340 (group 16818) | 24x24 | 32 | `icons/icon_16818_5_24x24.png` |
| shell32_mun | RT_ICON | 2341 (group 16818) | 20x20 | 32 | `icons/icon_16818_6_20x20.png` |
| shell32_mun | RT_ICON | 2342 (group 16818) | 16x16 | 32 | `icons/icon_16818_7_16x16.png` |
| shell32_mun | RT_ICON | 2343 (group 16827) | 80x80 | 32 | `icons/icon_16827_0_80x80.png` |
| shell32_mun | RT_ICON | 2344 (group 16827) | 64x64 | 32 | `icons/icon_16827_1_64x64.png` |
| shell32_mun | RT_ICON | 2345 (group 16827) | 48x48 | 32 | `icons/icon_16827_2_48x48.png` |
| shell32_mun | RT_ICON | 2346 (group 16827) | 42x41 | 32 | `icons/icon_16827_3_42x41.png` |
| shell32_mun | RT_ICON | 2347 (group 16827) | 34x33 | 32 | `icons/icon_16827_4_34x33.png` |
| shell32_mun | RT_ICON | 2348 (group 16827) | 24x24 | 32 | `icons/icon_16827_5_24x24.png` |
| shell32_mun | RT_ICON | 2349 (group 16827) | 20x20 | 32 | `icons/icon_16827_6_20x20.png` |
| shell32_mun | RT_ICON | 235 (group 21) | 24x24 | 32 | `icons/icon_21_5_24x24.png` |
| shell32_mun | RT_ICON | 2350 (group 16827) | 16x16 | 32 | `icons/icon_16827_7_16x16.png` |
| shell32_mun | RT_ICON | 2351 (group 16819) | 256x256 | 32 | `icons/icon_16819_0_256x256.png` |
| shell32_mun | RT_ICON | 2352 (group 16819) | 48x48 | 32 | `icons/icon_16819_1_48x48.png` |
| shell32_mun | RT_ICON | 2353 (group 16819) | 32x32 | 32 | `icons/icon_16819_2_32x32.png` |
| shell32_mun | RT_ICON | 2354 (group 16819) | 24x24 | 32 | `icons/icon_16819_3_24x24.png` |
| shell32_mun | RT_ICON | 2355 (group 16819) | 16x16 | 32 | `icons/icon_16819_4_16x16.png` |
| shell32_mun | RT_ICON | 2356 (group 16820) | 58x58 | 32 | `icons/icon_16820_0_58x58.png` |
| shell32_mun | RT_ICON | 2357 (group 16820) | 45x45 | 32 | `icons/icon_16820_1_45x45.png` |
| shell32_mun | RT_ICON | 2358 (group 16820) | 32x32 | 32 | `icons/icon_16820_2_32x32.png` |
| shell32_mun | RT_ICON | 2359 (group 16821) | 58x58 | 32 | `icons/icon_16821_0_58x58.png` |
| shell32_mun | RT_ICON | 236 (group 21) | 20x20 | 32 | `icons/icon_21_6_20x20.png` |
| shell32_mun | RT_ICON | 2360 (group 16821) | 45x45 | 32 | `icons/icon_16821_1_45x45.png` |
| shell32_mun | RT_ICON | 2361 (group 16821) | 32x32 | 32 | `icons/icon_16821_2_32x32.png` |
| shell32_mun | RT_ICON | 2362 (group 16822) | 256x256 | 32 | `icons/icon_16822_0_256x256.png` |
| shell32_mun | RT_ICON | 2363 (group 16822) | 64x64 | 32 | `icons/icon_16822_1_64x64.png` |
| shell32_mun | RT_ICON | 2364 (group 16822) | 48x48 | 32 | `icons/icon_16822_2_48x48.png` |
| shell32_mun | RT_ICON | 2365 (group 16822) | 40x40 | 32 | `icons/icon_16822_3_40x40.png` |
| shell32_mun | RT_ICON | 2366 (group 16822) | 32x32 | 32 | `icons/icon_16822_4_32x32.png` |
| shell32_mun | RT_ICON | 2367 (group 16822) | 24x24 | 32 | `icons/icon_16822_5_24x24.png` |
| shell32_mun | RT_ICON | 2368 (group 16822) | 20x20 | 32 | `icons/icon_16822_6_20x20.png` |
| shell32_mun | RT_ICON | 2369 (group 16822) | 16x16 | 32 | `icons/icon_16822_7_16x16.png` |
| shell32_mun | RT_ICON | 237 (group 21) | 16x16 | 32 | `icons/icon_21_7_16x16.png` |
| shell32_mun | RT_ICON | 2370 (group 16823) | 256x256 | 32 | `icons/icon_16823_0_256x256.png` |
| shell32_mun | RT_ICON | 2371 (group 16823) | 64x64 | 32 | `icons/icon_16823_1_64x64.png` |
| shell32_mun | RT_ICON | 2372 (group 16823) | 48x48 | 32 | `icons/icon_16823_2_48x48.png` |
| shell32_mun | RT_ICON | 2373 (group 16823) | 40x40 | 32 | `icons/icon_16823_3_40x40.png` |
| shell32_mun | RT_ICON | 2374 (group 16823) | 32x32 | 32 | `icons/icon_16823_4_32x32.png` |
| shell32_mun | RT_ICON | 2375 (group 16823) | 24x24 | 32 | `icons/icon_16823_5_24x24.png` |
| shell32_mun | RT_ICON | 2376 (group 16823) | 20x20 | 32 | `icons/icon_16823_6_20x20.png` |
| shell32_mun | RT_ICON | 2377 (group 16823) | 16x16 | 32 | `icons/icon_16823_7_16x16.png` |
| shell32_mun | RT_ICON | 2378 (group 16824) | 256x256 | 32 | `icons/icon_16824_0_256x256.png` |
| shell32_mun | RT_ICON | 2379 (group 16824) | 64x64 | 32 | `icons/icon_16824_1_64x64.png` |
| shell32_mun | RT_ICON | 238 (group 22) | 256x256 | 32 | `icons/icon_22_0_256x256.png` |
| shell32_mun | RT_ICON | 2380 (group 16824) | 48x48 | 32 | `icons/icon_16824_2_48x48.png` |
| shell32_mun | RT_ICON | 2381 (group 16824) | 40x40 | 32 | `icons/icon_16824_3_40x40.png` |
| shell32_mun | RT_ICON | 2382 (group 16824) | 32x32 | 32 | `icons/icon_16824_4_32x32.png` |
| shell32_mun | RT_ICON | 2383 (group 16824) | 24x24 | 32 | `icons/icon_16824_5_24x24.png` |
| shell32_mun | RT_ICON | 2384 (group 16824) | 20x20 | 32 | `icons/icon_16824_6_20x20.png` |
| shell32_mun | RT_ICON | 2385 (group 16824) | 16x16 | 32 | `icons/icon_16824_7_16x16.png` |
| shell32_mun | RT_ICON | 2386 (group 16825) | 256x256 | 32 | `icons/icon_16825_0_256x256.png` |
| shell32_mun | RT_ICON | 2387 (group 16825) | 64x64 | 32 | `icons/icon_16825_1_64x64.png` |
| shell32_mun | RT_ICON | 2388 (group 16825) | 48x48 | 32 | `icons/icon_16825_2_48x48.png` |
| shell32_mun | RT_ICON | 2389 (group 16825) | 40x40 | 32 | `icons/icon_16825_3_40x40.png` |
| shell32_mun | RT_ICON | 239 (group 22) | 64x64 | 32 | `icons/icon_22_1_64x64.png` |
| shell32_mun | RT_ICON | 2390 (group 16825) | 32x32 | 32 | `icons/icon_16825_4_32x32.png` |
| shell32_mun | RT_ICON | 2391 (group 16825) | 24x24 | 32 | `icons/icon_16825_5_24x24.png` |
| shell32_mun | RT_ICON | 2392 (group 16825) | 20x20 | 32 | `icons/icon_16825_6_20x20.png` |
| shell32_mun | RT_ICON | 2393 (group 16825) | 16x16 | 32 | `icons/icon_16825_7_16x16.png` |
| shell32_mun | RT_ICON | 2394 (group 16826) | 256x256 | 32 | `icons/icon_16826_0_256x256.png` |
| shell32_mun | RT_ICON | 2395 (group 16826) | 64x64 | 32 | `icons/icon_16826_1_64x64.png` |
| shell32_mun | RT_ICON | 2396 (group 16826) | 48x48 | 32 | `icons/icon_16826_2_48x48.png` |
| shell32_mun | RT_ICON | 2397 (group 16826) | 40x40 | 32 | `icons/icon_16826_3_40x40.png` |
| shell32_mun | RT_ICON | 2398 (group 16826) | 32x32 | 32 | `icons/icon_16826_4_32x32.png` |
| shell32_mun | RT_ICON | 2399 (group 16826) | 24x24 | 32 | `icons/icon_16826_5_24x24.png` |
| shell32_mun | RT_ICON | 24 (group 63000) | 16x16 | 32 | `icons/icon_63000_7_16x16.png` |
| shell32_mun | RT_ICON | 240 (group 22) | 48x48 | 32 | `icons/icon_22_2_48x48.png` |
| shell32_mun | RT_ICON | 2400 (group 16826) | 20x20 | 32 | `icons/icon_16826_6_20x20.png` |
| shell32_mun | RT_ICON | 2401 (group 16826) | 16x16 | 32 | `icons/icon_16826_7_16x16.png` |
| shell32_mun | RT_ICON | 2402 (group 200) | 256x256 | 32 | `icons/icon_200_0_256x256.png` |
| shell32_mun | RT_ICON | 2403 (group 200) | 64x64 | 32 | `icons/icon_200_1_64x64.png` |
| shell32_mun | RT_ICON | 2404 (group 200) | 48x48 | 32 | `icons/icon_200_2_48x48.png` |
| shell32_mun | RT_ICON | 2405 (group 200) | 40x40 | 32 | `icons/icon_200_3_40x40.png` |
| shell32_mun | RT_ICON | 2406 (group 200) | 32x32 | 32 | `icons/icon_200_4_32x32.png` |
| shell32_mun | RT_ICON | 2407 (group 200) | 24x24 | 32 | `icons/icon_200_5_24x24.png` |
| shell32_mun | RT_ICON | 2408 (group 200) | 20x20 | 32 | `icons/icon_200_6_20x20.png` |
| shell32_mun | RT_ICON | 2409 (group 200) | 16x16 | 32 | `icons/icon_200_7_16x16.png` |
| shell32_mun | RT_ICON | 241 (group 22) | 40x40 | 32 | `icons/icon_22_3_40x40.png` |
| shell32_mun | RT_ICON | 2410 (group 210) | 256x256 | 32 | `icons/icon_210_0_256x256.png` |
| shell32_mun | RT_ICON | 2411 (group 210) | 64x64 | 32 | `icons/icon_210_1_64x64.png` |
| shell32_mun | RT_ICON | 2412 (group 210) | 48x48 | 32 | `icons/icon_210_2_48x48.png` |
| shell32_mun | RT_ICON | 2413 (group 210) | 40x40 | 32 | `icons/icon_210_3_40x40.png` |
| shell32_mun | RT_ICON | 2414 (group 210) | 32x32 | 32 | `icons/icon_210_4_32x32.png` |
| shell32_mun | RT_ICON | 2415 (group 210) | 24x24 | 32 | `icons/icon_210_5_24x24.png` |
| shell32_mun | RT_ICON | 2416 (group 210) | 20x20 | 32 | `icons/icon_210_6_20x20.png` |
| shell32_mun | RT_ICON | 2417 (group 210) | 16x16 | 32 | `icons/icon_210_7_16x16.png` |
| shell32_mun | RT_ICON | 2418 (group 16744) | 48x48 | 4 | `icons/icon_16744_0_48x48.png` |
| shell32_mun | RT_ICON | 2419 (group 16744) | 32x32 | 4 | `icons/icon_16744_1_32x32.png` |
| shell32_mun | RT_ICON | 242 (group 22) | 32x32 | 32 | `icons/icon_22_4_32x32.png` |
| shell32_mun | RT_ICON | 2420 (group 16744) | 16x16 | 4 | `icons/icon_16744_2_16x16.png` |
| shell32_mun | RT_ICON | 2421 (group 16744) | 48x48 | 8 | `icons/icon_16744_3_48x48.png` |
| shell32_mun | RT_ICON | 2422 (group 16744) | 32x32 | 8 | `icons/icon_16744_4_32x32.png` |
| shell32_mun | RT_ICON | 2423 (group 16744) | 16x16 | 8 | `icons/icon_16744_5_16x16.png` |
| shell32_mun | RT_ICON | 2424 (group 16744) | 48x48 | 32 | `icons/icon_16744_6_48x48.png` |
| shell32_mun | RT_ICON | 2425 (group 16744) | 32x32 | 32 | `icons/icon_16744_7_32x32.png` |
| shell32_mun | RT_ICON | 2426 (group 16744) | 16x16 | 32 | `icons/icon_16744_8_16x16.png` |
| shell32_mun | RT_ICON | 2427 (group 51680) | 256x256 | 32 | `icons/icon_51680_0_256x256.png` |
| shell32_mun | RT_ICON | 2428 (group 51680) | 64x64 | 32 | `icons/icon_51680_1_64x64.png` |
| shell32_mun | RT_ICON | 2429 (group 51680) | 48x48 | 32 | `icons/icon_51680_2_48x48.png` |
| shell32_mun | RT_ICON | 243 (group 22) | 24x24 | 32 | `icons/icon_22_5_24x24.png` |
| shell32_mun | RT_ICON | 2430 (group 51680) | 40x40 | 32 | `icons/icon_51680_3_40x40.png` |
| shell32_mun | RT_ICON | 2431 (group 51680) | 32x32 | 32 | `icons/icon_51680_4_32x32.png` |
| shell32_mun | RT_ICON | 2432 (group 51680) | 24x24 | 32 | `icons/icon_51680_5_24x24.png` |
| shell32_mun | RT_ICON | 2433 (group 51680) | 20x20 | 32 | `icons/icon_51680_6_20x20.png` |
| shell32_mun | RT_ICON | 2434 (group 51680) | 16x16 | 32 | `icons/icon_51680_7_16x16.png` |
| shell32_mun | RT_ICON | 2435 (group 51681) | 256x256 | 32 | `icons/icon_51681_0_256x256.png` |
| shell32_mun | RT_ICON | 2436 (group 51681) | 64x64 | 32 | `icons/icon_51681_1_64x64.png` |
| shell32_mun | RT_ICON | 2437 (group 51681) | 48x48 | 32 | `icons/icon_51681_2_48x48.png` |
| shell32_mun | RT_ICON | 2438 (group 51681) | 40x40 | 32 | `icons/icon_51681_3_40x40.png` |
| shell32_mun | RT_ICON | 2439 (group 51681) | 32x32 | 32 | `icons/icon_51681_4_32x32.png` |
| shell32_mun | RT_ICON | 244 (group 22) | 20x20 | 32 | `icons/icon_22_6_20x20.png` |
| shell32_mun | RT_ICON | 2440 (group 51681) | 24x24 | 32 | `icons/icon_51681_5_24x24.png` |
| shell32_mun | RT_ICON | 2441 (group 51681) | 20x20 | 32 | `icons/icon_51681_6_20x20.png` |
| shell32_mun | RT_ICON | 2442 (group 51681) | 16x16 | 32 | `icons/icon_51681_7_16x16.png` |
| shell32_mun | RT_ICON | 2443 (group 51682) | 256x256 | 32 | `icons/icon_51682_0_256x256.png` |
| shell32_mun | RT_ICON | 2444 (group 51682) | 64x64 | 32 | `icons/icon_51682_1_64x64.png` |
| shell32_mun | RT_ICON | 2445 (group 51682) | 48x48 | 32 | `icons/icon_51682_2_48x48.png` |
| shell32_mun | RT_ICON | 2446 (group 51682) | 40x40 | 32 | `icons/icon_51682_3_40x40.png` |
| shell32_mun | RT_ICON | 2447 (group 51682) | 32x32 | 32 | `icons/icon_51682_4_32x32.png` |
| shell32_mun | RT_ICON | 2448 (group 51682) | 24x24 | 32 | `icons/icon_51682_5_24x24.png` |
| shell32_mun | RT_ICON | 2449 (group 51682) | 20x20 | 32 | `icons/icon_51682_6_20x20.png` |
| shell32_mun | RT_ICON | 245 (group 22) | 16x16 | 32 | `icons/icon_22_7_16x16.png` |
| shell32_mun | RT_ICON | 2450 (group 51682) | 16x16 | 32 | `icons/icon_51682_7_16x16.png` |
| shell32_mun | RT_ICON | 2451 (group 51683) | 256x256 | 32 | `icons/icon_51683_0_256x256.png` |
| shell32_mun | RT_ICON | 2452 (group 51683) | 64x64 | 32 | `icons/icon_51683_1_64x64.png` |
| shell32_mun | RT_ICON | 2453 (group 51683) | 48x48 | 32 | `icons/icon_51683_2_48x48.png` |
| shell32_mun | RT_ICON | 2454 (group 51683) | 40x40 | 32 | `icons/icon_51683_3_40x40.png` |
| shell32_mun | RT_ICON | 2455 (group 51683) | 32x32 | 32 | `icons/icon_51683_4_32x32.png` |
| shell32_mun | RT_ICON | 2456 (group 51683) | 24x24 | 32 | `icons/icon_51683_5_24x24.png` |
| shell32_mun | RT_ICON | 2457 (group 51683) | 20x20 | 32 | `icons/icon_51683_6_20x20.png` |
| shell32_mun | RT_ICON | 2458 (group 51683) | 16x16 | 32 | `icons/icon_51683_7_16x16.png` |
| shell32_mun | RT_ICON | 2459 (group 51684) | 256x256 | 32 | `icons/icon_51684_0_256x256.png` |
| shell32_mun | RT_ICON | 246 (group 23) | 256x256 | 32 | `icons/icon_23_0_256x256.png` |
| shell32_mun | RT_ICON | 2460 (group 51684) | 64x64 | 32 | `icons/icon_51684_1_64x64.png` |
| shell32_mun | RT_ICON | 2461 (group 51684) | 48x48 | 32 | `icons/icon_51684_2_48x48.png` |
| shell32_mun | RT_ICON | 2462 (group 51684) | 40x40 | 32 | `icons/icon_51684_3_40x40.png` |
| shell32_mun | RT_ICON | 2463 (group 51684) | 32x32 | 32 | `icons/icon_51684_4_32x32.png` |
| shell32_mun | RT_ICON | 2464 (group 51684) | 24x24 | 32 | `icons/icon_51684_5_24x24.png` |
| shell32_mun | RT_ICON | 2465 (group 51684) | 20x20 | 32 | `icons/icon_51684_6_20x20.png` |
| shell32_mun | RT_ICON | 2466 (group 51684) | 16x16 | 32 | `icons/icon_51684_7_16x16.png` |
| shell32_mun | RT_ICON | 2467 (group 51685) | 256x256 | 32 | `icons/icon_51685_0_256x256.png` |
| shell32_mun | RT_ICON | 2468 (group 51685) | 64x64 | 32 | `icons/icon_51685_1_64x64.png` |
| shell32_mun | RT_ICON | 2469 (group 51685) | 48x48 | 32 | `icons/icon_51685_2_48x48.png` |
| shell32_mun | RT_ICON | 247 (group 23) | 64x64 | 32 | `icons/icon_23_1_64x64.png` |
| shell32_mun | RT_ICON | 2470 (group 51685) | 40x40 | 32 | `icons/icon_51685_3_40x40.png` |
| shell32_mun | RT_ICON | 2471 (group 51685) | 32x32 | 32 | `icons/icon_51685_4_32x32.png` |
| shell32_mun | RT_ICON | 2472 (group 51685) | 24x24 | 32 | `icons/icon_51685_5_24x24.png` |
| shell32_mun | RT_ICON | 2473 (group 51685) | 20x20 | 32 | `icons/icon_51685_6_20x20.png` |
| shell32_mun | RT_ICON | 2474 (group 51685) | 16x16 | 32 | `icons/icon_51685_7_16x16.png` |
| shell32_mun | RT_ICON | 2475 (group 51686) | 256x256 | 32 | `icons/icon_51686_0_256x256.png` |
| shell32_mun | RT_ICON | 2476 (group 51686) | 64x64 | 32 | `icons/icon_51686_1_64x64.png` |
| shell32_mun | RT_ICON | 2477 (group 51686) | 48x48 | 32 | `icons/icon_51686_2_48x48.png` |
| shell32_mun | RT_ICON | 2478 (group 51686) | 40x40 | 32 | `icons/icon_51686_3_40x40.png` |
| shell32_mun | RT_ICON | 2479 (group 51686) | 32x32 | 32 | `icons/icon_51686_4_32x32.png` |
| shell32_mun | RT_ICON | 248 (group 23) | 48x48 | 32 | `icons/icon_23_2_48x48.png` |
| shell32_mun | RT_ICON | 2480 (group 51686) | 24x24 | 32 | `icons/icon_51686_5_24x24.png` |
| shell32_mun | RT_ICON | 2481 (group 51686) | 20x20 | 32 | `icons/icon_51686_6_20x20.png` |
| shell32_mun | RT_ICON | 2482 (group 51686) | 16x16 | 32 | `icons/icon_51686_7_16x16.png` |
| shell32_mun | RT_ICON | 2483 (group 51687) | 256x256 | 32 | `icons/icon_51687_0_256x256.png` |
| shell32_mun | RT_ICON | 2484 (group 51687) | 64x64 | 32 | `icons/icon_51687_1_64x64.png` |
| shell32_mun | RT_ICON | 2485 (group 51687) | 48x48 | 32 | `icons/icon_51687_2_48x48.png` |
| shell32_mun | RT_ICON | 2486 (group 51687) | 40x40 | 32 | `icons/icon_51687_3_40x40.png` |
| shell32_mun | RT_ICON | 2487 (group 51687) | 32x32 | 32 | `icons/icon_51687_4_32x32.png` |
| shell32_mun | RT_ICON | 2488 (group 51687) | 24x24 | 32 | `icons/icon_51687_5_24x24.png` |
| shell32_mun | RT_ICON | 2489 (group 51687) | 20x20 | 32 | `icons/icon_51687_6_20x20.png` |
| shell32_mun | RT_ICON | 249 (group 23) | 40x40 | 32 | `icons/icon_23_3_40x40.png` |
| shell32_mun | RT_ICON | 2490 (group 51687) | 16x16 | 32 | `icons/icon_51687_7_16x16.png` |
| shell32_mun | RT_ICON | 25 (group 63001) | 256x256 | 32 | `icons/icon_63001_0_256x256.png` |
| shell32_mun | RT_ICON | 250 (group 23) | 32x32 | 32 | `icons/icon_23_4_32x32.png` |
| shell32_mun | RT_ICON | 251 (group 23) | 24x24 | 32 | `icons/icon_23_5_24x24.png` |
| shell32_mun | RT_ICON | 252 (group 23) | 20x20 | 32 | `icons/icon_23_6_20x20.png` |
| shell32_mun | RT_ICON | 253 (group 23) | 16x16 | 32 | `icons/icon_23_7_16x16.png` |
| shell32_mun | RT_ICON | 254 (group 24) | 256x256 | 32 | `icons/icon_24_0_256x256.png` |
| shell32_mun | RT_ICON | 255 (group 24) | 64x64 | 32 | `icons/icon_24_1_64x64.png` |
| shell32_mun | RT_ICON | 256 (group 24) | 48x48 | 32 | `icons/icon_24_2_48x48.png` |
| shell32_mun | RT_ICON | 257 (group 24) | 40x40 | 32 | `icons/icon_24_3_40x40.png` |
| shell32_mun | RT_ICON | 258 (group 24) | 32x32 | 32 | `icons/icon_24_4_32x32.png` |
| shell32_mun | RT_ICON | 259 (group 24) | 24x24 | 32 | `icons/icon_24_5_24x24.png` |
| shell32_mun | RT_ICON | 26 (group 63001) | 64x64 | 32 | `icons/icon_63001_1_64x64.png` |
| shell32_mun | RT_ICON | 260 (group 24) | 20x20 | 32 | `icons/icon_24_6_20x20.png` |
| shell32_mun | RT_ICON | 261 (group 24) | 16x16 | 32 | `icons/icon_24_7_16x16.png` |
| shell32_mun | RT_ICON | 262 (group 25) | 256x256 | 32 | `icons/icon_25_0_256x256.png` |
| shell32_mun | RT_ICON | 263 (group 25) | 64x64 | 32 | `icons/icon_25_1_64x64.png` |
| shell32_mun | RT_ICON | 264 (group 25) | 48x48 | 32 | `icons/icon_25_2_48x48.png` |
| shell32_mun | RT_ICON | 265 (group 25) | 40x40 | 32 | `icons/icon_25_3_40x40.png` |
| shell32_mun | RT_ICON | 266 (group 25) | 32x32 | 32 | `icons/icon_25_4_32x32.png` |
| shell32_mun | RT_ICON | 267 (group 25) | 24x24 | 32 | `icons/icon_25_5_24x24.png` |
| shell32_mun | RT_ICON | 268 (group 25) | 20x20 | 32 | `icons/icon_25_6_20x20.png` |
| shell32_mun | RT_ICON | 269 (group 25) | 16x16 | 32 | `icons/icon_25_7_16x16.png` |
| shell32_mun | RT_ICON | 27 (group 63001) | 48x48 | 32 | `icons/icon_63001_2_48x48.png` |
| shell32_mun | RT_ICON | 270 (group 26) | 256x256 | 32 | `icons/icon_26_0_256x256.png` |
| shell32_mun | RT_ICON | 271 (group 26) | 64x64 | 32 | `icons/icon_26_1_64x64.png` |
| shell32_mun | RT_ICON | 272 (group 26) | 48x48 | 32 | `icons/icon_26_2_48x48.png` |
| shell32_mun | RT_ICON | 273 (group 26) | 40x40 | 32 | `icons/icon_26_3_40x40.png` |
| shell32_mun | RT_ICON | 274 (group 26) | 32x32 | 32 | `icons/icon_26_4_32x32.png` |
| shell32_mun | RT_ICON | 275 (group 26) | 24x24 | 32 | `icons/icon_26_5_24x24.png` |
| shell32_mun | RT_ICON | 276 (group 26) | 20x20 | 32 | `icons/icon_26_6_20x20.png` |
| shell32_mun | RT_ICON | 277 (group 26) | 16x16 | 32 | `icons/icon_26_7_16x16.png` |
| shell32_mun | RT_ICON | 278 (group 27) | 48x48 | 4 | `icons/icon_27_0_48x48.png` |
| shell32_mun | RT_ICON | 279 (group 27) | 32x32 | 4 | `icons/icon_27_1_32x32.png` |
| shell32_mun | RT_ICON | 28 (group 63001) | 40x40 | 32 | `icons/icon_63001_3_40x40.png` |
| shell32_mun | RT_ICON | 280 (group 27) | 16x16 | 4 | `icons/icon_27_2_16x16.png` |
| shell32_mun | RT_ICON | 281 (group 27) | 48x48 | 8 | `icons/icon_27_3_48x48.png` |
| shell32_mun | RT_ICON | 282 (group 27) | 32x32 | 8 | `icons/icon_27_4_32x32.png` |
| shell32_mun | RT_ICON | 283 (group 27) | 16x16 | 8 | `icons/icon_27_5_16x16.png` |
| shell32_mun | RT_ICON | 284 (group 27) | 48x48 | 32 | `icons/icon_27_6_48x48.png` |
| shell32_mun | RT_ICON | 285 (group 27) | 32x32 | 32 | `icons/icon_27_7_32x32.png` |
| shell32_mun | RT_ICON | 286 (group 27) | 16x16 | 32 | `icons/icon_27_8_16x16.png` |
| shell32_mun | RT_ICON | 287 (group 28) | 48x48 | 4 | `icons/icon_28_0_48x48.png` |
| shell32_mun | RT_ICON | 288 (group 28) | 32x32 | 4 | `icons/icon_28_1_32x32.png` |
| shell32_mun | RT_ICON | 289 (group 28) | 24x24 | 4 | `icons/icon_28_2_24x24.png` |
| shell32_mun | RT_ICON | 29 (group 63001) | 32x32 | 32 | `icons/icon_63001_4_32x32.png` |
| shell32_mun | RT_ICON | 290 (group 28) | 16x16 | 4 | `icons/icon_28_3_16x16.png` |
| shell32_mun | RT_ICON | 291 (group 28) | 48x48 | 8 | `icons/icon_28_4_48x48.png` |
| shell32_mun | RT_ICON | 292 (group 28) | 32x32 | 8 | `icons/icon_28_5_32x32.png` |
| shell32_mun | RT_ICON | 293 (group 28) | 24x24 | 8 | `icons/icon_28_6_24x24.png` |
| shell32_mun | RT_ICON | 294 (group 28) | 16x16 | 8 | `icons/icon_28_7_16x16.png` |
| shell32_mun | RT_ICON | 295 (group 28) | 48x48 | 32 | `icons/icon_28_8_48x48.png` |
| shell32_mun | RT_ICON | 296 (group 28) | 32x32 | 32 | `icons/icon_28_9_32x32.png` |
| shell32_mun | RT_ICON | 297 (group 28) | 24x24 | 32 | `icons/icon_28_10_24x24.png` |
| shell32_mun | RT_ICON | 298 (group 28) | 16x16 | 32 | `icons/icon_28_11_16x16.png` |
| shell32_mun | RT_ICON | 299 (group 16770) | 256x256 | 32 | `icons/icon_16770_0_256x256.png` |
| shell32_mun | RT_ICON | 3 (group 62998) | 48x48 | 32 | `icons/icon_62998_2_48x48.png` |
| shell32_mun | RT_ICON | 30 (group 63001) | 24x24 | 32 | `icons/icon_63001_5_24x24.png` |
| shell32_mun | RT_ICON | 300 (group 16770) | 64x64 | 32 | `icons/icon_16770_1_64x64.png` |
| shell32_mun | RT_ICON | 301 (group 16770) | 48x48 | 32 | `icons/icon_16770_2_48x48.png` |
| shell32_mun | RT_ICON | 302 (group 16770) | 40x40 | 32 | `icons/icon_16770_3_40x40.png` |
| shell32_mun | RT_ICON | 303 (group 16770) | 32x32 | 32 | `icons/icon_16770_4_32x32.png` |
| shell32_mun | RT_ICON | 304 (group 16770) | 24x24 | 32 | `icons/icon_16770_5_24x24.png` |
| shell32_mun | RT_ICON | 305 (group 16770) | 20x20 | 32 | `icons/icon_16770_6_20x20.png` |
| shell32_mun | RT_ICON | 306 (group 16770) | 16x16 | 32 | `icons/icon_16770_7_16x16.png` |
| shell32_mun | RT_ICON | 307 (group 29) | 256x256 | 32 | `icons/icon_29_0_256x256.png` |
| shell32_mun | RT_ICON | 308 (group 29) | 64x64 | 32 | `icons/icon_29_1_64x64.png` |
| shell32_mun | RT_ICON | 309 (group 29) | 48x48 | 32 | `icons/icon_29_2_48x48.png` |
| shell32_mun | RT_ICON | 31 (group 63001) | 20x20 | 32 | `icons/icon_63001_6_20x20.png` |
| shell32_mun | RT_ICON | 310 (group 29) | 40x40 | 32 | `icons/icon_29_3_40x40.png` |
| shell32_mun | RT_ICON | 311 (group 29) | 32x32 | 32 | `icons/icon_29_4_32x32.png` |
| shell32_mun | RT_ICON | 312 (group 29) | 24x24 | 32 | `icons/icon_29_5_24x24.png` |
| shell32_mun | RT_ICON | 313 (group 29) | 20x20 | 32 | `icons/icon_29_6_20x20.png` |
| shell32_mun | RT_ICON | 314 (group 29) | 16x16 | 32 | `icons/icon_29_7_16x16.png` |
| shell32_mun | RT_ICON | 315 (group 16769) | 256x256 | 32 | `icons/icon_16769_0_256x256.png` |
| shell32_mun | RT_ICON | 316 (group 16769) | 64x64 | 32 | `icons/icon_16769_1_64x64.png` |
| shell32_mun | RT_ICON | 317 (group 16769) | 48x48 | 32 | `icons/icon_16769_2_48x48.png` |
| shell32_mun | RT_ICON | 318 (group 16769) | 40x40 | 32 | `icons/icon_16769_3_40x40.png` |
| shell32_mun | RT_ICON | 319 (group 16769) | 32x32 | 32 | `icons/icon_16769_4_32x32.png` |
| shell32_mun | RT_ICON | 32 (group 63001) | 16x16 | 32 | `icons/icon_63001_7_16x16.png` |
| shell32_mun | RT_ICON | 320 (group 16769) | 24x24 | 32 | `icons/icon_16769_5_24x24.png` |
| shell32_mun | RT_ICON | 321 (group 16769) | 20x20 | 32 | `icons/icon_16769_6_20x20.png` |
| shell32_mun | RT_ICON | 322 (group 16769) | 16x16 | 32 | `icons/icon_16769_7_16x16.png` |
| shell32_mun | RT_ICON | 323 (group 30) | 256x256 | 32 | `icons/icon_30_0_256x256.png` |
| shell32_mun | RT_ICON | 324 (group 30) | 64x64 | 32 | `icons/icon_30_1_64x64.png` |
| shell32_mun | RT_ICON | 325 (group 30) | 48x48 | 32 | `icons/icon_30_2_48x48.png` |
| shell32_mun | RT_ICON | 326 (group 30) | 40x40 | 32 | `icons/icon_30_3_40x40.png` |
| shell32_mun | RT_ICON | 327 (group 30) | 32x32 | 32 | `icons/icon_30_4_32x32.png` |
| shell32_mun | RT_ICON | 328 (group 30) | 24x24 | 32 | `icons/icon_30_5_24x24.png` |
| shell32_mun | RT_ICON | 329 (group 30) | 20x20 | 32 | `icons/icon_30_6_20x20.png` |
| shell32_mun | RT_ICON | 33 (group 63008) | 256x256 | 32 | `icons/icon_63008_0_256x256.png` |
| shell32_mun | RT_ICON | 330 (group 30) | 16x16 | 32 | `icons/icon_30_7_16x16.png` |
| shell32_mun | RT_ICON | 331 (group 16771) | 256x256 | 32 | `icons/icon_16771_0_256x256.png` |
| shell32_mun | RT_ICON | 332 (group 16771) | 64x64 | 32 | `icons/icon_16771_1_64x64.png` |
| shell32_mun | RT_ICON | 333 (group 16771) | 48x48 | 32 | `icons/icon_16771_2_48x48.png` |
| shell32_mun | RT_ICON | 334 (group 16771) | 40x40 | 32 | `icons/icon_16771_3_40x40.png` |
| shell32_mun | RT_ICON | 335 (group 16771) | 32x32 | 32 | `icons/icon_16771_4_32x32.png` |
| shell32_mun | RT_ICON | 336 (group 16771) | 24x24 | 32 | `icons/icon_16771_5_24x24.png` |
| shell32_mun | RT_ICON | 337 (group 16771) | 20x20 | 32 | `icons/icon_16771_6_20x20.png` |
| shell32_mun | RT_ICON | 338 (group 16771) | 16x16 | 32 | `icons/icon_16771_7_16x16.png` |
| shell32_mun | RT_ICON | 339 (group 31) | 256x256 | 32 | `icons/icon_31_0_256x256.png` |
| shell32_mun | RT_ICON | 34 (group 63008) | 64x64 | 32 | `icons/icon_63008_1_64x64.png` |
| shell32_mun | RT_ICON | 340 (group 31) | 64x64 | 32 | `icons/icon_31_1_64x64.png` |
| shell32_mun | RT_ICON | 341 (group 31) | 48x48 | 32 | `icons/icon_31_2_48x48.png` |
| shell32_mun | RT_ICON | 342 (group 31) | 40x40 | 32 | `icons/icon_31_3_40x40.png` |
| shell32_mun | RT_ICON | 343 (group 31) | 32x32 | 32 | `icons/icon_31_4_32x32.png` |
| shell32_mun | RT_ICON | 344 (group 31) | 24x24 | 32 | `icons/icon_31_5_24x24.png` |
| shell32_mun | RT_ICON | 345 (group 31) | 20x20 | 32 | `icons/icon_31_6_20x20.png` |
| shell32_mun | RT_ICON | 346 (group 31) | 16x16 | 32 | `icons/icon_31_7_16x16.png` |
| shell32_mun | RT_ICON | 347 (group 32) | 256x256 | 32 | `icons/icon_32_0_256x256.png` |
| shell32_mun | RT_ICON | 348 (group 32) | 64x64 | 32 | `icons/icon_32_1_64x64.png` |
| shell32_mun | RT_ICON | 349 (group 32) | 48x48 | 32 | `icons/icon_32_2_48x48.png` |
| shell32_mun | RT_ICON | 35 (group 63008) | 48x48 | 32 | `icons/icon_63008_2_48x48.png` |
| shell32_mun | RT_ICON | 350 (group 32) | 40x40 | 32 | `icons/icon_32_3_40x40.png` |
| shell32_mun | RT_ICON | 351 (group 32) | 32x32 | 32 | `icons/icon_32_4_32x32.png` |
| shell32_mun | RT_ICON | 352 (group 32) | 24x24 | 32 | `icons/icon_32_5_24x24.png` |
| shell32_mun | RT_ICON | 353 (group 32) | 20x20 | 32 | `icons/icon_32_6_20x20.png` |
| shell32_mun | RT_ICON | 354 (group 32) | 16x16 | 32 | `icons/icon_32_7_16x16.png` |
| shell32_mun | RT_ICON | 355 (group 33) | 256x256 | 32 | `icons/icon_33_0_256x256.png` |
| shell32_mun | RT_ICON | 356 (group 33) | 64x64 | 32 | `icons/icon_33_1_64x64.png` |
| shell32_mun | RT_ICON | 357 (group 33) | 48x48 | 32 | `icons/icon_33_2_48x48.png` |
| shell32_mun | RT_ICON | 358 (group 33) | 40x40 | 32 | `icons/icon_33_3_40x40.png` |
| shell32_mun | RT_ICON | 359 (group 33) | 32x32 | 32 | `icons/icon_33_4_32x32.png` |
| shell32_mun | RT_ICON | 36 (group 63008) | 40x40 | 32 | `icons/icon_63008_3_40x40.png` |
| shell32_mun | RT_ICON | 360 (group 33) | 24x24 | 32 | `icons/icon_33_5_24x24.png` |
| shell32_mun | RT_ICON | 361 (group 33) | 20x20 | 32 | `icons/icon_33_6_20x20.png` |
| shell32_mun | RT_ICON | 362 (group 33) | 16x16 | 32 | `icons/icon_33_7_16x16.png` |
| shell32_mun | RT_ICON | 363 (group 36) | 32x32 | 4 | `icons/icon_36_0_32x32.png` |
| shell32_mun | RT_ICON | 364 (group 36) | 16x16 | 4 | `icons/icon_36_1_16x16.png` |
| shell32_mun | RT_ICON | 365 (group 37) | 256x256 | 32 | `icons/icon_37_0_256x256.png` |
| shell32_mun | RT_ICON | 366 (group 37) | 64x64 | 32 | `icons/icon_37_1_64x64.png` |
| shell32_mun | RT_ICON | 367 (group 37) | 48x48 | 32 | `icons/icon_37_2_48x48.png` |
| shell32_mun | RT_ICON | 368 (group 37) | 40x40 | 32 | `icons/icon_37_3_40x40.png` |
| shell32_mun | RT_ICON | 369 (group 37) | 32x32 | 32 | `icons/icon_37_4_32x32.png` |
| shell32_mun | RT_ICON | 37 (group 63008) | 32x32 | 32 | `icons/icon_63008_4_32x32.png` |
| shell32_mun | RT_ICON | 370 (group 37) | 24x24 | 32 | `icons/icon_37_5_24x24.png` |
| shell32_mun | RT_ICON | 371 (group 37) | 20x20 | 32 | `icons/icon_37_6_20x20.png` |
| shell32_mun | RT_ICON | 372 (group 37) | 16x16 | 32 | `icons/icon_37_7_16x16.png` |
| shell32_mun | RT_ICON | 373 (group 38) | 32x32 | 4 | `icons/icon_38_0_32x32.png` |
| shell32_mun | RT_ICON | 374 (group 38) | 16x16 | 4 | `icons/icon_38_1_16x16.png` |
| shell32_mun | RT_ICON | 375 (group 39) | 256x256 | 32 | `icons/icon_39_0_256x256.png` |
| shell32_mun | RT_ICON | 376 (group 39) | 64x64 | 32 | `icons/icon_39_1_64x64.png` |
| shell32_mun | RT_ICON | 377 (group 39) | 48x48 | 32 | `icons/icon_39_2_48x48.png` |
| shell32_mun | RT_ICON | 378 (group 39) | 40x40 | 32 | `icons/icon_39_3_40x40.png` |
| shell32_mun | RT_ICON | 379 (group 39) | 32x32 | 32 | `icons/icon_39_4_32x32.png` |
| shell32_mun | RT_ICON | 38 (group 63008) | 24x24 | 32 | `icons/icon_63008_5_24x24.png` |
| shell32_mun | RT_ICON | 380 (group 39) | 24x24 | 32 | `icons/icon_39_5_24x24.png` |
| shell32_mun | RT_ICON | 381 (group 39) | 20x20 | 32 | `icons/icon_39_6_20x20.png` |
| shell32_mun | RT_ICON | 382 (group 39) | 16x16 | 32 | `icons/icon_39_7_16x16.png` |
| shell32_mun | RT_ICON | 383 (group 40) | 256x256 | 32 | `icons/icon_40_0_256x256.png` |
| shell32_mun | RT_ICON | 384 (group 40) | 64x64 | 32 | `icons/icon_40_1_64x64.png` |
| shell32_mun | RT_ICON | 385 (group 40) | 48x48 | 32 | `icons/icon_40_2_48x48.png` |
| shell32_mun | RT_ICON | 386 (group 40) | 40x40 | 32 | `icons/icon_40_3_40x40.png` |
| shell32_mun | RT_ICON | 387 (group 40) | 32x32 | 32 | `icons/icon_40_4_32x32.png` |
| shell32_mun | RT_ICON | 388 (group 40) | 24x24 | 32 | `icons/icon_40_5_24x24.png` |
| shell32_mun | RT_ICON | 389 (group 40) | 20x20 | 32 | `icons/icon_40_6_20x20.png` |
| shell32_mun | RT_ICON | 39 (group 63008) | 20x20 | 32 | `icons/icon_63008_6_20x20.png` |
| shell32_mun | RT_ICON | 390 (group 40) | 16x16 | 32 | `icons/icon_40_7_16x16.png` |
| shell32_mun | RT_ICON | 391 (group 41) | 256x256 | 32 | `icons/icon_41_0_256x256.png` |
| shell32_mun | RT_ICON | 392 (group 41) | 64x64 | 32 | `icons/icon_41_1_64x64.png` |
| shell32_mun | RT_ICON | 393 (group 41) | 48x48 | 32 | `icons/icon_41_2_48x48.png` |
| shell32_mun | RT_ICON | 394 (group 41) | 40x40 | 32 | `icons/icon_41_3_40x40.png` |
| shell32_mun | RT_ICON | 395 (group 41) | 32x32 | 32 | `icons/icon_41_4_32x32.png` |
| shell32_mun | RT_ICON | 396 (group 41) | 24x24 | 32 | `icons/icon_41_5_24x24.png` |
| shell32_mun | RT_ICON | 397 (group 41) | 20x20 | 32 | `icons/icon_41_6_20x20.png` |
| shell32_mun | RT_ICON | 398 (group 41) | 16x16 | 32 | `icons/icon_41_7_16x16.png` |
| shell32_mun | RT_ICON | 399 (group 42) | 32x32 | 4 | `icons/icon_42_0_32x32.png` |
| shell32_mun | RT_ICON | 4 (group 62998) | 40x40 | 32 | `icons/icon_62998_3_40x40.png` |
| shell32_mun | RT_ICON | 40 (group 63008) | 16x16 | 32 | `icons/icon_63008_7_16x16.png` |
| shell32_mun | RT_ICON | 400 (group 42) | 16x16 | 4 | `icons/icon_42_1_16x16.png` |
| shell32_mun | RT_ICON | 401 (group 43) | 32x32 | 4 | `icons/icon_43_0_32x32.png` |
| shell32_mun | RT_ICON | 402 (group 43) | 16x16 | 4 | `icons/icon_43_1_16x16.png` |
| shell32_mun | RT_ICON | 403 (group 43) | 32x32 | 8 | `icons/icon_43_2_32x32.png` |
| shell32_mun | RT_ICON | 404 (group 43) | 16x16 | 8 | `icons/icon_43_3_16x16.png` |
| shell32_mun | RT_ICON | 405 (group 44) | 256x256 | 32 | `icons/icon_44_0_256x256.png` |
| shell32_mun | RT_ICON | 406 (group 44) | 64x64 | 32 | `icons/icon_44_1_64x64.png` |
| shell32_mun | RT_ICON | 407 (group 44) | 48x48 | 32 | `icons/icon_44_2_48x48.png` |
| shell32_mun | RT_ICON | 408 (group 44) | 40x40 | 32 | `icons/icon_44_3_40x40.png` |
| shell32_mun | RT_ICON | 409 (group 44) | 32x32 | 32 | `icons/icon_44_4_32x32.png` |
| shell32_mun | RT_ICON | 41 (group 63009) | 256x256 | 32 | `icons/icon_63009_0_256x256.png` |
| shell32_mun | RT_ICON | 410 (group 44) | 24x24 | 32 | `icons/icon_44_5_24x24.png` |
| shell32_mun | RT_ICON | 411 (group 44) | 20x20 | 32 | `icons/icon_44_6_20x20.png` |
| shell32_mun | RT_ICON | 412 (group 44) | 16x16 | 32 | `icons/icon_44_7_16x16.png` |
| shell32_mun | RT_ICON | 413 (group 45) | 48x48 | 4 | `icons/icon_45_0_48x48.png` |
| shell32_mun | RT_ICON | 414 (group 45) | 32x32 | 4 | `icons/icon_45_1_32x32.png` |
| shell32_mun | RT_ICON | 415 (group 45) | 24x24 | 4 | `icons/icon_45_2_24x24.png` |
| shell32_mun | RT_ICON | 416 (group 45) | 16x16 | 4 | `icons/icon_45_3_16x16.png` |
| shell32_mun | RT_ICON | 417 (group 45) | 48x48 | 8 | `icons/icon_45_4_48x48.png` |
| shell32_mun | RT_ICON | 418 (group 45) | 32x32 | 8 | `icons/icon_45_5_32x32.png` |
| shell32_mun | RT_ICON | 419 (group 45) | 24x24 | 8 | `icons/icon_45_6_24x24.png` |
| shell32_mun | RT_ICON | 42 (group 63009) | 64x64 | 32 | `icons/icon_63009_1_64x64.png` |
| shell32_mun | RT_ICON | 420 (group 45) | 16x16 | 8 | `icons/icon_45_7_16x16.png` |
| shell32_mun | RT_ICON | 421 (group 45) | 48x48 | 32 | `icons/icon_45_8_48x48.png` |
| shell32_mun | RT_ICON | 422 (group 45) | 32x32 | 32 | `icons/icon_45_9_32x32.png` |
| shell32_mun | RT_ICON | 423 (group 45) | 24x24 | 32 | `icons/icon_45_10_24x24.png` |
| shell32_mun | RT_ICON | 424 (group 45) | 16x16 | 32 | `icons/icon_45_11_16x16.png` |
| shell32_mun | RT_ICON | 425 (group 46) | 256x256 | 32 | `icons/icon_46_0_256x256.png` |
| shell32_mun | RT_ICON | 426 (group 46) | 64x64 | 32 | `icons/icon_46_1_64x64.png` |
| shell32_mun | RT_ICON | 427 (group 46) | 48x48 | 32 | `icons/icon_46_2_48x48.png` |
| shell32_mun | RT_ICON | 428 (group 46) | 40x40 | 32 | `icons/icon_46_3_40x40.png` |
| shell32_mun | RT_ICON | 429 (group 46) | 32x32 | 32 | `icons/icon_46_4_32x32.png` |
| shell32_mun | RT_ICON | 43 (group 63009) | 48x48 | 32 | `icons/icon_63009_2_48x48.png` |
| shell32_mun | RT_ICON | 430 (group 46) | 24x25 | 32 | `icons/icon_46_5_24x25.png` |
| shell32_mun | RT_ICON | 431 (group 46) | 20x21 | 32 | `icons/icon_46_6_20x21.png` |
| shell32_mun | RT_ICON | 432 (group 46) | 17x17 | 32 | `icons/icon_46_7_17x17.png` |
| shell32_mun | RT_ICON | 433 (group 47) | 48x48 | 4 | `icons/icon_47_0_48x48.png` |
| shell32_mun | RT_ICON | 434 (group 47) | 32x32 | 4 | `icons/icon_47_1_32x32.png` |
| shell32_mun | RT_ICON | 435 (group 47) | 24x24 | 4 | `icons/icon_47_2_24x24.png` |
| shell32_mun | RT_ICON | 436 (group 47) | 16x16 | 4 | `icons/icon_47_3_16x16.png` |
| shell32_mun | RT_ICON | 437 (group 47) | 64x64 | 8 | `icons/icon_47_4_64x64.png` |
| shell32_mun | RT_ICON | 438 (group 47) | 48x48 | 8 | `icons/icon_47_5_48x48.png` |
| shell32_mun | RT_ICON | 439 (group 47) | 32x32 | 8 | `icons/icon_47_6_32x32.png` |
| shell32_mun | RT_ICON | 44 (group 63009) | 40x40 | 32 | `icons/icon_63009_3_40x40.png` |
| shell32_mun | RT_ICON | 440 (group 47) | 24x24 | 8 | `icons/icon_47_7_24x24.png` |
| shell32_mun | RT_ICON | 441 (group 47) | 16x16 | 8 | `icons/icon_47_8_16x16.png` |
| shell32_mun | RT_ICON | 442 (group 47) | 256x256 | 32 | `icons/icon_47_9_256x256.png` |
| shell32_mun | RT_ICON | 443 (group 47) | 64x64 | 32 | `icons/icon_47_10_64x64.png` |
| shell32_mun | RT_ICON | 444 (group 47) | 48x48 | 32 | `icons/icon_47_11_48x48.png` |
| shell32_mun | RT_ICON | 445 (group 47) | 32x32 | 32 | `icons/icon_47_12_32x32.png` |
| shell32_mun | RT_ICON | 446 (group 47) | 25x24 | 32 | `icons/icon_47_13_25x24.png` |
| shell32_mun | RT_ICON | 447 (group 47) | 16x16 | 32 | `icons/icon_47_14_16x16.png` |
| shell32_mun | RT_ICON | 448 (group 48) | 256x256 | 32 | `icons/icon_48_0_256x256.png` |
| shell32_mun | RT_ICON | 449 (group 48) | 64x64 | 32 | `icons/icon_48_1_64x64.png` |
| shell32_mun | RT_ICON | 45 (group 63009) | 32x32 | 32 | `icons/icon_63009_4_32x32.png` |
| shell32_mun | RT_ICON | 450 (group 48) | 48x48 | 32 | `icons/icon_48_2_48x48.png` |
| shell32_mun | RT_ICON | 451 (group 48) | 40x40 | 32 | `icons/icon_48_3_40x40.png` |
| shell32_mun | RT_ICON | 452 (group 48) | 32x32 | 32 | `icons/icon_48_4_32x32.png` |
| shell32_mun | RT_ICON | 453 (group 48) | 24x24 | 32 | `icons/icon_48_5_24x24.png` |
| shell32_mun | RT_ICON | 454 (group 48) | 20x20 | 32 | `icons/icon_48_6_20x20.png` |
| shell32_mun | RT_ICON | 455 (group 48) | 16x16 | 32 | `icons/icon_48_7_16x16.png` |
| shell32_mun | RT_ICON | 456 (group 49) | 32x32 | 4 | `icons/icon_49_0_32x32.png` |
| shell32_mun | RT_ICON | 457 (group 49) | 16x16 | 4 | `icons/icon_49_1_16x16.png` |
| shell32_mun | RT_ICON | 458 (group 54) | 256x256 | 32 | `icons/icon_54_0_256x256.png` |
| shell32_mun | RT_ICON | 459 (group 54) | 64x64 | 32 | `icons/icon_54_1_64x64.png` |
| shell32_mun | RT_ICON | 46 (group 63009) | 24x24 | 32 | `icons/icon_63009_5_24x24.png` |
| shell32_mun | RT_ICON | 460 (group 54) | 48x48 | 32 | `icons/icon_54_2_48x48.png` |
| shell32_mun | RT_ICON | 461 (group 54) | 40x40 | 32 | `icons/icon_54_3_40x40.png` |
| shell32_mun | RT_ICON | 462 (group 54) | 32x32 | 32 | `icons/icon_54_4_32x32.png` |
| shell32_mun | RT_ICON | 463 (group 54) | 24x24 | 32 | `icons/icon_54_5_24x24.png` |
| shell32_mun | RT_ICON | 464 (group 54) | 20x20 | 32 | `icons/icon_54_6_20x20.png` |
| shell32_mun | RT_ICON | 465 (group 54) | 16x16 | 32 | `icons/icon_54_7_16x16.png` |
| shell32_mun | RT_ICON | 466 (group 231) | 256x256 | 32 | `icons/icon_231_0_256x256.png` |
| shell32_mun | RT_ICON | 467 (group 231) | 64x64 | 32 | `icons/icon_231_1_64x64.png` |
| shell32_mun | RT_ICON | 468 (group 231) | 48x48 | 32 | `icons/icon_231_2_48x48.png` |
| shell32_mun | RT_ICON | 469 (group 231) | 40x40 | 32 | `icons/icon_231_3_40x40.png` |
| shell32_mun | RT_ICON | 47 (group 63009) | 20x20 | 32 | `icons/icon_63009_6_20x20.png` |
| shell32_mun | RT_ICON | 470 (group 231) | 32x32 | 32 | `icons/icon_231_4_32x32.png` |
| shell32_mun | RT_ICON | 471 (group 231) | 24x24 | 32 | `icons/icon_231_5_24x24.png` |
| shell32_mun | RT_ICON | 472 (group 231) | 20x20 | 32 | `icons/icon_231_6_20x20.png` |
| shell32_mun | RT_ICON | 473 (group 231) | 16x16 | 32 | `icons/icon_231_7_16x16.png` |
| shell32_mun | RT_ICON | 474 (group 232) | 256x256 | 32 | `icons/icon_232_0_256x256.png` |
| shell32_mun | RT_ICON | 475 (group 232) | 64x64 | 32 | `icons/icon_232_1_64x64.png` |
| shell32_mun | RT_ICON | 476 (group 232) | 48x48 | 32 | `icons/icon_232_2_48x48.png` |
| shell32_mun | RT_ICON | 477 (group 232) | 40x40 | 32 | `icons/icon_232_3_40x40.png` |
| shell32_mun | RT_ICON | 478 (group 232) | 32x32 | 32 | `icons/icon_232_4_32x32.png` |
| shell32_mun | RT_ICON | 479 (group 232) | 24x24 | 32 | `icons/icon_232_5_24x24.png` |
| shell32_mun | RT_ICON | 48 (group 63009) | 16x16 | 32 | `icons/icon_63009_7_16x16.png` |
| shell32_mun | RT_ICON | 480 (group 232) | 20x20 | 32 | `icons/icon_232_6_20x20.png` |
| shell32_mun | RT_ICON | 481 (group 232) | 16x16 | 32 | `icons/icon_232_7_16x16.png` |
| shell32_mun | RT_ICON | 482 (group 220) | 32x32 | 4 | `icons/icon_220_0_32x32.png` |
| shell32_mun | RT_ICON | 483 (group 220) | 16x16 | 4 | `icons/icon_220_1_16x16.png` |
| shell32_mun | RT_ICON | 484 (group 220) | 48x48 | 4 | `icons/icon_220_2_48x48.png` |
| shell32_mun | RT_ICON | 485 (group 220) | 32x32 | 8 | `icons/icon_220_3_32x32.png` |
| shell32_mun | RT_ICON | 486 (group 220) | 48x48 | 8 | `icons/icon_220_4_48x48.png` |
| shell32_mun | RT_ICON | 487 (group 220) | 16x16 | 8 | `icons/icon_220_5_16x16.png` |
| shell32_mun | RT_ICON | 488 (group 221) | 32x32 | 4 | `icons/icon_221_0_32x32.png` |
| shell32_mun | RT_ICON | 489 (group 221) | 16x16 | 4 | `icons/icon_221_1_16x16.png` |
| shell32_mun | RT_ICON | 49 (group 63010) | 256x256 | 32 | `icons/icon_63010_0_256x256.png` |
| shell32_mun | RT_ICON | 490 (group 221) | 48x48 | 4 | `icons/icon_221_2_48x48.png` |
| shell32_mun | RT_ICON | 491 (group 221) | 32x32 | 8 | `icons/icon_221_3_32x32.png` |
| shell32_mun | RT_ICON | 492 (group 221) | 16x16 | 8 | `icons/icon_221_4_16x16.png` |
| shell32_mun | RT_ICON | 493 (group 221) | 48x48 | 8 | `icons/icon_221_5_48x48.png` |
| shell32_mun | RT_ICON | 494 (group 50) | 16x16 | 4 | `icons/icon_50_0_16x16.png` |
| shell32_mun | RT_ICON | 495 (group 51) | 16x16 | 4 | `icons/icon_51_0_16x16.png` |
| shell32_mun | RT_ICON | 496 (group 52) | 16x16 | 4 | `icons/icon_52_0_16x16.png` |
| shell32_mun | RT_ICON | 497 (group 53) | 16x16 | 4 | `icons/icon_53_0_16x16.png` |
| shell32_mun | RT_ICON | 498 (group 133) | 256x256 | 32 | `icons/icon_133_0_256x256.png` |
| shell32_mun | RT_ICON | 499 (group 133) | 64x64 | 32 | `icons/icon_133_1_64x64.png` |
| shell32_mun | RT_ICON | 5 (group 62998) | 32x32 | 32 | `icons/icon_62998_4_32x32.png` |
| shell32_mun | RT_ICON | 50 (group 63010) | 64x64 | 32 | `icons/icon_63010_1_64x64.png` |
| shell32_mun | RT_ICON | 500 (group 133) | 48x48 | 32 | `icons/icon_133_2_48x48.png` |
| shell32_mun | RT_ICON | 501 (group 133) | 40x40 | 32 | `icons/icon_133_3_40x40.png` |
| shell32_mun | RT_ICON | 502 (group 133) | 32x32 | 32 | `icons/icon_133_4_32x32.png` |
| shell32_mun | RT_ICON | 503 (group 133) | 24x24 | 32 | `icons/icon_133_5_24x24.png` |
| shell32_mun | RT_ICON | 504 (group 133) | 20x20 | 32 | `icons/icon_133_6_20x20.png` |
| shell32_mun | RT_ICON | 505 (group 133) | 16x16 | 32 | `icons/icon_133_7_16x16.png` |
| shell32_mun | RT_ICON | 506 (group 134) | 256x256 | 32 | `icons/icon_134_0_256x256.png` |
| shell32_mun | RT_ICON | 507 (group 134) | 64x64 | 32 | `icons/icon_134_1_64x64.png` |
| shell32_mun | RT_ICON | 508 (group 134) | 48x48 | 32 | `icons/icon_134_2_48x48.png` |
| shell32_mun | RT_ICON | 509 (group 134) | 40x40 | 32 | `icons/icon_134_3_40x40.png` |
| shell32_mun | RT_ICON | 51 (group 63010) | 48x48 | 32 | `icons/icon_63010_2_48x48.png` |
| shell32_mun | RT_ICON | 510 (group 134) | 32x32 | 32 | `icons/icon_134_4_32x32.png` |
| shell32_mun | RT_ICON | 511 (group 134) | 24x24 | 32 | `icons/icon_134_5_24x24.png` |
| shell32_mun | RT_ICON | 512 (group 134) | 20x20 | 32 | `icons/icon_134_6_20x20.png` |
| shell32_mun | RT_ICON | 513 (group 134) | 16x16 | 32 | `icons/icon_134_7_16x16.png` |
| shell32_mun | RT_ICON | 514 (group 135) | 16x16 | 8 | `icons/icon_135_0_16x16.png` |
| shell32_mun | RT_ICON | 515 (group 135) | 16x16 | 32 | `icons/icon_135_1_16x16.png` |
| shell32_mun | RT_ICON | 516 (group 137) | 256x256 | 32 | `icons/icon_137_0_256x256.png` |
| shell32_mun | RT_ICON | 517 (group 137) | 64x64 | 32 | `icons/icon_137_1_64x64.png` |
| shell32_mun | RT_ICON | 518 (group 137) | 48x48 | 32 | `icons/icon_137_2_48x48.png` |
| shell32_mun | RT_ICON | 519 (group 137) | 40x40 | 32 | `icons/icon_137_3_40x40.png` |
| shell32_mun | RT_ICON | 52 (group 63010) | 40x40 | 32 | `icons/icon_63010_3_40x40.png` |
| shell32_mun | RT_ICON | 520 (group 137) | 32x32 | 32 | `icons/icon_137_4_32x32.png` |
| shell32_mun | RT_ICON | 521 (group 137) | 24x24 | 32 | `icons/icon_137_5_24x24.png` |
| shell32_mun | RT_ICON | 522 (group 137) | 20x20 | 32 | `icons/icon_137_6_20x20.png` |
| shell32_mun | RT_ICON | 523 (group 137) | 16x16 | 32 | `icons/icon_137_7_16x16.png` |
| shell32_mun | RT_ICON | 524 (group 138) | 256x256 | 32 | `icons/icon_138_0_256x256.png` |
| shell32_mun | RT_ICON | 525 (group 138) | 64x64 | 32 | `icons/icon_138_1_64x64.png` |
| shell32_mun | RT_ICON | 526 (group 138) | 48x48 | 32 | `icons/icon_138_2_48x48.png` |
| shell32_mun | RT_ICON | 527 (group 138) | 40x40 | 32 | `icons/icon_138_3_40x40.png` |
| shell32_mun | RT_ICON | 528 (group 138) | 32x32 | 32 | `icons/icon_138_4_32x32.png` |
| shell32_mun | RT_ICON | 529 (group 138) | 24x24 | 32 | `icons/icon_138_5_24x24.png` |
| shell32_mun | RT_ICON | 53 (group 63010) | 32x32 | 32 | `icons/icon_63010_4_32x32.png` |
| shell32_mun | RT_ICON | 530 (group 138) | 20x20 | 32 | `icons/icon_138_6_20x20.png` |
| shell32_mun | RT_ICON | 531 (group 138) | 16x16 | 32 | `icons/icon_138_7_16x16.png` |
| shell32_mun | RT_ICON | 532 (group 139) | 256x256 | 32 | `icons/icon_139_0_256x256.png` |
| shell32_mun | RT_ICON | 533 (group 139) | 64x64 | 32 | `icons/icon_139_1_64x64.png` |
| shell32_mun | RT_ICON | 534 (group 139) | 48x48 | 32 | `icons/icon_139_2_48x48.png` |
| shell32_mun | RT_ICON | 535 (group 139) | 40x40 | 32 | `icons/icon_139_3_40x40.png` |
| shell32_mun | RT_ICON | 536 (group 139) | 32x32 | 32 | `icons/icon_139_4_32x32.png` |
| shell32_mun | RT_ICON | 537 (group 139) | 24x24 | 32 | `icons/icon_139_5_24x24.png` |
| shell32_mun | RT_ICON | 538 (group 139) | 20x20 | 32 | `icons/icon_139_6_20x20.png` |
| shell32_mun | RT_ICON | 539 (group 139) | 16x16 | 32 | `icons/icon_139_7_16x16.png` |
| shell32_mun | RT_ICON | 54 (group 63010) | 24x24 | 32 | `icons/icon_63010_5_24x24.png` |
| shell32_mun | RT_ICON | 540 (group 140) | 256x256 | 32 | `icons/icon_140_0_256x256.png` |
| shell32_mun | RT_ICON | 541 (group 140) | 64x64 | 32 | `icons/icon_140_1_64x64.png` |
| shell32_mun | RT_ICON | 542 (group 140) | 48x48 | 32 | `icons/icon_140_2_48x48.png` |
| shell32_mun | RT_ICON | 543 (group 140) | 40x40 | 32 | `icons/icon_140_3_40x40.png` |
| shell32_mun | RT_ICON | 544 (group 140) | 32x32 | 32 | `icons/icon_140_4_32x32.png` |
| shell32_mun | RT_ICON | 545 (group 140) | 24x24 | 32 | `icons/icon_140_5_24x24.png` |
| shell32_mun | RT_ICON | 546 (group 140) | 20x20 | 32 | `icons/icon_140_6_20x20.png` |
| shell32_mun | RT_ICON | 547 (group 140) | 16x16 | 32 | `icons/icon_140_7_16x16.png` |
| shell32_mun | RT_ICON | 548 (group 141) | 256x256 | 32 | `icons/icon_141_0_256x256.png` |
| shell32_mun | RT_ICON | 549 (group 141) | 64x64 | 32 | `icons/icon_141_1_64x64.png` |
| shell32_mun | RT_ICON | 55 (group 63010) | 20x20 | 32 | `icons/icon_63010_6_20x20.png` |
| shell32_mun | RT_ICON | 550 (group 141) | 48x48 | 32 | `icons/icon_141_2_48x48.png` |
| shell32_mun | RT_ICON | 551 (group 141) | 40x40 | 32 | `icons/icon_141_3_40x40.png` |
| shell32_mun | RT_ICON | 552 (group 141) | 32x32 | 32 | `icons/icon_141_4_32x32.png` |
| shell32_mun | RT_ICON | 553 (group 141) | 24x24 | 32 | `icons/icon_141_5_24x24.png` |
| shell32_mun | RT_ICON | 554 (group 141) | 20x20 | 32 | `icons/icon_141_6_20x20.png` |
| shell32_mun | RT_ICON | 555 (group 141) | 16x16 | 32 | `icons/icon_141_7_16x16.png` |
| shell32_mun | RT_ICON | 556 (group 142) | 256x256 | 32 | `icons/icon_142_0_256x256.png` |
| shell32_mun | RT_ICON | 557 (group 142) | 64x64 | 32 | `icons/icon_142_1_64x64.png` |
| shell32_mun | RT_ICON | 558 (group 142) | 48x48 | 32 | `icons/icon_142_2_48x48.png` |
| shell32_mun | RT_ICON | 559 (group 142) | 40x40 | 32 | `icons/icon_142_3_40x40.png` |
| shell32_mun | RT_ICON | 56 (group 63010) | 16x16 | 32 | `icons/icon_63010_7_16x16.png` |
| shell32_mun | RT_ICON | 560 (group 142) | 32x32 | 32 | `icons/icon_142_4_32x32.png` |
| shell32_mun | RT_ICON | 561 (group 142) | 24x24 | 32 | `icons/icon_142_5_24x24.png` |
| shell32_mun | RT_ICON | 562 (group 142) | 20x20 | 32 | `icons/icon_142_6_20x20.png` |
| shell32_mun | RT_ICON | 563 (group 142) | 16x16 | 32 | `icons/icon_142_7_16x16.png` |
| shell32_mun | RT_ICON | 564 (group 143) | 256x256 | 32 | `icons/icon_143_0_256x256.png` |
| shell32_mun | RT_ICON | 565 (group 143) | 64x64 | 32 | `icons/icon_143_1_64x64.png` |
| shell32_mun | RT_ICON | 566 (group 143) | 48x48 | 32 | `icons/icon_143_2_48x48.png` |
| shell32_mun | RT_ICON | 567 (group 143) | 40x40 | 32 | `icons/icon_143_3_40x40.png` |
| shell32_mun | RT_ICON | 568 (group 143) | 32x32 | 32 | `icons/icon_143_4_32x32.png` |
| shell32_mun | RT_ICON | 569 (group 143) | 24x24 | 32 | `icons/icon_143_5_24x24.png` |
| shell32_mun | RT_ICON | 57 (group 63011) | 256x256 | 32 | `icons/icon_63011_0_256x256.png` |
| shell32_mun | RT_ICON | 570 (group 143) | 20x20 | 32 | `icons/icon_143_6_20x20.png` |
| shell32_mun | RT_ICON | 571 (group 143) | 16x16 | 32 | `icons/icon_143_7_16x16.png` |
| shell32_mun | RT_ICON | 572 (group 144) | 256x256 | 32 | `icons/icon_144_0_256x256.png` |
| shell32_mun | RT_ICON | 573 (group 144) | 64x64 | 32 | `icons/icon_144_1_64x64.png` |
| shell32_mun | RT_ICON | 574 (group 144) | 48x48 | 32 | `icons/icon_144_2_48x48.png` |
| shell32_mun | RT_ICON | 575 (group 144) | 40x40 | 32 | `icons/icon_144_3_40x40.png` |
| shell32_mun | RT_ICON | 576 (group 144) | 32x32 | 32 | `icons/icon_144_4_32x32.png` |
| shell32_mun | RT_ICON | 577 (group 144) | 24x24 | 32 | `icons/icon_144_5_24x24.png` |
| shell32_mun | RT_ICON | 578 (group 144) | 20x20 | 32 | `icons/icon_144_6_20x20.png` |
| shell32_mun | RT_ICON | 579 (group 144) | 16x16 | 32 | `icons/icon_144_7_16x16.png` |
| shell32_mun | RT_ICON | 58 (group 63011) | 64x64 | 32 | `icons/icon_63011_1_64x64.png` |
| shell32_mun | RT_ICON | 580 (group 145) | 256x256 | 32 | `icons/icon_145_0_256x256.png` |
| shell32_mun | RT_ICON | 581 (group 145) | 64x64 | 32 | `icons/icon_145_1_64x64.png` |
| shell32_mun | RT_ICON | 582 (group 145) | 48x48 | 32 | `icons/icon_145_2_48x48.png` |
| shell32_mun | RT_ICON | 583 (group 145) | 40x40 | 32 | `icons/icon_145_3_40x40.png` |
| shell32_mun | RT_ICON | 584 (group 145) | 32x32 | 32 | `icons/icon_145_4_32x32.png` |
| shell32_mun | RT_ICON | 585 (group 145) | 24x24 | 32 | `icons/icon_145_5_24x24.png` |
| shell32_mun | RT_ICON | 586 (group 145) | 20x20 | 32 | `icons/icon_145_6_20x20.png` |
| shell32_mun | RT_ICON | 587 (group 145) | 16x16 | 32 | `icons/icon_145_7_16x16.png` |
| shell32_mun | RT_ICON | 588 (group 146) | 256x256 | 32 | `icons/icon_146_0_256x256.png` |
| shell32_mun | RT_ICON | 589 (group 146) | 64x64 | 32 | `icons/icon_146_1_64x64.png` |
| shell32_mun | RT_ICON | 59 (group 63011) | 48x48 | 32 | `icons/icon_63011_2_48x48.png` |
| shell32_mun | RT_ICON | 590 (group 146) | 48x48 | 32 | `icons/icon_146_2_48x48.png` |
| shell32_mun | RT_ICON | 591 (group 146) | 40x40 | 32 | `icons/icon_146_3_40x40.png` |
| shell32_mun | RT_ICON | 592 (group 146) | 32x32 | 32 | `icons/icon_146_4_32x32.png` |
| shell32_mun | RT_ICON | 593 (group 146) | 24x24 | 32 | `icons/icon_146_5_24x24.png` |
| shell32_mun | RT_ICON | 594 (group 146) | 20x20 | 32 | `icons/icon_146_6_20x20.png` |
| shell32_mun | RT_ICON | 595 (group 146) | 16x16 | 32 | `icons/icon_146_7_16x16.png` |
| shell32_mun | RT_ICON | 596 (group 148) | 256x256 | 32 | `icons/icon_148_0_256x256.png` |
| shell32_mun | RT_ICON | 597 (group 148) | 64x64 | 32 | `icons/icon_148_1_64x64.png` |
| shell32_mun | RT_ICON | 598 (group 148) | 48x48 | 32 | `icons/icon_148_2_48x48.png` |
| shell32_mun | RT_ICON | 599 (group 148) | 40x40 | 32 | `icons/icon_148_3_40x40.png` |
| shell32_mun | RT_ICON | 6 (group 62998) | 24x24 | 32 | `icons/icon_62998_5_24x24.png` |
| shell32_mun | RT_ICON | 60 (group 63011) | 40x40 | 32 | `icons/icon_63011_3_40x40.png` |
| shell32_mun | RT_ICON | 600 (group 148) | 32x32 | 32 | `icons/icon_148_4_32x32.png` |
| shell32_mun | RT_ICON | 601 (group 148) | 24x24 | 32 | `icons/icon_148_5_24x24.png` |
| shell32_mun | RT_ICON | 602 (group 148) | 20x20 | 32 | `icons/icon_148_6_20x20.png` |
| shell32_mun | RT_ICON | 603 (group 148) | 16x16 | 32 | `icons/icon_148_7_16x16.png` |
| shell32_mun | RT_ICON | 604 (group 147) | 32x32 | 8 | `icons/icon_147_0_32x32.png` |
| shell32_mun | RT_ICON | 605 (group 147) | 16x16 | 4 | `icons/icon_147_1_16x16.png` |
| shell32_mun | RT_ICON | 606 (group 147) | 48x48 | 8 | `icons/icon_147_2_48x48.png` |
| shell32_mun | RT_ICON | 607 (group 147) | 16x16 | 8 | `icons/icon_147_3_16x16.png` |
| shell32_mun | RT_ICON | 608 (group 147) | 32x32 | 4 | `icons/icon_147_4_32x32.png` |
| shell32_mun | RT_ICON | 609 (group 147) | 48x48 | 4 | `icons/icon_147_5_48x48.png` |
| shell32_mun | RT_ICON | 61 (group 63011) | 32x32 | 32 | `icons/icon_63011_4_32x32.png` |
| shell32_mun | RT_ICON | 610 (group 34) | 32x32 | 4 | `icons/icon_34_0_32x32.png` |
| shell32_mun | RT_ICON | 611 (group 34) | 32x32 | 8 | `icons/icon_34_1_32x32.png` |
| shell32_mun | RT_ICON | 612 (group 34) | 16x16 | 4 | `icons/icon_34_2_16x16.png` |
| shell32_mun | RT_ICON | 613 (group 34) | 48x48 | 8 | `icons/icon_34_3_48x48.png` |
| shell32_mun | RT_ICON | 614 (group 34) | 48x48 | 4 | `icons/icon_34_4_48x48.png` |
| shell32_mun | RT_ICON | 615 (group 34) | 16x16 | 8 | `icons/icon_34_5_16x16.png` |
| shell32_mun | RT_ICON | 616 (group 35) | 256x256 | 32 | `icons/icon_35_0_256x256.png` |
| shell32_mun | RT_ICON | 617 (group 35) | 64x64 | 32 | `icons/icon_35_1_64x64.png` |
| shell32_mun | RT_ICON | 618 (group 35) | 48x48 | 32 | `icons/icon_35_2_48x48.png` |
| shell32_mun | RT_ICON | 619 (group 35) | 40x40 | 32 | `icons/icon_35_3_40x40.png` |
| shell32_mun | RT_ICON | 62 (group 63011) | 24x24 | 32 | `icons/icon_63011_5_24x24.png` |
| shell32_mun | RT_ICON | 620 (group 35) | 32x32 | 32 | `icons/icon_35_4_32x32.png` |
| shell32_mun | RT_ICON | 621 (group 35) | 24x24 | 32 | `icons/icon_35_5_24x24.png` |
| shell32_mun | RT_ICON | 622 (group 35) | 20x20 | 32 | `icons/icon_35_6_20x20.png` |
| shell32_mun | RT_ICON | 623 (group 35) | 16x16 | 32 | `icons/icon_35_7_16x16.png` |
| shell32_mun | RT_ICON | 624 (group 151) | 256x256 | 32 | `icons/icon_151_0_256x256.png` |
| shell32_mun | RT_ICON | 625 (group 151) | 64x64 | 32 | `icons/icon_151_1_64x64.png` |
| shell32_mun | RT_ICON | 626 (group 151) | 48x48 | 32 | `icons/icon_151_2_48x48.png` |
| shell32_mun | RT_ICON | 627 (group 151) | 40x40 | 32 | `icons/icon_151_3_40x40.png` |
| shell32_mun | RT_ICON | 628 (group 151) | 32x32 | 32 | `icons/icon_151_4_32x32.png` |
| shell32_mun | RT_ICON | 629 (group 151) | 24x24 | 32 | `icons/icon_151_5_24x24.png` |
| shell32_mun | RT_ICON | 63 (group 63011) | 20x20 | 32 | `icons/icon_63011_6_20x20.png` |
| shell32_mun | RT_ICON | 630 (group 151) | 20x20 | 32 | `icons/icon_151_6_20x20.png` |
| shell32_mun | RT_ICON | 631 (group 151) | 16x16 | 32 | `icons/icon_151_7_16x16.png` |
| shell32_mun | RT_ICON | 632 (group 152) | 256x256 | 32 | `icons/icon_152_0_256x256.png` |
| shell32_mun | RT_ICON | 633 (group 152) | 64x64 | 32 | `icons/icon_152_1_64x64.png` |
| shell32_mun | RT_ICON | 634 (group 152) | 48x48 | 32 | `icons/icon_152_2_48x48.png` |
| shell32_mun | RT_ICON | 635 (group 152) | 40x40 | 32 | `icons/icon_152_3_40x40.png` |
| shell32_mun | RT_ICON | 636 (group 152) | 32x32 | 32 | `icons/icon_152_4_32x32.png` |
| shell32_mun | RT_ICON | 637 (group 152) | 24x24 | 32 | `icons/icon_152_5_24x24.png` |
| shell32_mun | RT_ICON | 638 (group 152) | 20x20 | 32 | `icons/icon_152_6_20x20.png` |
| shell32_mun | RT_ICON | 639 (group 152) | 16x16 | 32 | `icons/icon_152_7_16x16.png` |
| shell32_mun | RT_ICON | 64 (group 63011) | 16x16 | 32 | `icons/icon_63011_7_16x16.png` |
| shell32_mun | RT_ICON | 640 (group 154) | 256x256 | 32 | `icons/icon_154_0_256x256.png` |
| shell32_mun | RT_ICON | 641 (group 154) | 64x64 | 32 | `icons/icon_154_1_64x64.png` |
| shell32_mun | RT_ICON | 642 (group 154) | 48x48 | 32 | `icons/icon_154_2_48x48.png` |
| shell32_mun | RT_ICON | 643 (group 154) | 40x40 | 32 | `icons/icon_154_3_40x40.png` |
| shell32_mun | RT_ICON | 644 (group 154) | 32x32 | 32 | `icons/icon_154_4_32x32.png` |
| shell32_mun | RT_ICON | 645 (group 154) | 24x24 | 32 | `icons/icon_154_5_24x24.png` |
| shell32_mun | RT_ICON | 646 (group 154) | 20x20 | 32 | `icons/icon_154_6_20x20.png` |
| shell32_mun | RT_ICON | 647 (group 154) | 16x16 | 32 | `icons/icon_154_7_16x16.png` |
| shell32_mun | RT_ICON | 648 (group 153) | 256x256 | 32 | `icons/icon_153_0_256x256.png` |
| shell32_mun | RT_ICON | 649 (group 153) | 64x64 | 32 | `icons/icon_153_1_64x64.png` |
| shell32_mun | RT_ICON | 650 (group 153) | 48x48 | 32 | `icons/icon_153_2_48x48.png` |
| shell32_mun | RT_ICON | 651 (group 153) | 40x40 | 32 | `icons/icon_153_3_40x40.png` |
| shell32_mun | RT_ICON | 652 (group 153) | 32x32 | 32 | `icons/icon_153_4_32x32.png` |
| shell32_mun | RT_ICON | 653 (group 153) | 24x24 | 32 | `icons/icon_153_5_24x24.png` |
| shell32_mun | RT_ICON | 654 (group 153) | 20x20 | 32 | `icons/icon_153_6_20x20.png` |
| shell32_mun | RT_ICON | 655 (group 153) | 16x16 | 32 | `icons/icon_153_7_16x16.png` |
| shell32_mun | RT_ICON | 656 (group 155) | 256x256 | 32 | `icons/icon_155_0_256x256.png` |
| shell32_mun | RT_ICON | 657 (group 155) | 64x64 | 32 | `icons/icon_155_1_64x64.png` |
| shell32_mun | RT_ICON | 658 (group 155) | 48x48 | 32 | `icons/icon_155_2_48x48.png` |
| shell32_mun | RT_ICON | 659 (group 155) | 40x40 | 32 | `icons/icon_155_3_40x40.png` |
| shell32_mun | RT_ICON | 660 (group 155) | 32x32 | 32 | `icons/icon_155_4_32x32.png` |
| shell32_mun | RT_ICON | 661 (group 155) | 24x24 | 32 | `icons/icon_155_5_24x24.png` |
| shell32_mun | RT_ICON | 662 (group 155) | 20x20 | 32 | `icons/icon_155_6_20x20.png` |
| shell32_mun | RT_ICON | 663 (group 155) | 16x16 | 32 | `icons/icon_155_7_16x16.png` |
| shell32_mun | RT_ICON | 664 (group 156) | 256x256 | 32 | `icons/icon_156_0_256x256.png` |
| shell32_mun | RT_ICON | 665 (group 156) | 64x64 | 32 | `icons/icon_156_1_64x64.png` |
| shell32_mun | RT_ICON | 666 (group 156) | 48x48 | 32 | `icons/icon_156_2_48x48.png` |
| shell32_mun | RT_ICON | 667 (group 156) | 40x40 | 32 | `icons/icon_156_3_40x40.png` |
| shell32_mun | RT_ICON | 668 (group 156) | 32x32 | 32 | `icons/icon_156_4_32x32.png` |
| shell32_mun | RT_ICON | 669 (group 156) | 24x24 | 32 | `icons/icon_156_5_24x24.png` |
| shell32_mun | RT_ICON | 670 (group 156) | 20x20 | 32 | `icons/icon_156_6_20x20.png` |
| shell32_mun | RT_ICON | 671 (group 156) | 16x16 | 32 | `icons/icon_156_7_16x16.png` |
| shell32_mun | RT_ICON | 672 (group 157) | 256x256 | 32 | `icons/icon_157_0_256x256.png` |
| shell32_mun | RT_ICON | 673 (group 157) | 64x64 | 32 | `icons/icon_157_1_64x64.png` |
| shell32_mun | RT_ICON | 674 (group 157) | 48x48 | 32 | `icons/icon_157_2_48x48.png` |
| shell32_mun | RT_ICON | 675 (group 157) | 40x40 | 32 | `icons/icon_157_3_40x40.png` |
| shell32_mun | RT_ICON | 676 (group 157) | 32x32 | 32 | `icons/icon_157_4_32x32.png` |
| shell32_mun | RT_ICON | 677 (group 157) | 24x24 | 32 | `icons/icon_157_5_24x24.png` |
| shell32_mun | RT_ICON | 678 (group 157) | 20x20 | 32 | `icons/icon_157_6_20x20.png` |
| shell32_mun | RT_ICON | 679 (group 157) | 16x16 | 32 | `icons/icon_157_7_16x16.png` |
| shell32_mun | RT_ICON | 680 (group 174) | 256x256 | 32 | `icons/icon_174_0_256x256.png` |
| shell32_mun | RT_ICON | 681 (group 174) | 64x64 | 32 | `icons/icon_174_1_64x64.png` |
| shell32_mun | RT_ICON | 682 (group 174) | 48x48 | 32 | `icons/icon_174_2_48x48.png` |
| shell32_mun | RT_ICON | 683 (group 174) | 40x40 | 32 | `icons/icon_174_3_40x40.png` |
| shell32_mun | RT_ICON | 684 (group 174) | 32x32 | 32 | `icons/icon_174_4_32x32.png` |
| shell32_mun | RT_ICON | 685 (group 174) | 24x24 | 32 | `icons/icon_174_5_24x24.png` |
| shell32_mun | RT_ICON | 686 (group 174) | 20x20 | 32 | `icons/icon_174_6_20x20.png` |
| shell32_mun | RT_ICON | 687 (group 174) | 16x16 | 32 | `icons/icon_174_7_16x16.png` |
| shell32_mun | RT_ICON | 688 (group 160) | 256x256 | 32 | `icons/icon_160_0_256x256.png` |
| shell32_mun | RT_ICON | 689 (group 160) | 64x64 | 32 | `icons/icon_160_1_64x64.png` |
| shell32_mun | RT_ICON | 690 (group 160) | 48x48 | 32 | `icons/icon_160_2_48x48.png` |
| shell32_mun | RT_ICON | 691 (group 160) | 40x40 | 32 | `icons/icon_160_3_40x40.png` |
| shell32_mun | RT_ICON | 692 (group 160) | 32x32 | 32 | `icons/icon_160_4_32x32.png` |
| shell32_mun | RT_ICON | 693 (group 160) | 24x24 | 32 | `icons/icon_160_5_24x24.png` |
| shell32_mun | RT_ICON | 694 (group 160) | 20x20 | 32 | `icons/icon_160_6_20x20.png` |
| shell32_mun | RT_ICON | 695 (group 160) | 16x16 | 32 | `icons/icon_160_7_16x16.png` |
| shell32_mun | RT_ICON | 696 (group 161) | 256x256 | 32 | `icons/icon_161_0_256x256.png` |
| shell32_mun | RT_ICON | 697 (group 161) | 64x64 | 32 | `icons/icon_161_1_64x64.png` |
| shell32_mun | RT_ICON | 698 (group 161) | 48x48 | 32 | `icons/icon_161_2_48x48.png` |
| shell32_mun | RT_ICON | 699 (group 161) | 40x40 | 32 | `icons/icon_161_3_40x40.png` |
| shell32_mun | RT_ICON | 7 (group 62998) | 20x20 | 32 | `icons/icon_62998_6_20x20.png` |
| shell32_mun | RT_ICON | 70 (group 1) | 256x256 | 32 | `icons/icon_1_0_256x256.png` |
| shell32_mun | RT_ICON | 700 (group 161) | 32x32 | 32 | `icons/icon_161_4_32x32.png` |
| shell32_mun | RT_ICON | 701 (group 161) | 24x24 | 32 | `icons/icon_161_5_24x24.png` |
| shell32_mun | RT_ICON | 702 (group 161) | 20x20 | 32 | `icons/icon_161_6_20x20.png` |
| shell32_mun | RT_ICON | 703 (group 161) | 16x16 | 32 | `icons/icon_161_7_16x16.png` |
| shell32_mun | RT_ICON | 704 (group 16777) | 256x256 | 32 | `icons/icon_16777_0_256x256.png` |
| shell32_mun | RT_ICON | 705 (group 16777) | 64x64 | 32 | `icons/icon_16777_1_64x64.png` |
| shell32_mun | RT_ICON | 706 (group 16777) | 48x48 | 32 | `icons/icon_16777_2_48x48.png` |
| shell32_mun | RT_ICON | 707 (group 16777) | 40x40 | 32 | `icons/icon_16777_3_40x40.png` |
| shell32_mun | RT_ICON | 708 (group 16777) | 32x32 | 32 | `icons/icon_16777_4_32x32.png` |
| shell32_mun | RT_ICON | 709 (group 16777) | 24x24 | 32 | `icons/icon_16777_5_24x24.png` |
| shell32_mun | RT_ICON | 71 (group 1) | 64x64 | 32 | `icons/icon_1_1_64x64.png` |
| shell32_mun | RT_ICON | 710 (group 16777) | 20x20 | 32 | `icons/icon_16777_6_20x20.png` |
| shell32_mun | RT_ICON | 711 (group 16777) | 16x16 | 32 | `icons/icon_16777_7_16x16.png` |
| shell32_mun | RT_ICON | 712 (group 165) | 256x256 | 32 | `icons/icon_165_0_256x256.png` |
| shell32_mun | RT_ICON | 713 (group 165) | 64x64 | 32 | `icons/icon_165_1_64x64.png` |
| shell32_mun | RT_ICON | 714 (group 165) | 48x48 | 32 | `icons/icon_165_2_48x48.png` |
| shell32_mun | RT_ICON | 715 (group 165) | 40x40 | 32 | `icons/icon_165_3_40x40.png` |
| shell32_mun | RT_ICON | 716 (group 165) | 32x32 | 32 | `icons/icon_165_4_32x32.png` |
| shell32_mun | RT_ICON | 717 (group 165) | 24x24 | 32 | `icons/icon_165_5_24x24.png` |
| shell32_mun | RT_ICON | 718 (group 165) | 20x20 | 32 | `icons/icon_165_6_20x20.png` |
| shell32_mun | RT_ICON | 719 (group 165) | 16x16 | 32 | `icons/icon_165_7_16x16.png` |
| shell32_mun | RT_ICON | 72 (group 1) | 48x48 | 32 | `icons/icon_1_2_48x48.png` |
| shell32_mun | RT_ICON | 720 (group 166) | 256x256 | 32 | `icons/icon_166_0_256x256.png` |
| shell32_mun | RT_ICON | 721 (group 166) | 64x64 | 32 | `icons/icon_166_1_64x64.png` |
| shell32_mun | RT_ICON | 722 (group 166) | 48x48 | 32 | `icons/icon_166_2_48x48.png` |
| shell32_mun | RT_ICON | 723 (group 166) | 40x40 | 32 | `icons/icon_166_3_40x40.png` |
| shell32_mun | RT_ICON | 724 (group 166) | 32x32 | 32 | `icons/icon_166_4_32x32.png` |
| shell32_mun | RT_ICON | 725 (group 166) | 24x24 | 32 | `icons/icon_166_5_24x24.png` |
| shell32_mun | RT_ICON | 726 (group 166) | 20x20 | 32 | `icons/icon_166_6_20x20.png` |
| shell32_mun | RT_ICON | 727 (group 166) | 16x16 | 32 | `icons/icon_166_7_16x16.png` |
| shell32_mun | RT_ICON | 728 (group 167) | 256x256 | 32 | `icons/icon_167_0_256x256.png` |
| shell32_mun | RT_ICON | 729 (group 167) | 64x64 | 32 | `icons/icon_167_1_64x64.png` |
| shell32_mun | RT_ICON | 73 (group 1) | 40x40 | 32 | `icons/icon_1_3_40x40.png` |
| shell32_mun | RT_ICON | 730 (group 167) | 48x48 | 32 | `icons/icon_167_2_48x48.png` |
| shell32_mun | RT_ICON | 731 (group 167) | 40x40 | 32 | `icons/icon_167_3_40x40.png` |
| shell32_mun | RT_ICON | 732 (group 167) | 32x32 | 32 | `icons/icon_167_4_32x32.png` |
| shell32_mun | RT_ICON | 733 (group 167) | 24x24 | 32 | `icons/icon_167_5_24x24.png` |
| shell32_mun | RT_ICON | 734 (group 167) | 20x20 | 32 | `icons/icon_167_6_20x20.png` |
| shell32_mun | RT_ICON | 735 (group 167) | 16x16 | 32 | `icons/icon_167_7_16x16.png` |
| shell32_mun | RT_ICON | 736 (group 168) | 256x256 | 32 | `icons/icon_168_0_256x256.png` |
| shell32_mun | RT_ICON | 737 (group 168) | 64x64 | 32 | `icons/icon_168_1_64x64.png` |
| shell32_mun | RT_ICON | 738 (group 168) | 48x48 | 32 | `icons/icon_168_2_48x48.png` |
| shell32_mun | RT_ICON | 739 (group 168) | 40x40 | 32 | `icons/icon_168_3_40x40.png` |
| shell32_mun | RT_ICON | 74 (group 1) | 32x32 | 32 | `icons/icon_1_4_32x32.png` |
| shell32_mun | RT_ICON | 740 (group 168) | 32x32 | 32 | `icons/icon_168_4_32x32.png` |
| shell32_mun | RT_ICON | 741 (group 168) | 24x24 | 32 | `icons/icon_168_5_24x24.png` |
| shell32_mun | RT_ICON | 742 (group 168) | 20x20 | 32 | `icons/icon_168_6_20x20.png` |
| shell32_mun | RT_ICON | 743 (group 168) | 16x16 | 32 | `icons/icon_168_7_16x16.png` |
| shell32_mun | RT_ICON | 744 (group 169) | 256x256 | 32 | `icons/icon_169_0_256x256.png` |
| shell32_mun | RT_ICON | 745 (group 169) | 64x64 | 32 | `icons/icon_169_1_64x64.png` |
| shell32_mun | RT_ICON | 746 (group 169) | 48x48 | 32 | `icons/icon_169_2_48x48.png` |
| shell32_mun | RT_ICON | 747 (group 169) | 40x40 | 32 | `icons/icon_169_3_40x40.png` |
| shell32_mun | RT_ICON | 748 (group 169) | 32x32 | 32 | `icons/icon_169_4_32x32.png` |
| shell32_mun | RT_ICON | 749 (group 169) | 24x24 | 32 | `icons/icon_169_5_24x24.png` |
| shell32_mun | RT_ICON | 75 (group 1) | 24x24 | 32 | `icons/icon_1_5_24x24.png` |
| shell32_mun | RT_ICON | 750 (group 169) | 20x20 | 32 | `icons/icon_169_6_20x20.png` |
| shell32_mun | RT_ICON | 751 (group 169) | 16x16 | 32 | `icons/icon_169_7_16x16.png` |
| shell32_mun | RT_ICON | 752 (group 170) | 256x256 | 32 | `icons/icon_170_0_256x256.png` |
| shell32_mun | RT_ICON | 753 (group 170) | 64x64 | 32 | `icons/icon_170_1_64x64.png` |
| shell32_mun | RT_ICON | 754 (group 170) | 48x48 | 32 | `icons/icon_170_2_48x48.png` |
| shell32_mun | RT_ICON | 755 (group 170) | 40x40 | 32 | `icons/icon_170_3_40x40.png` |
| shell32_mun | RT_ICON | 756 (group 170) | 32x32 | 32 | `icons/icon_170_4_32x32.png` |
| shell32_mun | RT_ICON | 757 (group 170) | 24x24 | 32 | `icons/icon_170_5_24x24.png` |
| shell32_mun | RT_ICON | 758 (group 170) | 20x20 | 32 | `icons/icon_170_6_20x20.png` |
| shell32_mun | RT_ICON | 759 (group 170) | 16x16 | 32 | `icons/icon_170_7_16x16.png` |
| shell32_mun | RT_ICON | 76 (group 1) | 20x20 | 32 | `icons/icon_1_6_20x20.png` |
| shell32_mun | RT_ICON | 760 (group 171) | 32x32 | 4 | `icons/icon_171_0_32x32.png` |
| shell32_mun | RT_ICON | 761 (group 171) | 48x48 | 8 | `icons/icon_171_1_48x48.png` |
| shell32_mun | RT_ICON | 762 (group 171) | 16x16 | 4 | `icons/icon_171_2_16x16.png` |
| shell32_mun | RT_ICON | 763 (group 173) | 256x256 | 32 | `icons/icon_173_0_256x256.png` |
| shell32_mun | RT_ICON | 764 (group 173) | 64x64 | 32 | `icons/icon_173_1_64x64.png` |
| shell32_mun | RT_ICON | 765 (group 173) | 48x48 | 32 | `icons/icon_173_2_48x48.png` |
| shell32_mun | RT_ICON | 766 (group 173) | 40x40 | 32 | `icons/icon_173_3_40x40.png` |
| shell32_mun | RT_ICON | 767 (group 173) | 32x32 | 32 | `icons/icon_173_4_32x32.png` |
| shell32_mun | RT_ICON | 768 (group 173) | 24x24 | 32 | `icons/icon_173_5_24x24.png` |
| shell32_mun | RT_ICON | 769 (group 173) | 20x20 | 32 | `icons/icon_173_6_20x20.png` |
| shell32_mun | RT_ICON | 77 (group 1) | 16x16 | 32 | `icons/icon_1_7_16x16.png` |
| shell32_mun | RT_ICON | 770 (group 173) | 16x16 | 32 | `icons/icon_173_7_16x16.png` |
| shell32_mun | RT_ICON | 771 (group 37219) | 256x256 | 32 | `icons/icon_37219_0_256x256.png` |
| shell32_mun | RT_ICON | 772 (group 37219) | 64x64 | 32 | `icons/icon_37219_1_64x64.png` |
| shell32_mun | RT_ICON | 773 (group 37219) | 48x48 | 32 | `icons/icon_37219_2_48x48.png` |
| shell32_mun | RT_ICON | 774 (group 37219) | 40x40 | 32 | `icons/icon_37219_3_40x40.png` |
| shell32_mun | RT_ICON | 775 (group 37219) | 32x32 | 32 | `icons/icon_37219_4_32x32.png` |
| shell32_mun | RT_ICON | 776 (group 37219) | 24x24 | 32 | `icons/icon_37219_5_24x24.png` |
| shell32_mun | RT_ICON | 777 (group 37219) | 20x20 | 32 | `icons/icon_37219_6_20x20.png` |
| shell32_mun | RT_ICON | 778 (group 37219) | 16x16 | 32 | `icons/icon_37219_7_16x16.png` |
| shell32_mun | RT_ICON | 779 (group 51380) | 256x256 | 32 | `icons/icon_51380_0_256x256.png` |
| shell32_mun | RT_ICON | 78 (group 2) | 256x256 | 32 | `icons/icon_2_0_256x256.png` |
| shell32_mun | RT_ICON | 780 (group 51380) | 64x64 | 32 | `icons/icon_51380_1_64x64.png` |
| shell32_mun | RT_ICON | 781 (group 51380) | 48x48 | 32 | `icons/icon_51380_2_48x48.png` |
| shell32_mun | RT_ICON | 782 (group 51380) | 40x40 | 32 | `icons/icon_51380_3_40x40.png` |
| shell32_mun | RT_ICON | 783 (group 51380) | 32x32 | 32 | `icons/icon_51380_4_32x32.png` |
| shell32_mun | RT_ICON | 784 (group 51380) | 24x24 | 32 | `icons/icon_51380_5_24x24.png` |
| shell32_mun | RT_ICON | 785 (group 51380) | 20x20 | 32 | `icons/icon_51380_6_20x20.png` |
| shell32_mun | RT_ICON | 786 (group 51380) | 16x16 | 32 | `icons/icon_51380_7_16x16.png` |
| shell32_mun | RT_ICON | 787 (group 51586) | 256x256 | 32 | `icons/icon_51586_0_256x256.png` |
| shell32_mun | RT_ICON | 788 (group 51586) | 64x64 | 32 | `icons/icon_51586_1_64x64.png` |
| shell32_mun | RT_ICON | 789 (group 51586) | 48x48 | 32 | `icons/icon_51586_2_48x48.png` |
| shell32_mun | RT_ICON | 79 (group 2) | 64x64 | 32 | `icons/icon_2_1_64x64.png` |
| shell32_mun | RT_ICON | 790 (group 51586) | 40x40 | 32 | `icons/icon_51586_3_40x40.png` |
| shell32_mun | RT_ICON | 791 (group 51586) | 32x32 | 32 | `icons/icon_51586_4_32x32.png` |
| shell32_mun | RT_ICON | 792 (group 51586) | 24x24 | 32 | `icons/icon_51586_5_24x24.png` |
| shell32_mun | RT_ICON | 793 (group 51586) | 20x20 | 32 | `icons/icon_51586_6_20x20.png` |
| shell32_mun | RT_ICON | 794 (group 51586) | 16x16 | 32 | `icons/icon_51586_7_16x16.png` |
| shell32_mun | RT_ICON | 795 (group 175) | 256x256 | 32 | `icons/icon_175_0_256x256.png` |
| shell32_mun | RT_ICON | 796 (group 175) | 64x64 | 32 | `icons/icon_175_1_64x64.png` |
| shell32_mun | RT_ICON | 797 (group 175) | 48x48 | 32 | `icons/icon_175_2_48x48.png` |
| shell32_mun | RT_ICON | 798 (group 175) | 40x40 | 32 | `icons/icon_175_3_40x40.png` |
| shell32_mun | RT_ICON | 799 (group 175) | 32x32 | 32 | `icons/icon_175_4_32x32.png` |
| shell32_mun | RT_ICON | 8 (group 62998) | 16x16 | 32 | `icons/icon_62998_7_16x16.png` |
| shell32_mun | RT_ICON | 80 (group 2) | 48x48 | 32 | `icons/icon_2_2_48x48.png` |
| shell32_mun | RT_ICON | 800 (group 175) | 24x24 | 32 | `icons/icon_175_5_24x24.png` |
| shell32_mun | RT_ICON | 801 (group 175) | 20x20 | 32 | `icons/icon_175_6_20x20.png` |
| shell32_mun | RT_ICON | 802 (group 175) | 16x16 | 32 | `icons/icon_175_7_16x16.png` |
| shell32_mun | RT_ICON | 803 (group 176) | 256x256 | 32 | `icons/icon_176_0_256x256.png` |
| shell32_mun | RT_ICON | 804 (group 176) | 64x64 | 32 | `icons/icon_176_1_64x64.png` |
| shell32_mun | RT_ICON | 805 (group 176) | 48x48 | 32 | `icons/icon_176_2_48x48.png` |
| shell32_mun | RT_ICON | 806 (group 176) | 40x40 | 32 | `icons/icon_176_3_40x40.png` |
| shell32_mun | RT_ICON | 807 (group 176) | 32x32 | 32 | `icons/icon_176_4_32x32.png` |
| shell32_mun | RT_ICON | 808 (group 176) | 24x24 | 32 | `icons/icon_176_5_24x24.png` |
| shell32_mun | RT_ICON | 809 (group 176) | 20x20 | 32 | `icons/icon_176_6_20x20.png` |
| shell32_mun | RT_ICON | 81 (group 2) | 40x40 | 32 | `icons/icon_2_3_40x40.png` |
| shell32_mun | RT_ICON | 810 (group 176) | 16x16 | 32 | `icons/icon_176_7_16x16.png` |
| shell32_mun | RT_ICON | 811 (group 179) | 32x32 | 4 | `icons/icon_179_0_32x32.png` |
| shell32_mun | RT_ICON | 812 (group 179) | 16x16 | 4 | `icons/icon_179_1_16x16.png` |
| shell32_mun | RT_ICON | 813 (group 16780) | 256x256 | 32 | `icons/icon_16780_0_256x256.png` |
| shell32_mun | RT_ICON | 814 (group 16780) | 64x64 | 32 | `icons/icon_16780_1_64x64.png` |
| shell32_mun | RT_ICON | 815 (group 16780) | 48x48 | 32 | `icons/icon_16780_2_48x48.png` |
| shell32_mun | RT_ICON | 816 (group 16780) | 40x40 | 32 | `icons/icon_16780_3_40x40.png` |
| shell32_mun | RT_ICON | 817 (group 16780) | 32x32 | 32 | `icons/icon_16780_4_32x32.png` |
| shell32_mun | RT_ICON | 818 (group 16780) | 24x24 | 32 | `icons/icon_16780_5_24x24.png` |
| shell32_mun | RT_ICON | 819 (group 16780) | 20x20 | 32 | `icons/icon_16780_6_20x20.png` |
| shell32_mun | RT_ICON | 82 (group 2) | 32x32 | 32 | `icons/icon_2_4_32x32.png` |
| shell32_mun | RT_ICON | 820 (group 16780) | 16x16 | 32 | `icons/icon_16780_7_16x16.png` |
| shell32_mun | RT_ICON | 821 (group 16781) | 256x256 | 32 | `icons/icon_16781_0_256x256.png` |
| shell32_mun | RT_ICON | 822 (group 16781) | 64x64 | 32 | `icons/icon_16781_1_64x64.png` |
| shell32_mun | RT_ICON | 823 (group 16781) | 48x48 | 32 | `icons/icon_16781_2_48x48.png` |
| shell32_mun | RT_ICON | 824 (group 16781) | 40x40 | 32 | `icons/icon_16781_3_40x40.png` |
| shell32_mun | RT_ICON | 825 (group 16781) | 32x32 | 32 | `icons/icon_16781_4_32x32.png` |
| shell32_mun | RT_ICON | 826 (group 16781) | 24x24 | 32 | `icons/icon_16781_5_24x24.png` |
| shell32_mun | RT_ICON | 827 (group 16781) | 20x20 | 32 | `icons/icon_16781_6_20x20.png` |
| shell32_mun | RT_ICON | 828 (group 16781) | 16x16 | 32 | `icons/icon_16781_7_16x16.png` |
| shell32_mun | RT_ICON | 829 (group 177) | 32x32 | 4 | `icons/icon_177_0_32x32.png` |
| shell32_mun | RT_ICON | 83 (group 2) | 24x24 | 32 | `icons/icon_2_5_24x24.png` |
| shell32_mun | RT_ICON | 830 (group 177) | 16x16 | 4 | `icons/icon_177_1_16x16.png` |
| shell32_mun | RT_ICON | 831 (group 178) | 48x48 | 4 | `icons/icon_178_0_48x48.png` |
| shell32_mun | RT_ICON | 832 (group 178) | 32x32 | 4 | `icons/icon_178_1_32x32.png` |
| shell32_mun | RT_ICON | 833 (group 178) | 16x16 | 4 | `icons/icon_178_2_16x16.png` |
| shell32_mun | RT_ICON | 834 (group 178) | 48x48 | 8 | `icons/icon_178_3_48x48.png` |
| shell32_mun | RT_ICON | 835 (group 178) | 32x32 | 8 | `icons/icon_178_4_32x32.png` |
| shell32_mun | RT_ICON | 836 (group 178) | 16x16 | 8 | `icons/icon_178_5_16x16.png` |
| shell32_mun | RT_ICON | 837 (group 178) | 48x48 | 32 | `icons/icon_178_6_48x48.png` |
| shell32_mun | RT_ICON | 838 (group 178) | 32x32 | 32 | `icons/icon_178_7_32x32.png` |
| shell32_mun | RT_ICON | 839 (group 178) | 16x16 | 32 | `icons/icon_178_8_16x16.png` |
| shell32_mun | RT_ICON | 84 (group 2) | 20x20 | 32 | `icons/icon_2_6_20x20.png` |
| shell32_mun | RT_ICON | 840 (group 180) | 32x32 | 8 | `icons/icon_180_0_32x32.png` |
| shell32_mun | RT_ICON | 841 (group 181) | 32x32 | 8 | `icons/icon_181_0_32x32.png` |
| shell32_mun | RT_ICON | 842 (group 182) | 32x32 | 4 | `icons/icon_182_0_32x32.png` |
| shell32_mun | RT_ICON | 843 (group 182) | 32x32 | 8 | `icons/icon_182_1_32x32.png` |
| shell32_mun | RT_ICON | 844 (group 182) | 32x32 | 32 | `icons/icon_182_2_32x32.png` |
| shell32_mun | RT_ICON | 845 (group 183) | 32x32 | 4 | `icons/icon_183_0_32x32.png` |
| shell32_mun | RT_ICON | 846 (group 183) | 32x32 | 8 | `icons/icon_183_1_32x32.png` |
| shell32_mun | RT_ICON | 847 (group 183) | 32x32 | 32 | `icons/icon_183_2_32x32.png` |
| shell32_mun | RT_ICON | 848 (group 184) | 256x256 | 32 | `icons/icon_184_0_256x256.png` |
| shell32_mun | RT_ICON | 849 (group 184) | 64x64 | 32 | `icons/icon_184_1_64x64.png` |
| shell32_mun | RT_ICON | 85 (group 2) | 16x16 | 32 | `icons/icon_2_7_16x16.png` |
| shell32_mun | RT_ICON | 850 (group 184) | 48x48 | 32 | `icons/icon_184_2_48x48.png` |
| shell32_mun | RT_ICON | 851 (group 184) | 40x40 | 32 | `icons/icon_184_3_40x40.png` |
| shell32_mun | RT_ICON | 852 (group 184) | 32x32 | 32 | `icons/icon_184_4_32x32.png` |
| shell32_mun | RT_ICON | 853 (group 184) | 24x24 | 32 | `icons/icon_184_5_24x24.png` |
| shell32_mun | RT_ICON | 854 (group 184) | 20x20 | 32 | `icons/icon_184_6_20x20.png` |
| shell32_mun | RT_ICON | 855 (group 184) | 16x16 | 32 | `icons/icon_184_7_16x16.png` |
| shell32_mun | RT_ICON | 856 (group 185) | 256x256 | 32 | `icons/icon_185_0_256x256.png` |
| shell32_mun | RT_ICON | 857 (group 185) | 64x64 | 32 | `icons/icon_185_1_64x64.png` |
| shell32_mun | RT_ICON | 858 (group 185) | 48x48 | 32 | `icons/icon_185_2_48x48.png` |
| shell32_mun | RT_ICON | 859 (group 185) | 40x40 | 32 | `icons/icon_185_3_40x40.png` |
| shell32_mun | RT_ICON | 86 (group 3) | 256x256 | 32 | `icons/icon_3_0_256x256.png` |
| shell32_mun | RT_ICON | 860 (group 185) | 32x32 | 32 | `icons/icon_185_4_32x32.png` |
| shell32_mun | RT_ICON | 861 (group 185) | 24x24 | 32 | `icons/icon_185_5_24x24.png` |
| shell32_mun | RT_ICON | 862 (group 185) | 20x20 | 32 | `icons/icon_185_6_20x20.png` |
| shell32_mun | RT_ICON | 863 (group 185) | 16x16 | 32 | `icons/icon_185_7_16x16.png` |
| shell32_mun | RT_ICON | 864 (group 186) | 256x256 | 32 | `icons/icon_186_0_256x256.png` |
| shell32_mun | RT_ICON | 865 (group 186) | 64x64 | 32 | `icons/icon_186_1_64x64.png` |
| shell32_mun | RT_ICON | 866 (group 186) | 48x48 | 32 | `icons/icon_186_2_48x48.png` |
| shell32_mun | RT_ICON | 867 (group 186) | 40x40 | 32 | `icons/icon_186_3_40x40.png` |
| shell32_mun | RT_ICON | 868 (group 186) | 32x32 | 32 | `icons/icon_186_4_32x32.png` |
| shell32_mun | RT_ICON | 869 (group 186) | 24x24 | 32 | `icons/icon_186_5_24x24.png` |
| shell32_mun | RT_ICON | 87 (group 3) | 64x64 | 32 | `icons/icon_3_1_64x64.png` |
| shell32_mun | RT_ICON | 870 (group 186) | 20x20 | 32 | `icons/icon_186_6_20x20.png` |
| shell32_mun | RT_ICON | 871 (group 186) | 16x16 | 32 | `icons/icon_186_7_16x16.png` |
| shell32_mun | RT_ICON | 872 (group 187) | 256x256 | 32 | `icons/icon_187_0_256x256.png` |
| shell32_mun | RT_ICON | 873 (group 187) | 64x64 | 32 | `icons/icon_187_1_64x64.png` |
| shell32_mun | RT_ICON | 874 (group 187) | 48x48 | 32 | `icons/icon_187_2_48x48.png` |
| shell32_mun | RT_ICON | 875 (group 187) | 40x40 | 32 | `icons/icon_187_3_40x40.png` |
| shell32_mun | RT_ICON | 876 (group 187) | 32x32 | 32 | `icons/icon_187_4_32x32.png` |
| shell32_mun | RT_ICON | 877 (group 187) | 24x24 | 32 | `icons/icon_187_5_24x24.png` |
| shell32_mun | RT_ICON | 878 (group 187) | 20x20 | 32 | `icons/icon_187_6_20x20.png` |
| shell32_mun | RT_ICON | 879 (group 187) | 16x16 | 32 | `icons/icon_187_7_16x16.png` |
| shell32_mun | RT_ICON | 88 (group 3) | 48x48 | 32 | `icons/icon_3_2_48x48.png` |
| shell32_mun | RT_ICON | 880 (group 16806) | 256x256 | 32 | `icons/icon_16806_0_256x256.png` |
| shell32_mun | RT_ICON | 881 (group 16806) | 64x64 | 32 | `icons/icon_16806_1_64x64.png` |
| shell32_mun | RT_ICON | 882 (group 16806) | 48x48 | 32 | `icons/icon_16806_2_48x48.png` |
| shell32_mun | RT_ICON | 883 (group 16806) | 40x40 | 32 | `icons/icon_16806_3_40x40.png` |
| shell32_mun | RT_ICON | 884 (group 16806) | 32x32 | 32 | `icons/icon_16806_4_32x32.png` |
| shell32_mun | RT_ICON | 885 (group 16806) | 24x24 | 32 | `icons/icon_16806_5_24x24.png` |
| shell32_mun | RT_ICON | 886 (group 16806) | 20x20 | 32 | `icons/icon_16806_6_20x20.png` |
| shell32_mun | RT_ICON | 887 (group 16806) | 16x16 | 32 | `icons/icon_16806_7_16x16.png` |
| shell32_mun | RT_ICON | 888 (group 172) | 256x256 | 32 | `icons/icon_172_0_256x256.png` |
| shell32_mun | RT_ICON | 889 (group 172) | 64x64 | 32 | `icons/icon_172_1_64x64.png` |
| shell32_mun | RT_ICON | 89 (group 3) | 40x40 | 32 | `icons/icon_3_3_40x40.png` |
| shell32_mun | RT_ICON | 890 (group 172) | 48x48 | 32 | `icons/icon_172_2_48x48.png` |
| shell32_mun | RT_ICON | 891 (group 172) | 40x40 | 32 | `icons/icon_172_3_40x40.png` |
| shell32_mun | RT_ICON | 892 (group 172) | 32x32 | 32 | `icons/icon_172_4_32x32.png` |
| shell32_mun | RT_ICON | 893 (group 172) | 24x24 | 32 | `icons/icon_172_5_24x24.png` |
| shell32_mun | RT_ICON | 894 (group 172) | 20x20 | 32 | `icons/icon_172_6_20x20.png` |
| shell32_mun | RT_ICON | 895 (group 172) | 16x16 | 32 | `icons/icon_172_7_16x16.png` |
| shell32_mun | RT_ICON | 896 (group 191) | 32x32 | 4 | `icons/icon_191_0_32x32.png` |
| shell32_mun | RT_ICON | 897 (group 191) | 16x16 | 4 | `icons/icon_191_1_16x16.png` |
| shell32_mun | RT_ICON | 898 (group 191) | 32x32 | 8 | `icons/icon_191_2_32x32.png` |
| shell32_mun | RT_ICON | 899 (group 191) | 16x16 | 8 | `icons/icon_191_3_16x16.png` |
| shell32_mun | RT_ICON | 9 (group 62999) | 256x256 | 32 | `icons/icon_62999_0_256x256.png` |
| shell32_mun | RT_ICON | 90 (group 3) | 32x32 | 32 | `icons/icon_3_4_32x32.png` |
| shell32_mun | RT_ICON | 900 (group 191) | 48x48 | 8 | `icons/icon_191_4_48x48.png` |
| shell32_mun | RT_ICON | 901 (group 191) | 48x48 | 4 | `icons/icon_191_5_48x48.png` |
| shell32_mun | RT_ICON | 902 (group 192) | 32x32 | 4 | `icons/icon_192_0_32x32.png` |
| shell32_mun | RT_ICON | 903 (group 192) | 16x16 | 4 | `icons/icon_192_1_16x16.png` |
| shell32_mun | RT_ICON | 904 (group 192) | 48x48 | 4 | `icons/icon_192_2_48x48.png` |
| shell32_mun | RT_ICON | 905 (group 192) | 32x32 | 8 | `icons/icon_192_3_32x32.png` |
| shell32_mun | RT_ICON | 906 (group 192) | 48x48 | 8 | `icons/icon_192_4_48x48.png` |
| shell32_mun | RT_ICON | 907 (group 192) | 16x16 | 8 | `icons/icon_192_5_16x16.png` |
| shell32_mun | RT_ICON | 908 (group 193) | 256x256 | 32 | `icons/icon_193_0_256x256.png` |
| shell32_mun | RT_ICON | 909 (group 193) | 64x64 | 32 | `icons/icon_193_1_64x64.png` |
| shell32_mun | RT_ICON | 91 (group 3) | 24x24 | 32 | `icons/icon_3_5_24x24.png` |
| shell32_mun | RT_ICON | 910 (group 193) | 48x48 | 32 | `icons/icon_193_2_48x48.png` |
| shell32_mun | RT_ICON | 911 (group 193) | 40x40 | 32 | `icons/icon_193_3_40x40.png` |
| shell32_mun | RT_ICON | 912 (group 193) | 32x32 | 32 | `icons/icon_193_4_32x32.png` |
| shell32_mun | RT_ICON | 913 (group 193) | 24x24 | 32 | `icons/icon_193_5_24x24.png` |
| shell32_mun | RT_ICON | 914 (group 193) | 20x20 | 32 | `icons/icon_193_6_20x20.png` |
| shell32_mun | RT_ICON | 915 (group 193) | 16x16 | 32 | `icons/icon_193_7_16x16.png` |
| shell32_mun | RT_ICON | 916 (group 194) | 256x256 | 32 | `icons/icon_194_0_256x256.png` |
| shell32_mun | RT_ICON | 917 (group 194) | 64x64 | 32 | `icons/icon_194_1_64x64.png` |
| shell32_mun | RT_ICON | 918 (group 194) | 48x48 | 32 | `icons/icon_194_2_48x48.png` |
| shell32_mun | RT_ICON | 919 (group 194) | 40x40 | 32 | `icons/icon_194_3_40x40.png` |
| shell32_mun | RT_ICON | 92 (group 3) | 20x20 | 32 | `icons/icon_3_6_20x20.png` |
| shell32_mun | RT_ICON | 920 (group 194) | 32x32 | 32 | `icons/icon_194_4_32x32.png` |
| shell32_mun | RT_ICON | 921 (group 194) | 24x24 | 32 | `icons/icon_194_5_24x24.png` |
| shell32_mun | RT_ICON | 922 (group 194) | 20x20 | 32 | `icons/icon_194_6_20x20.png` |
| shell32_mun | RT_ICON | 923 (group 194) | 16x16 | 32 | `icons/icon_194_7_16x16.png` |
| shell32_mun | RT_ICON | 924 (group 196) | 256x256 | 32 | `icons/icon_196_0_256x256.png` |
| shell32_mun | RT_ICON | 925 (group 196) | 64x64 | 32 | `icons/icon_196_1_64x64.png` |
| shell32_mun | RT_ICON | 926 (group 196) | 48x48 | 32 | `icons/icon_196_2_48x48.png` |
| shell32_mun | RT_ICON | 927 (group 196) | 40x40 | 32 | `icons/icon_196_3_40x40.png` |
| shell32_mun | RT_ICON | 928 (group 196) | 32x32 | 32 | `icons/icon_196_4_32x32.png` |
| shell32_mun | RT_ICON | 929 (group 196) | 24x24 | 32 | `icons/icon_196_5_24x24.png` |
| shell32_mun | RT_ICON | 93 (group 3) | 16x16 | 32 | `icons/icon_3_7_16x16.png` |
| shell32_mun | RT_ICON | 930 (group 196) | 20x20 | 32 | `icons/icon_196_6_20x20.png` |
| shell32_mun | RT_ICON | 931 (group 196) | 16x16 | 32 | `icons/icon_196_7_16x16.png` |
| shell32_mun | RT_ICON | 932 (group 197) | 256x256 | 32 | `icons/icon_197_0_256x256.png` |
| shell32_mun | RT_ICON | 933 (group 197) | 64x64 | 32 | `icons/icon_197_1_64x64.png` |
| shell32_mun | RT_ICON | 934 (group 197) | 48x48 | 32 | `icons/icon_197_2_48x48.png` |
| shell32_mun | RT_ICON | 935 (group 197) | 40x40 | 32 | `icons/icon_197_3_40x40.png` |
| shell32_mun | RT_ICON | 936 (group 197) | 32x32 | 32 | `icons/icon_197_4_32x32.png` |
| shell32_mun | RT_ICON | 937 (group 197) | 24x24 | 32 | `icons/icon_197_5_24x24.png` |
| shell32_mun | RT_ICON | 938 (group 197) | 20x20 | 32 | `icons/icon_197_6_20x20.png` |
| shell32_mun | RT_ICON | 939 (group 197) | 16x16 | 32 | `icons/icon_197_7_16x16.png` |
| shell32_mun | RT_ICON | 94 (group 4) | 256x256 | 32 | `icons/icon_4_0_256x256.png` |
| shell32_mun | RT_ICON | 940 (group 199) | 256x256 | 32 | `icons/icon_199_0_256x256.png` |
| shell32_mun | RT_ICON | 941 (group 199) | 64x64 | 32 | `icons/icon_199_1_64x64.png` |
| shell32_mun | RT_ICON | 942 (group 199) | 48x48 | 32 | `icons/icon_199_2_48x48.png` |
| shell32_mun | RT_ICON | 943 (group 199) | 40x40 | 32 | `icons/icon_199_3_40x40.png` |
| shell32_mun | RT_ICON | 944 (group 199) | 32x32 | 32 | `icons/icon_199_4_32x32.png` |
| shell32_mun | RT_ICON | 945 (group 199) | 24x24 | 32 | `icons/icon_199_5_24x24.png` |
| shell32_mun | RT_ICON | 946 (group 199) | 20x20 | 32 | `icons/icon_199_6_20x20.png` |
| shell32_mun | RT_ICON | 947 (group 199) | 16x16 | 32 | `icons/icon_199_7_16x16.png` |
| shell32_mun | RT_ICON | 948 (group 198) | 256x256 | 32 | `icons/icon_198_0_256x256.png` |
| shell32_mun | RT_ICON | 949 (group 198) | 64x64 | 32 | `icons/icon_198_1_64x64.png` |
| shell32_mun | RT_ICON | 95 (group 4) | 64x64 | 32 | `icons/icon_4_1_64x64.png` |
| shell32_mun | RT_ICON | 950 (group 198) | 48x48 | 32 | `icons/icon_198_2_48x48.png` |
| shell32_mun | RT_ICON | 951 (group 198) | 40x40 | 32 | `icons/icon_198_3_40x40.png` |
| shell32_mun | RT_ICON | 952 (group 198) | 32x32 | 32 | `icons/icon_198_4_32x32.png` |
| shell32_mun | RT_ICON | 953 (group 198) | 24x24 | 32 | `icons/icon_198_5_24x24.png` |
| shell32_mun | RT_ICON | 954 (group 198) | 20x20 | 32 | `icons/icon_198_6_20x20.png` |
| shell32_mun | RT_ICON | 955 (group 198) | 16x16 | 32 | `icons/icon_198_7_16x16.png` |
| shell32_mun | RT_ICON | 956 (group 16710) | 256x256 | 32 | `icons/icon_16710_0_256x256.png` |
| shell32_mun | RT_ICON | 957 (group 16710) | 64x64 | 32 | `icons/icon_16710_1_64x64.png` |
| shell32_mun | RT_ICON | 958 (group 16710) | 48x48 | 32 | `icons/icon_16710_2_48x48.png` |
| shell32_mun | RT_ICON | 959 (group 16710) | 40x40 | 32 | `icons/icon_16710_3_40x40.png` |
| shell32_mun | RT_ICON | 96 (group 4) | 48x48 | 32 | `icons/icon_4_2_48x48.png` |
| shell32_mun | RT_ICON | 960 (group 16710) | 32x32 | 32 | `icons/icon_16710_4_32x32.png` |
| shell32_mun | RT_ICON | 961 (group 16710) | 24x24 | 32 | `icons/icon_16710_5_24x24.png` |
| shell32_mun | RT_ICON | 962 (group 16710) | 20x20 | 32 | `icons/icon_16710_6_20x20.png` |
| shell32_mun | RT_ICON | 963 (group 16710) | 16x16 | 32 | `icons/icon_16710_7_16x16.png` |
| shell32_mun | RT_ICON | 964 (group 16715) | 256x256 | 32 | `icons/icon_16715_0_256x256.png` |
| shell32_mun | RT_ICON | 965 (group 16715) | 64x64 | 32 | `icons/icon_16715_1_64x64.png` |
| shell32_mun | RT_ICON | 966 (group 16715) | 48x48 | 32 | `icons/icon_16715_2_48x48.png` |
| shell32_mun | RT_ICON | 967 (group 16715) | 40x40 | 32 | `icons/icon_16715_3_40x40.png` |
| shell32_mun | RT_ICON | 968 (group 16715) | 32x32 | 32 | `icons/icon_16715_4_32x32.png` |
| shell32_mun | RT_ICON | 969 (group 16715) | 24x24 | 32 | `icons/icon_16715_5_24x24.png` |
| shell32_mun | RT_ICON | 97 (group 4) | 40x40 | 32 | `icons/icon_4_3_40x40.png` |
| shell32_mun | RT_ICON | 970 (group 16715) | 20x20 | 32 | `icons/icon_16715_6_20x20.png` |
| shell32_mun | RT_ICON | 971 (group 16715) | 16x16 | 32 | `icons/icon_16715_7_16x16.png` |
| shell32_mun | RT_ICON | 972 (group 16717) | 256x256 | 32 | `icons/icon_16717_0_256x256.png` |
| shell32_mun | RT_ICON | 973 (group 16717) | 64x64 | 32 | `icons/icon_16717_1_64x64.png` |
| shell32_mun | RT_ICON | 974 (group 16717) | 48x48 | 32 | `icons/icon_16717_2_48x48.png` |
| shell32_mun | RT_ICON | 975 (group 16717) | 40x40 | 32 | `icons/icon_16717_3_40x40.png` |
| shell32_mun | RT_ICON | 976 (group 16717) | 32x32 | 32 | `icons/icon_16717_4_32x32.png` |
| shell32_mun | RT_ICON | 977 (group 16717) | 24x24 | 32 | `icons/icon_16717_5_24x24.png` |
| shell32_mun | RT_ICON | 978 (group 16717) | 20x20 | 32 | `icons/icon_16717_6_20x20.png` |
| shell32_mun | RT_ICON | 979 (group 16717) | 16x16 | 32 | `icons/icon_16717_7_16x16.png` |
| shell32_mun | RT_ICON | 98 (group 4) | 32x32 | 32 | `icons/icon_4_4_32x32.png` |
| shell32_mun | RT_ICON | 980 (group 16718) | 256x256 | 32 | `icons/icon_16718_0_256x256.png` |
| shell32_mun | RT_ICON | 981 (group 16718) | 64x64 | 32 | `icons/icon_16718_1_64x64.png` |
| shell32_mun | RT_ICON | 982 (group 16718) | 48x48 | 32 | `icons/icon_16718_2_48x48.png` |
| shell32_mun | RT_ICON | 983 (group 16718) | 40x40 | 32 | `icons/icon_16718_3_40x40.png` |
| shell32_mun | RT_ICON | 984 (group 16718) | 32x32 | 32 | `icons/icon_16718_4_32x32.png` |
| shell32_mun | RT_ICON | 985 (group 16718) | 24x24 | 32 | `icons/icon_16718_5_24x24.png` |
| shell32_mun | RT_ICON | 986 (group 16718) | 20x20 | 32 | `icons/icon_16718_6_20x20.png` |
| shell32_mun | RT_ICON | 987 (group 16718) | 16x16 | 32 | `icons/icon_16718_7_16x16.png` |
| shell32_mun | RT_ICON | 988 (group 16721) | 256x256 | 32 | `icons/icon_16721_0_256x256.png` |
| shell32_mun | RT_ICON | 989 (group 16721) | 64x64 | 32 | `icons/icon_16721_1_64x64.png` |
| shell32_mun | RT_ICON | 99 (group 4) | 24x24 | 32 | `icons/icon_4_5_24x24.png` |
| shell32_mun | RT_ICON | 990 (group 16721) | 48x48 | 32 | `icons/icon_16721_2_48x48.png` |
| shell32_mun | RT_ICON | 991 (group 16721) | 40x40 | 32 | `icons/icon_16721_3_40x40.png` |
| shell32_mun | RT_ICON | 992 (group 16721) | 32x32 | 32 | `icons/icon_16721_4_32x32.png` |
| shell32_mun | RT_ICON | 993 (group 16721) | 24x24 | 32 | `icons/icon_16721_5_24x24.png` |
| shell32_mun | RT_ICON | 994 (group 16721) | 20x20 | 32 | `icons/icon_16721_6_20x20.png` |
| shell32_mun | RT_ICON | 995 (group 16721) | 16x16 | 32 | `icons/icon_16721_7_16x16.png` |
| shell32_mun | RT_ICON | 996 (group 16810) | 256x256 | 32 | `icons/icon_16810_0_256x256.png` |
| shell32_mun | RT_ICON | 997 (group 16810) | 64x64 | 32 | `icons/icon_16810_1_64x64.png` |
| shell32_mun | RT_ICON | 998 (group 16810) | 48x48 | 32 | `icons/icon_16810_2_48x48.png` |
| shell32_mun | RT_ICON | 999 (group 16810) | 40x40 | 32 | `icons/icon_16810_3_40x40.png` |
| shell32_mun | RT_GROUP_CURSOR | 1001 | multi | - | `cursors/groupcursor_1001.cur` |
| shell32_mun | RT_GROUP_CURSOR | 1002 | multi | - | `cursors/groupcursor_1002.cur` |
| shell32_mun | RT_GROUP_CURSOR | 1003 | multi | - | `cursors/groupcursor_1003.cur` |
| shell32_mun | RT_GROUP_CURSOR | 1004 | multi | - | `cursors/groupcursor_1004.cur` |
| shell32_mun | RT_GROUP_CURSOR | 1005 | multi | - | `cursors/groupcursor_1005.cur` |
| shell32_mun | RT_CURSOR | 65 | 32x32 | 1 | `icons/cursor_1001_65_32x32.png` |
| shell32_mun | RT_CURSOR | 66 | 32x32 | 1 | `icons/cursor_1002_66_32x32.png` |
| shell32_mun | RT_CURSOR | 67 | 32x32 | 1 | `icons/cursor_1003_67_32x32.png` |
| shell32_mun | RT_CURSOR | 68 | 32x32 | 1 | `icons/cursor_1004_68_32x32.png` |
| shell32_mun | RT_CURSOR | 69 | 32x32 | 1 | `icons/cursor_1005_69_32x32.png` |
| imageres_mun | RT_BITMAP | 5000 | 216x12 | 32 | `bitmaps/bitmap_5000.png` |
| imageres_mun | RT_BITMAP | 5001 | 288x16 | 32 | `bitmaps/bitmap_5001.png` |
| imageres_mun | RT_BITMAP | 5002 | 360x20 | 32 | `bitmaps/bitmap_5002.png` |
| imageres_mun | RT_BITMAP | 5003 | 450x25 | 32 | `bitmaps/bitmap_5003.png` |
| imageres_mun | RT_BITMAP | 5004 | 540x30 | 32 | `bitmaps/bitmap_5004.png` |
| imageres_mun | RT_BITMAP | 5010 | 12x216 | 32 | `bitmaps/bitmap_5010.png` |
| imageres_mun | RT_BITMAP | 5011 | 16x288 | 32 | `bitmaps/bitmap_5011.png` |
| imageres_mun | RT_BITMAP | 5012 | 20x360 | 32 | `bitmaps/bitmap_5012.png` |
| imageres_mun | RT_BITMAP | 5013 | 25x450 | 32 | `bitmaps/bitmap_5013.png` |
| imageres_mun | RT_GROUP_ICON | "BLANK0" | 16x16 | 4 | `icons/groupicon_BLANK0.ico` |
| imageres_mun | RT_GROUP_ICON | "BLANK1" | 16x16 | 4 | `icons/groupicon_BLANK1.ico` |
| imageres_mun | RT_GROUP_ICON | 10 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_10.ico` |
| imageres_mun | RT_GROUP_ICON | 100 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_100.ico` |
| imageres_mun | RT_GROUP_ICON | 1001 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1001.ico` |
| imageres_mun | RT_GROUP_ICON | 1002 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1002.ico` |
| imageres_mun | RT_GROUP_ICON | 1003 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1003.ico` |
| imageres_mun | RT_GROUP_ICON | 1004 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1004.ico` |
| imageres_mun | RT_GROUP_ICON | 1005 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1005.ico` |
| imageres_mun | RT_GROUP_ICON | 1008 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1008.ico` |
| imageres_mun | RT_GROUP_ICON | 101 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_101.ico` |
| imageres_mun | RT_GROUP_ICON | 1010 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1010.ico` |
| imageres_mun | RT_GROUP_ICON | 1013 | 16x16 | 4 | `icons/groupicon_1013.ico` |
| imageres_mun | RT_GROUP_ICON | 1014 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1014.ico` |
| imageres_mun | RT_GROUP_ICON | 1015 | 256x256 | 32 | `icons/groupicon_1015.ico` |
| imageres_mun | RT_GROUP_ICON | 1017 | 256x256 | 32 | `icons/groupicon_1017.ico` |
| imageres_mun | RT_GROUP_ICON | 1018 | 256x256 | 32 | `icons/groupicon_1018.ico` |
| imageres_mun | RT_GROUP_ICON | 102 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_102.ico` |
| imageres_mun | RT_GROUP_ICON | 1020 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1020.ico` |
| imageres_mun | RT_GROUP_ICON | 1021 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1021.ico` |
| imageres_mun | RT_GROUP_ICON | 1022 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1022.ico` |
| imageres_mun | RT_GROUP_ICON | 1023 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1023.ico` |
| imageres_mun | RT_GROUP_ICON | 1024 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1024.ico` |
| imageres_mun | RT_GROUP_ICON | 1025 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1025.ico` |
| imageres_mun | RT_GROUP_ICON | 1026 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1026.ico` |
| imageres_mun | RT_GROUP_ICON | 1027 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1027.ico` |
| imageres_mun | RT_GROUP_ICON | 1028 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1028.ico` |
| imageres_mun | RT_GROUP_ICON | 1029 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1029.ico` |
| imageres_mun | RT_GROUP_ICON | 103 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_103.ico` |
| imageres_mun | RT_GROUP_ICON | 1030 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1030.ico` |
| imageres_mun | RT_GROUP_ICON | 1031 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1031.ico` |
| imageres_mun | RT_GROUP_ICON | 1032 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1032.ico` |
| imageres_mun | RT_GROUP_ICON | 1033 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1033.ico` |
| imageres_mun | RT_GROUP_ICON | 1034 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1034.ico` |
| imageres_mun | RT_GROUP_ICON | 1035 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1035.ico` |
| imageres_mun | RT_GROUP_ICON | 1036 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1036.ico` |
| imageres_mun | RT_GROUP_ICON | 1037 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1037.ico` |
| imageres_mun | RT_GROUP_ICON | 104 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_104.ico` |
| imageres_mun | RT_GROUP_ICON | 1040 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1040.ico` |
| imageres_mun | RT_GROUP_ICON | 1041 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1041.ico` |
| imageres_mun | RT_GROUP_ICON | 1042 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1042.ico` |
| imageres_mun | RT_GROUP_ICON | 1043 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1043.ico` |
| imageres_mun | RT_GROUP_ICON | 105 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_105.ico` |
| imageres_mun | RT_GROUP_ICON | 106 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_106.ico` |
| imageres_mun | RT_GROUP_ICON | 107 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_107.ico` |
| imageres_mun | RT_GROUP_ICON | 108 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_108.ico` |
| imageres_mun | RT_GROUP_ICON | 109 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_109.ico` |
| imageres_mun | RT_GROUP_ICON | 110 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_110.ico` |
| imageres_mun | RT_GROUP_ICON | 111 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_111.ico` |
| imageres_mun | RT_GROUP_ICON | 112 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_112.ico` |
| imageres_mun | RT_GROUP_ICON | 113 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_113.ico` |
| imageres_mun | RT_GROUP_ICON | 114 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_114.ico` |
| imageres_mun | RT_GROUP_ICON | 115 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_115.ico` |
| imageres_mun | RT_GROUP_ICON | 116 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_116.ico` |
| imageres_mun | RT_GROUP_ICON | 117 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_117.ico` |
| imageres_mun | RT_GROUP_ICON | 119 | 16x16 | 4 | `icons/groupicon_119.ico` |
| imageres_mun | RT_GROUP_ICON | 120 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_120.ico` |
| imageres_mun | RT_GROUP_ICON | 121 | 16x16 | 4 | `icons/groupicon_121.ico` |
| imageres_mun | RT_GROUP_ICON | 122 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_122.ico` |
| imageres_mun | RT_GROUP_ICON | 123 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_123.ico` |
| imageres_mun | RT_GROUP_ICON | 124 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_124.ico` |
| imageres_mun | RT_GROUP_ICON | 125 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_125.ico` |
| imageres_mun | RT_GROUP_ICON | 126 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_126.ico` |
| imageres_mun | RT_GROUP_ICON | 127 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_127.ico` |
| imageres_mun | RT_GROUP_ICON | 128 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_128.ico` |
| imageres_mun | RT_GROUP_ICON | 129 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_129.ico` |
| imageres_mun | RT_GROUP_ICON | 130 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_130.ico` |
| imageres_mun | RT_GROUP_ICON | 1301 | 16x16 | 4 | `icons/groupicon_1301.ico` |
| imageres_mun | RT_GROUP_ICON | 1302 | 16x16 | 4 | `icons/groupicon_1302.ico` |
| imageres_mun | RT_GROUP_ICON | 1303 | 16x16 | 4 | `icons/groupicon_1303.ico` |
| imageres_mun | RT_GROUP_ICON | 1304 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1304.ico` |
| imageres_mun | RT_GROUP_ICON | 1305 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1305.ico` |
| imageres_mun | RT_GROUP_ICON | 1306 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1306.ico` |
| imageres_mun | RT_GROUP_ICON | 131 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_131.ico` |
| imageres_mun | RT_GROUP_ICON | 132 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_132.ico` |
| imageres_mun | RT_GROUP_ICON | 133 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_133.ico` |
| imageres_mun | RT_GROUP_ICON | 134 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_134.ico` |
| imageres_mun | RT_GROUP_ICON | 135 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_135.ico` |
| imageres_mun | RT_GROUP_ICON | 136 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_136.ico` |
| imageres_mun | RT_GROUP_ICON | 137 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_137.ico` |
| imageres_mun | RT_GROUP_ICON | 138 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_138.ico` |
| imageres_mun | RT_GROUP_ICON | 139 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_139.ico` |
| imageres_mun | RT_GROUP_ICON | 14 | 16x16 | 4 | `icons/groupicon_14.ico` |
| imageres_mun | RT_GROUP_ICON | 140 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_140.ico` |
| imageres_mun | RT_GROUP_ICON | 1400 | 60x60+48x48+36x36+30x30+27x27+24x24+21x21+18x18+15x15+12x12 | 32,32,32,32,32,32,32,32,32,32 | `icons/groupicon_1400.ico` |
| imageres_mun | RT_GROUP_ICON | 1401 | 60x60+48x48+36x36+30x30+27x27+24x24+21x21+18x18+15x15+12x12 | 32,32,32,32,32,32,32,32,32,32 | `icons/groupicon_1401.ico` |
| imageres_mun | RT_GROUP_ICON | 1402 | 60x60+48x48+36x36+30x30+27x27+24x24+21x21+18x18+15x15+12x12 | 32,32,32,32,32,32,32,32,32,32 | `icons/groupicon_1402.ico` |
| imageres_mun | RT_GROUP_ICON | 1403 | 60x60+48x48+36x36+30x30+27x27+24x24+21x21+18x18+15x15+12x12 | 32,32,32,32,32,32,32,32,32,32 | `icons/groupicon_1403.ico` |
| imageres_mun | RT_GROUP_ICON | 1404 | 60x60+48x48+36x36+30x30+27x27+24x24+21x21+18x18+15x15+12x12 | 32,32,32,32,32,32,32,32,32,32 | `icons/groupicon_1404.ico` |
| imageres_mun | RT_GROUP_ICON | 1405 | 60x60+48x48+36x36+30x30+27x27+24x24+21x21+18x18+15x15+12x12 | 32,32,32,32,32,32,32,32,32,32 | `icons/groupicon_1405.ico` |
| imageres_mun | RT_GROUP_ICON | 141 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_141.ico` |
| imageres_mun | RT_GROUP_ICON | 142 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_142.ico` |
| imageres_mun | RT_GROUP_ICON | 143 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_143.ico` |
| imageres_mun | RT_GROUP_ICON | 144 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_144.ico` |
| imageres_mun | RT_GROUP_ICON | 145 | 48x48+32x32+16x16+48x48+32x32+16x16+256x256+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32,32 | `icons/groupicon_145.ico` |
| imageres_mun | RT_GROUP_ICON | 146 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_146.ico` |
| imageres_mun | RT_GROUP_ICON | 147 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_147.ico` |
| imageres_mun | RT_GROUP_ICON | 148 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_148.ico` |
| imageres_mun | RT_GROUP_ICON | 149 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_149.ico` |
| imageres_mun | RT_GROUP_ICON | 15 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_15.ico` |
| imageres_mun | RT_GROUP_ICON | 150 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_150.ico` |
| imageres_mun | RT_GROUP_ICON | 151 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_151.ico` |
| imageres_mun | RT_GROUP_ICON | 152 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_152.ico` |
| imageres_mun | RT_GROUP_ICON | 155 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_155.ico` |
| imageres_mun | RT_GROUP_ICON | 157 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_157.ico` |
| imageres_mun | RT_GROUP_ICON | 158 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_158.ico` |
| imageres_mun | RT_GROUP_ICON | 159 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_159.ico` |
| imageres_mun | RT_GROUP_ICON | 160 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_160.ico` |
| imageres_mun | RT_GROUP_ICON | 161 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_161.ico` |
| imageres_mun | RT_GROUP_ICON | 162 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_162.ico` |
| imageres_mun | RT_GROUP_ICON | 163 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_163.ico` |
| imageres_mun | RT_GROUP_ICON | 164 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_164.ico` |
| imageres_mun | RT_GROUP_ICON | 165 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_165.ico` |
| imageres_mun | RT_GROUP_ICON | 166 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_166.ico` |
| imageres_mun | RT_GROUP_ICON | 167 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_167.ico` |
| imageres_mun | RT_GROUP_ICON | 168 | 48x48+32x32+24x24+16x16+48x48+32x32+24x24+16x16+256x256+48x48+32x32+24x24+16x16 | 4,4,4,4,8,8,8,8,32,32,32,32,32 | `icons/groupicon_168.ico` |
| imageres_mun | RT_GROUP_ICON | 169 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_169.ico` |
| imageres_mun | RT_GROUP_ICON | 17 | 256x256+256x256 | 8,32 | `icons/groupicon_17.ico` |
| imageres_mun | RT_GROUP_ICON | 170 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_170.ico` |
| imageres_mun | RT_GROUP_ICON | 171 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_171.ico` |
| imageres_mun | RT_GROUP_ICON | 172 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_172.ico` |
| imageres_mun | RT_GROUP_ICON | 173 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_173.ico` |
| imageres_mun | RT_GROUP_ICON | 174 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_174.ico` |
| imageres_mun | RT_GROUP_ICON | 175 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_175.ico` |
| imageres_mun | RT_GROUP_ICON | 176 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_176.ico` |
| imageres_mun | RT_GROUP_ICON | 177 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_177.ico` |
| imageres_mun | RT_GROUP_ICON | 178 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_178.ico` |
| imageres_mun | RT_GROUP_ICON | 179 | 48x48+32x32+16x16+48x48+32x32+16x16+256x256+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32,32 | `icons/groupicon_179.ico` |
| imageres_mun | RT_GROUP_ICON | 18 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_18.ico` |
| imageres_mun | RT_GROUP_ICON | 180 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_180.ico` |
| imageres_mun | RT_GROUP_ICON | 181 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_181.ico` |
| imageres_mun | RT_GROUP_ICON | 182 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_182.ico` |
| imageres_mun | RT_GROUP_ICON | 183 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_183.ico` |
| imageres_mun | RT_GROUP_ICON | 184 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_184.ico` |
| imageres_mun | RT_GROUP_ICON | 185 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_185.ico` |
| imageres_mun | RT_GROUP_ICON | 186 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_186.ico` |
| imageres_mun | RT_GROUP_ICON | 189 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_189.ico` |
| imageres_mun | RT_GROUP_ICON | 19 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_19.ico` |
| imageres_mun | RT_GROUP_ICON | 190 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_190.ico` |
| imageres_mun | RT_GROUP_ICON | 191 | 16x16+256x256+96x96+48x48+32x32 | 24,32,32,32,32 | `icons/groupicon_191.ico` |
| imageres_mun | RT_GROUP_ICON | 192 | 16x16+256x256+96x96+48x48+32x32 | 24,32,32,32,32 | `icons/groupicon_192.ico` |
| imageres_mun | RT_GROUP_ICON | 193 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_193.ico` |
| imageres_mun | RT_GROUP_ICON | 194 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_194.ico` |
| imageres_mun | RT_GROUP_ICON | 195 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_195.ico` |
| imageres_mun | RT_GROUP_ICON | 196 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_196.ico` |
| imageres_mun | RT_GROUP_ICON | 197 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_197.ico` |
| imageres_mun | RT_GROUP_ICON | 198 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_198.ico` |
| imageres_mun | RT_GROUP_ICON | 2 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_2.ico` |
| imageres_mun | RT_GROUP_ICON | 20 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_20.ico` |
| imageres_mun | RT_GROUP_ICON | 21 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_21.ico` |
| imageres_mun | RT_GROUP_ICON | 22 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_22.ico` |
| imageres_mun | RT_GROUP_ICON | 23 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_23.ico` |
| imageres_mun | RT_GROUP_ICON | 24 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_24.ico` |
| imageres_mun | RT_GROUP_ICON | 25 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_25.ico` |
| imageres_mun | RT_GROUP_ICON | 26 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_26.ico` |
| imageres_mun | RT_GROUP_ICON | 27 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_27.ico` |
| imageres_mun | RT_GROUP_ICON | 28 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_28.ico` |
| imageres_mun | RT_GROUP_ICON | 29 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_29.ico` |
| imageres_mun | RT_GROUP_ICON | 3 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_3.ico` |
| imageres_mun | RT_GROUP_ICON | 30 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_30.ico` |
| imageres_mun | RT_GROUP_ICON | 31 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_31.ico` |
| imageres_mun | RT_GROUP_ICON | 32 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_32.ico` |
| imageres_mun | RT_GROUP_ICON | 33 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_33.ico` |
| imageres_mun | RT_GROUP_ICON | 34 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_34.ico` |
| imageres_mun | RT_GROUP_ICON | 35 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_35.ico` |
| imageres_mun | RT_GROUP_ICON | 36 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_36.ico` |
| imageres_mun | RT_GROUP_ICON | 37 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_37.ico` |
| imageres_mun | RT_GROUP_ICON | 38 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_38.ico` |
| imageres_mun | RT_GROUP_ICON | 39 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_39.ico` |
| imageres_mun | RT_GROUP_ICON | 4 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_4.ico` |
| imageres_mun | RT_GROUP_ICON | 40 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_40.ico` |
| imageres_mun | RT_GROUP_ICON | 41 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_41.ico` |
| imageres_mun | RT_GROUP_ICON | 42 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_42.ico` |
| imageres_mun | RT_GROUP_ICON | 43 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_43.ico` |
| imageres_mun | RT_GROUP_ICON | 44 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_44.ico` |
| imageres_mun | RT_GROUP_ICON | 45 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_45.ico` |
| imageres_mun | RT_GROUP_ICON | 46 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_46.ico` |
| imageres_mun | RT_GROUP_ICON | 47 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_47.ico` |
| imageres_mun | RT_GROUP_ICON | 48 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_48.ico` |
| imageres_mun | RT_GROUP_ICON | 49 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_49.ico` |
| imageres_mun | RT_GROUP_ICON | 5 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5.ico` |
| imageres_mun | RT_GROUP_ICON | 50 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_50.ico` |
| imageres_mun | RT_GROUP_ICON | 51 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_51.ico` |
| imageres_mun | RT_GROUP_ICON | 5100 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5100.ico` |
| imageres_mun | RT_GROUP_ICON | 5101 | 256x256+128x128+96x96+80x80+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32,32,32,32 | `icons/groupicon_5101.ico` |
| imageres_mun | RT_GROUP_ICON | 5102 | 32x32+24x24+20x20+16x16+32x32+24x24+20x20+16x16+32x32+24x24+20x20+16x16 | 4,4,4,4,8,8,8,8,32,32,32,32 | `icons/groupicon_5102.ico` |
| imageres_mun | RT_GROUP_ICON | 52 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_52.ico` |
| imageres_mun | RT_GROUP_ICON | 5201 | 48x48+32x32+16x16+48x48+32x32+16x16+256x256+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32,32 | `icons/groupicon_5201.ico` |
| imageres_mun | RT_GROUP_ICON | 5203 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5203.ico` |
| imageres_mun | RT_GROUP_ICON | 5205 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5205.ico` |
| imageres_mun | RT_GROUP_ICON | 5206 | 256x256+48x48+32x32+24x24+16x16 | 32,32,32,32,32 | `icons/groupicon_5206.ico` |
| imageres_mun | RT_GROUP_ICON | 5210 | 54x54+42x42+36x36+30x30+28x28+20x20 | 32,32,32,32,32,32 | `icons/groupicon_5210.ico` |
| imageres_mun | RT_GROUP_ICON | 53 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_53.ico` |
| imageres_mun | RT_GROUP_ICON | 5301 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5301.ico` |
| imageres_mun | RT_GROUP_ICON | 5302 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5302.ico` |
| imageres_mun | RT_GROUP_ICON | 5303 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5303.ico` |
| imageres_mun | RT_GROUP_ICON | 5304 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5304.ico` |
| imageres_mun | RT_GROUP_ICON | 5305 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5305.ico` |
| imageres_mun | RT_GROUP_ICON | 5306 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5306.ico` |
| imageres_mun | RT_GROUP_ICON | 5307 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5307.ico` |
| imageres_mun | RT_GROUP_ICON | 5308 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5308.ico` |
| imageres_mun | RT_GROUP_ICON | 5309 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5309.ico` |
| imageres_mun | RT_GROUP_ICON | 5310 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5310.ico` |
| imageres_mun | RT_GROUP_ICON | 5311 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5311.ico` |
| imageres_mun | RT_GROUP_ICON | 5312 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5312.ico` |
| imageres_mun | RT_GROUP_ICON | 5313 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5313.ico` |
| imageres_mun | RT_GROUP_ICON | 5314 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5314.ico` |
| imageres_mun | RT_GROUP_ICON | 5315 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5315.ico` |
| imageres_mun | RT_GROUP_ICON | 5316 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5316.ico` |
| imageres_mun | RT_GROUP_ICON | 5318 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5318.ico` |
| imageres_mun | RT_GROUP_ICON | 5319 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5319.ico` |
| imageres_mun | RT_GROUP_ICON | 5320 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5320.ico` |
| imageres_mun | RT_GROUP_ICON | 5321 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5321.ico` |
| imageres_mun | RT_GROUP_ICON | 5322 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5322.ico` |
| imageres_mun | RT_GROUP_ICON | 5323 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5323.ico` |
| imageres_mun | RT_GROUP_ICON | 5324 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5324.ico` |
| imageres_mun | RT_GROUP_ICON | 5325 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5325.ico` |
| imageres_mun | RT_GROUP_ICON | 5326 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5326.ico` |
| imageres_mun | RT_GROUP_ICON | 5327 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5327.ico` |
| imageres_mun | RT_GROUP_ICON | 5328 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5328.ico` |
| imageres_mun | RT_GROUP_ICON | 5329 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5329.ico` |
| imageres_mun | RT_GROUP_ICON | 5330 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5330.ico` |
| imageres_mun | RT_GROUP_ICON | 5331 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5331.ico` |
| imageres_mun | RT_GROUP_ICON | 5332 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5332.ico` |
| imageres_mun | RT_GROUP_ICON | 5333 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5333.ico` |
| imageres_mun | RT_GROUP_ICON | 5334 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5334.ico` |
| imageres_mun | RT_GROUP_ICON | 5335 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5335.ico` |
| imageres_mun | RT_GROUP_ICON | 5336 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5336.ico` |
| imageres_mun | RT_GROUP_ICON | 5337 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5337.ico` |
| imageres_mun | RT_GROUP_ICON | 5338 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5338.ico` |
| imageres_mun | RT_GROUP_ICON | 5339 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5339.ico` |
| imageres_mun | RT_GROUP_ICON | 5340 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5340.ico` |
| imageres_mun | RT_GROUP_ICON | 5341 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5341.ico` |
| imageres_mun | RT_GROUP_ICON | 5342 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5342.ico` |
| imageres_mun | RT_GROUP_ICON | 5343 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5343.ico` |
| imageres_mun | RT_GROUP_ICON | 5344 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5344.ico` |
| imageres_mun | RT_GROUP_ICON | 5345 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5345.ico` |
| imageres_mun | RT_GROUP_ICON | 5346 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5346.ico` |
| imageres_mun | RT_GROUP_ICON | 5347 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5347.ico` |
| imageres_mun | RT_GROUP_ICON | 5348 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5348.ico` |
| imageres_mun | RT_GROUP_ICON | 5349 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5349.ico` |
| imageres_mun | RT_GROUP_ICON | 5350 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5350.ico` |
| imageres_mun | RT_GROUP_ICON | 5351 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5351.ico` |
| imageres_mun | RT_GROUP_ICON | 5352 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5352.ico` |
| imageres_mun | RT_GROUP_ICON | 5353 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5353.ico` |
| imageres_mun | RT_GROUP_ICON | 5354 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5354.ico` |
| imageres_mun | RT_GROUP_ICON | 5355 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5355.ico` |
| imageres_mun | RT_GROUP_ICON | 5356 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5356.ico` |
| imageres_mun | RT_GROUP_ICON | 5357 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5357.ico` |
| imageres_mun | RT_GROUP_ICON | 5358 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5358.ico` |
| imageres_mun | RT_GROUP_ICON | 5359 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5359.ico` |
| imageres_mun | RT_GROUP_ICON | 5360 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5360.ico` |
| imageres_mun | RT_GROUP_ICON | 5361 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5361.ico` |
| imageres_mun | RT_GROUP_ICON | 5362 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5362.ico` |
| imageres_mun | RT_GROUP_ICON | 5363 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5363.ico` |
| imageres_mun | RT_GROUP_ICON | 5364 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5364.ico` |
| imageres_mun | RT_GROUP_ICON | 5365 | 16x16 | 4 | `icons/groupicon_5365.ico` |
| imageres_mun | RT_GROUP_ICON | 5366 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5366.ico` |
| imageres_mun | RT_GROUP_ICON | 5367 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5367.ico` |
| imageres_mun | RT_GROUP_ICON | 5368 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5368.ico` |
| imageres_mun | RT_GROUP_ICON | 5369 | 16x16 | 4 | `icons/groupicon_5369.ico` |
| imageres_mun | RT_GROUP_ICON | 5370 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5370.ico` |
| imageres_mun | RT_GROUP_ICON | 5371 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5371.ico` |
| imageres_mun | RT_GROUP_ICON | 5372 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5372.ico` |
| imageres_mun | RT_GROUP_ICON | 5373 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5373.ico` |
| imageres_mun | RT_GROUP_ICON | 5374 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5374.ico` |
| imageres_mun | RT_GROUP_ICON | 5375 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5375.ico` |
| imageres_mun | RT_GROUP_ICON | 5376 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5376.ico` |
| imageres_mun | RT_GROUP_ICON | 5377 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5377.ico` |
| imageres_mun | RT_GROUP_ICON | 5378 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5378.ico` |
| imageres_mun | RT_GROUP_ICON | 5379 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5379.ico` |
| imageres_mun | RT_GROUP_ICON | 5380 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5380.ico` |
| imageres_mun | RT_GROUP_ICON | 5381 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5381.ico` |
| imageres_mun | RT_GROUP_ICON | 5382 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5382.ico` |
| imageres_mun | RT_GROUP_ICON | 5383 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_5383.ico` |
| imageres_mun | RT_GROUP_ICON | 54 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_54.ico` |
| imageres_mun | RT_GROUP_ICON | 5402 | 64x64+50x50+36x36 | 32,32,32 | `icons/groupicon_5402.ico` |
| imageres_mun | RT_GROUP_ICON | 5403 | 64x64+50x50+36x36 | 32,32,32 | `icons/groupicon_5403.ico` |
| imageres_mun | RT_GROUP_ICON | 5404 | 64x64+50x50+36x36 | 32,32,32 | `icons/groupicon_5404.ico` |
| imageres_mun | RT_GROUP_ICON | 5405 | 64x64+50x50+36x36 | 32,32,32 | `icons/groupicon_5405.ico` |
| imageres_mun | RT_GROUP_ICON | 5406 | 64x64+50x50+36x36 | 32,32,32 | `icons/groupicon_5406.ico` |
| imageres_mun | RT_GROUP_ICON | 5407 | 64x64+50x50+36x36 | 32,32,32 | `icons/groupicon_5407.ico` |
| imageres_mun | RT_GROUP_ICON | 5408 | 64x64+50x50+36x36 | 32,32,32 | `icons/groupicon_5408.ico` |
| imageres_mun | RT_GROUP_ICON | 5409 | 64x64+50x50+36x36 | 32,32,32 | `icons/groupicon_5409.ico` |
| imageres_mun | RT_GROUP_ICON | 5411 | 28x28+22x22+16x16 | 32,32,32 | `icons/groupicon_5411.ico` |
| imageres_mun | RT_GROUP_ICON | 5412 | 28x28+22x22+16x16 | 32,32,32 | `icons/groupicon_5412.ico` |
| imageres_mun | RT_GROUP_ICON | 55 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_55.ico` |
| imageres_mun | RT_GROUP_ICON | 56 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_56.ico` |
| imageres_mun | RT_GROUP_ICON | 57 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_57.ico` |
| imageres_mun | RT_GROUP_ICON | 58 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_58.ico` |
| imageres_mun | RT_GROUP_ICON | 59 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_59.ico` |
| imageres_mun | RT_GROUP_ICON | 6 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_6.ico` |
| imageres_mun | RT_GROUP_ICON | 60 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_60.ico` |
| imageres_mun | RT_GROUP_ICON | 61 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_61.ico` |
| imageres_mun | RT_GROUP_ICON | 62 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_62.ico` |
| imageres_mun | RT_GROUP_ICON | 63 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_63.ico` |
| imageres_mun | RT_GROUP_ICON | 64 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_64.ico` |
| imageres_mun | RT_GROUP_ICON | 6400 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_6400.ico` |
| imageres_mun | RT_GROUP_ICON | 65 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_65.ico` |
| imageres_mun | RT_GROUP_ICON | 66 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_66.ico` |
| imageres_mun | RT_GROUP_ICON | 67 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_67.ico` |
| imageres_mun | RT_GROUP_ICON | 68 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_68.ico` |
| imageres_mun | RT_GROUP_ICON | 69 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_69.ico` |
| imageres_mun | RT_GROUP_ICON | 70 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_70.ico` |
| imageres_mun | RT_GROUP_ICON | 71 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_71.ico` |
| imageres_mun | RT_GROUP_ICON | 72 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_72.ico` |
| imageres_mun | RT_GROUP_ICON | 73 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_73.ico` |
| imageres_mun | RT_GROUP_ICON | 74 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_74.ico` |
| imageres_mun | RT_GROUP_ICON | 75 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_75.ico` |
| imageres_mun | RT_GROUP_ICON | 76 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_76.ico` |
| imageres_mun | RT_GROUP_ICON | 77 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_77.ico` |
| imageres_mun | RT_GROUP_ICON | 78 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_78.ico` |
| imageres_mun | RT_GROUP_ICON | 79 | 256x256+64x64+60x60+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32,32 | `icons/groupicon_79.ico` |
| imageres_mun | RT_GROUP_ICON | 8 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_8.ico` |
| imageres_mun | RT_GROUP_ICON | 80 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_80.ico` |
| imageres_mun | RT_GROUP_ICON | 81 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_81.ico` |
| imageres_mun | RT_GROUP_ICON | 82 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_82.ico` |
| imageres_mun | RT_GROUP_ICON | 8201 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_8201.ico` |
| imageres_mun | RT_GROUP_ICON | 8203 | 48x48+36x36+24x24+18x18+16x16+12x12 | 32,32,32,32,32,32 | `icons/groupicon_8203.ico` |
| imageres_mun | RT_GROUP_ICON | 8205 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_8205.ico` |
| imageres_mun | RT_GROUP_ICON | 83 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_83.ico` |
| imageres_mun | RT_GROUP_ICON | 8300 | 256x256+64x64+60x60+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32,32 | `icons/groupicon_8300.ico` |
| imageres_mun | RT_GROUP_ICON | 8301 | 256x256+64x64+60x60+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32,32 | `icons/groupicon_8301.ico` |
| imageres_mun | RT_GROUP_ICON | 8302 | 256x256+64x64+60x60+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32,32 | `icons/groupicon_8302.ico` |
| imageres_mun | RT_GROUP_ICON | 8303 | 256x256+64x64+60x60+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32,32 | `icons/groupicon_8303.ico` |
| imageres_mun | RT_GROUP_ICON | 8304 | 256x256+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32 | `icons/groupicon_8304.ico` |
| imageres_mun | RT_GROUP_ICON | 8305 | 256x256+96x96+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_8305.ico` |
| imageres_mun | RT_GROUP_ICON | 8306 | 256x256+96x96+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_8306.ico` |
| imageres_mun | RT_GROUP_ICON | 8307 | 256x256+64x64+60x60+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32,32 | `icons/groupicon_8307.ico` |
| imageres_mun | RT_GROUP_ICON | 8308 | 256x256+64x64+60x60+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32,32 | `icons/groupicon_8308.ico` |
| imageres_mun | RT_GROUP_ICON | 8309 | 256x256+64x64+60x60+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32,32 | `icons/groupicon_8309.ico` |
| imageres_mun | RT_GROUP_ICON | 8310 | 256x256+96x96+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_8310.ico` |
| imageres_mun | RT_GROUP_ICON | 8311 | 256x256+64x64+60x60+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32,32 | `icons/groupicon_8311.ico` |
| imageres_mun | RT_GROUP_ICON | 8312 | 256x256+64x64+60x60+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32,32 | `icons/groupicon_8312.ico` |
| imageres_mun | RT_GROUP_ICON | 8313 | 256x256+96x96+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_8313.ico` |
| imageres_mun | RT_GROUP_ICON | 8314 | 256x256+96x96+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_8314.ico` |
| imageres_mun | RT_GROUP_ICON | 8315 | 256x256+64x64+60x60+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32,32 | `icons/groupicon_8315.ico` |
| imageres_mun | RT_GROUP_ICON | 8316 | 256x256+96x96+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_8316.ico` |
| imageres_mun | RT_GROUP_ICON | 8317 | 256x256+96x96+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_8317.ico` |
| imageres_mun | RT_GROUP_ICON | 8318 | 256x256+96x96+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_8318.ico` |
| imageres_mun | RT_GROUP_ICON | 8319 | 256x256+96x96+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_8319.ico` |
| imageres_mun | RT_GROUP_ICON | 8320 | 256x256+64x64+60x60+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32,32 | `icons/groupicon_8320.ico` |
| imageres_mun | RT_GROUP_ICON | 8321 | 256x256+64x64+60x60+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32,32 | `icons/groupicon_8321.ico` |
| imageres_mun | RT_GROUP_ICON | 8322 | 256x256+96x96+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32 | `icons/groupicon_8322.ico` |
| imageres_mun | RT_GROUP_ICON | 8336 | 256x256+64x64+60x60+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32,32 | `icons/groupicon_8336.ico` |
| imageres_mun | RT_GROUP_ICON | 8337 | 256x256+96x96+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_8337.ico` |
| imageres_mun | RT_GROUP_ICON | 8338 | 256x256+64x64+60x60+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32,32 | `icons/groupicon_8338.ico` |
| imageres_mun | RT_GROUP_ICON | 8339 | 256x256+96x96+48x48+40x40+36x36+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_8339.ico` |
| imageres_mun | RT_GROUP_ICON | 8340 | 256x256+64x64+60x60+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32,32 | `icons/groupicon_8340.ico` |
| imageres_mun | RT_GROUP_ICON | 8341 | 256x256+64x64+60x60+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32,32 | `icons/groupicon_8341.ico` |
| imageres_mun | RT_GROUP_ICON | 8342 | 256x256+64x64+60x60+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32,32 | `icons/groupicon_8342.ico` |
| imageres_mun | RT_GROUP_ICON | 8343 | 256x256+64x64+60x60+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32,32 | `icons/groupicon_8343.ico` |
| imageres_mun | RT_GROUP_ICON | 84 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_84.ico` |
| imageres_mun | RT_GROUP_ICON | 85 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_85.ico` |
| imageres_mun | RT_GROUP_ICON | 86 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_86.ico` |
| imageres_mun | RT_GROUP_ICON | 87 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_87.ico` |
| imageres_mun | RT_GROUP_ICON | 88 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_88.ico` |
| imageres_mun | RT_GROUP_ICON | 89 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_89.ico` |
| imageres_mun | RT_GROUP_ICON | 9 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_9.ico` |
| imageres_mun | RT_GROUP_ICON | 90 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_90.ico` |
| imageres_mun | RT_GROUP_ICON | 91 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_91.ico` |
| imageres_mun | RT_GROUP_ICON | 92 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_92.ico` |
| imageres_mun | RT_GROUP_ICON | 93 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_93.ico` |
| imageres_mun | RT_GROUP_ICON | 94 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_94.ico` |
| imageres_mun | RT_GROUP_ICON | 95 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_95.ico` |
| imageres_mun | RT_GROUP_ICON | 96 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_96.ico` |
| imageres_mun | RT_GROUP_ICON | 97 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_97.ico` |
| imageres_mun | RT_GROUP_ICON | 98 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_98.ico` |
| imageres_mun | RT_GROUP_ICON | 99 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_99.ico` |
| imageres_mun | RT_ICON | 1 (group 2) | 256x256 | 32 | `icons/icon_2_0_256x256.png` |
| imageres_mun | RT_ICON | 10 (group 3) | 64x64 | 32 | `icons/icon_3_1_64x64.png` |
| imageres_mun | RT_ICON | 100 (group 20) | 40x40 | 32 | `icons/icon_20_3_40x40.png` |
| imageres_mun | RT_ICON | 1000 (group 6400) | 40x40 | 32 | `icons/icon_6400_3_40x40.png` |
| imageres_mun | RT_ICON | 1001 (group 6400) | 32x32 | 32 | `icons/icon_6400_4_32x32.png` |
| imageres_mun | RT_ICON | 1002 (group 6400) | 24x24 | 32 | `icons/icon_6400_5_24x24.png` |
| imageres_mun | RT_ICON | 1003 (group 6400) | 20x20 | 32 | `icons/icon_6400_6_20x20.png` |
| imageres_mun | RT_ICON | 1004 (group 6400) | 16x16 | 32 | `icons/icon_6400_7_16x16.png` |
| imageres_mun | RT_ICON | 1005 (group 122) | 256x256 | 32 | `icons/icon_122_0_256x256.png` |
| imageres_mun | RT_ICON | 1006 (group 122) | 64x64 | 32 | `icons/icon_122_1_64x64.png` |
| imageres_mun | RT_ICON | 1007 (group 122) | 48x48 | 32 | `icons/icon_122_2_48x48.png` |
| imageres_mun | RT_ICON | 1008 (group 122) | 40x40 | 32 | `icons/icon_122_3_40x40.png` |
| imageres_mun | RT_ICON | 1009 (group 122) | 32x32 | 32 | `icons/icon_122_4_32x32.png` |
| imageres_mun | RT_ICON | 101 (group 20) | 32x32 | 32 | `icons/icon_20_4_32x32.png` |
| imageres_mun | RT_ICON | 1010 (group 122) | 24x24 | 32 | `icons/icon_122_5_24x24.png` |
| imageres_mun | RT_ICON | 1011 (group 122) | 20x20 | 32 | `icons/icon_122_6_20x20.png` |
| imageres_mun | RT_ICON | 1012 (group 122) | 16x16 | 32 | `icons/icon_122_7_16x16.png` |
| imageres_mun | RT_ICON | 1013 (group 123) | 256x256 | 32 | `icons/icon_123_0_256x256.png` |
| imageres_mun | RT_ICON | 1014 (group 123) | 64x64 | 32 | `icons/icon_123_1_64x64.png` |
| imageres_mun | RT_ICON | 1015 (group 123) | 48x48 | 32 | `icons/icon_123_2_48x48.png` |
| imageres_mun | RT_ICON | 1016 (group 123) | 40x40 | 32 | `icons/icon_123_3_40x40.png` |
| imageres_mun | RT_ICON | 1017 (group 123) | 32x32 | 32 | `icons/icon_123_4_32x32.png` |
| imageres_mun | RT_ICON | 1018 (group 123) | 24x24 | 32 | `icons/icon_123_5_24x24.png` |
| imageres_mun | RT_ICON | 1019 (group 123) | 20x20 | 32 | `icons/icon_123_6_20x20.png` |
| imageres_mun | RT_ICON | 102 (group 20) | 24x24 | 32 | `icons/icon_20_5_24x24.png` |
| imageres_mun | RT_ICON | 1020 (group 123) | 16x16 | 32 | `icons/icon_123_7_16x16.png` |
| imageres_mun | RT_ICON | 1021 (group 124) | 256x256 | 32 | `icons/icon_124_0_256x256.png` |
| imageres_mun | RT_ICON | 1022 (group 124) | 64x64 | 32 | `icons/icon_124_1_64x64.png` |
| imageres_mun | RT_ICON | 1023 (group 124) | 48x48 | 32 | `icons/icon_124_2_48x48.png` |
| imageres_mun | RT_ICON | 1024 (group 124) | 40x40 | 32 | `icons/icon_124_3_40x40.png` |
| imageres_mun | RT_ICON | 1025 (group 124) | 32x32 | 32 | `icons/icon_124_4_32x32.png` |
| imageres_mun | RT_ICON | 1026 (group 124) | 24x24 | 32 | `icons/icon_124_5_24x24.png` |
| imageres_mun | RT_ICON | 1027 (group 124) | 20x20 | 32 | `icons/icon_124_6_20x20.png` |
| imageres_mun | RT_ICON | 1028 (group 124) | 16x16 | 32 | `icons/icon_124_7_16x16.png` |
| imageres_mun | RT_ICON | 1029 (group 125) | 256x256 | 32 | `icons/icon_125_0_256x256.png` |
| imageres_mun | RT_ICON | 103 (group 20) | 20x20 | 32 | `icons/icon_20_6_20x20.png` |
| imageres_mun | RT_ICON | 1030 (group 125) | 64x64 | 32 | `icons/icon_125_1_64x64.png` |
| imageres_mun | RT_ICON | 1031 (group 125) | 48x48 | 32 | `icons/icon_125_2_48x48.png` |
| imageres_mun | RT_ICON | 1032 (group 125) | 40x40 | 32 | `icons/icon_125_3_40x40.png` |
| imageres_mun | RT_ICON | 1033 (group 125) | 32x32 | 32 | `icons/icon_125_4_32x32.png` |
| imageres_mun | RT_ICON | 1034 (group 125) | 24x24 | 32 | `icons/icon_125_5_24x24.png` |
| imageres_mun | RT_ICON | 1035 (group 125) | 20x20 | 32 | `icons/icon_125_6_20x20.png` |
| imageres_mun | RT_ICON | 1036 (group 125) | 16x16 | 32 | `icons/icon_125_7_16x16.png` |
| imageres_mun | RT_ICON | 1037 (group 126) | 256x256 | 32 | `icons/icon_126_0_256x256.png` |
| imageres_mun | RT_ICON | 1038 (group 126) | 64x64 | 32 | `icons/icon_126_1_64x64.png` |
| imageres_mun | RT_ICON | 1039 (group 126) | 48x48 | 32 | `icons/icon_126_2_48x48.png` |
| imageres_mun | RT_ICON | 104 (group 20) | 16x16 | 32 | `icons/icon_20_7_16x16.png` |
| imageres_mun | RT_ICON | 1040 (group 126) | 40x40 | 32 | `icons/icon_126_3_40x40.png` |
| imageres_mun | RT_ICON | 1041 (group 126) | 32x32 | 32 | `icons/icon_126_4_32x32.png` |
| imageres_mun | RT_ICON | 1042 (group 126) | 24x24 | 32 | `icons/icon_126_5_24x24.png` |
| imageres_mun | RT_ICON | 1043 (group 126) | 20x20 | 32 | `icons/icon_126_6_20x20.png` |
| imageres_mun | RT_ICON | 1044 (group 126) | 16x16 | 32 | `icons/icon_126_7_16x16.png` |
| imageres_mun | RT_ICON | 1045 (group 127) | 256x256 | 32 | `icons/icon_127_0_256x256.png` |
| imageres_mun | RT_ICON | 1046 (group 127) | 64x64 | 32 | `icons/icon_127_1_64x64.png` |
| imageres_mun | RT_ICON | 1047 (group 127) | 48x48 | 32 | `icons/icon_127_2_48x48.png` |
| imageres_mun | RT_ICON | 1048 (group 127) | 40x40 | 32 | `icons/icon_127_3_40x40.png` |
| imageres_mun | RT_ICON | 1049 (group 127) | 32x32 | 32 | `icons/icon_127_4_32x32.png` |
| imageres_mun | RT_ICON | 105 (group 21) | 256x256 | 32 | `icons/icon_21_0_256x256.png` |
| imageres_mun | RT_ICON | 1050 (group 127) | 24x24 | 32 | `icons/icon_127_5_24x24.png` |
| imageres_mun | RT_ICON | 1051 (group 127) | 20x20 | 32 | `icons/icon_127_6_20x20.png` |
| imageres_mun | RT_ICON | 1052 (group 127) | 16x16 | 32 | `icons/icon_127_7_16x16.png` |
| imageres_mun | RT_ICON | 1053 (group 128) | 256x256 | 32 | `icons/icon_128_0_256x256.png` |
| imageres_mun | RT_ICON | 1054 (group 128) | 64x64 | 32 | `icons/icon_128_1_64x64.png` |
| imageres_mun | RT_ICON | 1055 (group 128) | 48x48 | 32 | `icons/icon_128_2_48x48.png` |
| imageres_mun | RT_ICON | 1056 (group 128) | 40x40 | 32 | `icons/icon_128_3_40x40.png` |
| imageres_mun | RT_ICON | 1057 (group 128) | 32x32 | 32 | `icons/icon_128_4_32x32.png` |
| imageres_mun | RT_ICON | 1058 (group 128) | 24x24 | 32 | `icons/icon_128_5_24x24.png` |
| imageres_mun | RT_ICON | 1059 (group 128) | 20x20 | 32 | `icons/icon_128_6_20x20.png` |
| imageres_mun | RT_ICON | 106 (group 21) | 64x64 | 32 | `icons/icon_21_1_64x64.png` |
| imageres_mun | RT_ICON | 1060 (group 128) | 16x16 | 32 | `icons/icon_128_7_16x16.png` |
| imageres_mun | RT_ICON | 1061 (group 129) | 256x256 | 32 | `icons/icon_129_0_256x256.png` |
| imageres_mun | RT_ICON | 1062 (group 129) | 64x64 | 32 | `icons/icon_129_1_64x64.png` |
| imageres_mun | RT_ICON | 1063 (group 129) | 48x48 | 32 | `icons/icon_129_2_48x48.png` |
| imageres_mun | RT_ICON | 1064 (group 129) | 40x40 | 32 | `icons/icon_129_3_40x40.png` |
| imageres_mun | RT_ICON | 1065 (group 129) | 32x32 | 32 | `icons/icon_129_4_32x32.png` |
| imageres_mun | RT_ICON | 1066 (group 129) | 24x24 | 32 | `icons/icon_129_5_24x24.png` |
| imageres_mun | RT_ICON | 1067 (group 129) | 20x20 | 32 | `icons/icon_129_6_20x20.png` |
| imageres_mun | RT_ICON | 1068 (group 129) | 16x16 | 32 | `icons/icon_129_7_16x16.png` |
| imageres_mun | RT_ICON | 1069 (group 130) | 256x256 | 32 | `icons/icon_130_0_256x256.png` |
| imageres_mun | RT_ICON | 107 (group 21) | 48x48 | 32 | `icons/icon_21_2_48x48.png` |
| imageres_mun | RT_ICON | 1070 (group 130) | 64x64 | 32 | `icons/icon_130_1_64x64.png` |
| imageres_mun | RT_ICON | 1071 (group 130) | 48x48 | 32 | `icons/icon_130_2_48x48.png` |
| imageres_mun | RT_ICON | 1072 (group 130) | 40x40 | 32 | `icons/icon_130_3_40x40.png` |
| imageres_mun | RT_ICON | 1073 (group 130) | 32x32 | 32 | `icons/icon_130_4_32x32.png` |
| imageres_mun | RT_ICON | 1074 (group 130) | 24x24 | 32 | `icons/icon_130_5_24x24.png` |
| imageres_mun | RT_ICON | 1075 (group 130) | 20x20 | 32 | `icons/icon_130_6_20x20.png` |
| imageres_mun | RT_ICON | 1076 (group 130) | 16x16 | 32 | `icons/icon_130_7_16x16.png` |
| imageres_mun | RT_ICON | 1077 (group 131) | 256x256 | 32 | `icons/icon_131_0_256x256.png` |
| imageres_mun | RT_ICON | 1078 (group 131) | 64x64 | 32 | `icons/icon_131_1_64x64.png` |
| imageres_mun | RT_ICON | 1079 (group 131) | 48x48 | 32 | `icons/icon_131_2_48x48.png` |
| imageres_mun | RT_ICON | 108 (group 21) | 40x40 | 32 | `icons/icon_21_3_40x40.png` |
| imageres_mun | RT_ICON | 1080 (group 131) | 40x40 | 32 | `icons/icon_131_3_40x40.png` |
| imageres_mun | RT_ICON | 1081 (group 131) | 32x32 | 32 | `icons/icon_131_4_32x32.png` |
| imageres_mun | RT_ICON | 1082 (group 131) | 24x24 | 32 | `icons/icon_131_5_24x24.png` |
| imageres_mun | RT_ICON | 1083 (group 131) | 20x20 | 32 | `icons/icon_131_6_20x20.png` |
| imageres_mun | RT_ICON | 1084 (group 131) | 16x16 | 32 | `icons/icon_131_7_16x16.png` |
| imageres_mun | RT_ICON | 1085 (group 132) | 256x256 | 32 | `icons/icon_132_0_256x256.png` |
| imageres_mun | RT_ICON | 1086 (group 132) | 64x64 | 32 | `icons/icon_132_1_64x64.png` |
| imageres_mun | RT_ICON | 1087 (group 132) | 48x48 | 32 | `icons/icon_132_2_48x48.png` |
| imageres_mun | RT_ICON | 1088 (group 132) | 40x40 | 32 | `icons/icon_132_3_40x40.png` |
| imageres_mun | RT_ICON | 1089 (group 132) | 32x32 | 32 | `icons/icon_132_4_32x32.png` |
| imageres_mun | RT_ICON | 109 (group 21) | 32x32 | 32 | `icons/icon_21_4_32x32.png` |
| imageres_mun | RT_ICON | 1090 (group 132) | 24x24 | 32 | `icons/icon_132_5_24x24.png` |
| imageres_mun | RT_ICON | 1091 (group 132) | 20x20 | 32 | `icons/icon_132_6_20x20.png` |
| imageres_mun | RT_ICON | 1092 (group 132) | 16x16 | 32 | `icons/icon_132_7_16x16.png` |
| imageres_mun | RT_ICON | 1093 (group 133) | 256x256 | 32 | `icons/icon_133_0_256x256.png` |
| imageres_mun | RT_ICON | 1094 (group 133) | 64x64 | 32 | `icons/icon_133_1_64x64.png` |
| imageres_mun | RT_ICON | 1095 (group 133) | 48x48 | 32 | `icons/icon_133_2_48x48.png` |
| imageres_mun | RT_ICON | 1096 (group 133) | 40x40 | 32 | `icons/icon_133_3_40x40.png` |
| imageres_mun | RT_ICON | 1097 (group 133) | 32x32 | 32 | `icons/icon_133_4_32x32.png` |
| imageres_mun | RT_ICON | 1098 (group 133) | 24x24 | 32 | `icons/icon_133_5_24x24.png` |
| imageres_mun | RT_ICON | 1099 (group 133) | 20x20 | 32 | `icons/icon_133_6_20x20.png` |
| imageres_mun | RT_ICON | 11 (group 3) | 48x48 | 32 | `icons/icon_3_2_48x48.png` |
| imageres_mun | RT_ICON | 110 (group 21) | 24x24 | 32 | `icons/icon_21_5_24x24.png` |
| imageres_mun | RT_ICON | 1100 (group 133) | 16x16 | 32 | `icons/icon_133_7_16x16.png` |
| imageres_mun | RT_ICON | 1101 (group 134) | 256x256 | 32 | `icons/icon_134_0_256x256.png` |
| imageres_mun | RT_ICON | 1102 (group 134) | 64x64 | 32 | `icons/icon_134_1_64x64.png` |
| imageres_mun | RT_ICON | 1103 (group 134) | 48x48 | 32 | `icons/icon_134_2_48x48.png` |
| imageres_mun | RT_ICON | 1104 (group 134) | 40x40 | 32 | `icons/icon_134_3_40x40.png` |
| imageres_mun | RT_ICON | 1105 (group 134) | 32x32 | 32 | `icons/icon_134_4_32x32.png` |
| imageres_mun | RT_ICON | 1106 (group 134) | 24x24 | 32 | `icons/icon_134_5_24x24.png` |
| imageres_mun | RT_ICON | 1107 (group 134) | 20x20 | 32 | `icons/icon_134_6_20x20.png` |
| imageres_mun | RT_ICON | 1108 (group 134) | 16x16 | 32 | `icons/icon_134_7_16x16.png` |
| imageres_mun | RT_ICON | 1109 (group 135) | 256x256 | 32 | `icons/icon_135_0_256x256.png` |
| imageres_mun | RT_ICON | 111 (group 21) | 20x20 | 32 | `icons/icon_21_6_20x20.png` |
| imageres_mun | RT_ICON | 1110 (group 135) | 64x64 | 32 | `icons/icon_135_1_64x64.png` |
| imageres_mun | RT_ICON | 1111 (group 135) | 48x48 | 32 | `icons/icon_135_2_48x48.png` |
| imageres_mun | RT_ICON | 1112 (group 135) | 40x40 | 32 | `icons/icon_135_3_40x40.png` |
| imageres_mun | RT_ICON | 1113 (group 135) | 32x32 | 32 | `icons/icon_135_4_32x32.png` |
| imageres_mun | RT_ICON | 1114 (group 135) | 24x24 | 32 | `icons/icon_135_5_24x24.png` |
| imageres_mun | RT_ICON | 1115 (group 135) | 20x20 | 32 | `icons/icon_135_6_20x20.png` |
| imageres_mun | RT_ICON | 1116 (group 135) | 16x16 | 32 | `icons/icon_135_7_16x16.png` |
| imageres_mun | RT_ICON | 1117 (group 136) | 256x256 | 32 | `icons/icon_136_0_256x256.png` |
| imageres_mun | RT_ICON | 1118 (group 136) | 64x64 | 32 | `icons/icon_136_1_64x64.png` |
| imageres_mun | RT_ICON | 1119 (group 136) | 48x48 | 32 | `icons/icon_136_2_48x48.png` |
| imageres_mun | RT_ICON | 112 (group 21) | 16x16 | 32 | `icons/icon_21_7_16x16.png` |
| imageres_mun | RT_ICON | 1120 (group 136) | 40x40 | 32 | `icons/icon_136_3_40x40.png` |
| imageres_mun | RT_ICON | 1121 (group 136) | 32x32 | 32 | `icons/icon_136_4_32x32.png` |
| imageres_mun | RT_ICON | 1122 (group 136) | 24x24 | 32 | `icons/icon_136_5_24x24.png` |
| imageres_mun | RT_ICON | 1123 (group 136) | 20x20 | 32 | `icons/icon_136_6_20x20.png` |
| imageres_mun | RT_ICON | 1124 (group 136) | 16x16 | 32 | `icons/icon_136_7_16x16.png` |
| imageres_mun | RT_ICON | 1125 (group 137) | 256x256 | 32 | `icons/icon_137_0_256x256.png` |
| imageres_mun | RT_ICON | 1126 (group 137) | 64x64 | 32 | `icons/icon_137_1_64x64.png` |
| imageres_mun | RT_ICON | 1127 (group 137) | 48x48 | 32 | `icons/icon_137_2_48x48.png` |
| imageres_mun | RT_ICON | 1128 (group 137) | 40x40 | 32 | `icons/icon_137_3_40x40.png` |
| imageres_mun | RT_ICON | 1129 (group 137) | 32x32 | 32 | `icons/icon_137_4_32x32.png` |
| imageres_mun | RT_ICON | 113 (group 22) | 256x256 | 32 | `icons/icon_22_0_256x256.png` |
| imageres_mun | RT_ICON | 1130 (group 137) | 24x24 | 32 | `icons/icon_137_5_24x24.png` |
| imageres_mun | RT_ICON | 1131 (group 137) | 20x20 | 32 | `icons/icon_137_6_20x20.png` |
| imageres_mun | RT_ICON | 1132 (group 137) | 16x16 | 32 | `icons/icon_137_7_16x16.png` |
| imageres_mun | RT_ICON | 1133 (group 138) | 256x256 | 32 | `icons/icon_138_0_256x256.png` |
| imageres_mun | RT_ICON | 1134 (group 138) | 64x64 | 32 | `icons/icon_138_1_64x64.png` |
| imageres_mun | RT_ICON | 1135 (group 138) | 48x48 | 32 | `icons/icon_138_2_48x48.png` |
| imageres_mun | RT_ICON | 1136 (group 138) | 40x40 | 32 | `icons/icon_138_3_40x40.png` |
| imageres_mun | RT_ICON | 1137 (group 138) | 32x32 | 32 | `icons/icon_138_4_32x32.png` |
| imageres_mun | RT_ICON | 1138 (group 138) | 24x24 | 32 | `icons/icon_138_5_24x24.png` |
| imageres_mun | RT_ICON | 1139 (group 138) | 20x20 | 32 | `icons/icon_138_6_20x20.png` |
| imageres_mun | RT_ICON | 114 (group 22) | 64x64 | 32 | `icons/icon_22_1_64x64.png` |
| imageres_mun | RT_ICON | 1140 (group 138) | 16x16 | 32 | `icons/icon_138_7_16x16.png` |
| imageres_mun | RT_ICON | 1141 (group 139) | 256x256 | 32 | `icons/icon_139_0_256x256.png` |
| imageres_mun | RT_ICON | 1142 (group 139) | 64x64 | 32 | `icons/icon_139_1_64x64.png` |
| imageres_mun | RT_ICON | 1143 (group 139) | 48x48 | 32 | `icons/icon_139_2_48x48.png` |
| imageres_mun | RT_ICON | 1144 (group 139) | 40x40 | 32 | `icons/icon_139_3_40x40.png` |
| imageres_mun | RT_ICON | 1145 (group 139) | 32x32 | 32 | `icons/icon_139_4_32x32.png` |
| imageres_mun | RT_ICON | 1146 (group 139) | 24x24 | 32 | `icons/icon_139_5_24x24.png` |
| imageres_mun | RT_ICON | 1147 (group 139) | 20x20 | 32 | `icons/icon_139_6_20x20.png` |
| imageres_mun | RT_ICON | 1148 (group 139) | 16x16 | 32 | `icons/icon_139_7_16x16.png` |
| imageres_mun | RT_ICON | 1149 (group 140) | 256x256 | 32 | `icons/icon_140_0_256x256.png` |
| imageres_mun | RT_ICON | 115 (group 22) | 48x48 | 32 | `icons/icon_22_2_48x48.png` |
| imageres_mun | RT_ICON | 1150 (group 140) | 64x64 | 32 | `icons/icon_140_1_64x64.png` |
| imageres_mun | RT_ICON | 1151 (group 140) | 48x48 | 32 | `icons/icon_140_2_48x48.png` |
| imageres_mun | RT_ICON | 1152 (group 140) | 40x40 | 32 | `icons/icon_140_3_40x40.png` |
| imageres_mun | RT_ICON | 1153 (group 140) | 32x32 | 32 | `icons/icon_140_4_32x32.png` |
| imageres_mun | RT_ICON | 1154 (group 140) | 24x24 | 32 | `icons/icon_140_5_24x24.png` |
| imageres_mun | RT_ICON | 1155 (group 140) | 20x20 | 32 | `icons/icon_140_6_20x20.png` |
| imageres_mun | RT_ICON | 1156 (group 140) | 16x16 | 32 | `icons/icon_140_7_16x16.png` |
| imageres_mun | RT_ICON | 1157 (group 141) | 256x256 | 32 | `icons/icon_141_0_256x256.png` |
| imageres_mun | RT_ICON | 1158 (group 141) | 64x64 | 32 | `icons/icon_141_1_64x64.png` |
| imageres_mun | RT_ICON | 1159 (group 141) | 48x48 | 32 | `icons/icon_141_2_48x48.png` |
| imageres_mun | RT_ICON | 116 (group 22) | 40x40 | 32 | `icons/icon_22_3_40x40.png` |
| imageres_mun | RT_ICON | 1160 (group 141) | 40x40 | 32 | `icons/icon_141_3_40x40.png` |
| imageres_mun | RT_ICON | 1161 (group 141) | 32x32 | 32 | `icons/icon_141_4_32x32.png` |
| imageres_mun | RT_ICON | 1162 (group 141) | 24x24 | 32 | `icons/icon_141_5_24x24.png` |
| imageres_mun | RT_ICON | 1163 (group 141) | 20x20 | 32 | `icons/icon_141_6_20x20.png` |
| imageres_mun | RT_ICON | 1164 (group 141) | 16x16 | 32 | `icons/icon_141_7_16x16.png` |
| imageres_mun | RT_ICON | 1165 (group 142) | 256x256 | 32 | `icons/icon_142_0_256x256.png` |
| imageres_mun | RT_ICON | 1166 (group 142) | 64x64 | 32 | `icons/icon_142_1_64x64.png` |
| imageres_mun | RT_ICON | 1167 (group 142) | 48x48 | 32 | `icons/icon_142_2_48x48.png` |
| imageres_mun | RT_ICON | 1168 (group 142) | 40x40 | 32 | `icons/icon_142_3_40x40.png` |
| imageres_mun | RT_ICON | 1169 (group 142) | 32x32 | 32 | `icons/icon_142_4_32x32.png` |
| imageres_mun | RT_ICON | 117 (group 22) | 32x32 | 32 | `icons/icon_22_4_32x32.png` |
| imageres_mun | RT_ICON | 1170 (group 142) | 24x24 | 32 | `icons/icon_142_5_24x24.png` |
| imageres_mun | RT_ICON | 1171 (group 142) | 20x20 | 32 | `icons/icon_142_6_20x20.png` |
| imageres_mun | RT_ICON | 1172 (group 142) | 16x16 | 32 | `icons/icon_142_7_16x16.png` |
| imageres_mun | RT_ICON | 1173 (group 76) | 256x256 | 32 | `icons/icon_76_0_256x256.png` |
| imageres_mun | RT_ICON | 1174 (group 76) | 64x64 | 32 | `icons/icon_76_1_64x64.png` |
| imageres_mun | RT_ICON | 1175 (group 76) | 48x48 | 32 | `icons/icon_76_2_48x48.png` |
| imageres_mun | RT_ICON | 1176 (group 76) | 40x40 | 32 | `icons/icon_76_3_40x40.png` |
| imageres_mun | RT_ICON | 1177 (group 76) | 32x32 | 32 | `icons/icon_76_4_32x32.png` |
| imageres_mun | RT_ICON | 1178 (group 76) | 24x24 | 32 | `icons/icon_76_5_24x24.png` |
| imageres_mun | RT_ICON | 1179 (group 76) | 20x20 | 32 | `icons/icon_76_6_20x20.png` |
| imageres_mun | RT_ICON | 118 (group 22) | 24x24 | 32 | `icons/icon_22_5_24x24.png` |
| imageres_mun | RT_ICON | 1180 (group 76) | 16x16 | 32 | `icons/icon_76_7_16x16.png` |
| imageres_mun | RT_ICON | 1181 (group 143) | 256x256 | 32 | `icons/icon_143_0_256x256.png` |
| imageres_mun | RT_ICON | 1182 (group 143) | 64x64 | 32 | `icons/icon_143_1_64x64.png` |
| imageres_mun | RT_ICON | 1183 (group 143) | 48x48 | 32 | `icons/icon_143_2_48x48.png` |
| imageres_mun | RT_ICON | 1184 (group 143) | 40x40 | 32 | `icons/icon_143_3_40x40.png` |
| imageres_mun | RT_ICON | 1185 (group 143) | 32x32 | 32 | `icons/icon_143_4_32x32.png` |
| imageres_mun | RT_ICON | 1186 (group 143) | 24x24 | 32 | `icons/icon_143_5_24x24.png` |
| imageres_mun | RT_ICON | 1187 (group 143) | 20x20 | 32 | `icons/icon_143_6_20x20.png` |
| imageres_mun | RT_ICON | 1188 (group 143) | 16x16 | 32 | `icons/icon_143_7_16x16.png` |
| imageres_mun | RT_ICON | 1189 (group 144) | 256x256 | 32 | `icons/icon_144_0_256x256.png` |
| imageres_mun | RT_ICON | 119 (group 22) | 20x20 | 32 | `icons/icon_22_6_20x20.png` |
| imageres_mun | RT_ICON | 1190 (group 144) | 64x64 | 32 | `icons/icon_144_1_64x64.png` |
| imageres_mun | RT_ICON | 1191 (group 144) | 48x48 | 32 | `icons/icon_144_2_48x48.png` |
| imageres_mun | RT_ICON | 1192 (group 144) | 40x40 | 32 | `icons/icon_144_3_40x40.png` |
| imageres_mun | RT_ICON | 1193 (group 144) | 32x32 | 32 | `icons/icon_144_4_32x32.png` |
| imageres_mun | RT_ICON | 1194 (group 144) | 24x24 | 32 | `icons/icon_144_5_24x24.png` |
| imageres_mun | RT_ICON | 1195 (group 144) | 20x20 | 32 | `icons/icon_144_6_20x20.png` |
| imageres_mun | RT_ICON | 1196 (group 144) | 16x16 | 32 | `icons/icon_144_7_16x16.png` |
| imageres_mun | RT_ICON | 1197 (group 145) | 48x48 | 4 | `icons/icon_145_0_48x48.png` |
| imageres_mun | RT_ICON | 1198 (group 145) | 32x32 | 4 | `icons/icon_145_1_32x32.png` |
| imageres_mun | RT_ICON | 1199 (group 145) | 16x16 | 4 | `icons/icon_145_2_16x16.png` |
| imageres_mun | RT_ICON | 12 (group 3) | 40x40 | 32 | `icons/icon_3_3_40x40.png` |
| imageres_mun | RT_ICON | 120 (group 22) | 16x16 | 32 | `icons/icon_22_7_16x16.png` |
| imageres_mun | RT_ICON | 1200 (group 145) | 48x48 | 8 | `icons/icon_145_3_48x48.png` |
| imageres_mun | RT_ICON | 1201 (group 145) | 32x32 | 8 | `icons/icon_145_4_32x32.png` |
| imageres_mun | RT_ICON | 1202 (group 145) | 16x16 | 8 | `icons/icon_145_5_16x16.png` |
| imageres_mun | RT_ICON | 1203 (group 145) | 256x256 | 32 | `icons/icon_145_6_256x256.png` |
| imageres_mun | RT_ICON | 1204 (group 145) | 48x48 | 32 | `icons/icon_145_7_48x48.png` |
| imageres_mun | RT_ICON | 1205 (group 145) | 32x32 | 32 | `icons/icon_145_8_32x32.png` |
| imageres_mun | RT_ICON | 1206 (group 145) | 16x16 | 32 | `icons/icon_145_9_16x16.png` |
| imageres_mun | RT_ICON | 1207 (group 146) | 256x256 | 32 | `icons/icon_146_0_256x256.png` |
| imageres_mun | RT_ICON | 1208 (group 146) | 64x64 | 32 | `icons/icon_146_1_64x64.png` |
| imageres_mun | RT_ICON | 1209 (group 146) | 48x48 | 32 | `icons/icon_146_2_48x48.png` |
| imageres_mun | RT_ICON | 121 (group 23) | 256x256 | 32 | `icons/icon_23_0_256x256.png` |
| imageres_mun | RT_ICON | 1210 (group 146) | 40x40 | 32 | `icons/icon_146_3_40x40.png` |
| imageres_mun | RT_ICON | 1211 (group 146) | 32x32 | 32 | `icons/icon_146_4_32x32.png` |
| imageres_mun | RT_ICON | 1212 (group 146) | 24x24 | 32 | `icons/icon_146_5_24x24.png` |
| imageres_mun | RT_ICON | 1213 (group 146) | 20x20 | 32 | `icons/icon_146_6_20x20.png` |
| imageres_mun | RT_ICON | 1214 (group 146) | 16x16 | 32 | `icons/icon_146_7_16x16.png` |
| imageres_mun | RT_ICON | 1215 (group 147) | 256x256 | 32 | `icons/icon_147_0_256x256.png` |
| imageres_mun | RT_ICON | 1216 (group 147) | 64x64 | 32 | `icons/icon_147_1_64x64.png` |
| imageres_mun | RT_ICON | 1217 (group 147) | 48x48 | 32 | `icons/icon_147_2_48x48.png` |
| imageres_mun | RT_ICON | 1218 (group 147) | 40x40 | 32 | `icons/icon_147_3_40x40.png` |
| imageres_mun | RT_ICON | 1219 (group 147) | 32x32 | 32 | `icons/icon_147_4_32x32.png` |
| imageres_mun | RT_ICON | 122 (group 23) | 64x64 | 32 | `icons/icon_23_1_64x64.png` |
| imageres_mun | RT_ICON | 1220 (group 147) | 24x24 | 32 | `icons/icon_147_5_24x24.png` |
| imageres_mun | RT_ICON | 1221 (group 147) | 20x20 | 32 | `icons/icon_147_6_20x20.png` |
| imageres_mun | RT_ICON | 1222 (group 147) | 16x16 | 32 | `icons/icon_147_7_16x16.png` |
| imageres_mun | RT_ICON | 1223 (group 148) | 256x256 | 32 | `icons/icon_148_0_256x256.png` |
| imageres_mun | RT_ICON | 1224 (group 148) | 64x64 | 32 | `icons/icon_148_1_64x64.png` |
| imageres_mun | RT_ICON | 1225 (group 148) | 48x48 | 32 | `icons/icon_148_2_48x48.png` |
| imageres_mun | RT_ICON | 1226 (group 148) | 40x40 | 32 | `icons/icon_148_3_40x40.png` |
| imageres_mun | RT_ICON | 1227 (group 148) | 32x32 | 32 | `icons/icon_148_4_32x32.png` |
| imageres_mun | RT_ICON | 1228 (group 148) | 24x24 | 32 | `icons/icon_148_5_24x24.png` |
| imageres_mun | RT_ICON | 1229 (group 148) | 20x20 | 32 | `icons/icon_148_6_20x20.png` |
| imageres_mun | RT_ICON | 123 (group 23) | 48x48 | 32 | `icons/icon_23_2_48x48.png` |
| imageres_mun | RT_ICON | 1230 (group 148) | 16x16 | 32 | `icons/icon_148_7_16x16.png` |
| imageres_mun | RT_ICON | 1231 (group 149) | 256x256 | 32 | `icons/icon_149_0_256x256.png` |
| imageres_mun | RT_ICON | 1232 (group 149) | 64x64 | 32 | `icons/icon_149_1_64x64.png` |
| imageres_mun | RT_ICON | 1233 (group 149) | 48x48 | 32 | `icons/icon_149_2_48x48.png` |
| imageres_mun | RT_ICON | 1234 (group 149) | 40x40 | 32 | `icons/icon_149_3_40x40.png` |
| imageres_mun | RT_ICON | 1235 (group 149) | 32x32 | 32 | `icons/icon_149_4_32x32.png` |
| imageres_mun | RT_ICON | 1236 (group 149) | 24x24 | 32 | `icons/icon_149_5_24x24.png` |
| imageres_mun | RT_ICON | 1237 (group 149) | 20x20 | 32 | `icons/icon_149_6_20x20.png` |
| imageres_mun | RT_ICON | 1238 (group 149) | 16x16 | 32 | `icons/icon_149_7_16x16.png` |
| imageres_mun | RT_ICON | 1239 (group 150) | 256x256 | 32 | `icons/icon_150_0_256x256.png` |
| imageres_mun | RT_ICON | 124 (group 23) | 40x40 | 32 | `icons/icon_23_3_40x40.png` |
| imageres_mun | RT_ICON | 1240 (group 150) | 64x64 | 32 | `icons/icon_150_1_64x64.png` |
| imageres_mun | RT_ICON | 1241 (group 150) | 48x48 | 32 | `icons/icon_150_2_48x48.png` |
| imageres_mun | RT_ICON | 1242 (group 150) | 40x40 | 32 | `icons/icon_150_3_40x40.png` |
| imageres_mun | RT_ICON | 1243 (group 150) | 32x32 | 32 | `icons/icon_150_4_32x32.png` |
| imageres_mun | RT_ICON | 1244 (group 150) | 24x24 | 32 | `icons/icon_150_5_24x24.png` |
| imageres_mun | RT_ICON | 1245 (group 150) | 20x20 | 32 | `icons/icon_150_6_20x20.png` |
| imageres_mun | RT_ICON | 1246 (group 150) | 16x16 | 32 | `icons/icon_150_7_16x16.png` |
| imageres_mun | RT_ICON | 1247 (group 151) | 256x256 | 32 | `icons/icon_151_0_256x256.png` |
| imageres_mun | RT_ICON | 1248 (group 151) | 64x64 | 32 | `icons/icon_151_1_64x64.png` |
| imageres_mun | RT_ICON | 1249 (group 151) | 48x48 | 32 | `icons/icon_151_2_48x48.png` |
| imageres_mun | RT_ICON | 125 (group 23) | 32x32 | 32 | `icons/icon_23_4_32x32.png` |
| imageres_mun | RT_ICON | 1250 (group 151) | 40x40 | 32 | `icons/icon_151_3_40x40.png` |
| imageres_mun | RT_ICON | 1251 (group 151) | 32x32 | 32 | `icons/icon_151_4_32x32.png` |
| imageres_mun | RT_ICON | 1252 (group 151) | 24x24 | 32 | `icons/icon_151_5_24x24.png` |
| imageres_mun | RT_ICON | 1253 (group 151) | 20x20 | 32 | `icons/icon_151_6_20x20.png` |
| imageres_mun | RT_ICON | 1254 (group 151) | 16x16 | 32 | `icons/icon_151_7_16x16.png` |
| imageres_mun | RT_ICON | 1255 (group 152) | 256x256 | 32 | `icons/icon_152_0_256x256.png` |
| imageres_mun | RT_ICON | 1256 (group 152) | 64x64 | 32 | `icons/icon_152_1_64x64.png` |
| imageres_mun | RT_ICON | 1257 (group 152) | 48x48 | 32 | `icons/icon_152_2_48x48.png` |
| imageres_mun | RT_ICON | 1258 (group 152) | 40x40 | 32 | `icons/icon_152_3_40x40.png` |
| imageres_mun | RT_ICON | 1259 (group 152) | 32x32 | 32 | `icons/icon_152_4_32x32.png` |
| imageres_mun | RT_ICON | 126 (group 23) | 24x24 | 32 | `icons/icon_23_5_24x24.png` |
| imageres_mun | RT_ICON | 1260 (group 152) | 24x24 | 32 | `icons/icon_152_5_24x24.png` |
| imageres_mun | RT_ICON | 1261 (group 152) | 20x20 | 32 | `icons/icon_152_6_20x20.png` |
| imageres_mun | RT_ICON | 1262 (group 152) | 16x16 | 32 | `icons/icon_152_7_16x16.png` |
| imageres_mun | RT_ICON | 1263 (group "BLANK0") | 16x16 | 4 | `icons/icon_BLANK0_0_16x16.png` |
| imageres_mun | RT_ICON | 1264 (group 155) | 256x256 | 32 | `icons/icon_155_0_256x256.png` |
| imageres_mun | RT_ICON | 1265 (group 155) | 64x64 | 32 | `icons/icon_155_1_64x64.png` |
| imageres_mun | RT_ICON | 1266 (group 155) | 48x48 | 32 | `icons/icon_155_2_48x48.png` |
| imageres_mun | RT_ICON | 1267 (group 155) | 40x40 | 32 | `icons/icon_155_3_40x40.png` |
| imageres_mun | RT_ICON | 1268 (group 155) | 32x32 | 32 | `icons/icon_155_4_32x32.png` |
| imageres_mun | RT_ICON | 1269 (group 155) | 24x24 | 32 | `icons/icon_155_5_24x24.png` |
| imageres_mun | RT_ICON | 127 (group 23) | 20x20 | 32 | `icons/icon_23_6_20x20.png` |
| imageres_mun | RT_ICON | 1270 (group 155) | 20x20 | 32 | `icons/icon_155_6_20x20.png` |
| imageres_mun | RT_ICON | 1271 (group 155) | 16x16 | 32 | `icons/icon_155_7_16x16.png` |
| imageres_mun | RT_ICON | 1272 (group 157) | 256x256 | 32 | `icons/icon_157_0_256x256.png` |
| imageres_mun | RT_ICON | 1273 (group 157) | 64x64 | 32 | `icons/icon_157_1_64x64.png` |
| imageres_mun | RT_ICON | 1274 (group 157) | 48x48 | 32 | `icons/icon_157_2_48x48.png` |
| imageres_mun | RT_ICON | 1275 (group 157) | 40x40 | 32 | `icons/icon_157_3_40x40.png` |
| imageres_mun | RT_ICON | 1276 (group 157) | 32x32 | 32 | `icons/icon_157_4_32x32.png` |
| imageres_mun | RT_ICON | 1277 (group 157) | 24x24 | 32 | `icons/icon_157_5_24x24.png` |
| imageres_mun | RT_ICON | 1278 (group 157) | 20x20 | 32 | `icons/icon_157_6_20x20.png` |
| imageres_mun | RT_ICON | 1279 (group 157) | 16x16 | 32 | `icons/icon_157_7_16x16.png` |
| imageres_mun | RT_ICON | 128 (group 23) | 16x16 | 32 | `icons/icon_23_7_16x16.png` |
| imageres_mun | RT_ICON | 1280 (group 158) | 256x256 | 32 | `icons/icon_158_0_256x256.png` |
| imageres_mun | RT_ICON | 1281 (group 158) | 64x64 | 32 | `icons/icon_158_1_64x64.png` |
| imageres_mun | RT_ICON | 1282 (group 158) | 48x48 | 32 | `icons/icon_158_2_48x48.png` |
| imageres_mun | RT_ICON | 1283 (group 158) | 40x40 | 32 | `icons/icon_158_3_40x40.png` |
| imageres_mun | RT_ICON | 1284 (group 158) | 32x32 | 32 | `icons/icon_158_4_32x32.png` |
| imageres_mun | RT_ICON | 1285 (group 158) | 24x24 | 32 | `icons/icon_158_5_24x24.png` |
| imageres_mun | RT_ICON | 1286 (group 158) | 20x20 | 32 | `icons/icon_158_6_20x20.png` |
| imageres_mun | RT_ICON | 1287 (group 158) | 16x16 | 32 | `icons/icon_158_7_16x16.png` |
| imageres_mun | RT_ICON | 1288 (group 159) | 256x256 | 32 | `icons/icon_159_0_256x256.png` |
| imageres_mun | RT_ICON | 1289 (group 159) | 64x64 | 32 | `icons/icon_159_1_64x64.png` |
| imageres_mun | RT_ICON | 129 (group 24) | 256x256 | 32 | `icons/icon_24_0_256x256.png` |
| imageres_mun | RT_ICON | 1290 (group 159) | 48x48 | 32 | `icons/icon_159_2_48x48.png` |
| imageres_mun | RT_ICON | 1291 (group 159) | 40x40 | 32 | `icons/icon_159_3_40x40.png` |
| imageres_mun | RT_ICON | 1292 (group 159) | 32x32 | 32 | `icons/icon_159_4_32x32.png` |
| imageres_mun | RT_ICON | 1293 (group 159) | 24x24 | 32 | `icons/icon_159_5_24x24.png` |
| imageres_mun | RT_ICON | 1294 (group 159) | 20x20 | 32 | `icons/icon_159_6_20x20.png` |
| imageres_mun | RT_ICON | 1295 (group 159) | 16x16 | 32 | `icons/icon_159_7_16x16.png` |
| imageres_mun | RT_ICON | 1296 (group 160) | 256x256 | 32 | `icons/icon_160_0_256x256.png` |
| imageres_mun | RT_ICON | 1297 (group 160) | 64x64 | 32 | `icons/icon_160_1_64x64.png` |
| imageres_mun | RT_ICON | 1298 (group 160) | 48x48 | 32 | `icons/icon_160_2_48x48.png` |
| imageres_mun | RT_ICON | 1299 (group 160) | 40x40 | 32 | `icons/icon_160_3_40x40.png` |
| imageres_mun | RT_ICON | 13 (group 3) | 32x32 | 32 | `icons/icon_3_4_32x32.png` |
| imageres_mun | RT_ICON | 130 (group 24) | 64x64 | 32 | `icons/icon_24_1_64x64.png` |
| imageres_mun | RT_ICON | 1300 (group 160) | 32x32 | 32 | `icons/icon_160_4_32x32.png` |
| imageres_mun | RT_ICON | 1301 (group 160) | 24x24 | 32 | `icons/icon_160_5_24x24.png` |
| imageres_mun | RT_ICON | 1302 (group 160) | 20x20 | 32 | `icons/icon_160_6_20x20.png` |
| imageres_mun | RT_ICON | 1303 (group 160) | 16x16 | 32 | `icons/icon_160_7_16x16.png` |
| imageres_mun | RT_ICON | 1304 (group 161) | 256x256 | 32 | `icons/icon_161_0_256x256.png` |
| imageres_mun | RT_ICON | 1305 (group 161) | 64x64 | 32 | `icons/icon_161_1_64x64.png` |
| imageres_mun | RT_ICON | 1306 (group 161) | 48x48 | 32 | `icons/icon_161_2_48x48.png` |
| imageres_mun | RT_ICON | 1307 (group 161) | 40x40 | 32 | `icons/icon_161_3_40x40.png` |
| imageres_mun | RT_ICON | 1308 (group 161) | 32x32 | 32 | `icons/icon_161_4_32x32.png` |
| imageres_mun | RT_ICON | 1309 (group 161) | 24x24 | 32 | `icons/icon_161_5_24x24.png` |
| imageres_mun | RT_ICON | 131 (group 24) | 48x48 | 32 | `icons/icon_24_2_48x48.png` |
| imageres_mun | RT_ICON | 1310 (group 161) | 20x20 | 32 | `icons/icon_161_6_20x20.png` |
| imageres_mun | RT_ICON | 1311 (group 161) | 16x16 | 32 | `icons/icon_161_7_16x16.png` |
| imageres_mun | RT_ICON | 1312 (group 162) | 256x256 | 32 | `icons/icon_162_0_256x256.png` |
| imageres_mun | RT_ICON | 1313 (group 162) | 64x64 | 32 | `icons/icon_162_1_64x64.png` |
| imageres_mun | RT_ICON | 1314 (group 162) | 48x48 | 32 | `icons/icon_162_2_48x48.png` |
| imageres_mun | RT_ICON | 1315 (group 162) | 40x40 | 32 | `icons/icon_162_3_40x40.png` |
| imageres_mun | RT_ICON | 1316 (group 162) | 32x32 | 32 | `icons/icon_162_4_32x32.png` |
| imageres_mun | RT_ICON | 1317 (group 162) | 24x24 | 32 | `icons/icon_162_5_24x24.png` |
| imageres_mun | RT_ICON | 1318 (group 162) | 20x20 | 32 | `icons/icon_162_6_20x20.png` |
| imageres_mun | RT_ICON | 1319 (group 162) | 16x16 | 32 | `icons/icon_162_7_16x16.png` |
| imageres_mun | RT_ICON | 132 (group 24) | 40x40 | 32 | `icons/icon_24_3_40x40.png` |
| imageres_mun | RT_ICON | 1320 (group 163) | 256x256 | 32 | `icons/icon_163_0_256x256.png` |
| imageres_mun | RT_ICON | 1321 (group 163) | 64x64 | 32 | `icons/icon_163_1_64x64.png` |
| imageres_mun | RT_ICON | 1322 (group 163) | 48x48 | 32 | `icons/icon_163_2_48x48.png` |
| imageres_mun | RT_ICON | 1323 (group 163) | 40x40 | 32 | `icons/icon_163_3_40x40.png` |
| imageres_mun | RT_ICON | 1324 (group 163) | 32x32 | 32 | `icons/icon_163_4_32x32.png` |
| imageres_mun | RT_ICON | 1325 (group 163) | 24x24 | 32 | `icons/icon_163_5_24x24.png` |
| imageres_mun | RT_ICON | 1326 (group 163) | 20x20 | 32 | `icons/icon_163_6_20x20.png` |
| imageres_mun | RT_ICON | 1327 (group 163) | 16x16 | 32 | `icons/icon_163_7_16x16.png` |
| imageres_mun | RT_ICON | 1328 (group 164) | 256x256 | 32 | `icons/icon_164_0_256x256.png` |
| imageres_mun | RT_ICON | 1329 (group 164) | 64x64 | 32 | `icons/icon_164_1_64x64.png` |
| imageres_mun | RT_ICON | 133 (group 24) | 32x32 | 32 | `icons/icon_24_4_32x32.png` |
| imageres_mun | RT_ICON | 1330 (group 164) | 48x48 | 32 | `icons/icon_164_2_48x48.png` |
| imageres_mun | RT_ICON | 1331 (group 164) | 40x40 | 32 | `icons/icon_164_3_40x40.png` |
| imageres_mun | RT_ICON | 1332 (group 164) | 32x32 | 32 | `icons/icon_164_4_32x32.png` |
| imageres_mun | RT_ICON | 1333 (group 164) | 24x24 | 32 | `icons/icon_164_5_24x24.png` |
| imageres_mun | RT_ICON | 1334 (group 164) | 20x20 | 32 | `icons/icon_164_6_20x20.png` |
| imageres_mun | RT_ICON | 1335 (group 164) | 16x16 | 32 | `icons/icon_164_7_16x16.png` |
| imageres_mun | RT_ICON | 1336 (group 165) | 256x256 | 32 | `icons/icon_165_0_256x256.png` |
| imageres_mun | RT_ICON | 1337 (group 165) | 64x64 | 32 | `icons/icon_165_1_64x64.png` |
| imageres_mun | RT_ICON | 1338 (group 165) | 48x48 | 32 | `icons/icon_165_2_48x48.png` |
| imageres_mun | RT_ICON | 1339 (group 165) | 40x40 | 32 | `icons/icon_165_3_40x40.png` |
| imageres_mun | RT_ICON | 134 (group 24) | 24x24 | 32 | `icons/icon_24_5_24x24.png` |
| imageres_mun | RT_ICON | 1340 (group 165) | 32x32 | 32 | `icons/icon_165_4_32x32.png` |
| imageres_mun | RT_ICON | 1341 (group 165) | 24x24 | 32 | `icons/icon_165_5_24x24.png` |
| imageres_mun | RT_ICON | 1342 (group 165) | 20x20 | 32 | `icons/icon_165_6_20x20.png` |
| imageres_mun | RT_ICON | 1343 (group 165) | 16x16 | 32 | `icons/icon_165_7_16x16.png` |
| imageres_mun | RT_ICON | 1344 (group 166) | 256x256 | 32 | `icons/icon_166_0_256x256.png` |
| imageres_mun | RT_ICON | 1345 (group 166) | 64x64 | 32 | `icons/icon_166_1_64x64.png` |
| imageres_mun | RT_ICON | 1346 (group 166) | 48x48 | 32 | `icons/icon_166_2_48x48.png` |
| imageres_mun | RT_ICON | 1347 (group 166) | 40x40 | 32 | `icons/icon_166_3_40x40.png` |
| imageres_mun | RT_ICON | 1348 (group 166) | 32x32 | 32 | `icons/icon_166_4_32x32.png` |
| imageres_mun | RT_ICON | 1349 (group 166) | 24x24 | 32 | `icons/icon_166_5_24x24.png` |
| imageres_mun | RT_ICON | 135 (group 24) | 20x20 | 32 | `icons/icon_24_6_20x20.png` |
| imageres_mun | RT_ICON | 1350 (group 166) | 20x20 | 32 | `icons/icon_166_6_20x20.png` |
| imageres_mun | RT_ICON | 1351 (group 166) | 16x16 | 32 | `icons/icon_166_7_16x16.png` |
| imageres_mun | RT_ICON | 1352 (group 167) | 256x256 | 32 | `icons/icon_167_0_256x256.png` |
| imageres_mun | RT_ICON | 1353 (group 167) | 64x64 | 32 | `icons/icon_167_1_64x64.png` |
| imageres_mun | RT_ICON | 1354 (group 167) | 48x48 | 32 | `icons/icon_167_2_48x48.png` |
| imageres_mun | RT_ICON | 1355 (group 167) | 40x40 | 32 | `icons/icon_167_3_40x40.png` |
| imageres_mun | RT_ICON | 1356 (group 167) | 32x32 | 32 | `icons/icon_167_4_32x32.png` |
| imageres_mun | RT_ICON | 1357 (group 167) | 24x24 | 32 | `icons/icon_167_5_24x24.png` |
| imageres_mun | RT_ICON | 1358 (group 167) | 20x20 | 32 | `icons/icon_167_6_20x20.png` |
| imageres_mun | RT_ICON | 1359 (group 167) | 16x16 | 32 | `icons/icon_167_7_16x16.png` |
| imageres_mun | RT_ICON | 136 (group 24) | 16x16 | 32 | `icons/icon_24_7_16x16.png` |
| imageres_mun | RT_ICON | 1360 (group 168) | 48x48 | 4 | `icons/icon_168_0_48x48.png` |
| imageres_mun | RT_ICON | 1361 (group 168) | 32x32 | 4 | `icons/icon_168_1_32x32.png` |
| imageres_mun | RT_ICON | 1362 (group 168) | 24x24 | 4 | `icons/icon_168_2_24x24.png` |
| imageres_mun | RT_ICON | 1363 (group 168) | 16x16 | 4 | `icons/icon_168_3_16x16.png` |
| imageres_mun | RT_ICON | 1364 (group 168) | 48x48 | 8 | `icons/icon_168_4_48x48.png` |
| imageres_mun | RT_ICON | 1365 (group 168) | 32x32 | 8 | `icons/icon_168_5_32x32.png` |
| imageres_mun | RT_ICON | 1366 (group 168) | 24x24 | 8 | `icons/icon_168_6_24x24.png` |
| imageres_mun | RT_ICON | 1367 (group 168) | 16x16 | 8 | `icons/icon_168_7_16x16.png` |
| imageres_mun | RT_ICON | 1368 (group 168) | 256x256 | 32 | `icons/icon_168_8_256x256.png` |
| imageres_mun | RT_ICON | 1369 (group 168) | 48x48 | 32 | `icons/icon_168_9_48x48.png` |
| imageres_mun | RT_ICON | 137 (group 25) | 256x256 | 32 | `icons/icon_25_0_256x256.png` |
| imageres_mun | RT_ICON | 1370 (group 168) | 32x32 | 32 | `icons/icon_168_10_32x32.png` |
| imageres_mun | RT_ICON | 1371 (group 168) | 24x24 | 32 | `icons/icon_168_11_24x24.png` |
| imageres_mun | RT_ICON | 1372 (group 168) | 16x16 | 32 | `icons/icon_168_12_16x16.png` |
| imageres_mun | RT_ICON | 1373 (group 169) | 256x256 | 32 | `icons/icon_169_0_256x256.png` |
| imageres_mun | RT_ICON | 1374 (group 169) | 64x64 | 32 | `icons/icon_169_1_64x64.png` |
| imageres_mun | RT_ICON | 1375 (group 169) | 48x48 | 32 | `icons/icon_169_2_48x48.png` |
| imageres_mun | RT_ICON | 1376 (group 169) | 40x40 | 32 | `icons/icon_169_3_40x40.png` |
| imageres_mun | RT_ICON | 1377 (group 169) | 32x32 | 32 | `icons/icon_169_4_32x32.png` |
| imageres_mun | RT_ICON | 1378 (group 169) | 24x24 | 32 | `icons/icon_169_5_24x24.png` |
| imageres_mun | RT_ICON | 1379 (group 169) | 20x20 | 32 | `icons/icon_169_6_20x20.png` |
| imageres_mun | RT_ICON | 138 (group 25) | 64x64 | 32 | `icons/icon_25_1_64x64.png` |
| imageres_mun | RT_ICON | 1380 (group 169) | 16x16 | 32 | `icons/icon_169_7_16x16.png` |
| imageres_mun | RT_ICON | 1381 (group 170) | 256x256 | 32 | `icons/icon_170_0_256x256.png` |
| imageres_mun | RT_ICON | 1382 (group 170) | 64x64 | 32 | `icons/icon_170_1_64x64.png` |
| imageres_mun | RT_ICON | 1383 (group 170) | 48x48 | 32 | `icons/icon_170_2_48x48.png` |
| imageres_mun | RT_ICON | 1384 (group 170) | 40x40 | 32 | `icons/icon_170_3_40x40.png` |
| imageres_mun | RT_ICON | 1385 (group 170) | 32x32 | 32 | `icons/icon_170_4_32x32.png` |
| imageres_mun | RT_ICON | 1386 (group 170) | 24x24 | 32 | `icons/icon_170_5_24x24.png` |
| imageres_mun | RT_ICON | 1387 (group 170) | 20x20 | 32 | `icons/icon_170_6_20x20.png` |
| imageres_mun | RT_ICON | 1388 (group 170) | 16x16 | 32 | `icons/icon_170_7_16x16.png` |
| imageres_mun | RT_ICON | 1389 (group 171) | 256x256 | 32 | `icons/icon_171_0_256x256.png` |
| imageres_mun | RT_ICON | 139 (group 25) | 48x48 | 32 | `icons/icon_25_2_48x48.png` |
| imageres_mun | RT_ICON | 1390 (group 171) | 64x64 | 32 | `icons/icon_171_1_64x64.png` |
| imageres_mun | RT_ICON | 1391 (group 171) | 48x48 | 32 | `icons/icon_171_2_48x48.png` |
| imageres_mun | RT_ICON | 1392 (group 171) | 40x40 | 32 | `icons/icon_171_3_40x40.png` |
| imageres_mun | RT_ICON | 1393 (group 171) | 32x32 | 32 | `icons/icon_171_4_32x32.png` |
| imageres_mun | RT_ICON | 1394 (group 171) | 24x24 | 32 | `icons/icon_171_5_24x24.png` |
| imageres_mun | RT_ICON | 1395 (group 171) | 20x20 | 32 | `icons/icon_171_6_20x20.png` |
| imageres_mun | RT_ICON | 1396 (group 171) | 16x16 | 32 | `icons/icon_171_7_16x16.png` |
| imageres_mun | RT_ICON | 1397 (group 172) | 256x256 | 32 | `icons/icon_172_0_256x256.png` |
| imageres_mun | RT_ICON | 1398 (group 172) | 64x64 | 32 | `icons/icon_172_1_64x64.png` |
| imageres_mun | RT_ICON | 1399 (group 172) | 48x48 | 32 | `icons/icon_172_2_48x48.png` |
| imageres_mun | RT_ICON | 14 (group 3) | 24x24 | 32 | `icons/icon_3_5_24x24.png` |
| imageres_mun | RT_ICON | 140 (group 25) | 40x40 | 32 | `icons/icon_25_3_40x40.png` |
| imageres_mun | RT_ICON | 1400 (group 172) | 40x40 | 32 | `icons/icon_172_3_40x40.png` |
| imageres_mun | RT_ICON | 1401 (group 172) | 32x32 | 32 | `icons/icon_172_4_32x32.png` |
| imageres_mun | RT_ICON | 1402 (group 172) | 24x24 | 32 | `icons/icon_172_5_24x24.png` |
| imageres_mun | RT_ICON | 1403 (group 172) | 20x20 | 32 | `icons/icon_172_6_20x20.png` |
| imageres_mun | RT_ICON | 1404 (group 172) | 16x16 | 32 | `icons/icon_172_7_16x16.png` |
| imageres_mun | RT_ICON | 1405 (group 173) | 256x256 | 32 | `icons/icon_173_0_256x256.png` |
| imageres_mun | RT_ICON | 1406 (group 173) | 64x64 | 32 | `icons/icon_173_1_64x64.png` |
| imageres_mun | RT_ICON | 1407 (group 173) | 48x48 | 32 | `icons/icon_173_2_48x48.png` |
| imageres_mun | RT_ICON | 1408 (group 173) | 40x40 | 32 | `icons/icon_173_3_40x40.png` |
| imageres_mun | RT_ICON | 1409 (group 173) | 32x32 | 32 | `icons/icon_173_4_32x32.png` |
| imageres_mun | RT_ICON | 141 (group 25) | 32x32 | 32 | `icons/icon_25_4_32x32.png` |
| imageres_mun | RT_ICON | 1410 (group 173) | 24x24 | 32 | `icons/icon_173_5_24x24.png` |
| imageres_mun | RT_ICON | 1411 (group 173) | 20x20 | 32 | `icons/icon_173_6_20x20.png` |
| imageres_mun | RT_ICON | 1412 (group 173) | 16x16 | 32 | `icons/icon_173_7_16x16.png` |
| imageres_mun | RT_ICON | 1413 (group 174) | 256x256 | 32 | `icons/icon_174_0_256x256.png` |
| imageres_mun | RT_ICON | 1414 (group 174) | 64x64 | 32 | `icons/icon_174_1_64x64.png` |
| imageres_mun | RT_ICON | 1415 (group 174) | 48x48 | 32 | `icons/icon_174_2_48x48.png` |
| imageres_mun | RT_ICON | 1416 (group 174) | 40x40 | 32 | `icons/icon_174_3_40x40.png` |
| imageres_mun | RT_ICON | 1417 (group 174) | 32x32 | 32 | `icons/icon_174_4_32x32.png` |
| imageres_mun | RT_ICON | 1418 (group 174) | 24x24 | 32 | `icons/icon_174_5_24x24.png` |
| imageres_mun | RT_ICON | 1419 (group 174) | 20x20 | 32 | `icons/icon_174_6_20x20.png` |
| imageres_mun | RT_ICON | 142 (group 25) | 24x24 | 32 | `icons/icon_25_5_24x24.png` |
| imageres_mun | RT_ICON | 1420 (group 174) | 16x16 | 32 | `icons/icon_174_7_16x16.png` |
| imageres_mun | RT_ICON | 1421 (group 175) | 256x256 | 32 | `icons/icon_175_0_256x256.png` |
| imageres_mun | RT_ICON | 1422 (group 175) | 64x64 | 32 | `icons/icon_175_1_64x64.png` |
| imageres_mun | RT_ICON | 1423 (group 175) | 48x48 | 32 | `icons/icon_175_2_48x48.png` |
| imageres_mun | RT_ICON | 1424 (group 175) | 40x40 | 32 | `icons/icon_175_3_40x40.png` |
| imageres_mun | RT_ICON | 1425 (group 175) | 32x32 | 32 | `icons/icon_175_4_32x32.png` |
| imageres_mun | RT_ICON | 1426 (group 175) | 24x24 | 32 | `icons/icon_175_5_24x24.png` |
| imageres_mun | RT_ICON | 1427 (group 175) | 20x20 | 32 | `icons/icon_175_6_20x20.png` |
| imageres_mun | RT_ICON | 1428 (group 175) | 16x16 | 32 | `icons/icon_175_7_16x16.png` |
| imageres_mun | RT_ICON | 1429 (group 176) | 256x256 | 32 | `icons/icon_176_0_256x256.png` |
| imageres_mun | RT_ICON | 143 (group 25) | 20x20 | 32 | `icons/icon_25_6_20x20.png` |
| imageres_mun | RT_ICON | 1430 (group 176) | 64x64 | 32 | `icons/icon_176_1_64x64.png` |
| imageres_mun | RT_ICON | 1431 (group 176) | 48x48 | 32 | `icons/icon_176_2_48x48.png` |
| imageres_mun | RT_ICON | 1432 (group 176) | 40x40 | 32 | `icons/icon_176_3_40x40.png` |
| imageres_mun | RT_ICON | 1433 (group 176) | 32x32 | 32 | `icons/icon_176_4_32x32.png` |
| imageres_mun | RT_ICON | 1434 (group 176) | 24x24 | 32 | `icons/icon_176_5_24x24.png` |
| imageres_mun | RT_ICON | 1435 (group 176) | 20x20 | 32 | `icons/icon_176_6_20x20.png` |
| imageres_mun | RT_ICON | 1436 (group 176) | 16x16 | 32 | `icons/icon_176_7_16x16.png` |
| imageres_mun | RT_ICON | 1437 (group 177) | 256x256 | 32 | `icons/icon_177_0_256x256.png` |
| imageres_mun | RT_ICON | 1438 (group 177) | 64x64 | 32 | `icons/icon_177_1_64x64.png` |
| imageres_mun | RT_ICON | 1439 (group 177) | 48x48 | 32 | `icons/icon_177_2_48x48.png` |
| imageres_mun | RT_ICON | 144 (group 25) | 16x16 | 32 | `icons/icon_25_7_16x16.png` |
| imageres_mun | RT_ICON | 1440 (group 177) | 40x40 | 32 | `icons/icon_177_3_40x40.png` |
| imageres_mun | RT_ICON | 1441 (group 177) | 32x32 | 32 | `icons/icon_177_4_32x32.png` |
| imageres_mun | RT_ICON | 1442 (group 177) | 24x24 | 32 | `icons/icon_177_5_24x24.png` |
| imageres_mun | RT_ICON | 1443 (group 177) | 20x20 | 32 | `icons/icon_177_6_20x20.png` |
| imageres_mun | RT_ICON | 1444 (group 177) | 16x16 | 32 | `icons/icon_177_7_16x16.png` |
| imageres_mun | RT_ICON | 1445 (group 178) | 256x256 | 32 | `icons/icon_178_0_256x256.png` |
| imageres_mun | RT_ICON | 1446 (group 178) | 64x64 | 32 | `icons/icon_178_1_64x64.png` |
| imageres_mun | RT_ICON | 1447 (group 178) | 48x48 | 32 | `icons/icon_178_2_48x48.png` |
| imageres_mun | RT_ICON | 1448 (group 178) | 40x40 | 32 | `icons/icon_178_3_40x40.png` |
| imageres_mun | RT_ICON | 1449 (group 178) | 32x32 | 32 | `icons/icon_178_4_32x32.png` |
| imageres_mun | RT_ICON | 145 (group 26) | 256x256 | 32 | `icons/icon_26_0_256x256.png` |
| imageres_mun | RT_ICON | 1450 (group 178) | 24x24 | 32 | `icons/icon_178_5_24x24.png` |
| imageres_mun | RT_ICON | 1451 (group 178) | 20x20 | 32 | `icons/icon_178_6_20x20.png` |
| imageres_mun | RT_ICON | 1452 (group 178) | 16x16 | 32 | `icons/icon_178_7_16x16.png` |
| imageres_mun | RT_ICON | 1453 (group 179) | 48x48 | 4 | `icons/icon_179_0_48x48.png` |
| imageres_mun | RT_ICON | 1454 (group 179) | 32x32 | 4 | `icons/icon_179_1_32x32.png` |
| imageres_mun | RT_ICON | 1455 (group 179) | 16x16 | 4 | `icons/icon_179_2_16x16.png` |
| imageres_mun | RT_ICON | 1456 (group 179) | 48x48 | 8 | `icons/icon_179_3_48x48.png` |
| imageres_mun | RT_ICON | 1457 (group 179) | 32x32 | 8 | `icons/icon_179_4_32x32.png` |
| imageres_mun | RT_ICON | 1458 (group 179) | 16x16 | 8 | `icons/icon_179_5_16x16.png` |
| imageres_mun | RT_ICON | 1459 (group 179) | 256x256 | 32 | `icons/icon_179_6_256x256.png` |
| imageres_mun | RT_ICON | 146 (group 26) | 64x64 | 32 | `icons/icon_26_1_64x64.png` |
| imageres_mun | RT_ICON | 1460 (group 179) | 48x48 | 32 | `icons/icon_179_7_48x48.png` |
| imageres_mun | RT_ICON | 1461 (group 179) | 32x32 | 32 | `icons/icon_179_8_32x32.png` |
| imageres_mun | RT_ICON | 1462 (group 179) | 16x16 | 32 | `icons/icon_179_9_16x16.png` |
| imageres_mun | RT_ICON | 1463 (group 180) | 256x256 | 32 | `icons/icon_180_0_256x256.png` |
| imageres_mun | RT_ICON | 1464 (group 180) | 64x64 | 32 | `icons/icon_180_1_64x64.png` |
| imageres_mun | RT_ICON | 1465 (group 180) | 48x48 | 32 | `icons/icon_180_2_48x48.png` |
| imageres_mun | RT_ICON | 1466 (group 180) | 40x40 | 32 | `icons/icon_180_3_40x40.png` |
| imageres_mun | RT_ICON | 1467 (group 180) | 32x32 | 32 | `icons/icon_180_4_32x32.png` |
| imageres_mun | RT_ICON | 1468 (group 180) | 24x24 | 32 | `icons/icon_180_5_24x24.png` |
| imageres_mun | RT_ICON | 1469 (group 180) | 20x20 | 32 | `icons/icon_180_6_20x20.png` |
| imageres_mun | RT_ICON | 147 (group 26) | 48x48 | 32 | `icons/icon_26_2_48x48.png` |
| imageres_mun | RT_ICON | 1470 (group 180) | 16x16 | 32 | `icons/icon_180_7_16x16.png` |
| imageres_mun | RT_ICON | 1471 (group 181) | 256x256 | 32 | `icons/icon_181_0_256x256.png` |
| imageres_mun | RT_ICON | 1472 (group 181) | 64x64 | 32 | `icons/icon_181_1_64x64.png` |
| imageres_mun | RT_ICON | 1473 (group 181) | 48x48 | 32 | `icons/icon_181_2_48x48.png` |
| imageres_mun | RT_ICON | 1474 (group 181) | 40x40 | 32 | `icons/icon_181_3_40x40.png` |
| imageres_mun | RT_ICON | 1475 (group 181) | 32x32 | 32 | `icons/icon_181_4_32x32.png` |
| imageres_mun | RT_ICON | 1476 (group 181) | 24x24 | 32 | `icons/icon_181_5_24x24.png` |
| imageres_mun | RT_ICON | 1477 (group 181) | 20x20 | 32 | `icons/icon_181_6_20x20.png` |
| imageres_mun | RT_ICON | 1478 (group 181) | 16x16 | 32 | `icons/icon_181_7_16x16.png` |
| imageres_mun | RT_ICON | 1479 (group 182) | 256x256 | 32 | `icons/icon_182_0_256x256.png` |
| imageres_mun | RT_ICON | 148 (group 26) | 40x40 | 32 | `icons/icon_26_3_40x40.png` |
| imageres_mun | RT_ICON | 1480 (group 182) | 64x64 | 32 | `icons/icon_182_1_64x64.png` |
| imageres_mun | RT_ICON | 1481 (group 182) | 48x48 | 32 | `icons/icon_182_2_48x48.png` |
| imageres_mun | RT_ICON | 1482 (group 182) | 40x40 | 32 | `icons/icon_182_3_40x40.png` |
| imageres_mun | RT_ICON | 1483 (group 182) | 32x32 | 32 | `icons/icon_182_4_32x32.png` |
| imageres_mun | RT_ICON | 1484 (group 182) | 24x24 | 32 | `icons/icon_182_5_24x24.png` |
| imageres_mun | RT_ICON | 1485 (group 182) | 20x20 | 32 | `icons/icon_182_6_20x20.png` |
| imageres_mun | RT_ICON | 1486 (group 182) | 16x16 | 32 | `icons/icon_182_7_16x16.png` |
| imageres_mun | RT_ICON | 1487 (group 183) | 256x256 | 32 | `icons/icon_183_0_256x256.png` |
| imageres_mun | RT_ICON | 1488 (group 183) | 64x64 | 32 | `icons/icon_183_1_64x64.png` |
| imageres_mun | RT_ICON | 1489 (group 183) | 48x48 | 32 | `icons/icon_183_2_48x48.png` |
| imageres_mun | RT_ICON | 149 (group 26) | 32x32 | 32 | `icons/icon_26_4_32x32.png` |
| imageres_mun | RT_ICON | 1490 (group 183) | 40x40 | 32 | `icons/icon_183_3_40x40.png` |
| imageres_mun | RT_ICON | 1491 (group 183) | 32x32 | 32 | `icons/icon_183_4_32x32.png` |
| imageres_mun | RT_ICON | 1492 (group 183) | 24x24 | 32 | `icons/icon_183_5_24x24.png` |
| imageres_mun | RT_ICON | 1493 (group 183) | 20x20 | 32 | `icons/icon_183_6_20x20.png` |
| imageres_mun | RT_ICON | 1494 (group 183) | 16x16 | 32 | `icons/icon_183_7_16x16.png` |
| imageres_mun | RT_ICON | 1495 (group 184) | 256x256 | 32 | `icons/icon_184_0_256x256.png` |
| imageres_mun | RT_ICON | 1496 (group 184) | 64x64 | 32 | `icons/icon_184_1_64x64.png` |
| imageres_mun | RT_ICON | 1497 (group 184) | 48x48 | 32 | `icons/icon_184_2_48x48.png` |
| imageres_mun | RT_ICON | 1498 (group 184) | 40x40 | 32 | `icons/icon_184_3_40x40.png` |
| imageres_mun | RT_ICON | 1499 (group 184) | 32x32 | 32 | `icons/icon_184_4_32x32.png` |
| imageres_mun | RT_ICON | 15 (group 3) | 20x20 | 32 | `icons/icon_3_6_20x20.png` |
| imageres_mun | RT_ICON | 150 (group 26) | 24x24 | 32 | `icons/icon_26_5_24x24.png` |
| imageres_mun | RT_ICON | 1500 (group 184) | 24x24 | 32 | `icons/icon_184_5_24x24.png` |
| imageres_mun | RT_ICON | 1501 (group 184) | 20x20 | 32 | `icons/icon_184_6_20x20.png` |
| imageres_mun | RT_ICON | 1502 (group 184) | 16x16 | 32 | `icons/icon_184_7_16x16.png` |
| imageres_mun | RT_ICON | 1503 (group 185) | 256x256 | 32 | `icons/icon_185_0_256x256.png` |
| imageres_mun | RT_ICON | 1504 (group 185) | 64x64 | 32 | `icons/icon_185_1_64x64.png` |
| imageres_mun | RT_ICON | 1505 (group 185) | 48x48 | 32 | `icons/icon_185_2_48x48.png` |
| imageres_mun | RT_ICON | 1506 (group 185) | 40x40 | 32 | `icons/icon_185_3_40x40.png` |
| imageres_mun | RT_ICON | 1507 (group 185) | 32x32 | 32 | `icons/icon_185_4_32x32.png` |
| imageres_mun | RT_ICON | 1508 (group 185) | 24x24 | 32 | `icons/icon_185_5_24x24.png` |
| imageres_mun | RT_ICON | 1509 (group 185) | 20x20 | 32 | `icons/icon_185_6_20x20.png` |
| imageres_mun | RT_ICON | 151 (group 26) | 20x20 | 32 | `icons/icon_26_6_20x20.png` |
| imageres_mun | RT_ICON | 1510 (group 185) | 16x16 | 32 | `icons/icon_185_7_16x16.png` |
| imageres_mun | RT_ICON | 1511 (group 186) | 256x256 | 32 | `icons/icon_186_0_256x256.png` |
| imageres_mun | RT_ICON | 1512 (group 186) | 64x64 | 32 | `icons/icon_186_1_64x64.png` |
| imageres_mun | RT_ICON | 1513 (group 186) | 48x48 | 32 | `icons/icon_186_2_48x48.png` |
| imageres_mun | RT_ICON | 1514 (group 186) | 40x40 | 32 | `icons/icon_186_3_40x40.png` |
| imageres_mun | RT_ICON | 1515 (group 186) | 32x32 | 32 | `icons/icon_186_4_32x32.png` |
| imageres_mun | RT_ICON | 1516 (group 186) | 24x24 | 32 | `icons/icon_186_5_24x24.png` |
| imageres_mun | RT_ICON | 1517 (group 186) | 20x20 | 32 | `icons/icon_186_6_20x20.png` |
| imageres_mun | RT_ICON | 1518 (group 186) | 16x16 | 32 | `icons/icon_186_7_16x16.png` |
| imageres_mun | RT_ICON | 1519 (group 190) | 256x256 | 32 | `icons/icon_190_0_256x256.png` |
| imageres_mun | RT_ICON | 152 (group 26) | 16x16 | 32 | `icons/icon_26_7_16x16.png` |
| imageres_mun | RT_ICON | 1520 (group 190) | 64x64 | 32 | `icons/icon_190_1_64x64.png` |
| imageres_mun | RT_ICON | 1521 (group 190) | 48x48 | 32 | `icons/icon_190_2_48x48.png` |
| imageres_mun | RT_ICON | 1522 (group 190) | 40x40 | 32 | `icons/icon_190_3_40x40.png` |
| imageres_mun | RT_ICON | 1523 (group 190) | 32x32 | 32 | `icons/icon_190_4_32x32.png` |
| imageres_mun | RT_ICON | 1524 (group 190) | 24x24 | 32 | `icons/icon_190_5_24x24.png` |
| imageres_mun | RT_ICON | 1525 (group 190) | 20x20 | 32 | `icons/icon_190_6_20x20.png` |
| imageres_mun | RT_ICON | 1526 (group 190) | 16x16 | 32 | `icons/icon_190_7_16x16.png` |
| imageres_mun | RT_ICON | 1527 (group 191) | 16x16 | 24 | `icons/icon_191_0_16x16.png` |
| imageres_mun | RT_ICON | 1528 (group 191) | 256x256 | 32 | `icons/icon_191_1_256x256.png` |
| imageres_mun | RT_ICON | 1529 (group 191) | 96x96 | 32 | `icons/icon_191_2_96x96.png` |
| imageres_mun | RT_ICON | 153 (group 77) | 256x256 | 32 | `icons/icon_77_0_256x256.png` |
| imageres_mun | RT_ICON | 1530 (group 191) | 48x48 | 32 | `icons/icon_191_3_48x48.png` |
| imageres_mun | RT_ICON | 1531 (group 191) | 32x32 | 32 | `icons/icon_191_4_32x32.png` |
| imageres_mun | RT_ICON | 1532 (group 192) | 16x16 | 24 | `icons/icon_192_0_16x16.png` |
| imageres_mun | RT_ICON | 1533 (group 192) | 256x256 | 32 | `icons/icon_192_1_256x256.png` |
| imageres_mun | RT_ICON | 1534 (group 192) | 96x96 | 32 | `icons/icon_192_2_96x96.png` |
| imageres_mun | RT_ICON | 1535 (group 192) | 48x48 | 32 | `icons/icon_192_3_48x48.png` |
| imageres_mun | RT_ICON | 1536 (group 192) | 32x32 | 32 | `icons/icon_192_4_32x32.png` |
| imageres_mun | RT_ICON | 1537 (group 193) | 256x256 | 32 | `icons/icon_193_0_256x256.png` |
| imageres_mun | RT_ICON | 1538 (group 193) | 64x64 | 32 | `icons/icon_193_1_64x64.png` |
| imageres_mun | RT_ICON | 1539 (group 193) | 48x48 | 32 | `icons/icon_193_2_48x48.png` |
| imageres_mun | RT_ICON | 154 (group 77) | 64x64 | 32 | `icons/icon_77_1_64x64.png` |
| imageres_mun | RT_ICON | 1540 (group 193) | 40x40 | 32 | `icons/icon_193_3_40x40.png` |
| imageres_mun | RT_ICON | 1541 (group 193) | 32x32 | 32 | `icons/icon_193_4_32x32.png` |
| imageres_mun | RT_ICON | 1542 (group 193) | 24x24 | 32 | `icons/icon_193_5_24x24.png` |
| imageres_mun | RT_ICON | 1543 (group 193) | 20x20 | 32 | `icons/icon_193_6_20x20.png` |
| imageres_mun | RT_ICON | 1544 (group 193) | 16x16 | 32 | `icons/icon_193_7_16x16.png` |
| imageres_mun | RT_ICON | 1545 (group 194) | 256x256 | 32 | `icons/icon_194_0_256x256.png` |
| imageres_mun | RT_ICON | 1546 (group 194) | 64x64 | 32 | `icons/icon_194_1_64x64.png` |
| imageres_mun | RT_ICON | 1547 (group 194) | 48x48 | 32 | `icons/icon_194_2_48x48.png` |
| imageres_mun | RT_ICON | 1548 (group 194) | 40x40 | 32 | `icons/icon_194_3_40x40.png` |
| imageres_mun | RT_ICON | 1549 (group 194) | 32x32 | 32 | `icons/icon_194_4_32x32.png` |
| imageres_mun | RT_ICON | 155 (group 77) | 48x48 | 32 | `icons/icon_77_2_48x48.png` |
| imageres_mun | RT_ICON | 1550 (group 194) | 24x24 | 32 | `icons/icon_194_5_24x24.png` |
| imageres_mun | RT_ICON | 1551 (group 194) | 20x20 | 32 | `icons/icon_194_6_20x20.png` |
| imageres_mun | RT_ICON | 1552 (group 194) | 16x16 | 32 | `icons/icon_194_7_16x16.png` |
| imageres_mun | RT_ICON | 1553 (group 1001) | 256x256 | 32 | `icons/icon_1001_0_256x256.png` |
| imageres_mun | RT_ICON | 1554 (group 1001) | 64x64 | 32 | `icons/icon_1001_1_64x64.png` |
| imageres_mun | RT_ICON | 1555 (group 1001) | 48x48 | 32 | `icons/icon_1001_2_48x48.png` |
| imageres_mun | RT_ICON | 1556 (group 1001) | 40x40 | 32 | `icons/icon_1001_3_40x40.png` |
| imageres_mun | RT_ICON | 1557 (group 1001) | 32x32 | 32 | `icons/icon_1001_4_32x32.png` |
| imageres_mun | RT_ICON | 1558 (group 1001) | 24x24 | 32 | `icons/icon_1001_5_24x24.png` |
| imageres_mun | RT_ICON | 1559 (group 1001) | 20x20 | 32 | `icons/icon_1001_6_20x20.png` |
| imageres_mun | RT_ICON | 156 (group 77) | 40x40 | 32 | `icons/icon_77_3_40x40.png` |
| imageres_mun | RT_ICON | 1560 (group 1001) | 16x16 | 32 | `icons/icon_1001_7_16x16.png` |
| imageres_mun | RT_ICON | 1561 (group 1014) | 256x256 | 32 | `icons/icon_1014_0_256x256.png` |
| imageres_mun | RT_ICON | 1562 (group 1014) | 64x64 | 32 | `icons/icon_1014_1_64x64.png` |
| imageres_mun | RT_ICON | 1563 (group 1014) | 48x48 | 32 | `icons/icon_1014_2_48x48.png` |
| imageres_mun | RT_ICON | 1564 (group 1014) | 40x40 | 32 | `icons/icon_1014_3_40x40.png` |
| imageres_mun | RT_ICON | 1565 (group 1014) | 32x32 | 32 | `icons/icon_1014_4_32x32.png` |
| imageres_mun | RT_ICON | 1566 (group 1014) | 24x24 | 32 | `icons/icon_1014_5_24x24.png` |
| imageres_mun | RT_ICON | 1567 (group 1014) | 20x20 | 32 | `icons/icon_1014_6_20x20.png` |
| imageres_mun | RT_ICON | 1568 (group 1014) | 16x16 | 32 | `icons/icon_1014_7_16x16.png` |
| imageres_mun | RT_ICON | 1569 (group 1002) | 256x256 | 32 | `icons/icon_1002_0_256x256.png` |
| imageres_mun | RT_ICON | 157 (group 77) | 32x32 | 32 | `icons/icon_77_4_32x32.png` |
| imageres_mun | RT_ICON | 1570 (group 1002) | 64x64 | 32 | `icons/icon_1002_1_64x64.png` |
| imageres_mun | RT_ICON | 1571 (group 1002) | 48x48 | 32 | `icons/icon_1002_2_48x48.png` |
| imageres_mun | RT_ICON | 1572 (group 1002) | 40x40 | 32 | `icons/icon_1002_3_40x40.png` |
| imageres_mun | RT_ICON | 1573 (group 1002) | 32x32 | 32 | `icons/icon_1002_4_32x32.png` |
| imageres_mun | RT_ICON | 1574 (group 1002) | 24x24 | 32 | `icons/icon_1002_5_24x24.png` |
| imageres_mun | RT_ICON | 1575 (group 1002) | 20x20 | 32 | `icons/icon_1002_6_20x20.png` |
| imageres_mun | RT_ICON | 1576 (group 1002) | 16x16 | 32 | `icons/icon_1002_7_16x16.png` |
| imageres_mun | RT_ICON | 1577 (group 1003) | 256x256 | 32 | `icons/icon_1003_0_256x256.png` |
| imageres_mun | RT_ICON | 1578 (group 1003) | 64x64 | 32 | `icons/icon_1003_1_64x64.png` |
| imageres_mun | RT_ICON | 1579 (group 1003) | 48x48 | 32 | `icons/icon_1003_2_48x48.png` |
| imageres_mun | RT_ICON | 158 (group 77) | 24x24 | 32 | `icons/icon_77_5_24x24.png` |
| imageres_mun | RT_ICON | 1580 (group 1003) | 40x40 | 32 | `icons/icon_1003_3_40x40.png` |
| imageres_mun | RT_ICON | 1581 (group 1003) | 32x32 | 32 | `icons/icon_1003_4_32x32.png` |
| imageres_mun | RT_ICON | 1582 (group 1003) | 24x24 | 32 | `icons/icon_1003_5_24x24.png` |
| imageres_mun | RT_ICON | 1583 (group 1003) | 20x20 | 32 | `icons/icon_1003_6_20x20.png` |
| imageres_mun | RT_ICON | 1584 (group 1003) | 16x16 | 32 | `icons/icon_1003_7_16x16.png` |
| imageres_mun | RT_ICON | 1585 (group 1004) | 256x256 | 32 | `icons/icon_1004_0_256x256.png` |
| imageres_mun | RT_ICON | 1586 (group 1004) | 64x64 | 32 | `icons/icon_1004_1_64x64.png` |
| imageres_mun | RT_ICON | 1587 (group 1004) | 48x48 | 32 | `icons/icon_1004_2_48x48.png` |
| imageres_mun | RT_ICON | 1588 (group 1004) | 40x40 | 32 | `icons/icon_1004_3_40x40.png` |
| imageres_mun | RT_ICON | 1589 (group 1004) | 32x32 | 32 | `icons/icon_1004_4_32x32.png` |
| imageres_mun | RT_ICON | 159 (group 77) | 20x20 | 32 | `icons/icon_77_6_20x20.png` |
| imageres_mun | RT_ICON | 1590 (group 1004) | 24x24 | 32 | `icons/icon_1004_5_24x24.png` |
| imageres_mun | RT_ICON | 1591 (group 1004) | 20x20 | 32 | `icons/icon_1004_6_20x20.png` |
| imageres_mun | RT_ICON | 1592 (group 1004) | 16x16 | 32 | `icons/icon_1004_7_16x16.png` |
| imageres_mun | RT_ICON | 1593 (group 1005) | 256x256 | 32 | `icons/icon_1005_0_256x256.png` |
| imageres_mun | RT_ICON | 1594 (group 1005) | 64x64 | 32 | `icons/icon_1005_1_64x64.png` |
| imageres_mun | RT_ICON | 1595 (group 1005) | 48x48 | 32 | `icons/icon_1005_2_48x48.png` |
| imageres_mun | RT_ICON | 1596 (group 1005) | 40x40 | 32 | `icons/icon_1005_3_40x40.png` |
| imageres_mun | RT_ICON | 1597 (group 1005) | 32x32 | 32 | `icons/icon_1005_4_32x32.png` |
| imageres_mun | RT_ICON | 1598 (group 1005) | 24x24 | 32 | `icons/icon_1005_5_24x24.png` |
| imageres_mun | RT_ICON | 1599 (group 1005) | 20x20 | 32 | `icons/icon_1005_6_20x20.png` |
| imageres_mun | RT_ICON | 16 (group 3) | 16x16 | 32 | `icons/icon_3_7_16x16.png` |
| imageres_mun | RT_ICON | 160 (group 77) | 16x16 | 32 | `icons/icon_77_7_16x16.png` |
| imageres_mun | RT_ICON | 1600 (group 1005) | 16x16 | 32 | `icons/icon_1005_7_16x16.png` |
| imageres_mun | RT_ICON | 1601 (group 1008) | 256x256 | 32 | `icons/icon_1008_0_256x256.png` |
| imageres_mun | RT_ICON | 1602 (group 1008) | 64x64 | 32 | `icons/icon_1008_1_64x64.png` |
| imageres_mun | RT_ICON | 1603 (group 1008) | 48x48 | 32 | `icons/icon_1008_2_48x48.png` |
| imageres_mun | RT_ICON | 1604 (group 1008) | 40x40 | 32 | `icons/icon_1008_3_40x40.png` |
| imageres_mun | RT_ICON | 1605 (group 1008) | 32x32 | 32 | `icons/icon_1008_4_32x32.png` |
| imageres_mun | RT_ICON | 1606 (group 1008) | 24x24 | 32 | `icons/icon_1008_5_24x24.png` |
| imageres_mun | RT_ICON | 1607 (group 1008) | 20x20 | 32 | `icons/icon_1008_6_20x20.png` |
| imageres_mun | RT_ICON | 1608 (group 1008) | 16x16 | 32 | `icons/icon_1008_7_16x16.png` |
| imageres_mun | RT_ICON | 1609 (group 1010) | 256x256 | 32 | `icons/icon_1010_0_256x256.png` |
| imageres_mun | RT_ICON | 161 (group 80) | 256x256 | 32 | `icons/icon_80_0_256x256.png` |
| imageres_mun | RT_ICON | 1610 (group 1010) | 64x64 | 32 | `icons/icon_1010_1_64x64.png` |
| imageres_mun | RT_ICON | 1611 (group 1010) | 48x48 | 32 | `icons/icon_1010_2_48x48.png` |
| imageres_mun | RT_ICON | 1612 (group 1010) | 40x40 | 32 | `icons/icon_1010_3_40x40.png` |
| imageres_mun | RT_ICON | 1613 (group 1010) | 32x32 | 32 | `icons/icon_1010_4_32x32.png` |
| imageres_mun | RT_ICON | 1614 (group 1010) | 24x24 | 32 | `icons/icon_1010_5_24x24.png` |
| imageres_mun | RT_ICON | 1615 (group 1010) | 20x20 | 32 | `icons/icon_1010_6_20x20.png` |
| imageres_mun | RT_ICON | 1616 (group 1010) | 16x16 | 32 | `icons/icon_1010_7_16x16.png` |
| imageres_mun | RT_ICON | 1617 (group 1013) | 16x16 | 4 | `icons/icon_1013_0_16x16.png` |
| imageres_mun | RT_ICON | 1618 (group 1015) | 256x256 | 32 | `icons/icon_1015_0_256x256.png` |
| imageres_mun | RT_ICON | 1619 (group 1017) | 256x256 | 32 | `icons/icon_1017_0_256x256.png` |
| imageres_mun | RT_ICON | 162 (group 80) | 64x64 | 32 | `icons/icon_80_1_64x64.png` |
| imageres_mun | RT_ICON | 1620 (group 1018) | 256x256 | 32 | `icons/icon_1018_0_256x256.png` |
| imageres_mun | RT_ICON | 1621 (group 1020) | 256x256 | 32 | `icons/icon_1020_0_256x256.png` |
| imageres_mun | RT_ICON | 1622 (group 1020) | 64x64 | 32 | `icons/icon_1020_1_64x64.png` |
| imageres_mun | RT_ICON | 1623 (group 1020) | 48x48 | 32 | `icons/icon_1020_2_48x48.png` |
| imageres_mun | RT_ICON | 1624 (group 1020) | 40x40 | 32 | `icons/icon_1020_3_40x40.png` |
| imageres_mun | RT_ICON | 1625 (group 1020) | 32x32 | 32 | `icons/icon_1020_4_32x32.png` |
| imageres_mun | RT_ICON | 1626 (group 1020) | 24x24 | 32 | `icons/icon_1020_5_24x24.png` |
| imageres_mun | RT_ICON | 1627 (group 1020) | 20x20 | 32 | `icons/icon_1020_6_20x20.png` |
| imageres_mun | RT_ICON | 1628 (group 1020) | 16x16 | 32 | `icons/icon_1020_7_16x16.png` |
| imageres_mun | RT_ICON | 1629 (group 1021) | 256x256 | 32 | `icons/icon_1021_0_256x256.png` |
| imageres_mun | RT_ICON | 163 (group 80) | 48x48 | 32 | `icons/icon_80_2_48x48.png` |
| imageres_mun | RT_ICON | 1630 (group 1021) | 64x64 | 32 | `icons/icon_1021_1_64x64.png` |
| imageres_mun | RT_ICON | 1631 (group 1021) | 48x48 | 32 | `icons/icon_1021_2_48x48.png` |
| imageres_mun | RT_ICON | 1632 (group 1021) | 40x40 | 32 | `icons/icon_1021_3_40x40.png` |
| imageres_mun | RT_ICON | 1633 (group 1021) | 32x32 | 32 | `icons/icon_1021_4_32x32.png` |
| imageres_mun | RT_ICON | 1634 (group 1021) | 24x24 | 32 | `icons/icon_1021_5_24x24.png` |
| imageres_mun | RT_ICON | 1635 (group 1021) | 20x20 | 32 | `icons/icon_1021_6_20x20.png` |
| imageres_mun | RT_ICON | 1636 (group 1021) | 16x16 | 32 | `icons/icon_1021_7_16x16.png` |
| imageres_mun | RT_ICON | 1637 (group 1022) | 256x256 | 32 | `icons/icon_1022_0_256x256.png` |
| imageres_mun | RT_ICON | 1638 (group 1022) | 64x64 | 32 | `icons/icon_1022_1_64x64.png` |
| imageres_mun | RT_ICON | 1639 (group 1022) | 48x48 | 32 | `icons/icon_1022_2_48x48.png` |
| imageres_mun | RT_ICON | 164 (group 80) | 40x40 | 32 | `icons/icon_80_3_40x40.png` |
| imageres_mun | RT_ICON | 1640 (group 1022) | 40x40 | 32 | `icons/icon_1022_3_40x40.png` |
| imageres_mun | RT_ICON | 1641 (group 1022) | 32x32 | 32 | `icons/icon_1022_4_32x32.png` |
| imageres_mun | RT_ICON | 1642 (group 1022) | 24x24 | 32 | `icons/icon_1022_5_24x24.png` |
| imageres_mun | RT_ICON | 1643 (group 1022) | 20x20 | 32 | `icons/icon_1022_6_20x20.png` |
| imageres_mun | RT_ICON | 1644 (group 1022) | 16x16 | 32 | `icons/icon_1022_7_16x16.png` |
| imageres_mun | RT_ICON | 1645 (group 1023) | 256x256 | 32 | `icons/icon_1023_0_256x256.png` |
| imageres_mun | RT_ICON | 1646 (group 1023) | 64x64 | 32 | `icons/icon_1023_1_64x64.png` |
| imageres_mun | RT_ICON | 1647 (group 1023) | 48x48 | 32 | `icons/icon_1023_2_48x48.png` |
| imageres_mun | RT_ICON | 1648 (group 1023) | 40x40 | 32 | `icons/icon_1023_3_40x40.png` |
| imageres_mun | RT_ICON | 1649 (group 1023) | 32x32 | 32 | `icons/icon_1023_4_32x32.png` |
| imageres_mun | RT_ICON | 165 (group 80) | 32x32 | 32 | `icons/icon_80_4_32x32.png` |
| imageres_mun | RT_ICON | 1650 (group 1023) | 24x24 | 32 | `icons/icon_1023_5_24x24.png` |
| imageres_mun | RT_ICON | 1651 (group 1023) | 20x20 | 32 | `icons/icon_1023_6_20x20.png` |
| imageres_mun | RT_ICON | 1652 (group 1023) | 16x16 | 32 | `icons/icon_1023_7_16x16.png` |
| imageres_mun | RT_ICON | 1653 (group 1025) | 256x256 | 32 | `icons/icon_1025_0_256x256.png` |
| imageres_mun | RT_ICON | 1654 (group 1025) | 64x64 | 32 | `icons/icon_1025_1_64x64.png` |
| imageres_mun | RT_ICON | 1655 (group 1025) | 48x48 | 32 | `icons/icon_1025_2_48x48.png` |
| imageres_mun | RT_ICON | 1656 (group 1025) | 40x40 | 32 | `icons/icon_1025_3_40x40.png` |
| imageres_mun | RT_ICON | 1657 (group 1025) | 32x32 | 32 | `icons/icon_1025_4_32x32.png` |
| imageres_mun | RT_ICON | 1658 (group 1025) | 24x24 | 32 | `icons/icon_1025_5_24x24.png` |
| imageres_mun | RT_ICON | 1659 (group 1025) | 20x20 | 32 | `icons/icon_1025_6_20x20.png` |
| imageres_mun | RT_ICON | 166 (group 80) | 24x24 | 32 | `icons/icon_80_5_24x24.png` |
| imageres_mun | RT_ICON | 1660 (group 1025) | 16x16 | 32 | `icons/icon_1025_7_16x16.png` |
| imageres_mun | RT_ICON | 1661 (group 1024) | 256x256 | 32 | `icons/icon_1024_0_256x256.png` |
| imageres_mun | RT_ICON | 1662 (group 1024) | 64x64 | 32 | `icons/icon_1024_1_64x64.png` |
| imageres_mun | RT_ICON | 1663 (group 1024) | 48x48 | 32 | `icons/icon_1024_2_48x48.png` |
| imageres_mun | RT_ICON | 1664 (group 1024) | 40x40 | 32 | `icons/icon_1024_3_40x40.png` |
| imageres_mun | RT_ICON | 1665 (group 1024) | 32x32 | 32 | `icons/icon_1024_4_32x32.png` |
| imageres_mun | RT_ICON | 1666 (group 1024) | 24x24 | 32 | `icons/icon_1024_5_24x24.png` |
| imageres_mun | RT_ICON | 1667 (group 1024) | 20x20 | 32 | `icons/icon_1024_6_20x20.png` |
| imageres_mun | RT_ICON | 1668 (group 1024) | 16x16 | 32 | `icons/icon_1024_7_16x16.png` |
| imageres_mun | RT_ICON | 1669 (group 1026) | 256x256 | 32 | `icons/icon_1026_0_256x256.png` |
| imageres_mun | RT_ICON | 167 (group 80) | 20x20 | 32 | `icons/icon_80_6_20x20.png` |
| imageres_mun | RT_ICON | 1670 (group 1026) | 64x64 | 32 | `icons/icon_1026_1_64x64.png` |
| imageres_mun | RT_ICON | 1671 (group 1026) | 48x48 | 32 | `icons/icon_1026_2_48x48.png` |
| imageres_mun | RT_ICON | 1672 (group 1026) | 40x40 | 32 | `icons/icon_1026_3_40x40.png` |
| imageres_mun | RT_ICON | 1673 (group 1026) | 32x32 | 32 | `icons/icon_1026_4_32x32.png` |
| imageres_mun | RT_ICON | 1674 (group 1026) | 24x24 | 32 | `icons/icon_1026_5_24x24.png` |
| imageres_mun | RT_ICON | 1675 (group 1026) | 20x20 | 32 | `icons/icon_1026_6_20x20.png` |
| imageres_mun | RT_ICON | 1676 (group 1026) | 16x16 | 32 | `icons/icon_1026_7_16x16.png` |
| imageres_mun | RT_ICON | 1677 (group 1027) | 256x256 | 32 | `icons/icon_1027_0_256x256.png` |
| imageres_mun | RT_ICON | 1678 (group 1027) | 64x64 | 32 | `icons/icon_1027_1_64x64.png` |
| imageres_mun | RT_ICON | 1679 (group 1027) | 48x48 | 32 | `icons/icon_1027_2_48x48.png` |
| imageres_mun | RT_ICON | 168 (group 80) | 16x16 | 32 | `icons/icon_80_7_16x16.png` |
| imageres_mun | RT_ICON | 1680 (group 1027) | 40x40 | 32 | `icons/icon_1027_3_40x40.png` |
| imageres_mun | RT_ICON | 1681 (group 1027) | 32x32 | 32 | `icons/icon_1027_4_32x32.png` |
| imageres_mun | RT_ICON | 1682 (group 1027) | 24x24 | 32 | `icons/icon_1027_5_24x24.png` |
| imageres_mun | RT_ICON | 1683 (group 1027) | 20x20 | 32 | `icons/icon_1027_6_20x20.png` |
| imageres_mun | RT_ICON | 1684 (group 1027) | 16x16 | 32 | `icons/icon_1027_7_16x16.png` |
| imageres_mun | RT_ICON | 1685 (group 1029) | 256x256 | 32 | `icons/icon_1029_0_256x256.png` |
| imageres_mun | RT_ICON | 1686 (group 1029) | 64x64 | 32 | `icons/icon_1029_1_64x64.png` |
| imageres_mun | RT_ICON | 1687 (group 1029) | 48x48 | 32 | `icons/icon_1029_2_48x48.png` |
| imageres_mun | RT_ICON | 1688 (group 1029) | 40x40 | 32 | `icons/icon_1029_3_40x40.png` |
| imageres_mun | RT_ICON | 1689 (group 1029) | 32x32 | 32 | `icons/icon_1029_4_32x32.png` |
| imageres_mun | RT_ICON | 169 (group 27) | 256x256 | 32 | `icons/icon_27_0_256x256.png` |
| imageres_mun | RT_ICON | 1690 (group 1029) | 24x24 | 32 | `icons/icon_1029_5_24x24.png` |
| imageres_mun | RT_ICON | 1691 (group 1029) | 20x20 | 32 | `icons/icon_1029_6_20x20.png` |
| imageres_mun | RT_ICON | 1692 (group 1029) | 16x16 | 32 | `icons/icon_1029_7_16x16.png` |
| imageres_mun | RT_ICON | 1693 (group 5205) | 256x256 | 32 | `icons/icon_5205_0_256x256.png` |
| imageres_mun | RT_ICON | 1694 (group 5205) | 64x64 | 32 | `icons/icon_5205_1_64x64.png` |
| imageres_mun | RT_ICON | 1695 (group 5205) | 48x48 | 32 | `icons/icon_5205_2_48x48.png` |
| imageres_mun | RT_ICON | 1696 (group 5205) | 40x40 | 32 | `icons/icon_5205_3_40x40.png` |
| imageres_mun | RT_ICON | 1697 (group 5205) | 32x32 | 32 | `icons/icon_5205_4_32x32.png` |
| imageres_mun | RT_ICON | 1698 (group 5205) | 24x24 | 32 | `icons/icon_5205_5_24x24.png` |
| imageres_mun | RT_ICON | 1699 (group 5205) | 20x20 | 32 | `icons/icon_5205_6_20x20.png` |
| imageres_mun | RT_ICON | 17 (group 4) | 256x256 | 32 | `icons/icon_4_0_256x256.png` |
| imageres_mun | RT_ICON | 170 (group 27) | 64x64 | 32 | `icons/icon_27_1_64x64.png` |
| imageres_mun | RT_ICON | 1700 (group 5205) | 16x16 | 32 | `icons/icon_5205_7_16x16.png` |
| imageres_mun | RT_ICON | 1701 (group 1041) | 256x256 | 32 | `icons/icon_1041_0_256x256.png` |
| imageres_mun | RT_ICON | 1702 (group 1041) | 64x64 | 32 | `icons/icon_1041_1_64x64.png` |
| imageres_mun | RT_ICON | 1703 (group 1041) | 48x48 | 32 | `icons/icon_1041_2_48x48.png` |
| imageres_mun | RT_ICON | 1704 (group 1041) | 40x40 | 32 | `icons/icon_1041_3_40x40.png` |
| imageres_mun | RT_ICON | 1705 (group 1041) | 32x32 | 32 | `icons/icon_1041_4_32x32.png` |
| imageres_mun | RT_ICON | 1706 (group 1041) | 24x24 | 32 | `icons/icon_1041_5_24x24.png` |
| imageres_mun | RT_ICON | 1707 (group 1041) | 20x20 | 32 | `icons/icon_1041_6_20x20.png` |
| imageres_mun | RT_ICON | 1708 (group 1041) | 16x16 | 32 | `icons/icon_1041_7_16x16.png` |
| imageres_mun | RT_ICON | 1709 (group 1042) | 256x256 | 32 | `icons/icon_1042_0_256x256.png` |
| imageres_mun | RT_ICON | 171 (group 27) | 48x48 | 32 | `icons/icon_27_2_48x48.png` |
| imageres_mun | RT_ICON | 1710 (group 1042) | 64x64 | 32 | `icons/icon_1042_1_64x64.png` |
| imageres_mun | RT_ICON | 1711 (group 1042) | 48x48 | 32 | `icons/icon_1042_2_48x48.png` |
| imageres_mun | RT_ICON | 1712 (group 1042) | 40x40 | 32 | `icons/icon_1042_3_40x40.png` |
| imageres_mun | RT_ICON | 1713 (group 1042) | 32x32 | 32 | `icons/icon_1042_4_32x32.png` |
| imageres_mun | RT_ICON | 1714 (group 1042) | 24x24 | 32 | `icons/icon_1042_5_24x24.png` |
| imageres_mun | RT_ICON | 1715 (group 1042) | 20x20 | 32 | `icons/icon_1042_6_20x20.png` |
| imageres_mun | RT_ICON | 1716 (group 1042) | 16x16 | 32 | `icons/icon_1042_7_16x16.png` |
| imageres_mun | RT_ICON | 1717 (group 198) | 256x256 | 32 | `icons/icon_198_0_256x256.png` |
| imageres_mun | RT_ICON | 1718 (group 198) | 64x64 | 32 | `icons/icon_198_1_64x64.png` |
| imageres_mun | RT_ICON | 1719 (group 198) | 48x48 | 32 | `icons/icon_198_2_48x48.png` |
| imageres_mun | RT_ICON | 172 (group 27) | 40x40 | 32 | `icons/icon_27_3_40x40.png` |
| imageres_mun | RT_ICON | 1720 (group 198) | 40x40 | 32 | `icons/icon_198_3_40x40.png` |
| imageres_mun | RT_ICON | 1721 (group 198) | 32x32 | 32 | `icons/icon_198_4_32x32.png` |
| imageres_mun | RT_ICON | 1722 (group 198) | 24x24 | 32 | `icons/icon_198_5_24x24.png` |
| imageres_mun | RT_ICON | 1723 (group 198) | 20x20 | 32 | `icons/icon_198_6_20x20.png` |
| imageres_mun | RT_ICON | 1724 (group 198) | 16x16 | 32 | `icons/icon_198_7_16x16.png` |
| imageres_mun | RT_ICON | 1725 (group 5100) | 256x256 | 32 | `icons/icon_5100_0_256x256.png` |
| imageres_mun | RT_ICON | 1726 (group 5100) | 64x64 | 32 | `icons/icon_5100_1_64x64.png` |
| imageres_mun | RT_ICON | 1727 (group 5100) | 48x48 | 32 | `icons/icon_5100_2_48x48.png` |
| imageres_mun | RT_ICON | 1728 (group 5100) | 40x40 | 32 | `icons/icon_5100_3_40x40.png` |
| imageres_mun | RT_ICON | 1729 (group 5100) | 32x32 | 32 | `icons/icon_5100_4_32x32.png` |
| imageres_mun | RT_ICON | 173 (group 27) | 32x32 | 32 | `icons/icon_27_4_32x32.png` |
| imageres_mun | RT_ICON | 1730 (group 5100) | 24x24 | 32 | `icons/icon_5100_5_24x24.png` |
| imageres_mun | RT_ICON | 1731 (group 5100) | 20x20 | 32 | `icons/icon_5100_6_20x20.png` |
| imageres_mun | RT_ICON | 1732 (group 5100) | 16x16 | 32 | `icons/icon_5100_7_16x16.png` |
| imageres_mun | RT_ICON | 1733 (group 5101) | 256x256 | 32 | `icons/icon_5101_0_256x256.png` |
| imageres_mun | RT_ICON | 1734 (group 5101) | 128x128 | 32 | `icons/icon_5101_1_128x128.png` |
| imageres_mun | RT_ICON | 1735 (group 5101) | 96x96 | 32 | `icons/icon_5101_2_96x96.png` |
| imageres_mun | RT_ICON | 1736 (group 5101) | 80x80 | 32 | `icons/icon_5101_3_80x80.png` |
| imageres_mun | RT_ICON | 1737 (group 5101) | 64x64 | 32 | `icons/icon_5101_4_64x64.png` |
| imageres_mun | RT_ICON | 1738 (group 5101) | 48x48 | 32 | `icons/icon_5101_5_48x48.png` |
| imageres_mun | RT_ICON | 1739 (group 5101) | 40x40 | 32 | `icons/icon_5101_6_40x40.png` |
| imageres_mun | RT_ICON | 174 (group 27) | 24x24 | 32 | `icons/icon_27_5_24x24.png` |
| imageres_mun | RT_ICON | 1740 (group 5101) | 32x32 | 32 | `icons/icon_5101_7_32x32.png` |
| imageres_mun | RT_ICON | 1741 (group 5101) | 24x24 | 32 | `icons/icon_5101_8_24x24.png` |
| imageres_mun | RT_ICON | 1742 (group 5101) | 20x20 | 32 | `icons/icon_5101_9_20x20.png` |
| imageres_mun | RT_ICON | 1743 (group 5101) | 16x16 | 32 | `icons/icon_5101_10_16x16.png` |
| imageres_mun | RT_ICON | 1744 (group 5102) | 32x32 | 4 | `icons/icon_5102_0_32x32.png` |
| imageres_mun | RT_ICON | 1745 (group 5102) | 24x24 | 4 | `icons/icon_5102_1_24x24.png` |
| imageres_mun | RT_ICON | 1746 (group 5102) | 20x20 | 4 | `icons/icon_5102_2_20x20.png` |
| imageres_mun | RT_ICON | 1747 (group 5102) | 16x16 | 4 | `icons/icon_5102_3_16x16.png` |
| imageres_mun | RT_ICON | 1748 (group 5102) | 32x32 | 8 | `icons/icon_5102_4_32x32.png` |
| imageres_mun | RT_ICON | 1749 (group 5102) | 24x24 | 8 | `icons/icon_5102_5_24x24.png` |
| imageres_mun | RT_ICON | 175 (group 27) | 20x20 | 32 | `icons/icon_27_6_20x20.png` |
| imageres_mun | RT_ICON | 1750 (group 5102) | 20x20 | 8 | `icons/icon_5102_6_20x20.png` |
| imageres_mun | RT_ICON | 1751 (group 5102) | 16x16 | 8 | `icons/icon_5102_7_16x16.png` |
| imageres_mun | RT_ICON | 1752 (group 5102) | 32x32 | 32 | `icons/icon_5102_8_32x32.png` |
| imageres_mun | RT_ICON | 1753 (group 5102) | 24x24 | 32 | `icons/icon_5102_9_24x24.png` |
| imageres_mun | RT_ICON | 1754 (group 5102) | 20x20 | 32 | `icons/icon_5102_10_20x20.png` |
| imageres_mun | RT_ICON | 1755 (group 5102) | 16x16 | 32 | `icons/icon_5102_11_16x16.png` |
| imageres_mun | RT_ICON | 1756 (group 5201) | 48x48 | 4 | `icons/icon_5201_0_48x48.png` |
| imageres_mun | RT_ICON | 1757 (group 5201) | 32x32 | 4 | `icons/icon_5201_1_32x32.png` |
| imageres_mun | RT_ICON | 1758 (group 5201) | 16x16 | 4 | `icons/icon_5201_2_16x16.png` |
| imageres_mun | RT_ICON | 1759 (group 5201) | 48x48 | 8 | `icons/icon_5201_3_48x48.png` |
| imageres_mun | RT_ICON | 176 (group 27) | 16x16 | 32 | `icons/icon_27_7_16x16.png` |
| imageres_mun | RT_ICON | 1760 (group 5201) | 32x32 | 8 | `icons/icon_5201_4_32x32.png` |
| imageres_mun | RT_ICON | 1761 (group 5201) | 16x16 | 8 | `icons/icon_5201_5_16x16.png` |
| imageres_mun | RT_ICON | 1762 (group 5201) | 256x256 | 32 | `icons/icon_5201_6_256x256.png` |
| imageres_mun | RT_ICON | 1763 (group 5201) | 48x48 | 32 | `icons/icon_5201_7_48x48.png` |
| imageres_mun | RT_ICON | 1764 (group 5201) | 32x32 | 32 | `icons/icon_5201_8_32x32.png` |
| imageres_mun | RT_ICON | 1765 (group 5201) | 16x16 | 32 | `icons/icon_5201_9_16x16.png` |
| imageres_mun | RT_ICON | 1766 (group 5203) | 256x256 | 32 | `icons/icon_5203_0_256x256.png` |
| imageres_mun | RT_ICON | 1767 (group 5203) | 64x64 | 32 | `icons/icon_5203_1_64x64.png` |
| imageres_mun | RT_ICON | 1768 (group 5203) | 48x48 | 32 | `icons/icon_5203_2_48x48.png` |
| imageres_mun | RT_ICON | 1769 (group 5203) | 40x40 | 32 | `icons/icon_5203_3_40x40.png` |
| imageres_mun | RT_ICON | 177 (group 28) | 256x256 | 32 | `icons/icon_28_0_256x256.png` |
| imageres_mun | RT_ICON | 1770 (group 5203) | 32x32 | 32 | `icons/icon_5203_4_32x32.png` |
| imageres_mun | RT_ICON | 1771 (group 5203) | 24x24 | 32 | `icons/icon_5203_5_24x24.png` |
| imageres_mun | RT_ICON | 1772 (group 5203) | 20x20 | 32 | `icons/icon_5203_6_20x20.png` |
| imageres_mun | RT_ICON | 1773 (group 5203) | 16x16 | 32 | `icons/icon_5203_7_16x16.png` |
| imageres_mun | RT_ICON | 1774 (group 5210) | 54x54 | 32 | `icons/icon_5210_0_54x54.png` |
| imageres_mun | RT_ICON | 1775 (group 5210) | 42x42 | 32 | `icons/icon_5210_1_42x42.png` |
| imageres_mun | RT_ICON | 1776 (group 5210) | 36x36 | 32 | `icons/icon_5210_2_36x36.png` |
| imageres_mun | RT_ICON | 1777 (group 5210) | 30x30 | 32 | `icons/icon_5210_3_30x30.png` |
| imageres_mun | RT_ICON | 1778 (group 5210) | 28x28 | 32 | `icons/icon_5210_4_28x28.png` |
| imageres_mun | RT_ICON | 1779 (group 5210) | 20x20 | 32 | `icons/icon_5210_5_20x20.png` |
| imageres_mun | RT_ICON | 178 (group 28) | 64x64 | 32 | `icons/icon_28_1_64x64.png` |
| imageres_mun | RT_ICON | 1780 (group 5301) | 256x256 | 32 | `icons/icon_5301_0_256x256.png` |
| imageres_mun | RT_ICON | 1781 (group 5301) | 64x64 | 32 | `icons/icon_5301_1_64x64.png` |
| imageres_mun | RT_ICON | 1782 (group 5301) | 48x48 | 32 | `icons/icon_5301_2_48x48.png` |
| imageres_mun | RT_ICON | 1783 (group 5301) | 40x40 | 32 | `icons/icon_5301_3_40x40.png` |
| imageres_mun | RT_ICON | 1784 (group 5301) | 32x32 | 32 | `icons/icon_5301_4_32x32.png` |
| imageres_mun | RT_ICON | 1785 (group 5301) | 24x24 | 32 | `icons/icon_5301_5_24x24.png` |
| imageres_mun | RT_ICON | 1786 (group 5301) | 20x20 | 32 | `icons/icon_5301_6_20x20.png` |
| imageres_mun | RT_ICON | 1787 (group 5301) | 16x16 | 32 | `icons/icon_5301_7_16x16.png` |
| imageres_mun | RT_ICON | 1788 (group 5302) | 256x256 | 32 | `icons/icon_5302_0_256x256.png` |
| imageres_mun | RT_ICON | 1789 (group 5302) | 64x64 | 32 | `icons/icon_5302_1_64x64.png` |
| imageres_mun | RT_ICON | 179 (group 28) | 48x48 | 32 | `icons/icon_28_2_48x48.png` |
| imageres_mun | RT_ICON | 1790 (group 5302) | 48x48 | 32 | `icons/icon_5302_2_48x48.png` |
| imageres_mun | RT_ICON | 1791 (group 5302) | 40x40 | 32 | `icons/icon_5302_3_40x40.png` |
| imageres_mun | RT_ICON | 1792 (group 5302) | 32x32 | 32 | `icons/icon_5302_4_32x32.png` |
| imageres_mun | RT_ICON | 1793 (group 5302) | 24x24 | 32 | `icons/icon_5302_5_24x24.png` |
| imageres_mun | RT_ICON | 1794 (group 5302) | 20x20 | 32 | `icons/icon_5302_6_20x20.png` |
| imageres_mun | RT_ICON | 1795 (group 5302) | 16x16 | 32 | `icons/icon_5302_7_16x16.png` |
| imageres_mun | RT_ICON | 1796 (group 5303) | 256x256 | 32 | `icons/icon_5303_0_256x256.png` |
| imageres_mun | RT_ICON | 1797 (group 5303) | 64x64 | 32 | `icons/icon_5303_1_64x64.png` |
| imageres_mun | RT_ICON | 1798 (group 5303) | 48x48 | 32 | `icons/icon_5303_2_48x48.png` |
| imageres_mun | RT_ICON | 1799 (group 5303) | 40x40 | 32 | `icons/icon_5303_3_40x40.png` |
| imageres_mun | RT_ICON | 18 (group 4) | 64x64 | 32 | `icons/icon_4_1_64x64.png` |
| imageres_mun | RT_ICON | 180 (group 28) | 40x40 | 32 | `icons/icon_28_3_40x40.png` |
| imageres_mun | RT_ICON | 1800 (group 5303) | 32x32 | 32 | `icons/icon_5303_4_32x32.png` |
| imageres_mun | RT_ICON | 1801 (group 5303) | 24x24 | 32 | `icons/icon_5303_5_24x24.png` |
| imageres_mun | RT_ICON | 1802 (group 5303) | 20x20 | 32 | `icons/icon_5303_6_20x20.png` |
| imageres_mun | RT_ICON | 1803 (group 5303) | 16x16 | 32 | `icons/icon_5303_7_16x16.png` |
| imageres_mun | RT_ICON | 1804 (group 5304) | 256x256 | 32 | `icons/icon_5304_0_256x256.png` |
| imageres_mun | RT_ICON | 1805 (group 5304) | 64x64 | 32 | `icons/icon_5304_1_64x64.png` |
| imageres_mun | RT_ICON | 1806 (group 5304) | 48x48 | 32 | `icons/icon_5304_2_48x48.png` |
| imageres_mun | RT_ICON | 1807 (group 5304) | 40x40 | 32 | `icons/icon_5304_3_40x40.png` |
| imageres_mun | RT_ICON | 1808 (group 5304) | 32x32 | 32 | `icons/icon_5304_4_32x32.png` |
| imageres_mun | RT_ICON | 1809 (group 5304) | 24x24 | 32 | `icons/icon_5304_5_24x24.png` |
| imageres_mun | RT_ICON | 181 (group 28) | 32x32 | 32 | `icons/icon_28_4_32x32.png` |
| imageres_mun | RT_ICON | 1810 (group 5304) | 20x20 | 32 | `icons/icon_5304_6_20x20.png` |
| imageres_mun | RT_ICON | 1811 (group 5304) | 16x16 | 32 | `icons/icon_5304_7_16x16.png` |
| imageres_mun | RT_ICON | 1812 (group 5305) | 256x256 | 32 | `icons/icon_5305_0_256x256.png` |
| imageres_mun | RT_ICON | 1813 (group 5305) | 64x64 | 32 | `icons/icon_5305_1_64x64.png` |
| imageres_mun | RT_ICON | 1814 (group 5305) | 48x48 | 32 | `icons/icon_5305_2_48x48.png` |
| imageres_mun | RT_ICON | 1815 (group 5305) | 40x40 | 32 | `icons/icon_5305_3_40x40.png` |
| imageres_mun | RT_ICON | 1816 (group 5305) | 32x32 | 32 | `icons/icon_5305_4_32x32.png` |
| imageres_mun | RT_ICON | 1817 (group 5305) | 24x24 | 32 | `icons/icon_5305_5_24x24.png` |
| imageres_mun | RT_ICON | 1818 (group 5305) | 20x20 | 32 | `icons/icon_5305_6_20x20.png` |
| imageres_mun | RT_ICON | 1819 (group 5305) | 16x16 | 32 | `icons/icon_5305_7_16x16.png` |
| imageres_mun | RT_ICON | 182 (group 28) | 24x24 | 32 | `icons/icon_28_5_24x24.png` |
| imageres_mun | RT_ICON | 1820 (group 5306) | 256x256 | 32 | `icons/icon_5306_0_256x256.png` |
| imageres_mun | RT_ICON | 1821 (group 5306) | 64x64 | 32 | `icons/icon_5306_1_64x64.png` |
| imageres_mun | RT_ICON | 1822 (group 5306) | 48x48 | 32 | `icons/icon_5306_2_48x48.png` |
| imageres_mun | RT_ICON | 1823 (group 5306) | 40x40 | 32 | `icons/icon_5306_3_40x40.png` |
| imageres_mun | RT_ICON | 1824 (group 5306) | 32x32 | 32 | `icons/icon_5306_4_32x32.png` |
| imageres_mun | RT_ICON | 1825 (group 5306) | 24x24 | 32 | `icons/icon_5306_5_24x24.png` |
| imageres_mun | RT_ICON | 1826 (group 5306) | 20x20 | 32 | `icons/icon_5306_6_20x20.png` |
| imageres_mun | RT_ICON | 1827 (group 5306) | 16x16 | 32 | `icons/icon_5306_7_16x16.png` |
| imageres_mun | RT_ICON | 1828 (group 5307) | 256x256 | 32 | `icons/icon_5307_0_256x256.png` |
| imageres_mun | RT_ICON | 1829 (group 5307) | 64x64 | 32 | `icons/icon_5307_1_64x64.png` |
| imageres_mun | RT_ICON | 183 (group 28) | 20x20 | 32 | `icons/icon_28_6_20x20.png` |
| imageres_mun | RT_ICON | 1830 (group 5307) | 48x48 | 32 | `icons/icon_5307_2_48x48.png` |
| imageres_mun | RT_ICON | 1831 (group 5307) | 40x40 | 32 | `icons/icon_5307_3_40x40.png` |
| imageres_mun | RT_ICON | 1832 (group 5307) | 32x32 | 32 | `icons/icon_5307_4_32x32.png` |
| imageres_mun | RT_ICON | 1833 (group 5307) | 24x24 | 32 | `icons/icon_5307_5_24x24.png` |
| imageres_mun | RT_ICON | 1834 (group 5307) | 20x20 | 32 | `icons/icon_5307_6_20x20.png` |
| imageres_mun | RT_ICON | 1835 (group 5307) | 16x16 | 32 | `icons/icon_5307_7_16x16.png` |
| imageres_mun | RT_ICON | 1836 (group 5308) | 256x256 | 32 | `icons/icon_5308_0_256x256.png` |
| imageres_mun | RT_ICON | 1837 (group 5308) | 64x64 | 32 | `icons/icon_5308_1_64x64.png` |
| imageres_mun | RT_ICON | 1838 (group 5308) | 48x48 | 32 | `icons/icon_5308_2_48x48.png` |
| imageres_mun | RT_ICON | 1839 (group 5308) | 40x40 | 32 | `icons/icon_5308_3_40x40.png` |
| imageres_mun | RT_ICON | 184 (group 28) | 16x16 | 32 | `icons/icon_28_7_16x16.png` |
| imageres_mun | RT_ICON | 1840 (group 5308) | 32x32 | 32 | `icons/icon_5308_4_32x32.png` |
| imageres_mun | RT_ICON | 1841 (group 5308) | 24x24 | 32 | `icons/icon_5308_5_24x24.png` |
| imageres_mun | RT_ICON | 1842 (group 5308) | 20x20 | 32 | `icons/icon_5308_6_20x20.png` |
| imageres_mun | RT_ICON | 1843 (group 5308) | 16x16 | 32 | `icons/icon_5308_7_16x16.png` |
| imageres_mun | RT_ICON | 1844 (group 5309) | 256x256 | 32 | `icons/icon_5309_0_256x256.png` |
| imageres_mun | RT_ICON | 1845 (group 5309) | 64x64 | 32 | `icons/icon_5309_1_64x64.png` |
| imageres_mun | RT_ICON | 1846 (group 5309) | 48x48 | 32 | `icons/icon_5309_2_48x48.png` |
| imageres_mun | RT_ICON | 1847 (group 5309) | 40x40 | 32 | `icons/icon_5309_3_40x40.png` |
| imageres_mun | RT_ICON | 1848 (group 5309) | 32x32 | 32 | `icons/icon_5309_4_32x32.png` |
| imageres_mun | RT_ICON | 1849 (group 5309) | 24x24 | 32 | `icons/icon_5309_5_24x24.png` |
| imageres_mun | RT_ICON | 185 (group 29) | 256x256 | 32 | `icons/icon_29_0_256x256.png` |
| imageres_mun | RT_ICON | 1850 (group 5309) | 20x20 | 32 | `icons/icon_5309_6_20x20.png` |
| imageres_mun | RT_ICON | 1851 (group 5309) | 16x16 | 32 | `icons/icon_5309_7_16x16.png` |
| imageres_mun | RT_ICON | 1852 (group 5310) | 256x256 | 32 | `icons/icon_5310_0_256x256.png` |
| imageres_mun | RT_ICON | 1853 (group 5310) | 64x64 | 32 | `icons/icon_5310_1_64x64.png` |
| imageres_mun | RT_ICON | 1854 (group 5310) | 48x48 | 32 | `icons/icon_5310_2_48x48.png` |
| imageres_mun | RT_ICON | 1855 (group 5310) | 40x40 | 32 | `icons/icon_5310_3_40x40.png` |
| imageres_mun | RT_ICON | 1856 (group 5310) | 32x32 | 32 | `icons/icon_5310_4_32x32.png` |
| imageres_mun | RT_ICON | 1857 (group 5310) | 24x24 | 32 | `icons/icon_5310_5_24x24.png` |
| imageres_mun | RT_ICON | 1858 (group 5310) | 20x20 | 32 | `icons/icon_5310_6_20x20.png` |
| imageres_mun | RT_ICON | 1859 (group 5310) | 16x16 | 32 | `icons/icon_5310_7_16x16.png` |
| imageres_mun | RT_ICON | 186 (group 29) | 64x64 | 32 | `icons/icon_29_1_64x64.png` |
| imageres_mun | RT_ICON | 1860 (group 5311) | 256x256 | 32 | `icons/icon_5311_0_256x256.png` |
| imageres_mun | RT_ICON | 1861 (group 5311) | 64x64 | 32 | `icons/icon_5311_1_64x64.png` |
| imageres_mun | RT_ICON | 1862 (group 5311) | 48x48 | 32 | `icons/icon_5311_2_48x48.png` |
| imageres_mun | RT_ICON | 1863 (group 5311) | 40x40 | 32 | `icons/icon_5311_3_40x40.png` |
| imageres_mun | RT_ICON | 1864 (group 5311) | 32x32 | 32 | `icons/icon_5311_4_32x32.png` |
| imageres_mun | RT_ICON | 1865 (group 5311) | 24x24 | 32 | `icons/icon_5311_5_24x24.png` |
| imageres_mun | RT_ICON | 1866 (group 5311) | 20x20 | 32 | `icons/icon_5311_6_20x20.png` |
| imageres_mun | RT_ICON | 1867 (group 5311) | 16x16 | 32 | `icons/icon_5311_7_16x16.png` |
| imageres_mun | RT_ICON | 1868 (group 5312) | 256x256 | 32 | `icons/icon_5312_0_256x256.png` |
| imageres_mun | RT_ICON | 1869 (group 5312) | 64x64 | 32 | `icons/icon_5312_1_64x64.png` |
| imageres_mun | RT_ICON | 187 (group 29) | 48x48 | 32 | `icons/icon_29_2_48x48.png` |
| imageres_mun | RT_ICON | 1870 (group 5312) | 48x48 | 32 | `icons/icon_5312_2_48x48.png` |
| imageres_mun | RT_ICON | 1871 (group 5312) | 40x40 | 32 | `icons/icon_5312_3_40x40.png` |
| imageres_mun | RT_ICON | 1872 (group 5312) | 32x32 | 32 | `icons/icon_5312_4_32x32.png` |
| imageres_mun | RT_ICON | 1873 (group 5312) | 24x24 | 32 | `icons/icon_5312_5_24x24.png` |
| imageres_mun | RT_ICON | 1874 (group 5312) | 20x20 | 32 | `icons/icon_5312_6_20x20.png` |
| imageres_mun | RT_ICON | 1875 (group 5312) | 16x16 | 32 | `icons/icon_5312_7_16x16.png` |
| imageres_mun | RT_ICON | 1876 (group 5313) | 256x256 | 32 | `icons/icon_5313_0_256x256.png` |
| imageres_mun | RT_ICON | 1877 (group 5313) | 64x64 | 32 | `icons/icon_5313_1_64x64.png` |
| imageres_mun | RT_ICON | 1878 (group 5313) | 48x48 | 32 | `icons/icon_5313_2_48x48.png` |
| imageres_mun | RT_ICON | 1879 (group 5313) | 40x40 | 32 | `icons/icon_5313_3_40x40.png` |
| imageres_mun | RT_ICON | 188 (group 29) | 40x40 | 32 | `icons/icon_29_3_40x40.png` |
| imageres_mun | RT_ICON | 1880 (group 5313) | 32x32 | 32 | `icons/icon_5313_4_32x32.png` |
| imageres_mun | RT_ICON | 1881 (group 5313) | 24x24 | 32 | `icons/icon_5313_5_24x24.png` |
| imageres_mun | RT_ICON | 1882 (group 5313) | 20x20 | 32 | `icons/icon_5313_6_20x20.png` |
| imageres_mun | RT_ICON | 1883 (group 5313) | 16x16 | 32 | `icons/icon_5313_7_16x16.png` |
| imageres_mun | RT_ICON | 1884 (group 5314) | 256x256 | 32 | `icons/icon_5314_0_256x256.png` |
| imageres_mun | RT_ICON | 1885 (group 5314) | 64x64 | 32 | `icons/icon_5314_1_64x64.png` |
| imageres_mun | RT_ICON | 1886 (group 5314) | 48x48 | 32 | `icons/icon_5314_2_48x48.png` |
| imageres_mun | RT_ICON | 1887 (group 5314) | 40x40 | 32 | `icons/icon_5314_3_40x40.png` |
| imageres_mun | RT_ICON | 1888 (group 5314) | 32x32 | 32 | `icons/icon_5314_4_32x32.png` |
| imageres_mun | RT_ICON | 1889 (group 5314) | 24x24 | 32 | `icons/icon_5314_5_24x24.png` |
| imageres_mun | RT_ICON | 189 (group 29) | 32x32 | 32 | `icons/icon_29_4_32x32.png` |
| imageres_mun | RT_ICON | 1890 (group 5314) | 20x20 | 32 | `icons/icon_5314_6_20x20.png` |
| imageres_mun | RT_ICON | 1891 (group 5314) | 16x16 | 32 | `icons/icon_5314_7_16x16.png` |
| imageres_mun | RT_ICON | 1892 (group 5315) | 256x256 | 32 | `icons/icon_5315_0_256x256.png` |
| imageres_mun | RT_ICON | 1893 (group 5315) | 64x64 | 32 | `icons/icon_5315_1_64x64.png` |
| imageres_mun | RT_ICON | 1894 (group 5315) | 48x48 | 32 | `icons/icon_5315_2_48x48.png` |
| imageres_mun | RT_ICON | 1895 (group 5315) | 40x40 | 32 | `icons/icon_5315_3_40x40.png` |
| imageres_mun | RT_ICON | 1896 (group 5315) | 32x32 | 32 | `icons/icon_5315_4_32x32.png` |
| imageres_mun | RT_ICON | 1897 (group 5315) | 24x24 | 32 | `icons/icon_5315_5_24x24.png` |
| imageres_mun | RT_ICON | 1898 (group 5315) | 20x20 | 32 | `icons/icon_5315_6_20x20.png` |
| imageres_mun | RT_ICON | 1899 (group 5315) | 16x16 | 32 | `icons/icon_5315_7_16x16.png` |
| imageres_mun | RT_ICON | 19 (group 4) | 48x48 | 32 | `icons/icon_4_2_48x48.png` |
| imageres_mun | RT_ICON | 190 (group 29) | 24x24 | 32 | `icons/icon_29_5_24x24.png` |
| imageres_mun | RT_ICON | 1900 (group 5316) | 256x256 | 32 | `icons/icon_5316_0_256x256.png` |
| imageres_mun | RT_ICON | 1901 (group 5316) | 64x64 | 32 | `icons/icon_5316_1_64x64.png` |
| imageres_mun | RT_ICON | 1902 (group 5316) | 48x48 | 32 | `icons/icon_5316_2_48x48.png` |
| imageres_mun | RT_ICON | 1903 (group 5316) | 40x40 | 32 | `icons/icon_5316_3_40x40.png` |
| imageres_mun | RT_ICON | 1904 (group 5316) | 32x32 | 32 | `icons/icon_5316_4_32x32.png` |
| imageres_mun | RT_ICON | 1905 (group 5316) | 24x24 | 32 | `icons/icon_5316_5_24x24.png` |
| imageres_mun | RT_ICON | 1906 (group 5316) | 20x20 | 32 | `icons/icon_5316_6_20x20.png` |
| imageres_mun | RT_ICON | 1907 (group 5316) | 16x16 | 32 | `icons/icon_5316_7_16x16.png` |
| imageres_mun | RT_ICON | 1908 (group 5318) | 256x256 | 32 | `icons/icon_5318_0_256x256.png` |
| imageres_mun | RT_ICON | 1909 (group 5318) | 64x64 | 32 | `icons/icon_5318_1_64x64.png` |
| imageres_mun | RT_ICON | 191 (group 29) | 20x20 | 32 | `icons/icon_29_6_20x20.png` |
| imageres_mun | RT_ICON | 1910 (group 5318) | 48x48 | 32 | `icons/icon_5318_2_48x48.png` |
| imageres_mun | RT_ICON | 1911 (group 5318) | 40x40 | 32 | `icons/icon_5318_3_40x40.png` |
| imageres_mun | RT_ICON | 1912 (group 5318) | 32x32 | 32 | `icons/icon_5318_4_32x32.png` |
| imageres_mun | RT_ICON | 1913 (group 5318) | 24x24 | 32 | `icons/icon_5318_5_24x24.png` |
| imageres_mun | RT_ICON | 1914 (group 5318) | 20x20 | 32 | `icons/icon_5318_6_20x20.png` |
| imageres_mun | RT_ICON | 1915 (group 5318) | 16x16 | 32 | `icons/icon_5318_7_16x16.png` |
| imageres_mun | RT_ICON | 1916 (group 5319) | 256x256 | 32 | `icons/icon_5319_0_256x256.png` |
| imageres_mun | RT_ICON | 1917 (group 5319) | 64x64 | 32 | `icons/icon_5319_1_64x64.png` |
| imageres_mun | RT_ICON | 1918 (group 5319) | 48x48 | 32 | `icons/icon_5319_2_48x48.png` |
| imageres_mun | RT_ICON | 1919 (group 5319) | 40x40 | 32 | `icons/icon_5319_3_40x40.png` |
| imageres_mun | RT_ICON | 192 (group 29) | 16x16 | 32 | `icons/icon_29_7_16x16.png` |
| imageres_mun | RT_ICON | 1920 (group 5319) | 32x32 | 32 | `icons/icon_5319_4_32x32.png` |
| imageres_mun | RT_ICON | 1921 (group 5319) | 24x24 | 32 | `icons/icon_5319_5_24x24.png` |
| imageres_mun | RT_ICON | 1922 (group 5319) | 20x20 | 32 | `icons/icon_5319_6_20x20.png` |
| imageres_mun | RT_ICON | 1923 (group 5319) | 16x16 | 32 | `icons/icon_5319_7_16x16.png` |
| imageres_mun | RT_ICON | 1924 (group 5320) | 256x256 | 32 | `icons/icon_5320_0_256x256.png` |
| imageres_mun | RT_ICON | 1925 (group 5320) | 64x64 | 32 | `icons/icon_5320_1_64x64.png` |
| imageres_mun | RT_ICON | 1926 (group 5320) | 48x48 | 32 | `icons/icon_5320_2_48x48.png` |
| imageres_mun | RT_ICON | 1927 (group 5320) | 40x40 | 32 | `icons/icon_5320_3_40x40.png` |
| imageres_mun | RT_ICON | 1928 (group 5320) | 32x32 | 32 | `icons/icon_5320_4_32x32.png` |
| imageres_mun | RT_ICON | 1929 (group 5320) | 24x24 | 32 | `icons/icon_5320_5_24x24.png` |
| imageres_mun | RT_ICON | 193 (group 30) | 256x256 | 32 | `icons/icon_30_0_256x256.png` |
| imageres_mun | RT_ICON | 1930 (group 5320) | 20x20 | 32 | `icons/icon_5320_6_20x20.png` |
| imageres_mun | RT_ICON | 1931 (group 5320) | 16x16 | 32 | `icons/icon_5320_7_16x16.png` |
| imageres_mun | RT_ICON | 1932 (group 5321) | 256x256 | 32 | `icons/icon_5321_0_256x256.png` |
| imageres_mun | RT_ICON | 1933 (group 5321) | 64x64 | 32 | `icons/icon_5321_1_64x64.png` |
| imageres_mun | RT_ICON | 1934 (group 5321) | 48x48 | 32 | `icons/icon_5321_2_48x48.png` |
| imageres_mun | RT_ICON | 1935 (group 5321) | 40x40 | 32 | `icons/icon_5321_3_40x40.png` |
| imageres_mun | RT_ICON | 1936 (group 5321) | 32x32 | 32 | `icons/icon_5321_4_32x32.png` |
| imageres_mun | RT_ICON | 1937 (group 5321) | 24x24 | 32 | `icons/icon_5321_5_24x24.png` |
| imageres_mun | RT_ICON | 1938 (group 5321) | 20x20 | 32 | `icons/icon_5321_6_20x20.png` |
| imageres_mun | RT_ICON | 1939 (group 5321) | 16x16 | 32 | `icons/icon_5321_7_16x16.png` |
| imageres_mun | RT_ICON | 194 (group 30) | 64x64 | 32 | `icons/icon_30_1_64x64.png` |
| imageres_mun | RT_ICON | 1940 (group 5322) | 256x256 | 32 | `icons/icon_5322_0_256x256.png` |
| imageres_mun | RT_ICON | 1941 (group 5322) | 64x64 | 32 | `icons/icon_5322_1_64x64.png` |
| imageres_mun | RT_ICON | 1942 (group 5322) | 48x48 | 32 | `icons/icon_5322_2_48x48.png` |
| imageres_mun | RT_ICON | 1943 (group 5322) | 40x40 | 32 | `icons/icon_5322_3_40x40.png` |
| imageres_mun | RT_ICON | 1944 (group 5322) | 32x32 | 32 | `icons/icon_5322_4_32x32.png` |
| imageres_mun | RT_ICON | 1945 (group 5322) | 24x24 | 32 | `icons/icon_5322_5_24x24.png` |
| imageres_mun | RT_ICON | 1946 (group 5322) | 20x20 | 32 | `icons/icon_5322_6_20x20.png` |
| imageres_mun | RT_ICON | 1947 (group 5322) | 16x16 | 32 | `icons/icon_5322_7_16x16.png` |
| imageres_mun | RT_ICON | 1948 (group 5323) | 256x256 | 32 | `icons/icon_5323_0_256x256.png` |
| imageres_mun | RT_ICON | 1949 (group 5323) | 64x64 | 32 | `icons/icon_5323_1_64x64.png` |
| imageres_mun | RT_ICON | 195 (group 30) | 48x48 | 32 | `icons/icon_30_2_48x48.png` |
| imageres_mun | RT_ICON | 1950 (group 5323) | 48x48 | 32 | `icons/icon_5323_2_48x48.png` |
| imageres_mun | RT_ICON | 1951 (group 5323) | 40x40 | 32 | `icons/icon_5323_3_40x40.png` |
| imageres_mun | RT_ICON | 1952 (group 5323) | 32x32 | 32 | `icons/icon_5323_4_32x32.png` |
| imageres_mun | RT_ICON | 1953 (group 5323) | 24x24 | 32 | `icons/icon_5323_5_24x24.png` |
| imageres_mun | RT_ICON | 1954 (group 5323) | 20x20 | 32 | `icons/icon_5323_6_20x20.png` |
| imageres_mun | RT_ICON | 1955 (group 5323) | 16x16 | 32 | `icons/icon_5323_7_16x16.png` |
| imageres_mun | RT_ICON | 1956 (group 5324) | 256x256 | 32 | `icons/icon_5324_0_256x256.png` |
| imageres_mun | RT_ICON | 1957 (group 5324) | 64x64 | 32 | `icons/icon_5324_1_64x64.png` |
| imageres_mun | RT_ICON | 1958 (group 5324) | 48x48 | 32 | `icons/icon_5324_2_48x48.png` |
| imageres_mun | RT_ICON | 1959 (group 5324) | 40x40 | 32 | `icons/icon_5324_3_40x40.png` |
| imageres_mun | RT_ICON | 196 (group 30) | 40x40 | 32 | `icons/icon_30_3_40x40.png` |
| imageres_mun | RT_ICON | 1960 (group 5324) | 32x32 | 32 | `icons/icon_5324_4_32x32.png` |
| imageres_mun | RT_ICON | 1961 (group 5324) | 24x24 | 32 | `icons/icon_5324_5_24x24.png` |
| imageres_mun | RT_ICON | 1962 (group 5324) | 20x20 | 32 | `icons/icon_5324_6_20x20.png` |
| imageres_mun | RT_ICON | 1963 (group 5324) | 16x16 | 32 | `icons/icon_5324_7_16x16.png` |
| imageres_mun | RT_ICON | 1964 (group 5325) | 256x256 | 32 | `icons/icon_5325_0_256x256.png` |
| imageres_mun | RT_ICON | 1965 (group 5325) | 64x64 | 32 | `icons/icon_5325_1_64x64.png` |
| imageres_mun | RT_ICON | 1966 (group 5325) | 48x48 | 32 | `icons/icon_5325_2_48x48.png` |
| imageres_mun | RT_ICON | 1967 (group 5325) | 40x40 | 32 | `icons/icon_5325_3_40x40.png` |
| imageres_mun | RT_ICON | 1968 (group 5325) | 32x32 | 32 | `icons/icon_5325_4_32x32.png` |
| imageres_mun | RT_ICON | 1969 (group 5325) | 24x24 | 32 | `icons/icon_5325_5_24x24.png` |
| imageres_mun | RT_ICON | 197 (group 30) | 32x32 | 32 | `icons/icon_30_4_32x32.png` |
| imageres_mun | RT_ICON | 1970 (group 5325) | 20x20 | 32 | `icons/icon_5325_6_20x20.png` |
| imageres_mun | RT_ICON | 1971 (group 5325) | 16x16 | 32 | `icons/icon_5325_7_16x16.png` |
| imageres_mun | RT_ICON | 1972 (group 5326) | 256x256 | 32 | `icons/icon_5326_0_256x256.png` |
| imageres_mun | RT_ICON | 1973 (group 5326) | 64x64 | 32 | `icons/icon_5326_1_64x64.png` |
| imageres_mun | RT_ICON | 1974 (group 5326) | 48x48 | 32 | `icons/icon_5326_2_48x48.png` |
| imageres_mun | RT_ICON | 1975 (group 5326) | 40x40 | 32 | `icons/icon_5326_3_40x40.png` |
| imageres_mun | RT_ICON | 1976 (group 5326) | 32x32 | 32 | `icons/icon_5326_4_32x32.png` |
| imageres_mun | RT_ICON | 1977 (group 5326) | 24x24 | 32 | `icons/icon_5326_5_24x24.png` |
| imageres_mun | RT_ICON | 1978 (group 5326) | 20x20 | 32 | `icons/icon_5326_6_20x20.png` |
| imageres_mun | RT_ICON | 1979 (group 5326) | 16x16 | 32 | `icons/icon_5326_7_16x16.png` |
| imageres_mun | RT_ICON | 198 (group 30) | 24x24 | 32 | `icons/icon_30_5_24x24.png` |
| imageres_mun | RT_ICON | 1980 (group 5327) | 256x256 | 32 | `icons/icon_5327_0_256x256.png` |
| imageres_mun | RT_ICON | 1981 (group 5327) | 64x64 | 32 | `icons/icon_5327_1_64x64.png` |
| imageres_mun | RT_ICON | 1982 (group 5327) | 48x48 | 32 | `icons/icon_5327_2_48x48.png` |
| imageres_mun | RT_ICON | 1983 (group 5327) | 40x40 | 32 | `icons/icon_5327_3_40x40.png` |
| imageres_mun | RT_ICON | 1984 (group 5327) | 32x32 | 32 | `icons/icon_5327_4_32x32.png` |
| imageres_mun | RT_ICON | 1985 (group 5327) | 24x24 | 32 | `icons/icon_5327_5_24x24.png` |
| imageres_mun | RT_ICON | 1986 (group 5327) | 20x20 | 32 | `icons/icon_5327_6_20x20.png` |
| imageres_mun | RT_ICON | 1987 (group 5327) | 16x16 | 32 | `icons/icon_5327_7_16x16.png` |
| imageres_mun | RT_ICON | 1988 (group 5328) | 256x256 | 32 | `icons/icon_5328_0_256x256.png` |
| imageres_mun | RT_ICON | 1989 (group 5328) | 64x64 | 32 | `icons/icon_5328_1_64x64.png` |
| imageres_mun | RT_ICON | 199 (group 30) | 20x20 | 32 | `icons/icon_30_6_20x20.png` |
| imageres_mun | RT_ICON | 1990 (group 5328) | 48x48 | 32 | `icons/icon_5328_2_48x48.png` |
| imageres_mun | RT_ICON | 1991 (group 5328) | 40x40 | 32 | `icons/icon_5328_3_40x40.png` |
| imageres_mun | RT_ICON | 1992 (group 5328) | 32x32 | 32 | `icons/icon_5328_4_32x32.png` |
| imageres_mun | RT_ICON | 1993 (group 5328) | 24x24 | 32 | `icons/icon_5328_5_24x24.png` |
| imageres_mun | RT_ICON | 1994 (group 5328) | 20x20 | 32 | `icons/icon_5328_6_20x20.png` |
| imageres_mun | RT_ICON | 1995 (group 5328) | 16x16 | 32 | `icons/icon_5328_7_16x16.png` |
| imageres_mun | RT_ICON | 1996 (group 5329) | 256x256 | 32 | `icons/icon_5329_0_256x256.png` |
| imageres_mun | RT_ICON | 1997 (group 5329) | 64x64 | 32 | `icons/icon_5329_1_64x64.png` |
| imageres_mun | RT_ICON | 1998 (group 5329) | 48x48 | 32 | `icons/icon_5329_2_48x48.png` |
| imageres_mun | RT_ICON | 1999 (group 5329) | 40x40 | 32 | `icons/icon_5329_3_40x40.png` |
| imageres_mun | RT_ICON | 2 (group 2) | 64x64 | 32 | `icons/icon_2_1_64x64.png` |
| imageres_mun | RT_ICON | 20 (group 4) | 40x40 | 32 | `icons/icon_4_3_40x40.png` |
| imageres_mun | RT_ICON | 200 (group 30) | 16x16 | 32 | `icons/icon_30_7_16x16.png` |
| imageres_mun | RT_ICON | 2000 (group 5329) | 32x32 | 32 | `icons/icon_5329_4_32x32.png` |
| imageres_mun | RT_ICON | 2001 (group 5329) | 24x24 | 32 | `icons/icon_5329_5_24x24.png` |
| imageres_mun | RT_ICON | 2002 (group 5329) | 20x20 | 32 | `icons/icon_5329_6_20x20.png` |
| imageres_mun | RT_ICON | 2003 (group 5329) | 16x16 | 32 | `icons/icon_5329_7_16x16.png` |
| imageres_mun | RT_ICON | 2004 (group 5330) | 256x256 | 32 | `icons/icon_5330_0_256x256.png` |
| imageres_mun | RT_ICON | 2005 (group 5330) | 64x64 | 32 | `icons/icon_5330_1_64x64.png` |
| imageres_mun | RT_ICON | 2006 (group 5330) | 48x48 | 32 | `icons/icon_5330_2_48x48.png` |
| imageres_mun | RT_ICON | 2007 (group 5330) | 40x40 | 32 | `icons/icon_5330_3_40x40.png` |
| imageres_mun | RT_ICON | 2008 (group 5330) | 32x32 | 32 | `icons/icon_5330_4_32x32.png` |
| imageres_mun | RT_ICON | 2009 (group 5330) | 24x24 | 32 | `icons/icon_5330_5_24x24.png` |
| imageres_mun | RT_ICON | 201 (group 31) | 256x256 | 32 | `icons/icon_31_0_256x256.png` |
| imageres_mun | RT_ICON | 2010 (group 5330) | 20x20 | 32 | `icons/icon_5330_6_20x20.png` |
| imageres_mun | RT_ICON | 2011 (group 5330) | 16x16 | 32 | `icons/icon_5330_7_16x16.png` |
| imageres_mun | RT_ICON | 2012 (group 5331) | 256x256 | 32 | `icons/icon_5331_0_256x256.png` |
| imageres_mun | RT_ICON | 2013 (group 5331) | 64x64 | 32 | `icons/icon_5331_1_64x64.png` |
| imageres_mun | RT_ICON | 2014 (group 5331) | 48x48 | 32 | `icons/icon_5331_2_48x48.png` |
| imageres_mun | RT_ICON | 2015 (group 5331) | 40x40 | 32 | `icons/icon_5331_3_40x40.png` |
| imageres_mun | RT_ICON | 2016 (group 5331) | 32x32 | 32 | `icons/icon_5331_4_32x32.png` |
| imageres_mun | RT_ICON | 2017 (group 5331) | 24x24 | 32 | `icons/icon_5331_5_24x24.png` |
| imageres_mun | RT_ICON | 2018 (group 5331) | 20x20 | 32 | `icons/icon_5331_6_20x20.png` |
| imageres_mun | RT_ICON | 2019 (group 5331) | 16x16 | 32 | `icons/icon_5331_7_16x16.png` |
| imageres_mun | RT_ICON | 202 (group 31) | 64x64 | 32 | `icons/icon_31_1_64x64.png` |
| imageres_mun | RT_ICON | 2020 (group 5332) | 256x256 | 32 | `icons/icon_5332_0_256x256.png` |
| imageres_mun | RT_ICON | 2021 (group 5332) | 64x64 | 32 | `icons/icon_5332_1_64x64.png` |
| imageres_mun | RT_ICON | 2022 (group 5332) | 48x48 | 32 | `icons/icon_5332_2_48x48.png` |
| imageres_mun | RT_ICON | 2023 (group 5332) | 40x40 | 32 | `icons/icon_5332_3_40x40.png` |
| imageres_mun | RT_ICON | 2024 (group 5332) | 32x32 | 32 | `icons/icon_5332_4_32x32.png` |
| imageres_mun | RT_ICON | 2025 (group 5332) | 24x24 | 32 | `icons/icon_5332_5_24x24.png` |
| imageres_mun | RT_ICON | 2026 (group 5332) | 20x20 | 32 | `icons/icon_5332_6_20x20.png` |
| imageres_mun | RT_ICON | 2027 (group 5332) | 16x16 | 32 | `icons/icon_5332_7_16x16.png` |
| imageres_mun | RT_ICON | 2028 (group 5333) | 256x256 | 32 | `icons/icon_5333_0_256x256.png` |
| imageres_mun | RT_ICON | 2029 (group 5333) | 64x64 | 32 | `icons/icon_5333_1_64x64.png` |
| imageres_mun | RT_ICON | 203 (group 31) | 48x48 | 32 | `icons/icon_31_2_48x48.png` |
| imageres_mun | RT_ICON | 2030 (group 5333) | 48x48 | 32 | `icons/icon_5333_2_48x48.png` |
| imageres_mun | RT_ICON | 2031 (group 5333) | 40x40 | 32 | `icons/icon_5333_3_40x40.png` |
| imageres_mun | RT_ICON | 2032 (group 5333) | 32x32 | 32 | `icons/icon_5333_4_32x32.png` |
| imageres_mun | RT_ICON | 2033 (group 5333) | 24x24 | 32 | `icons/icon_5333_5_24x24.png` |
| imageres_mun | RT_ICON | 2034 (group 5333) | 20x20 | 32 | `icons/icon_5333_6_20x20.png` |
| imageres_mun | RT_ICON | 2035 (group 5333) | 16x16 | 32 | `icons/icon_5333_7_16x16.png` |
| imageres_mun | RT_ICON | 2036 (group 5334) | 256x256 | 32 | `icons/icon_5334_0_256x256.png` |
| imageres_mun | RT_ICON | 2037 (group 5334) | 64x64 | 32 | `icons/icon_5334_1_64x64.png` |
| imageres_mun | RT_ICON | 2038 (group 5334) | 48x48 | 32 | `icons/icon_5334_2_48x48.png` |
| imageres_mun | RT_ICON | 2039 (group 5334) | 40x40 | 32 | `icons/icon_5334_3_40x40.png` |
| imageres_mun | RT_ICON | 204 (group 31) | 40x40 | 32 | `icons/icon_31_3_40x40.png` |
| imageres_mun | RT_ICON | 2040 (group 5334) | 32x32 | 32 | `icons/icon_5334_4_32x32.png` |
| imageres_mun | RT_ICON | 2041 (group 5334) | 24x24 | 32 | `icons/icon_5334_5_24x24.png` |
| imageres_mun | RT_ICON | 2042 (group 5334) | 20x20 | 32 | `icons/icon_5334_6_20x20.png` |
| imageres_mun | RT_ICON | 2043 (group 5334) | 16x16 | 32 | `icons/icon_5334_7_16x16.png` |
| imageres_mun | RT_ICON | 2044 (group 5335) | 256x256 | 32 | `icons/icon_5335_0_256x256.png` |
| imageres_mun | RT_ICON | 2045 (group 5335) | 64x64 | 32 | `icons/icon_5335_1_64x64.png` |
| imageres_mun | RT_ICON | 2046 (group 5335) | 48x48 | 32 | `icons/icon_5335_2_48x48.png` |
| imageres_mun | RT_ICON | 2047 (group 5335) | 40x40 | 32 | `icons/icon_5335_3_40x40.png` |
| imageres_mun | RT_ICON | 2048 (group 5335) | 32x32 | 32 | `icons/icon_5335_4_32x32.png` |
| imageres_mun | RT_ICON | 2049 (group 5335) | 24x24 | 32 | `icons/icon_5335_5_24x24.png` |
| imageres_mun | RT_ICON | 205 (group 31) | 32x32 | 32 | `icons/icon_31_4_32x32.png` |
| imageres_mun | RT_ICON | 2050 (group 5335) | 20x20 | 32 | `icons/icon_5335_6_20x20.png` |
| imageres_mun | RT_ICON | 2051 (group 5335) | 16x16 | 32 | `icons/icon_5335_7_16x16.png` |
| imageres_mun | RT_ICON | 2052 (group 5336) | 256x256 | 32 | `icons/icon_5336_0_256x256.png` |
| imageres_mun | RT_ICON | 2053 (group 5336) | 64x64 | 32 | `icons/icon_5336_1_64x64.png` |
| imageres_mun | RT_ICON | 2054 (group 5336) | 48x48 | 32 | `icons/icon_5336_2_48x48.png` |
| imageres_mun | RT_ICON | 2055 (group 5336) | 40x40 | 32 | `icons/icon_5336_3_40x40.png` |
| imageres_mun | RT_ICON | 2056 (group 5336) | 32x32 | 32 | `icons/icon_5336_4_32x32.png` |
| imageres_mun | RT_ICON | 2057 (group 5336) | 24x24 | 32 | `icons/icon_5336_5_24x24.png` |
| imageres_mun | RT_ICON | 2058 (group 5336) | 20x20 | 32 | `icons/icon_5336_6_20x20.png` |
| imageres_mun | RT_ICON | 2059 (group 5336) | 16x16 | 32 | `icons/icon_5336_7_16x16.png` |
| imageres_mun | RT_ICON | 206 (group 31) | 24x24 | 32 | `icons/icon_31_5_24x24.png` |
| imageres_mun | RT_ICON | 2060 (group 5337) | 256x256 | 32 | `icons/icon_5337_0_256x256.png` |
| imageres_mun | RT_ICON | 2061 (group 5337) | 64x64 | 32 | `icons/icon_5337_1_64x64.png` |
| imageres_mun | RT_ICON | 2062 (group 5337) | 48x48 | 32 | `icons/icon_5337_2_48x48.png` |
| imageres_mun | RT_ICON | 2063 (group 5337) | 40x40 | 32 | `icons/icon_5337_3_40x40.png` |
| imageres_mun | RT_ICON | 2064 (group 5337) | 32x32 | 32 | `icons/icon_5337_4_32x32.png` |
| imageres_mun | RT_ICON | 2065 (group 5337) | 24x24 | 32 | `icons/icon_5337_5_24x24.png` |
| imageres_mun | RT_ICON | 2066 (group 5337) | 20x20 | 32 | `icons/icon_5337_6_20x20.png` |
| imageres_mun | RT_ICON | 2067 (group 5337) | 16x16 | 32 | `icons/icon_5337_7_16x16.png` |
| imageres_mun | RT_ICON | 2068 (group 5338) | 256x256 | 32 | `icons/icon_5338_0_256x256.png` |
| imageres_mun | RT_ICON | 2069 (group 5338) | 64x64 | 32 | `icons/icon_5338_1_64x64.png` |
| imageres_mun | RT_ICON | 207 (group 31) | 20x20 | 32 | `icons/icon_31_6_20x20.png` |
| imageres_mun | RT_ICON | 2070 (group 5338) | 48x48 | 32 | `icons/icon_5338_2_48x48.png` |
| imageres_mun | RT_ICON | 2071 (group 5338) | 40x40 | 32 | `icons/icon_5338_3_40x40.png` |
| imageres_mun | RT_ICON | 2072 (group 5338) | 32x32 | 32 | `icons/icon_5338_4_32x32.png` |
| imageres_mun | RT_ICON | 2073 (group 5338) | 24x24 | 32 | `icons/icon_5338_5_24x24.png` |
| imageres_mun | RT_ICON | 2074 (group 5338) | 20x20 | 32 | `icons/icon_5338_6_20x20.png` |
| imageres_mun | RT_ICON | 2075 (group 5338) | 16x16 | 32 | `icons/icon_5338_7_16x16.png` |
| imageres_mun | RT_ICON | 2076 (group 5339) | 256x256 | 32 | `icons/icon_5339_0_256x256.png` |
| imageres_mun | RT_ICON | 2077 (group 5339) | 64x64 | 32 | `icons/icon_5339_1_64x64.png` |
| imageres_mun | RT_ICON | 2078 (group 5339) | 48x48 | 32 | `icons/icon_5339_2_48x48.png` |
| imageres_mun | RT_ICON | 2079 (group 5339) | 40x40 | 32 | `icons/icon_5339_3_40x40.png` |
| imageres_mun | RT_ICON | 208 (group 31) | 16x16 | 32 | `icons/icon_31_7_16x16.png` |
| imageres_mun | RT_ICON | 2080 (group 5339) | 32x32 | 32 | `icons/icon_5339_4_32x32.png` |
| imageres_mun | RT_ICON | 2081 (group 5339) | 24x24 | 32 | `icons/icon_5339_5_24x24.png` |
| imageres_mun | RT_ICON | 2082 (group 5339) | 20x20 | 32 | `icons/icon_5339_6_20x20.png` |
| imageres_mun | RT_ICON | 2083 (group 5339) | 16x16 | 32 | `icons/icon_5339_7_16x16.png` |
| imageres_mun | RT_ICON | 2084 (group 5340) | 256x256 | 32 | `icons/icon_5340_0_256x256.png` |
| imageres_mun | RT_ICON | 2085 (group 5340) | 64x64 | 32 | `icons/icon_5340_1_64x64.png` |
| imageres_mun | RT_ICON | 2086 (group 5340) | 48x48 | 32 | `icons/icon_5340_2_48x48.png` |
| imageres_mun | RT_ICON | 2087 (group 5340) | 40x40 | 32 | `icons/icon_5340_3_40x40.png` |
| imageres_mun | RT_ICON | 2088 (group 5340) | 32x32 | 32 | `icons/icon_5340_4_32x32.png` |
| imageres_mun | RT_ICON | 2089 (group 5340) | 24x24 | 32 | `icons/icon_5340_5_24x24.png` |
| imageres_mun | RT_ICON | 209 (group 32) | 256x256 | 32 | `icons/icon_32_0_256x256.png` |
| imageres_mun | RT_ICON | 2090 (group 5340) | 20x20 | 32 | `icons/icon_5340_6_20x20.png` |
| imageres_mun | RT_ICON | 2091 (group 5340) | 16x16 | 32 | `icons/icon_5340_7_16x16.png` |
| imageres_mun | RT_ICON | 2092 (group 5341) | 256x256 | 32 | `icons/icon_5341_0_256x256.png` |
| imageres_mun | RT_ICON | 2093 (group 5341) | 64x64 | 32 | `icons/icon_5341_1_64x64.png` |
| imageres_mun | RT_ICON | 2094 (group 5341) | 48x48 | 32 | `icons/icon_5341_2_48x48.png` |
| imageres_mun | RT_ICON | 2095 (group 5341) | 40x40 | 32 | `icons/icon_5341_3_40x40.png` |
| imageres_mun | RT_ICON | 2096 (group 5341) | 32x32 | 32 | `icons/icon_5341_4_32x32.png` |
| imageres_mun | RT_ICON | 2097 (group 5341) | 24x24 | 32 | `icons/icon_5341_5_24x24.png` |
| imageres_mun | RT_ICON | 2098 (group 5341) | 20x20 | 32 | `icons/icon_5341_6_20x20.png` |
| imageres_mun | RT_ICON | 2099 (group 5341) | 16x16 | 32 | `icons/icon_5341_7_16x16.png` |
| imageres_mun | RT_ICON | 21 (group 4) | 32x32 | 32 | `icons/icon_4_4_32x32.png` |
| imageres_mun | RT_ICON | 210 (group 32) | 64x64 | 32 | `icons/icon_32_1_64x64.png` |
| imageres_mun | RT_ICON | 2100 (group 5342) | 256x256 | 32 | `icons/icon_5342_0_256x256.png` |
| imageres_mun | RT_ICON | 2101 (group 5342) | 64x64 | 32 | `icons/icon_5342_1_64x64.png` |
| imageres_mun | RT_ICON | 2102 (group 5342) | 48x48 | 32 | `icons/icon_5342_2_48x48.png` |
| imageres_mun | RT_ICON | 2103 (group 5342) | 40x40 | 32 | `icons/icon_5342_3_40x40.png` |
| imageres_mun | RT_ICON | 2104 (group 5342) | 32x32 | 32 | `icons/icon_5342_4_32x32.png` |
| imageres_mun | RT_ICON | 2105 (group 5342) | 24x24 | 32 | `icons/icon_5342_5_24x24.png` |
| imageres_mun | RT_ICON | 2106 (group 5342) | 20x20 | 32 | `icons/icon_5342_6_20x20.png` |
| imageres_mun | RT_ICON | 2107 (group 5342) | 16x16 | 32 | `icons/icon_5342_7_16x16.png` |
| imageres_mun | RT_ICON | 2108 (group 5343) | 256x256 | 32 | `icons/icon_5343_0_256x256.png` |
| imageres_mun | RT_ICON | 2109 (group 5343) | 64x64 | 32 | `icons/icon_5343_1_64x64.png` |
| imageres_mun | RT_ICON | 211 (group 32) | 48x48 | 32 | `icons/icon_32_2_48x48.png` |
| imageres_mun | RT_ICON | 2110 (group 5343) | 48x48 | 32 | `icons/icon_5343_2_48x48.png` |
| imageres_mun | RT_ICON | 2111 (group 5343) | 40x40 | 32 | `icons/icon_5343_3_40x40.png` |
| imageres_mun | RT_ICON | 2112 (group 5343) | 32x32 | 32 | `icons/icon_5343_4_32x32.png` |
| imageres_mun | RT_ICON | 2113 (group 5343) | 24x24 | 32 | `icons/icon_5343_5_24x24.png` |
| imageres_mun | RT_ICON | 2114 (group 5343) | 20x20 | 32 | `icons/icon_5343_6_20x20.png` |
| imageres_mun | RT_ICON | 2115 (group 5343) | 16x16 | 32 | `icons/icon_5343_7_16x16.png` |
| imageres_mun | RT_ICON | 2116 (group 5344) | 256x256 | 32 | `icons/icon_5344_0_256x256.png` |
| imageres_mun | RT_ICON | 2117 (group 5344) | 64x64 | 32 | `icons/icon_5344_1_64x64.png` |
| imageres_mun | RT_ICON | 2118 (group 5344) | 48x48 | 32 | `icons/icon_5344_2_48x48.png` |
| imageres_mun | RT_ICON | 2119 (group 5344) | 40x40 | 32 | `icons/icon_5344_3_40x40.png` |
| imageres_mun | RT_ICON | 212 (group 32) | 40x40 | 32 | `icons/icon_32_3_40x40.png` |
| imageres_mun | RT_ICON | 2120 (group 5344) | 32x32 | 32 | `icons/icon_5344_4_32x32.png` |
| imageres_mun | RT_ICON | 2121 (group 5344) | 24x24 | 32 | `icons/icon_5344_5_24x24.png` |
| imageres_mun | RT_ICON | 2122 (group 5344) | 20x20 | 32 | `icons/icon_5344_6_20x20.png` |
| imageres_mun | RT_ICON | 2123 (group 5344) | 16x16 | 32 | `icons/icon_5344_7_16x16.png` |
| imageres_mun | RT_ICON | 2124 (group 5345) | 256x256 | 32 | `icons/icon_5345_0_256x256.png` |
| imageres_mun | RT_ICON | 2125 (group 5345) | 64x64 | 32 | `icons/icon_5345_1_64x64.png` |
| imageres_mun | RT_ICON | 2126 (group 5345) | 48x48 | 32 | `icons/icon_5345_2_48x48.png` |
| imageres_mun | RT_ICON | 2127 (group 5345) | 40x40 | 32 | `icons/icon_5345_3_40x40.png` |
| imageres_mun | RT_ICON | 2128 (group 5345) | 32x32 | 32 | `icons/icon_5345_4_32x32.png` |
| imageres_mun | RT_ICON | 2129 (group 5345) | 24x24 | 32 | `icons/icon_5345_5_24x24.png` |
| imageres_mun | RT_ICON | 213 (group 32) | 32x32 | 32 | `icons/icon_32_4_32x32.png` |
| imageres_mun | RT_ICON | 2130 (group 5345) | 20x20 | 32 | `icons/icon_5345_6_20x20.png` |
| imageres_mun | RT_ICON | 2131 (group 5345) | 16x16 | 32 | `icons/icon_5345_7_16x16.png` |
| imageres_mun | RT_ICON | 2132 (group 5346) | 256x256 | 32 | `icons/icon_5346_0_256x256.png` |
| imageres_mun | RT_ICON | 2133 (group 5346) | 64x64 | 32 | `icons/icon_5346_1_64x64.png` |
| imageres_mun | RT_ICON | 2134 (group 5346) | 48x48 | 32 | `icons/icon_5346_2_48x48.png` |
| imageres_mun | RT_ICON | 2135 (group 5346) | 40x40 | 32 | `icons/icon_5346_3_40x40.png` |
| imageres_mun | RT_ICON | 2136 (group 5346) | 32x32 | 32 | `icons/icon_5346_4_32x32.png` |
| imageres_mun | RT_ICON | 2137 (group 5346) | 24x24 | 32 | `icons/icon_5346_5_24x24.png` |
| imageres_mun | RT_ICON | 2138 (group 5346) | 20x20 | 32 | `icons/icon_5346_6_20x20.png` |
| imageres_mun | RT_ICON | 2139 (group 5346) | 16x16 | 32 | `icons/icon_5346_7_16x16.png` |
| imageres_mun | RT_ICON | 214 (group 32) | 24x24 | 32 | `icons/icon_32_5_24x24.png` |
| imageres_mun | RT_ICON | 2140 (group 5347) | 256x256 | 32 | `icons/icon_5347_0_256x256.png` |
| imageres_mun | RT_ICON | 2141 (group 5347) | 64x64 | 32 | `icons/icon_5347_1_64x64.png` |
| imageres_mun | RT_ICON | 2142 (group 5347) | 48x48 | 32 | `icons/icon_5347_2_48x48.png` |
| imageres_mun | RT_ICON | 2143 (group 5347) | 40x40 | 32 | `icons/icon_5347_3_40x40.png` |
| imageres_mun | RT_ICON | 2144 (group 5347) | 32x32 | 32 | `icons/icon_5347_4_32x32.png` |
| imageres_mun | RT_ICON | 2145 (group 5347) | 24x24 | 32 | `icons/icon_5347_5_24x24.png` |
| imageres_mun | RT_ICON | 2146 (group 5347) | 20x20 | 32 | `icons/icon_5347_6_20x20.png` |
| imageres_mun | RT_ICON | 2147 (group 5347) | 16x16 | 32 | `icons/icon_5347_7_16x16.png` |
| imageres_mun | RT_ICON | 2148 (group 5348) | 256x256 | 32 | `icons/icon_5348_0_256x256.png` |
| imageres_mun | RT_ICON | 2149 (group 5348) | 64x64 | 32 | `icons/icon_5348_1_64x64.png` |
| imageres_mun | RT_ICON | 215 (group 32) | 20x20 | 32 | `icons/icon_32_6_20x20.png` |
| imageres_mun | RT_ICON | 2150 (group 5348) | 48x48 | 32 | `icons/icon_5348_2_48x48.png` |
| imageres_mun | RT_ICON | 2151 (group 5348) | 40x40 | 32 | `icons/icon_5348_3_40x40.png` |
| imageres_mun | RT_ICON | 2152 (group 5348) | 32x32 | 32 | `icons/icon_5348_4_32x32.png` |
| imageres_mun | RT_ICON | 2153 (group 5348) | 24x24 | 32 | `icons/icon_5348_5_24x24.png` |
| imageres_mun | RT_ICON | 2154 (group 5348) | 20x20 | 32 | `icons/icon_5348_6_20x20.png` |
| imageres_mun | RT_ICON | 2155 (group 5348) | 16x16 | 32 | `icons/icon_5348_7_16x16.png` |
| imageres_mun | RT_ICON | 2156 (group 5349) | 256x256 | 32 | `icons/icon_5349_0_256x256.png` |
| imageres_mun | RT_ICON | 2157 (group 5349) | 64x64 | 32 | `icons/icon_5349_1_64x64.png` |
| imageres_mun | RT_ICON | 2158 (group 5349) | 48x48 | 32 | `icons/icon_5349_2_48x48.png` |
| imageres_mun | RT_ICON | 2159 (group 5349) | 40x40 | 32 | `icons/icon_5349_3_40x40.png` |
| imageres_mun | RT_ICON | 216 (group 32) | 16x16 | 32 | `icons/icon_32_7_16x16.png` |
| imageres_mun | RT_ICON | 2160 (group 5349) | 32x32 | 32 | `icons/icon_5349_4_32x32.png` |
| imageres_mun | RT_ICON | 2161 (group 5349) | 24x24 | 32 | `icons/icon_5349_5_24x24.png` |
| imageres_mun | RT_ICON | 2162 (group 5349) | 20x20 | 32 | `icons/icon_5349_6_20x20.png` |
| imageres_mun | RT_ICON | 2163 (group 5349) | 16x16 | 32 | `icons/icon_5349_7_16x16.png` |
| imageres_mun | RT_ICON | 2164 (group 5350) | 256x256 | 32 | `icons/icon_5350_0_256x256.png` |
| imageres_mun | RT_ICON | 2165 (group 5350) | 64x64 | 32 | `icons/icon_5350_1_64x64.png` |
| imageres_mun | RT_ICON | 2166 (group 5350) | 48x48 | 32 | `icons/icon_5350_2_48x48.png` |
| imageres_mun | RT_ICON | 2167 (group 5350) | 40x40 | 32 | `icons/icon_5350_3_40x40.png` |
| imageres_mun | RT_ICON | 2168 (group 5350) | 32x32 | 32 | `icons/icon_5350_4_32x32.png` |
| imageres_mun | RT_ICON | 2169 (group 5350) | 24x24 | 32 | `icons/icon_5350_5_24x24.png` |
| imageres_mun | RT_ICON | 217 (group 1030) | 256x256 | 32 | `icons/icon_1030_0_256x256.png` |
| imageres_mun | RT_ICON | 2170 (group 5350) | 20x20 | 32 | `icons/icon_5350_6_20x20.png` |
| imageres_mun | RT_ICON | 2171 (group 5350) | 16x16 | 32 | `icons/icon_5350_7_16x16.png` |
| imageres_mun | RT_ICON | 2172 (group 5351) | 256x256 | 32 | `icons/icon_5351_0_256x256.png` |
| imageres_mun | RT_ICON | 2173 (group 5351) | 64x64 | 32 | `icons/icon_5351_1_64x64.png` |
| imageres_mun | RT_ICON | 2174 (group 5351) | 48x48 | 32 | `icons/icon_5351_2_48x48.png` |
| imageres_mun | RT_ICON | 2175 (group 5351) | 40x40 | 32 | `icons/icon_5351_3_40x40.png` |
| imageres_mun | RT_ICON | 2176 (group 5351) | 32x32 | 32 | `icons/icon_5351_4_32x32.png` |
| imageres_mun | RT_ICON | 2177 (group 5351) | 24x24 | 32 | `icons/icon_5351_5_24x24.png` |
| imageres_mun | RT_ICON | 2178 (group 5351) | 20x20 | 32 | `icons/icon_5351_6_20x20.png` |
| imageres_mun | RT_ICON | 2179 (group 5351) | 16x16 | 32 | `icons/icon_5351_7_16x16.png` |
| imageres_mun | RT_ICON | 218 (group 1030) | 64x64 | 32 | `icons/icon_1030_1_64x64.png` |
| imageres_mun | RT_ICON | 2180 (group 5352) | 256x256 | 32 | `icons/icon_5352_0_256x256.png` |
| imageres_mun | RT_ICON | 2181 (group 5352) | 64x64 | 32 | `icons/icon_5352_1_64x64.png` |
| imageres_mun | RT_ICON | 2182 (group 5352) | 48x48 | 32 | `icons/icon_5352_2_48x48.png` |
| imageres_mun | RT_ICON | 2183 (group 5352) | 40x40 | 32 | `icons/icon_5352_3_40x40.png` |
| imageres_mun | RT_ICON | 2184 (group 5352) | 32x32 | 32 | `icons/icon_5352_4_32x32.png` |
| imageres_mun | RT_ICON | 2185 (group 5352) | 24x24 | 32 | `icons/icon_5352_5_24x24.png` |
| imageres_mun | RT_ICON | 2186 (group 5352) | 20x20 | 32 | `icons/icon_5352_6_20x20.png` |
| imageres_mun | RT_ICON | 2187 (group 5352) | 16x16 | 32 | `icons/icon_5352_7_16x16.png` |
| imageres_mun | RT_ICON | 2188 (group 5353) | 256x256 | 32 | `icons/icon_5353_0_256x256.png` |
| imageres_mun | RT_ICON | 2189 (group 5353) | 64x64 | 32 | `icons/icon_5353_1_64x64.png` |
| imageres_mun | RT_ICON | 219 (group 1030) | 48x48 | 32 | `icons/icon_1030_2_48x48.png` |
| imageres_mun | RT_ICON | 2190 (group 5353) | 48x48 | 32 | `icons/icon_5353_2_48x48.png` |
| imageres_mun | RT_ICON | 2191 (group 5353) | 40x40 | 32 | `icons/icon_5353_3_40x40.png` |
| imageres_mun | RT_ICON | 2192 (group 5353) | 32x32 | 32 | `icons/icon_5353_4_32x32.png` |
| imageres_mun | RT_ICON | 2193 (group 5353) | 24x24 | 32 | `icons/icon_5353_5_24x24.png` |
| imageres_mun | RT_ICON | 2194 (group 5353) | 20x20 | 32 | `icons/icon_5353_6_20x20.png` |
| imageres_mun | RT_ICON | 2195 (group 5353) | 16x16 | 32 | `icons/icon_5353_7_16x16.png` |
| imageres_mun | RT_ICON | 2196 (group 5354) | 256x256 | 32 | `icons/icon_5354_0_256x256.png` |
| imageres_mun | RT_ICON | 2197 (group 5354) | 64x64 | 32 | `icons/icon_5354_1_64x64.png` |
| imageres_mun | RT_ICON | 2198 (group 5354) | 48x48 | 32 | `icons/icon_5354_2_48x48.png` |
| imageres_mun | RT_ICON | 2199 (group 5354) | 40x40 | 32 | `icons/icon_5354_3_40x40.png` |
| imageres_mun | RT_ICON | 22 (group 4) | 24x24 | 32 | `icons/icon_4_5_24x24.png` |
| imageres_mun | RT_ICON | 220 (group 1030) | 40x40 | 32 | `icons/icon_1030_3_40x40.png` |
| imageres_mun | RT_ICON | 2200 (group 5354) | 32x32 | 32 | `icons/icon_5354_4_32x32.png` |
| imageres_mun | RT_ICON | 2201 (group 5354) | 24x24 | 32 | `icons/icon_5354_5_24x24.png` |
| imageres_mun | RT_ICON | 2202 (group 5354) | 20x20 | 32 | `icons/icon_5354_6_20x20.png` |
| imageres_mun | RT_ICON | 2203 (group 5354) | 16x16 | 32 | `icons/icon_5354_7_16x16.png` |
| imageres_mun | RT_ICON | 2204 (group 5355) | 256x256 | 32 | `icons/icon_5355_0_256x256.png` |
| imageres_mun | RT_ICON | 2205 (group 5355) | 64x64 | 32 | `icons/icon_5355_1_64x64.png` |
| imageres_mun | RT_ICON | 2206 (group 5355) | 48x48 | 32 | `icons/icon_5355_2_48x48.png` |
| imageres_mun | RT_ICON | 2207 (group 5355) | 40x40 | 32 | `icons/icon_5355_3_40x40.png` |
| imageres_mun | RT_ICON | 2208 (group 5355) | 32x32 | 32 | `icons/icon_5355_4_32x32.png` |
| imageres_mun | RT_ICON | 2209 (group 5355) | 24x24 | 32 | `icons/icon_5355_5_24x24.png` |
| imageres_mun | RT_ICON | 221 (group 1030) | 32x32 | 32 | `icons/icon_1030_4_32x32.png` |
| imageres_mun | RT_ICON | 2210 (group 5355) | 20x20 | 32 | `icons/icon_5355_6_20x20.png` |
| imageres_mun | RT_ICON | 2211 (group 5355) | 16x16 | 32 | `icons/icon_5355_7_16x16.png` |
| imageres_mun | RT_ICON | 2212 (group 5356) | 256x256 | 32 | `icons/icon_5356_0_256x256.png` |
| imageres_mun | RT_ICON | 2213 (group 5356) | 64x64 | 32 | `icons/icon_5356_1_64x64.png` |
| imageres_mun | RT_ICON | 2214 (group 5356) | 48x48 | 32 | `icons/icon_5356_2_48x48.png` |
| imageres_mun | RT_ICON | 2215 (group 5356) | 40x40 | 32 | `icons/icon_5356_3_40x40.png` |
| imageres_mun | RT_ICON | 2216 (group 5356) | 32x32 | 32 | `icons/icon_5356_4_32x32.png` |
| imageres_mun | RT_ICON | 2217 (group 5356) | 24x24 | 32 | `icons/icon_5356_5_24x24.png` |
| imageres_mun | RT_ICON | 2218 (group 5356) | 20x20 | 32 | `icons/icon_5356_6_20x20.png` |
| imageres_mun | RT_ICON | 2219 (group 5356) | 16x16 | 32 | `icons/icon_5356_7_16x16.png` |
| imageres_mun | RT_ICON | 222 (group 1030) | 24x24 | 32 | `icons/icon_1030_5_24x24.png` |
| imageres_mun | RT_ICON | 2220 (group 5357) | 256x256 | 32 | `icons/icon_5357_0_256x256.png` |
| imageres_mun | RT_ICON | 2221 (group 5357) | 64x64 | 32 | `icons/icon_5357_1_64x64.png` |
| imageres_mun | RT_ICON | 2222 (group 5357) | 48x48 | 32 | `icons/icon_5357_2_48x48.png` |
| imageres_mun | RT_ICON | 2223 (group 5357) | 40x40 | 32 | `icons/icon_5357_3_40x40.png` |
| imageres_mun | RT_ICON | 2224 (group 5357) | 32x32 | 32 | `icons/icon_5357_4_32x32.png` |
| imageres_mun | RT_ICON | 2225 (group 5357) | 24x24 | 32 | `icons/icon_5357_5_24x24.png` |
| imageres_mun | RT_ICON | 2226 (group 5357) | 20x20 | 32 | `icons/icon_5357_6_20x20.png` |
| imageres_mun | RT_ICON | 2227 (group 5357) | 16x16 | 32 | `icons/icon_5357_7_16x16.png` |
| imageres_mun | RT_ICON | 2228 (group 5358) | 256x256 | 32 | `icons/icon_5358_0_256x256.png` |
| imageres_mun | RT_ICON | 2229 (group 5358) | 64x64 | 32 | `icons/icon_5358_1_64x64.png` |
| imageres_mun | RT_ICON | 223 (group 1030) | 20x20 | 32 | `icons/icon_1030_6_20x20.png` |
| imageres_mun | RT_ICON | 2230 (group 5358) | 48x48 | 32 | `icons/icon_5358_2_48x48.png` |
| imageres_mun | RT_ICON | 2231 (group 5358) | 40x40 | 32 | `icons/icon_5358_3_40x40.png` |
| imageres_mun | RT_ICON | 2232 (group 5358) | 32x32 | 32 | `icons/icon_5358_4_32x32.png` |
| imageres_mun | RT_ICON | 2233 (group 5358) | 24x24 | 32 | `icons/icon_5358_5_24x24.png` |
| imageres_mun | RT_ICON | 2234 (group 5358) | 20x20 | 32 | `icons/icon_5358_6_20x20.png` |
| imageres_mun | RT_ICON | 2235 (group 5358) | 16x16 | 32 | `icons/icon_5358_7_16x16.png` |
| imageres_mun | RT_ICON | 2236 (group 5359) | 256x256 | 32 | `icons/icon_5359_0_256x256.png` |
| imageres_mun | RT_ICON | 2237 (group 5359) | 64x64 | 32 | `icons/icon_5359_1_64x64.png` |
| imageres_mun | RT_ICON | 2238 (group 5359) | 48x48 | 32 | `icons/icon_5359_2_48x48.png` |
| imageres_mun | RT_ICON | 2239 (group 5359) | 40x40 | 32 | `icons/icon_5359_3_40x40.png` |
| imageres_mun | RT_ICON | 224 (group 1030) | 16x16 | 32 | `icons/icon_1030_7_16x16.png` |
| imageres_mun | RT_ICON | 2240 (group 5359) | 32x32 | 32 | `icons/icon_5359_4_32x32.png` |
| imageres_mun | RT_ICON | 2241 (group 5359) | 24x24 | 32 | `icons/icon_5359_5_24x24.png` |
| imageres_mun | RT_ICON | 2242 (group 5359) | 20x20 | 32 | `icons/icon_5359_6_20x20.png` |
| imageres_mun | RT_ICON | 2243 (group 5359) | 16x16 | 32 | `icons/icon_5359_7_16x16.png` |
| imageres_mun | RT_ICON | 2244 (group 5360) | 256x256 | 32 | `icons/icon_5360_0_256x256.png` |
| imageres_mun | RT_ICON | 2245 (group 5360) | 64x64 | 32 | `icons/icon_5360_1_64x64.png` |
| imageres_mun | RT_ICON | 2246 (group 5360) | 48x48 | 32 | `icons/icon_5360_2_48x48.png` |
| imageres_mun | RT_ICON | 2247 (group 5360) | 40x40 | 32 | `icons/icon_5360_3_40x40.png` |
| imageres_mun | RT_ICON | 2248 (group 5360) | 32x32 | 32 | `icons/icon_5360_4_32x32.png` |
| imageres_mun | RT_ICON | 2249 (group 5360) | 24x24 | 32 | `icons/icon_5360_5_24x24.png` |
| imageres_mun | RT_ICON | 225 (group 1031) | 256x256 | 32 | `icons/icon_1031_0_256x256.png` |
| imageres_mun | RT_ICON | 2250 (group 5360) | 20x20 | 32 | `icons/icon_5360_6_20x20.png` |
| imageres_mun | RT_ICON | 2251 (group 5360) | 16x16 | 32 | `icons/icon_5360_7_16x16.png` |
| imageres_mun | RT_ICON | 2252 (group 5361) | 256x256 | 32 | `icons/icon_5361_0_256x256.png` |
| imageres_mun | RT_ICON | 2253 (group 5361) | 64x64 | 32 | `icons/icon_5361_1_64x64.png` |
| imageres_mun | RT_ICON | 2254 (group 5361) | 48x48 | 32 | `icons/icon_5361_2_48x48.png` |
| imageres_mun | RT_ICON | 2255 (group 5361) | 40x40 | 32 | `icons/icon_5361_3_40x40.png` |
| imageres_mun | RT_ICON | 2256 (group 5361) | 32x32 | 32 | `icons/icon_5361_4_32x32.png` |
| imageres_mun | RT_ICON | 2257 (group 5361) | 24x24 | 32 | `icons/icon_5361_5_24x24.png` |
| imageres_mun | RT_ICON | 2258 (group 5361) | 20x20 | 32 | `icons/icon_5361_6_20x20.png` |
| imageres_mun | RT_ICON | 2259 (group 5361) | 16x16 | 32 | `icons/icon_5361_7_16x16.png` |
| imageres_mun | RT_ICON | 226 (group 1031) | 64x64 | 32 | `icons/icon_1031_1_64x64.png` |
| imageres_mun | RT_ICON | 2260 (group 5362) | 256x256 | 32 | `icons/icon_5362_0_256x256.png` |
| imageres_mun | RT_ICON | 2261 (group 5362) | 64x64 | 32 | `icons/icon_5362_1_64x64.png` |
| imageres_mun | RT_ICON | 2262 (group 5362) | 48x48 | 32 | `icons/icon_5362_2_48x48.png` |
| imageres_mun | RT_ICON | 2263 (group 5362) | 40x40 | 32 | `icons/icon_5362_3_40x40.png` |
| imageres_mun | RT_ICON | 2264 (group 5362) | 32x32 | 32 | `icons/icon_5362_4_32x32.png` |
| imageres_mun | RT_ICON | 2265 (group 5362) | 24x24 | 32 | `icons/icon_5362_5_24x24.png` |
| imageres_mun | RT_ICON | 2266 (group 5362) | 20x20 | 32 | `icons/icon_5362_6_20x20.png` |
| imageres_mun | RT_ICON | 2267 (group 5362) | 16x16 | 32 | `icons/icon_5362_7_16x16.png` |
| imageres_mun | RT_ICON | 2268 (group 5363) | 256x256 | 32 | `icons/icon_5363_0_256x256.png` |
| imageres_mun | RT_ICON | 2269 (group 5363) | 64x64 | 32 | `icons/icon_5363_1_64x64.png` |
| imageres_mun | RT_ICON | 227 (group 1031) | 48x48 | 32 | `icons/icon_1031_2_48x48.png` |
| imageres_mun | RT_ICON | 2270 (group 5363) | 48x48 | 32 | `icons/icon_5363_2_48x48.png` |
| imageres_mun | RT_ICON | 2271 (group 5363) | 40x40 | 32 | `icons/icon_5363_3_40x40.png` |
| imageres_mun | RT_ICON | 2272 (group 5363) | 32x32 | 32 | `icons/icon_5363_4_32x32.png` |
| imageres_mun | RT_ICON | 2273 (group 5363) | 24x24 | 32 | `icons/icon_5363_5_24x24.png` |
| imageres_mun | RT_ICON | 2274 (group 5363) | 20x20 | 32 | `icons/icon_5363_6_20x20.png` |
| imageres_mun | RT_ICON | 2275 (group 5363) | 16x16 | 32 | `icons/icon_5363_7_16x16.png` |
| imageres_mun | RT_ICON | 2276 (group 5364) | 256x256 | 32 | `icons/icon_5364_0_256x256.png` |
| imageres_mun | RT_ICON | 2277 (group 5364) | 64x64 | 32 | `icons/icon_5364_1_64x64.png` |
| imageres_mun | RT_ICON | 2278 (group 5364) | 48x48 | 32 | `icons/icon_5364_2_48x48.png` |
| imageres_mun | RT_ICON | 2279 (group 5364) | 40x40 | 32 | `icons/icon_5364_3_40x40.png` |
| imageres_mun | RT_ICON | 228 (group 1031) | 40x40 | 32 | `icons/icon_1031_3_40x40.png` |
| imageres_mun | RT_ICON | 2280 (group 5364) | 32x32 | 32 | `icons/icon_5364_4_32x32.png` |
| imageres_mun | RT_ICON | 2281 (group 5364) | 24x24 | 32 | `icons/icon_5364_5_24x24.png` |
| imageres_mun | RT_ICON | 2282 (group 5364) | 20x20 | 32 | `icons/icon_5364_6_20x20.png` |
| imageres_mun | RT_ICON | 2283 (group 5364) | 16x16 | 32 | `icons/icon_5364_7_16x16.png` |
| imageres_mun | RT_ICON | 2284 (group 5365) | 16x16 | 4 | `icons/icon_5365_0_16x16.png` |
| imageres_mun | RT_ICON | 2285 (group 5366) | 256x256 | 32 | `icons/icon_5366_0_256x256.png` |
| imageres_mun | RT_ICON | 2286 (group 5366) | 64x64 | 32 | `icons/icon_5366_1_64x64.png` |
| imageres_mun | RT_ICON | 2287 (group 5366) | 48x48 | 32 | `icons/icon_5366_2_48x48.png` |
| imageres_mun | RT_ICON | 2288 (group 5366) | 40x40 | 32 | `icons/icon_5366_3_40x40.png` |
| imageres_mun | RT_ICON | 2289 (group 5366) | 32x32 | 32 | `icons/icon_5366_4_32x32.png` |
| imageres_mun | RT_ICON | 229 (group 1031) | 32x32 | 32 | `icons/icon_1031_4_32x32.png` |
| imageres_mun | RT_ICON | 2290 (group 5366) | 24x24 | 32 | `icons/icon_5366_5_24x24.png` |
| imageres_mun | RT_ICON | 2291 (group 5366) | 20x20 | 32 | `icons/icon_5366_6_20x20.png` |
| imageres_mun | RT_ICON | 2292 (group 5366) | 16x16 | 32 | `icons/icon_5366_7_16x16.png` |
| imageres_mun | RT_ICON | 2293 (group 5367) | 256x256 | 32 | `icons/icon_5367_0_256x256.png` |
| imageres_mun | RT_ICON | 2294 (group 5367) | 64x64 | 32 | `icons/icon_5367_1_64x64.png` |
| imageres_mun | RT_ICON | 2295 (group 5367) | 48x48 | 32 | `icons/icon_5367_2_48x48.png` |
| imageres_mun | RT_ICON | 2296 (group 5367) | 40x40 | 32 | `icons/icon_5367_3_40x40.png` |
| imageres_mun | RT_ICON | 2297 (group 5367) | 32x32 | 32 | `icons/icon_5367_4_32x32.png` |
| imageres_mun | RT_ICON | 2298 (group 5367) | 24x24 | 32 | `icons/icon_5367_5_24x24.png` |
| imageres_mun | RT_ICON | 2299 (group 5367) | 20x20 | 32 | `icons/icon_5367_6_20x20.png` |
| imageres_mun | RT_ICON | 23 (group 4) | 20x20 | 32 | `icons/icon_4_6_20x20.png` |
| imageres_mun | RT_ICON | 230 (group 1031) | 24x24 | 32 | `icons/icon_1031_5_24x24.png` |
| imageres_mun | RT_ICON | 2300 (group 5367) | 16x16 | 32 | `icons/icon_5367_7_16x16.png` |
| imageres_mun | RT_ICON | 2301 (group 5368) | 256x256 | 32 | `icons/icon_5368_0_256x256.png` |
| imageres_mun | RT_ICON | 2302 (group 5368) | 64x64 | 32 | `icons/icon_5368_1_64x64.png` |
| imageres_mun | RT_ICON | 2303 (group 5368) | 48x48 | 32 | `icons/icon_5368_2_48x48.png` |
| imageres_mun | RT_ICON | 2304 (group 5368) | 40x40 | 32 | `icons/icon_5368_3_40x40.png` |
| imageres_mun | RT_ICON | 2305 (group 5368) | 32x32 | 32 | `icons/icon_5368_4_32x32.png` |
| imageres_mun | RT_ICON | 2306 (group 5368) | 24x24 | 32 | `icons/icon_5368_5_24x24.png` |
| imageres_mun | RT_ICON | 2307 (group 5368) | 20x20 | 32 | `icons/icon_5368_6_20x20.png` |
| imageres_mun | RT_ICON | 2308 (group 5368) | 16x16 | 32 | `icons/icon_5368_7_16x16.png` |
| imageres_mun | RT_ICON | 2309 (group 5369) | 16x16 | 4 | `icons/icon_5369_0_16x16.png` |
| imageres_mun | RT_ICON | 231 (group 1031) | 20x20 | 32 | `icons/icon_1031_6_20x20.png` |
| imageres_mun | RT_ICON | 2310 (group 5370) | 256x256 | 32 | `icons/icon_5370_0_256x256.png` |
| imageres_mun | RT_ICON | 2311 (group 5370) | 64x64 | 32 | `icons/icon_5370_1_64x64.png` |
| imageres_mun | RT_ICON | 2312 (group 5370) | 48x48 | 32 | `icons/icon_5370_2_48x48.png` |
| imageres_mun | RT_ICON | 2313 (group 5370) | 40x40 | 32 | `icons/icon_5370_3_40x40.png` |
| imageres_mun | RT_ICON | 2314 (group 5370) | 32x32 | 32 | `icons/icon_5370_4_32x32.png` |
| imageres_mun | RT_ICON | 2315 (group 5370) | 24x24 | 32 | `icons/icon_5370_5_24x24.png` |
| imageres_mun | RT_ICON | 2316 (group 5370) | 20x20 | 32 | `icons/icon_5370_6_20x20.png` |
| imageres_mun | RT_ICON | 2317 (group 5370) | 16x16 | 32 | `icons/icon_5370_7_16x16.png` |
| imageres_mun | RT_ICON | 2318 (group 5371) | 256x256 | 32 | `icons/icon_5371_0_256x256.png` |
| imageres_mun | RT_ICON | 2319 (group 5371) | 64x64 | 32 | `icons/icon_5371_1_64x64.png` |
| imageres_mun | RT_ICON | 232 (group 1031) | 16x16 | 32 | `icons/icon_1031_7_16x16.png` |
| imageres_mun | RT_ICON | 2320 (group 5371) | 48x48 | 32 | `icons/icon_5371_2_48x48.png` |
| imageres_mun | RT_ICON | 2321 (group 5371) | 40x40 | 32 | `icons/icon_5371_3_40x40.png` |
| imageres_mun | RT_ICON | 2322 (group 5371) | 32x32 | 32 | `icons/icon_5371_4_32x32.png` |
| imageres_mun | RT_ICON | 2323 (group 5371) | 24x24 | 32 | `icons/icon_5371_5_24x24.png` |
| imageres_mun | RT_ICON | 2324 (group 5371) | 20x20 | 32 | `icons/icon_5371_6_20x20.png` |
| imageres_mun | RT_ICON | 2325 (group 5371) | 16x16 | 32 | `icons/icon_5371_7_16x16.png` |
| imageres_mun | RT_ICON | 2326 (group 5372) | 256x256 | 32 | `icons/icon_5372_0_256x256.png` |
| imageres_mun | RT_ICON | 2327 (group 5372) | 64x64 | 32 | `icons/icon_5372_1_64x64.png` |
| imageres_mun | RT_ICON | 2328 (group 5372) | 48x48 | 32 | `icons/icon_5372_2_48x48.png` |
| imageres_mun | RT_ICON | 2329 (group 5372) | 40x40 | 32 | `icons/icon_5372_3_40x40.png` |
| imageres_mun | RT_ICON | 233 (group 1032) | 256x256 | 32 | `icons/icon_1032_0_256x256.png` |
| imageres_mun | RT_ICON | 2330 (group 5372) | 32x32 | 32 | `icons/icon_5372_4_32x32.png` |
| imageres_mun | RT_ICON | 2331 (group 5372) | 24x24 | 32 | `icons/icon_5372_5_24x24.png` |
| imageres_mun | RT_ICON | 2332 (group 5372) | 20x20 | 32 | `icons/icon_5372_6_20x20.png` |
| imageres_mun | RT_ICON | 2333 (group 5372) | 16x16 | 32 | `icons/icon_5372_7_16x16.png` |
| imageres_mun | RT_ICON | 2334 (group 5373) | 256x256 | 32 | `icons/icon_5373_0_256x256.png` |
| imageres_mun | RT_ICON | 2335 (group 5373) | 64x64 | 32 | `icons/icon_5373_1_64x64.png` |
| imageres_mun | RT_ICON | 2336 (group 5373) | 48x48 | 32 | `icons/icon_5373_2_48x48.png` |
| imageres_mun | RT_ICON | 2337 (group 5373) | 40x40 | 32 | `icons/icon_5373_3_40x40.png` |
| imageres_mun | RT_ICON | 2338 (group 5373) | 32x32 | 32 | `icons/icon_5373_4_32x32.png` |
| imageres_mun | RT_ICON | 2339 (group 5373) | 24x24 | 32 | `icons/icon_5373_5_24x24.png` |
| imageres_mun | RT_ICON | 234 (group 1032) | 64x64 | 32 | `icons/icon_1032_1_64x64.png` |
| imageres_mun | RT_ICON | 2340 (group 5373) | 20x20 | 32 | `icons/icon_5373_6_20x20.png` |
| imageres_mun | RT_ICON | 2341 (group 5373) | 16x16 | 32 | `icons/icon_5373_7_16x16.png` |
| imageres_mun | RT_ICON | 2342 (group 5374) | 256x256 | 32 | `icons/icon_5374_0_256x256.png` |
| imageres_mun | RT_ICON | 2343 (group 5374) | 64x64 | 32 | `icons/icon_5374_1_64x64.png` |
| imageres_mun | RT_ICON | 2344 (group 5374) | 48x48 | 32 | `icons/icon_5374_2_48x48.png` |
| imageres_mun | RT_ICON | 2345 (group 5374) | 40x40 | 32 | `icons/icon_5374_3_40x40.png` |
| imageres_mun | RT_ICON | 2346 (group 5374) | 32x32 | 32 | `icons/icon_5374_4_32x32.png` |
| imageres_mun | RT_ICON | 2347 (group 5374) | 24x24 | 32 | `icons/icon_5374_5_24x24.png` |
| imageres_mun | RT_ICON | 2348 (group 5374) | 20x20 | 32 | `icons/icon_5374_6_20x20.png` |
| imageres_mun | RT_ICON | 2349 (group 5374) | 16x16 | 32 | `icons/icon_5374_7_16x16.png` |
| imageres_mun | RT_ICON | 235 (group 1032) | 48x48 | 32 | `icons/icon_1032_2_48x48.png` |
| imageres_mun | RT_ICON | 2350 (group 5375) | 256x256 | 32 | `icons/icon_5375_0_256x256.png` |
| imageres_mun | RT_ICON | 2351 (group 5375) | 64x64 | 32 | `icons/icon_5375_1_64x64.png` |
| imageres_mun | RT_ICON | 2352 (group 5375) | 48x48 | 32 | `icons/icon_5375_2_48x48.png` |
| imageres_mun | RT_ICON | 2353 (group 5375) | 40x40 | 32 | `icons/icon_5375_3_40x40.png` |
| imageres_mun | RT_ICON | 2354 (group 5375) | 32x32 | 32 | `icons/icon_5375_4_32x32.png` |
| imageres_mun | RT_ICON | 2355 (group 5375) | 24x24 | 32 | `icons/icon_5375_5_24x24.png` |
| imageres_mun | RT_ICON | 2356 (group 5375) | 20x20 | 32 | `icons/icon_5375_6_20x20.png` |
| imageres_mun | RT_ICON | 2357 (group 5375) | 16x16 | 32 | `icons/icon_5375_7_16x16.png` |
| imageres_mun | RT_ICON | 2358 (group 5376) | 256x256 | 32 | `icons/icon_5376_0_256x256.png` |
| imageres_mun | RT_ICON | 2359 (group 5376) | 64x64 | 32 | `icons/icon_5376_1_64x64.png` |
| imageres_mun | RT_ICON | 236 (group 1032) | 40x40 | 32 | `icons/icon_1032_3_40x40.png` |
| imageres_mun | RT_ICON | 2360 (group 5376) | 48x48 | 32 | `icons/icon_5376_2_48x48.png` |
| imageres_mun | RT_ICON | 2361 (group 5376) | 40x40 | 32 | `icons/icon_5376_3_40x40.png` |
| imageres_mun | RT_ICON | 2362 (group 5376) | 32x32 | 32 | `icons/icon_5376_4_32x32.png` |
| imageres_mun | RT_ICON | 2363 (group 5376) | 24x24 | 32 | `icons/icon_5376_5_24x24.png` |
| imageres_mun | RT_ICON | 2364 (group 5376) | 20x20 | 32 | `icons/icon_5376_6_20x20.png` |
| imageres_mun | RT_ICON | 2365 (group 5376) | 16x16 | 32 | `icons/icon_5376_7_16x16.png` |
| imageres_mun | RT_ICON | 2366 (group 5377) | 256x256 | 32 | `icons/icon_5377_0_256x256.png` |
| imageres_mun | RT_ICON | 2367 (group 5377) | 64x64 | 32 | `icons/icon_5377_1_64x64.png` |
| imageres_mun | RT_ICON | 2368 (group 5377) | 48x48 | 32 | `icons/icon_5377_2_48x48.png` |
| imageres_mun | RT_ICON | 2369 (group 5377) | 40x40 | 32 | `icons/icon_5377_3_40x40.png` |
| imageres_mun | RT_ICON | 237 (group 1032) | 32x32 | 32 | `icons/icon_1032_4_32x32.png` |
| imageres_mun | RT_ICON | 2370 (group 5377) | 32x32 | 32 | `icons/icon_5377_4_32x32.png` |
| imageres_mun | RT_ICON | 2371 (group 5377) | 24x24 | 32 | `icons/icon_5377_5_24x24.png` |
| imageres_mun | RT_ICON | 2372 (group 5377) | 20x20 | 32 | `icons/icon_5377_6_20x20.png` |
| imageres_mun | RT_ICON | 2373 (group 5377) | 16x16 | 32 | `icons/icon_5377_7_16x16.png` |
| imageres_mun | RT_ICON | 2374 (group 5378) | 256x256 | 32 | `icons/icon_5378_0_256x256.png` |
| imageres_mun | RT_ICON | 2375 (group 5378) | 64x64 | 32 | `icons/icon_5378_1_64x64.png` |
| imageres_mun | RT_ICON | 2376 (group 5378) | 48x48 | 32 | `icons/icon_5378_2_48x48.png` |
| imageres_mun | RT_ICON | 2377 (group 5378) | 40x40 | 32 | `icons/icon_5378_3_40x40.png` |
| imageres_mun | RT_ICON | 2378 (group 5378) | 32x32 | 32 | `icons/icon_5378_4_32x32.png` |
| imageres_mun | RT_ICON | 2379 (group 5378) | 24x24 | 32 | `icons/icon_5378_5_24x24.png` |
| imageres_mun | RT_ICON | 238 (group 1032) | 24x24 | 32 | `icons/icon_1032_5_24x24.png` |
| imageres_mun | RT_ICON | 2380 (group 5378) | 20x20 | 32 | `icons/icon_5378_6_20x20.png` |
| imageres_mun | RT_ICON | 2381 (group 5378) | 16x16 | 32 | `icons/icon_5378_7_16x16.png` |
| imageres_mun | RT_ICON | 2382 (group 5379) | 256x256 | 32 | `icons/icon_5379_0_256x256.png` |
| imageres_mun | RT_ICON | 2383 (group 5379) | 64x64 | 32 | `icons/icon_5379_1_64x64.png` |
| imageres_mun | RT_ICON | 2384 (group 5379) | 48x48 | 32 | `icons/icon_5379_2_48x48.png` |
| imageres_mun | RT_ICON | 2385 (group 5379) | 40x40 | 32 | `icons/icon_5379_3_40x40.png` |
| imageres_mun | RT_ICON | 2386 (group 5379) | 32x32 | 32 | `icons/icon_5379_4_32x32.png` |
| imageres_mun | RT_ICON | 2387 (group 5379) | 24x24 | 32 | `icons/icon_5379_5_24x24.png` |
| imageres_mun | RT_ICON | 2388 (group 5379) | 20x20 | 32 | `icons/icon_5379_6_20x20.png` |
| imageres_mun | RT_ICON | 2389 (group 5379) | 16x16 | 32 | `icons/icon_5379_7_16x16.png` |
| imageres_mun | RT_ICON | 239 (group 1032) | 20x20 | 32 | `icons/icon_1032_6_20x20.png` |
| imageres_mun | RT_ICON | 2390 (group 5380) | 256x256 | 32 | `icons/icon_5380_0_256x256.png` |
| imageres_mun | RT_ICON | 2391 (group 5380) | 64x64 | 32 | `icons/icon_5380_1_64x64.png` |
| imageres_mun | RT_ICON | 2392 (group 5380) | 48x48 | 32 | `icons/icon_5380_2_48x48.png` |
| imageres_mun | RT_ICON | 2393 (group 5380) | 40x40 | 32 | `icons/icon_5380_3_40x40.png` |
| imageres_mun | RT_ICON | 2394 (group 5380) | 32x32 | 32 | `icons/icon_5380_4_32x32.png` |
| imageres_mun | RT_ICON | 2395 (group 5380) | 24x24 | 32 | `icons/icon_5380_5_24x24.png` |
| imageres_mun | RT_ICON | 2396 (group 5380) | 20x20 | 32 | `icons/icon_5380_6_20x20.png` |
| imageres_mun | RT_ICON | 2397 (group 5380) | 16x16 | 32 | `icons/icon_5380_7_16x16.png` |
| imageres_mun | RT_ICON | 2398 (group 5381) | 256x256 | 32 | `icons/icon_5381_0_256x256.png` |
| imageres_mun | RT_ICON | 2399 (group 5381) | 64x64 | 32 | `icons/icon_5381_1_64x64.png` |
| imageres_mun | RT_ICON | 24 (group 4) | 16x16 | 32 | `icons/icon_4_7_16x16.png` |
| imageres_mun | RT_ICON | 240 (group 1032) | 16x16 | 32 | `icons/icon_1032_7_16x16.png` |
| imageres_mun | RT_ICON | 2400 (group 5381) | 48x48 | 32 | `icons/icon_5381_2_48x48.png` |
| imageres_mun | RT_ICON | 2401 (group 5381) | 40x40 | 32 | `icons/icon_5381_3_40x40.png` |
| imageres_mun | RT_ICON | 2402 (group 5381) | 32x32 | 32 | `icons/icon_5381_4_32x32.png` |
| imageres_mun | RT_ICON | 2403 (group 5381) | 24x24 | 32 | `icons/icon_5381_5_24x24.png` |
| imageres_mun | RT_ICON | 2404 (group 5381) | 20x20 | 32 | `icons/icon_5381_6_20x20.png` |
| imageres_mun | RT_ICON | 2405 (group 5381) | 16x16 | 32 | `icons/icon_5381_7_16x16.png` |
| imageres_mun | RT_ICON | 2406 (group 5382) | 256x256 | 32 | `icons/icon_5382_0_256x256.png` |
| imageres_mun | RT_ICON | 2407 (group 5382) | 64x64 | 32 | `icons/icon_5382_1_64x64.png` |
| imageres_mun | RT_ICON | 2408 (group 5382) | 48x48 | 32 | `icons/icon_5382_2_48x48.png` |
| imageres_mun | RT_ICON | 2409 (group 5382) | 40x40 | 32 | `icons/icon_5382_3_40x40.png` |
| imageres_mun | RT_ICON | 241 (group 33) | 256x256 | 32 | `icons/icon_33_0_256x256.png` |
| imageres_mun | RT_ICON | 2410 (group 5382) | 32x32 | 32 | `icons/icon_5382_4_32x32.png` |
| imageres_mun | RT_ICON | 2411 (group 5382) | 24x24 | 32 | `icons/icon_5382_5_24x24.png` |
| imageres_mun | RT_ICON | 2412 (group 5382) | 20x20 | 32 | `icons/icon_5382_6_20x20.png` |
| imageres_mun | RT_ICON | 2413 (group 5382) | 16x16 | 32 | `icons/icon_5382_7_16x16.png` |
| imageres_mun | RT_ICON | 2414 (group 5383) | 256x256 | 32 | `icons/icon_5383_0_256x256.png` |
| imageres_mun | RT_ICON | 2415 (group 5383) | 64x64 | 32 | `icons/icon_5383_1_64x64.png` |
| imageres_mun | RT_ICON | 2416 (group 5383) | 48x48 | 32 | `icons/icon_5383_2_48x48.png` |
| imageres_mun | RT_ICON | 2417 (group 5383) | 40x40 | 32 | `icons/icon_5383_3_40x40.png` |
| imageres_mun | RT_ICON | 2418 (group 5383) | 32x32 | 32 | `icons/icon_5383_4_32x32.png` |
| imageres_mun | RT_ICON | 2419 (group 5383) | 24x24 | 32 | `icons/icon_5383_5_24x24.png` |
| imageres_mun | RT_ICON | 242 (group 33) | 64x64 | 32 | `icons/icon_33_1_64x64.png` |
| imageres_mun | RT_ICON | 2420 (group 5383) | 20x20 | 32 | `icons/icon_5383_6_20x20.png` |
| imageres_mun | RT_ICON | 2421 (group 5383) | 16x16 | 32 | `icons/icon_5383_7_16x16.png` |
| imageres_mun | RT_ICON | 2422 (group 8300) | 256x256 | 32 | `icons/icon_8300_0_256x256.png` |
| imageres_mun | RT_ICON | 2423 (group 8300) | 64x64 | 32 | `icons/icon_8300_1_64x64.png` |
| imageres_mun | RT_ICON | 2424 (group 8300) | 60x60 | 32 | `icons/icon_8300_2_60x60.png` |
| imageres_mun | RT_ICON | 2425 (group 8300) | 48x48 | 32 | `icons/icon_8300_3_48x48.png` |
| imageres_mun | RT_ICON | 2426 (group 8300) | 40x40 | 32 | `icons/icon_8300_4_40x40.png` |
| imageres_mun | RT_ICON | 2427 (group 8300) | 32x32 | 32 | `icons/icon_8300_5_32x32.png` |
| imageres_mun | RT_ICON | 2428 (group 8300) | 24x24 | 32 | `icons/icon_8300_6_24x24.png` |
| imageres_mun | RT_ICON | 2429 (group 8300) | 20x20 | 32 | `icons/icon_8300_7_20x20.png` |
| imageres_mun | RT_ICON | 243 (group 33) | 48x48 | 32 | `icons/icon_33_2_48x48.png` |
| imageres_mun | RT_ICON | 2430 (group 8300) | 16x16 | 32 | `icons/icon_8300_8_16x16.png` |
| imageres_mun | RT_ICON | 2431 (group 8301) | 256x256 | 32 | `icons/icon_8301_0_256x256.png` |
| imageres_mun | RT_ICON | 2432 (group 8301) | 64x64 | 32 | `icons/icon_8301_1_64x64.png` |
| imageres_mun | RT_ICON | 2433 (group 8301) | 60x60 | 32 | `icons/icon_8301_2_60x60.png` |
| imageres_mun | RT_ICON | 2434 (group 8301) | 48x48 | 32 | `icons/icon_8301_3_48x48.png` |
| imageres_mun | RT_ICON | 2435 (group 8301) | 40x40 | 32 | `icons/icon_8301_4_40x40.png` |
| imageres_mun | RT_ICON | 2436 (group 8301) | 32x32 | 32 | `icons/icon_8301_5_32x32.png` |
| imageres_mun | RT_ICON | 2437 (group 8301) | 24x24 | 32 | `icons/icon_8301_6_24x24.png` |
| imageres_mun | RT_ICON | 2438 (group 8301) | 20x20 | 32 | `icons/icon_8301_7_20x20.png` |
| imageres_mun | RT_ICON | 2439 (group 8301) | 16x16 | 32 | `icons/icon_8301_8_16x16.png` |
| imageres_mun | RT_ICON | 244 (group 33) | 40x40 | 32 | `icons/icon_33_3_40x40.png` |
| imageres_mun | RT_ICON | 2440 (group 8340) | 256x256 | 32 | `icons/icon_8340_0_256x256.png` |
| imageres_mun | RT_ICON | 2441 (group 8340) | 64x64 | 32 | `icons/icon_8340_1_64x64.png` |
| imageres_mun | RT_ICON | 2442 (group 8340) | 60x60 | 32 | `icons/icon_8340_2_60x60.png` |
| imageres_mun | RT_ICON | 2443 (group 8340) | 48x48 | 32 | `icons/icon_8340_3_48x48.png` |
| imageres_mun | RT_ICON | 2444 (group 8340) | 40x40 | 32 | `icons/icon_8340_4_40x40.png` |
| imageres_mun | RT_ICON | 2445 (group 8340) | 32x32 | 32 | `icons/icon_8340_5_32x32.png` |
| imageres_mun | RT_ICON | 2446 (group 8340) | 24x24 | 32 | `icons/icon_8340_6_24x24.png` |
| imageres_mun | RT_ICON | 2447 (group 8340) | 20x20 | 32 | `icons/icon_8340_7_20x20.png` |
| imageres_mun | RT_ICON | 2448 (group 8340) | 16x16 | 32 | `icons/icon_8340_8_16x16.png` |
| imageres_mun | RT_ICON | 2449 (group 8302) | 256x256 | 32 | `icons/icon_8302_0_256x256.png` |
| imageres_mun | RT_ICON | 245 (group 33) | 32x32 | 32 | `icons/icon_33_4_32x32.png` |
| imageres_mun | RT_ICON | 2450 (group 8302) | 64x64 | 32 | `icons/icon_8302_1_64x64.png` |
| imageres_mun | RT_ICON | 2451 (group 8302) | 60x60 | 32 | `icons/icon_8302_2_60x60.png` |
| imageres_mun | RT_ICON | 2452 (group 8302) | 48x48 | 32 | `icons/icon_8302_3_48x48.png` |
| imageres_mun | RT_ICON | 2453 (group 8302) | 40x40 | 32 | `icons/icon_8302_4_40x40.png` |
| imageres_mun | RT_ICON | 2454 (group 8302) | 32x32 | 32 | `icons/icon_8302_5_32x32.png` |
| imageres_mun | RT_ICON | 2455 (group 8302) | 24x24 | 32 | `icons/icon_8302_6_24x24.png` |
| imageres_mun | RT_ICON | 2456 (group 8302) | 20x20 | 32 | `icons/icon_8302_7_20x20.png` |
| imageres_mun | RT_ICON | 2457 (group 8302) | 16x16 | 32 | `icons/icon_8302_8_16x16.png` |
| imageres_mun | RT_ICON | 2458 (group 8303) | 256x256 | 32 | `icons/icon_8303_0_256x256.png` |
| imageres_mun | RT_ICON | 2459 (group 8303) | 64x64 | 32 | `icons/icon_8303_1_64x64.png` |
| imageres_mun | RT_ICON | 246 (group 33) | 24x24 | 32 | `icons/icon_33_5_24x24.png` |
| imageres_mun | RT_ICON | 2460 (group 8303) | 60x60 | 32 | `icons/icon_8303_2_60x60.png` |
| imageres_mun | RT_ICON | 2461 (group 8303) | 48x48 | 32 | `icons/icon_8303_3_48x48.png` |
| imageres_mun | RT_ICON | 2462 (group 8303) | 40x40 | 32 | `icons/icon_8303_4_40x40.png` |
| imageres_mun | RT_ICON | 2463 (group 8303) | 32x32 | 32 | `icons/icon_8303_5_32x32.png` |
| imageres_mun | RT_ICON | 2464 (group 8303) | 24x24 | 32 | `icons/icon_8303_6_24x24.png` |
| imageres_mun | RT_ICON | 2465 (group 8303) | 20x20 | 32 | `icons/icon_8303_7_20x20.png` |
| imageres_mun | RT_ICON | 2466 (group 8303) | 16x16 | 32 | `icons/icon_8303_8_16x16.png` |
| imageres_mun | RT_ICON | 2467 (group 8304) | 256x256 | 32 | `icons/icon_8304_0_256x256.png` |
| imageres_mun | RT_ICON | 2468 (group 8304) | 48x48 | 32 | `icons/icon_8304_1_48x48.png` |
| imageres_mun | RT_ICON | 2469 (group 8304) | 40x40 | 32 | `icons/icon_8304_2_40x40.png` |
| imageres_mun | RT_ICON | 247 (group 33) | 20x20 | 32 | `icons/icon_33_6_20x20.png` |
| imageres_mun | RT_ICON | 2470 (group 8304) | 32x32 | 32 | `icons/icon_8304_3_32x32.png` |
| imageres_mun | RT_ICON | 2471 (group 8304) | 24x24 | 32 | `icons/icon_8304_4_24x24.png` |
| imageres_mun | RT_ICON | 2472 (group 8304) | 20x20 | 32 | `icons/icon_8304_5_20x20.png` |
| imageres_mun | RT_ICON | 2473 (group 8304) | 16x16 | 32 | `icons/icon_8304_6_16x16.png` |
| imageres_mun | RT_ICON | 2474 (group 8305) | 256x256 | 32 | `icons/icon_8305_0_256x256.png` |
| imageres_mun | RT_ICON | 2475 (group 8305) | 96x96 | 32 | `icons/icon_8305_1_96x96.png` |
| imageres_mun | RT_ICON | 2476 (group 8305) | 48x48 | 32 | `icons/icon_8305_2_48x48.png` |
| imageres_mun | RT_ICON | 2477 (group 8305) | 40x40 | 32 | `icons/icon_8305_3_40x40.png` |
| imageres_mun | RT_ICON | 2478 (group 8305) | 32x32 | 32 | `icons/icon_8305_4_32x32.png` |
| imageres_mun | RT_ICON | 2479 (group 8305) | 24x24 | 32 | `icons/icon_8305_5_24x24.png` |
| imageres_mun | RT_ICON | 248 (group 33) | 16x16 | 32 | `icons/icon_33_7_16x16.png` |
| imageres_mun | RT_ICON | 2480 (group 8305) | 20x20 | 32 | `icons/icon_8305_6_20x20.png` |
| imageres_mun | RT_ICON | 2481 (group 8305) | 16x16 | 32 | `icons/icon_8305_7_16x16.png` |
| imageres_mun | RT_ICON | 2482 (group 8306) | 256x256 | 32 | `icons/icon_8306_0_256x256.png` |
| imageres_mun | RT_ICON | 2483 (group 8306) | 96x96 | 32 | `icons/icon_8306_1_96x96.png` |
| imageres_mun | RT_ICON | 2484 (group 8306) | 48x48 | 32 | `icons/icon_8306_2_48x48.png` |
| imageres_mun | RT_ICON | 2485 (group 8306) | 40x40 | 32 | `icons/icon_8306_3_40x40.png` |
| imageres_mun | RT_ICON | 2486 (group 8306) | 32x32 | 32 | `icons/icon_8306_4_32x32.png` |
| imageres_mun | RT_ICON | 2487 (group 8306) | 24x24 | 32 | `icons/icon_8306_5_24x24.png` |
| imageres_mun | RT_ICON | 2488 (group 8306) | 20x20 | 32 | `icons/icon_8306_6_20x20.png` |
| imageres_mun | RT_ICON | 2489 (group 8306) | 16x16 | 32 | `icons/icon_8306_7_16x16.png` |
| imageres_mun | RT_ICON | 249 (group 34) | 256x256 | 32 | `icons/icon_34_0_256x256.png` |
| imageres_mun | RT_ICON | 2490 (group 8307) | 256x256 | 32 | `icons/icon_8307_0_256x256.png` |
| imageres_mun | RT_ICON | 2491 (group 8307) | 64x64 | 32 | `icons/icon_8307_1_64x64.png` |
| imageres_mun | RT_ICON | 2492 (group 8307) | 60x60 | 32 | `icons/icon_8307_2_60x60.png` |
| imageres_mun | RT_ICON | 2493 (group 8307) | 48x48 | 32 | `icons/icon_8307_3_48x48.png` |
| imageres_mun | RT_ICON | 2494 (group 8307) | 40x40 | 32 | `icons/icon_8307_4_40x40.png` |
| imageres_mun | RT_ICON | 2495 (group 8307) | 32x32 | 32 | `icons/icon_8307_5_32x32.png` |
| imageres_mun | RT_ICON | 2496 (group 8307) | 24x24 | 32 | `icons/icon_8307_6_24x24.png` |
| imageres_mun | RT_ICON | 2497 (group 8307) | 20x20 | 32 | `icons/icon_8307_7_20x20.png` |
| imageres_mun | RT_ICON | 2498 (group 8307) | 16x16 | 32 | `icons/icon_8307_8_16x16.png` |
| imageres_mun | RT_ICON | 2499 (group 8308) | 256x256 | 32 | `icons/icon_8308_0_256x256.png` |
| imageres_mun | RT_ICON | 25 (group 5) | 256x256 | 32 | `icons/icon_5_0_256x256.png` |
| imageres_mun | RT_ICON | 250 (group 34) | 64x64 | 32 | `icons/icon_34_1_64x64.png` |
| imageres_mun | RT_ICON | 2500 (group 8308) | 64x64 | 32 | `icons/icon_8308_1_64x64.png` |
| imageres_mun | RT_ICON | 2501 (group 8308) | 60x60 | 32 | `icons/icon_8308_2_60x60.png` |
| imageres_mun | RT_ICON | 2502 (group 8308) | 48x48 | 32 | `icons/icon_8308_3_48x48.png` |
| imageres_mun | RT_ICON | 2503 (group 8308) | 40x40 | 32 | `icons/icon_8308_4_40x40.png` |
| imageres_mun | RT_ICON | 2504 (group 8308) | 32x32 | 32 | `icons/icon_8308_5_32x32.png` |
| imageres_mun | RT_ICON | 2505 (group 8308) | 24x24 | 32 | `icons/icon_8308_6_24x24.png` |
| imageres_mun | RT_ICON | 2506 (group 8308) | 20x20 | 32 | `icons/icon_8308_7_20x20.png` |
| imageres_mun | RT_ICON | 2507 (group 8308) | 16x16 | 32 | `icons/icon_8308_8_16x16.png` |
| imageres_mun | RT_ICON | 2508 (group 8309) | 256x256 | 32 | `icons/icon_8309_0_256x256.png` |
| imageres_mun | RT_ICON | 2509 (group 8309) | 64x64 | 32 | `icons/icon_8309_1_64x64.png` |
| imageres_mun | RT_ICON | 251 (group 34) | 48x48 | 32 | `icons/icon_34_2_48x48.png` |
| imageres_mun | RT_ICON | 2510 (group 8309) | 60x60 | 32 | `icons/icon_8309_2_60x60.png` |
| imageres_mun | RT_ICON | 2511 (group 8309) | 48x48 | 32 | `icons/icon_8309_3_48x48.png` |
| imageres_mun | RT_ICON | 2512 (group 8309) | 40x40 | 32 | `icons/icon_8309_4_40x40.png` |
| imageres_mun | RT_ICON | 2513 (group 8309) | 32x32 | 32 | `icons/icon_8309_5_32x32.png` |
| imageres_mun | RT_ICON | 2514 (group 8309) | 24x24 | 32 | `icons/icon_8309_6_24x24.png` |
| imageres_mun | RT_ICON | 2515 (group 8309) | 20x20 | 32 | `icons/icon_8309_7_20x20.png` |
| imageres_mun | RT_ICON | 2516 (group 8309) | 16x16 | 32 | `icons/icon_8309_8_16x16.png` |
| imageres_mun | RT_ICON | 2517 (group 8310) | 256x256 | 32 | `icons/icon_8310_0_256x256.png` |
| imageres_mun | RT_ICON | 2518 (group 8310) | 96x96 | 32 | `icons/icon_8310_1_96x96.png` |
| imageres_mun | RT_ICON | 2519 (group 8310) | 48x48 | 32 | `icons/icon_8310_2_48x48.png` |
| imageres_mun | RT_ICON | 252 (group 34) | 40x40 | 32 | `icons/icon_34_3_40x40.png` |
| imageres_mun | RT_ICON | 2520 (group 8310) | 40x40 | 32 | `icons/icon_8310_3_40x40.png` |
| imageres_mun | RT_ICON | 2521 (group 8310) | 32x32 | 32 | `icons/icon_8310_4_32x32.png` |
| imageres_mun | RT_ICON | 2522 (group 8310) | 24x24 | 32 | `icons/icon_8310_5_24x24.png` |
| imageres_mun | RT_ICON | 2523 (group 8310) | 20x20 | 32 | `icons/icon_8310_6_20x20.png` |
| imageres_mun | RT_ICON | 2524 (group 8310) | 16x16 | 32 | `icons/icon_8310_7_16x16.png` |
| imageres_mun | RT_ICON | 2525 (group 8311) | 256x256 | 32 | `icons/icon_8311_0_256x256.png` |
| imageres_mun | RT_ICON | 2526 (group 8311) | 64x64 | 32 | `icons/icon_8311_1_64x64.png` |
| imageres_mun | RT_ICON | 2527 (group 8311) | 60x60 | 32 | `icons/icon_8311_2_60x60.png` |
| imageres_mun | RT_ICON | 2528 (group 8311) | 48x48 | 32 | `icons/icon_8311_3_48x48.png` |
| imageres_mun | RT_ICON | 2529 (group 8311) | 40x40 | 32 | `icons/icon_8311_4_40x40.png` |
| imageres_mun | RT_ICON | 253 (group 34) | 32x32 | 32 | `icons/icon_34_4_32x32.png` |
| imageres_mun | RT_ICON | 2530 (group 8311) | 32x32 | 32 | `icons/icon_8311_5_32x32.png` |
| imageres_mun | RT_ICON | 2531 (group 8311) | 24x24 | 32 | `icons/icon_8311_6_24x24.png` |
| imageres_mun | RT_ICON | 2532 (group 8311) | 20x20 | 32 | `icons/icon_8311_7_20x20.png` |
| imageres_mun | RT_ICON | 2533 (group 8311) | 16x16 | 32 | `icons/icon_8311_8_16x16.png` |
| imageres_mun | RT_ICON | 2534 (group 8341) | 256x256 | 32 | `icons/icon_8341_0_256x256.png` |
| imageres_mun | RT_ICON | 2535 (group 8341) | 64x64 | 32 | `icons/icon_8341_1_64x64.png` |
| imageres_mun | RT_ICON | 2536 (group 8341) | 60x60 | 32 | `icons/icon_8341_2_60x60.png` |
| imageres_mun | RT_ICON | 2537 (group 8341) | 48x48 | 32 | `icons/icon_8341_3_48x48.png` |
| imageres_mun | RT_ICON | 2538 (group 8341) | 40x40 | 32 | `icons/icon_8341_4_40x40.png` |
| imageres_mun | RT_ICON | 2539 (group 8341) | 32x32 | 32 | `icons/icon_8341_5_32x32.png` |
| imageres_mun | RT_ICON | 254 (group 34) | 24x24 | 32 | `icons/icon_34_5_24x24.png` |
| imageres_mun | RT_ICON | 2540 (group 8341) | 24x24 | 32 | `icons/icon_8341_6_24x24.png` |
| imageres_mun | RT_ICON | 2541 (group 8341) | 20x20 | 32 | `icons/icon_8341_7_20x20.png` |
| imageres_mun | RT_ICON | 2542 (group 8341) | 16x16 | 32 | `icons/icon_8341_8_16x16.png` |
| imageres_mun | RT_ICON | 2543 (group 8312) | 256x256 | 32 | `icons/icon_8312_0_256x256.png` |
| imageres_mun | RT_ICON | 2544 (group 8312) | 64x64 | 32 | `icons/icon_8312_1_64x64.png` |
| imageres_mun | RT_ICON | 2545 (group 8312) | 60x60 | 32 | `icons/icon_8312_2_60x60.png` |
| imageres_mun | RT_ICON | 2546 (group 8312) | 48x48 | 32 | `icons/icon_8312_3_48x48.png` |
| imageres_mun | RT_ICON | 2547 (group 8312) | 40x40 | 32 | `icons/icon_8312_4_40x40.png` |
| imageres_mun | RT_ICON | 2548 (group 8312) | 32x32 | 32 | `icons/icon_8312_5_32x32.png` |
| imageres_mun | RT_ICON | 2549 (group 8312) | 24x24 | 32 | `icons/icon_8312_6_24x24.png` |
| imageres_mun | RT_ICON | 255 (group 34) | 20x20 | 32 | `icons/icon_34_6_20x20.png` |
| imageres_mun | RT_ICON | 2550 (group 8312) | 20x20 | 32 | `icons/icon_8312_7_20x20.png` |
| imageres_mun | RT_ICON | 2551 (group 8312) | 16x16 | 32 | `icons/icon_8312_8_16x16.png` |
| imageres_mun | RT_ICON | 2552 (group 8313) | 256x256 | 32 | `icons/icon_8313_0_256x256.png` |
| imageres_mun | RT_ICON | 2553 (group 8313) | 96x96 | 32 | `icons/icon_8313_1_96x96.png` |
| imageres_mun | RT_ICON | 2554 (group 8313) | 48x48 | 32 | `icons/icon_8313_2_48x48.png` |
| imageres_mun | RT_ICON | 2555 (group 8313) | 40x40 | 32 | `icons/icon_8313_3_40x40.png` |
| imageres_mun | RT_ICON | 2556 (group 8313) | 32x32 | 32 | `icons/icon_8313_4_32x32.png` |
| imageres_mun | RT_ICON | 2557 (group 8313) | 24x24 | 32 | `icons/icon_8313_5_24x24.png` |
| imageres_mun | RT_ICON | 2558 (group 8313) | 20x20 | 32 | `icons/icon_8313_6_20x20.png` |
| imageres_mun | RT_ICON | 2559 (group 8313) | 16x16 | 32 | `icons/icon_8313_7_16x16.png` |
| imageres_mun | RT_ICON | 256 (group 34) | 16x16 | 32 | `icons/icon_34_7_16x16.png` |
| imageres_mun | RT_ICON | 2560 (group 8314) | 256x256 | 32 | `icons/icon_8314_0_256x256.png` |
| imageres_mun | RT_ICON | 2561 (group 8314) | 96x96 | 32 | `icons/icon_8314_1_96x96.png` |
| imageres_mun | RT_ICON | 2562 (group 8314) | 48x48 | 32 | `icons/icon_8314_2_48x48.png` |
| imageres_mun | RT_ICON | 2563 (group 8314) | 40x40 | 32 | `icons/icon_8314_3_40x40.png` |
| imageres_mun | RT_ICON | 2564 (group 8314) | 32x32 | 32 | `icons/icon_8314_4_32x32.png` |
| imageres_mun | RT_ICON | 2565 (group 8314) | 24x24 | 32 | `icons/icon_8314_5_24x24.png` |
| imageres_mun | RT_ICON | 2566 (group 8314) | 20x20 | 32 | `icons/icon_8314_6_20x20.png` |
| imageres_mun | RT_ICON | 2567 (group 8314) | 16x16 | 32 | `icons/icon_8314_7_16x16.png` |
| imageres_mun | RT_ICON | 2568 (group 8315) | 256x256 | 32 | `icons/icon_8315_0_256x256.png` |
| imageres_mun | RT_ICON | 2569 (group 8315) | 64x64 | 32 | `icons/icon_8315_1_64x64.png` |
| imageres_mun | RT_ICON | 257 (group 35) | 256x256 | 32 | `icons/icon_35_0_256x256.png` |
| imageres_mun | RT_ICON | 2570 (group 8315) | 60x60 | 32 | `icons/icon_8315_2_60x60.png` |
| imageres_mun | RT_ICON | 2571 (group 8315) | 48x48 | 32 | `icons/icon_8315_3_48x48.png` |
| imageres_mun | RT_ICON | 2572 (group 8315) | 40x40 | 32 | `icons/icon_8315_4_40x40.png` |
| imageres_mun | RT_ICON | 2573 (group 8315) | 32x32 | 32 | `icons/icon_8315_5_32x32.png` |
| imageres_mun | RT_ICON | 2574 (group 8315) | 24x24 | 32 | `icons/icon_8315_6_24x24.png` |
| imageres_mun | RT_ICON | 2575 (group 8315) | 20x20 | 32 | `icons/icon_8315_7_20x20.png` |
| imageres_mun | RT_ICON | 2576 (group 8315) | 16x16 | 32 | `icons/icon_8315_8_16x16.png` |
| imageres_mun | RT_ICON | 2577 (group 8316) | 256x256 | 32 | `icons/icon_8316_0_256x256.png` |
| imageres_mun | RT_ICON | 2578 (group 8316) | 96x96 | 32 | `icons/icon_8316_1_96x96.png` |
| imageres_mun | RT_ICON | 2579 (group 8316) | 48x48 | 32 | `icons/icon_8316_2_48x48.png` |
| imageres_mun | RT_ICON | 258 (group 35) | 64x64 | 32 | `icons/icon_35_1_64x64.png` |
| imageres_mun | RT_ICON | 2580 (group 8316) | 40x40 | 32 | `icons/icon_8316_3_40x40.png` |
| imageres_mun | RT_ICON | 2581 (group 8316) | 32x32 | 32 | `icons/icon_8316_4_32x32.png` |
| imageres_mun | RT_ICON | 2582 (group 8316) | 24x24 | 32 | `icons/icon_8316_5_24x24.png` |
| imageres_mun | RT_ICON | 2583 (group 8316) | 20x20 | 32 | `icons/icon_8316_6_20x20.png` |
| imageres_mun | RT_ICON | 2584 (group 8316) | 16x16 | 32 | `icons/icon_8316_7_16x16.png` |
| imageres_mun | RT_ICON | 2585 (group 8342) | 256x256 | 32 | `icons/icon_8342_0_256x256.png` |
| imageres_mun | RT_ICON | 2586 (group 8342) | 64x64 | 32 | `icons/icon_8342_1_64x64.png` |
| imageres_mun | RT_ICON | 2587 (group 8342) | 60x60 | 32 | `icons/icon_8342_2_60x60.png` |
| imageres_mun | RT_ICON | 2588 (group 8342) | 48x48 | 32 | `icons/icon_8342_3_48x48.png` |
| imageres_mun | RT_ICON | 2589 (group 8342) | 40x40 | 32 | `icons/icon_8342_4_40x40.png` |
| imageres_mun | RT_ICON | 259 (group 35) | 48x48 | 32 | `icons/icon_35_2_48x48.png` |
| imageres_mun | RT_ICON | 2590 (group 8342) | 32x32 | 32 | `icons/icon_8342_5_32x32.png` |
| imageres_mun | RT_ICON | 2591 (group 8342) | 24x24 | 32 | `icons/icon_8342_6_24x24.png` |
| imageres_mun | RT_ICON | 2592 (group 8342) | 20x20 | 32 | `icons/icon_8342_7_20x20.png` |
| imageres_mun | RT_ICON | 2593 (group 8342) | 16x16 | 32 | `icons/icon_8342_8_16x16.png` |
| imageres_mun | RT_ICON | 2594 (group 8317) | 256x256 | 32 | `icons/icon_8317_0_256x256.png` |
| imageres_mun | RT_ICON | 2595 (group 8317) | 96x96 | 32 | `icons/icon_8317_1_96x96.png` |
| imageres_mun | RT_ICON | 2596 (group 8317) | 48x48 | 32 | `icons/icon_8317_2_48x48.png` |
| imageres_mun | RT_ICON | 2597 (group 8317) | 40x40 | 32 | `icons/icon_8317_3_40x40.png` |
| imageres_mun | RT_ICON | 2598 (group 8317) | 32x32 | 32 | `icons/icon_8317_4_32x32.png` |
| imageres_mun | RT_ICON | 2599 (group 8317) | 24x24 | 32 | `icons/icon_8317_5_24x24.png` |
| imageres_mun | RT_ICON | 26 (group 5) | 64x64 | 32 | `icons/icon_5_1_64x64.png` |
| imageres_mun | RT_ICON | 260 (group 35) | 40x40 | 32 | `icons/icon_35_3_40x40.png` |
| imageres_mun | RT_ICON | 2600 (group 8317) | 20x20 | 32 | `icons/icon_8317_6_20x20.png` |
| imageres_mun | RT_ICON | 2601 (group 8317) | 16x16 | 32 | `icons/icon_8317_7_16x16.png` |
| imageres_mun | RT_ICON | 2602 (group 8318) | 256x256 | 32 | `icons/icon_8318_0_256x256.png` |
| imageres_mun | RT_ICON | 2603 (group 8318) | 96x96 | 32 | `icons/icon_8318_1_96x96.png` |
| imageres_mun | RT_ICON | 2604 (group 8318) | 48x48 | 32 | `icons/icon_8318_2_48x48.png` |
| imageres_mun | RT_ICON | 2605 (group 8318) | 40x40 | 32 | `icons/icon_8318_3_40x40.png` |
| imageres_mun | RT_ICON | 2606 (group 8318) | 32x32 | 32 | `icons/icon_8318_4_32x32.png` |
| imageres_mun | RT_ICON | 2607 (group 8318) | 24x24 | 32 | `icons/icon_8318_5_24x24.png` |
| imageres_mun | RT_ICON | 2608 (group 8318) | 20x20 | 32 | `icons/icon_8318_6_20x20.png` |
| imageres_mun | RT_ICON | 2609 (group 8318) | 16x16 | 32 | `icons/icon_8318_7_16x16.png` |
| imageres_mun | RT_ICON | 261 (group 35) | 32x32 | 32 | `icons/icon_35_4_32x32.png` |
| imageres_mun | RT_ICON | 2610 (group 8319) | 256x256 | 32 | `icons/icon_8319_0_256x256.png` |
| imageres_mun | RT_ICON | 2611 (group 8319) | 96x96 | 32 | `icons/icon_8319_1_96x96.png` |
| imageres_mun | RT_ICON | 2612 (group 8319) | 48x48 | 32 | `icons/icon_8319_2_48x48.png` |
| imageres_mun | RT_ICON | 2613 (group 8319) | 40x40 | 32 | `icons/icon_8319_3_40x40.png` |
| imageres_mun | RT_ICON | 2614 (group 8319) | 32x32 | 32 | `icons/icon_8319_4_32x32.png` |
| imageres_mun | RT_ICON | 2615 (group 8319) | 24x24 | 32 | `icons/icon_8319_5_24x24.png` |
| imageres_mun | RT_ICON | 2616 (group 8319) | 20x20 | 32 | `icons/icon_8319_6_20x20.png` |
| imageres_mun | RT_ICON | 2617 (group 8319) | 16x16 | 32 | `icons/icon_8319_7_16x16.png` |
| imageres_mun | RT_ICON | 2618 (group 8343) | 256x256 | 32 | `icons/icon_8343_0_256x256.png` |
| imageres_mun | RT_ICON | 2619 (group 8343) | 64x64 | 32 | `icons/icon_8343_1_64x64.png` |
| imageres_mun | RT_ICON | 262 (group 35) | 24x24 | 32 | `icons/icon_35_5_24x24.png` |
| imageres_mun | RT_ICON | 2620 (group 8343) | 60x60 | 32 | `icons/icon_8343_2_60x60.png` |
| imageres_mun | RT_ICON | 2621 (group 8343) | 48x48 | 32 | `icons/icon_8343_3_48x48.png` |
| imageres_mun | RT_ICON | 2622 (group 8343) | 40x40 | 32 | `icons/icon_8343_4_40x40.png` |
| imageres_mun | RT_ICON | 2623 (group 8343) | 32x32 | 32 | `icons/icon_8343_5_32x32.png` |
| imageres_mun | RT_ICON | 2624 (group 8343) | 24x24 | 32 | `icons/icon_8343_6_24x24.png` |
| imageres_mun | RT_ICON | 2625 (group 8343) | 20x20 | 32 | `icons/icon_8343_7_20x20.png` |
| imageres_mun | RT_ICON | 2626 (group 8343) | 16x16 | 32 | `icons/icon_8343_8_16x16.png` |
| imageres_mun | RT_ICON | 2627 (group 8320) | 256x256 | 32 | `icons/icon_8320_0_256x256.png` |
| imageres_mun | RT_ICON | 2628 (group 8320) | 64x64 | 32 | `icons/icon_8320_1_64x64.png` |
| imageres_mun | RT_ICON | 2629 (group 8320) | 60x60 | 32 | `icons/icon_8320_2_60x60.png` |
| imageres_mun | RT_ICON | 263 (group 35) | 20x20 | 32 | `icons/icon_35_6_20x20.png` |
| imageres_mun | RT_ICON | 2630 (group 8320) | 48x48 | 32 | `icons/icon_8320_3_48x48.png` |
| imageres_mun | RT_ICON | 2631 (group 8320) | 40x40 | 32 | `icons/icon_8320_4_40x40.png` |
| imageres_mun | RT_ICON | 2632 (group 8320) | 32x32 | 32 | `icons/icon_8320_5_32x32.png` |
| imageres_mun | RT_ICON | 2633 (group 8320) | 24x24 | 32 | `icons/icon_8320_6_24x24.png` |
| imageres_mun | RT_ICON | 2634 (group 8320) | 20x20 | 32 | `icons/icon_8320_7_20x20.png` |
| imageres_mun | RT_ICON | 2635 (group 8320) | 16x16 | 32 | `icons/icon_8320_8_16x16.png` |
| imageres_mun | RT_ICON | 2636 (group 8321) | 256x256 | 32 | `icons/icon_8321_0_256x256.png` |
| imageres_mun | RT_ICON | 2637 (group 8321) | 64x64 | 32 | `icons/icon_8321_1_64x64.png` |
| imageres_mun | RT_ICON | 2638 (group 8321) | 60x60 | 32 | `icons/icon_8321_2_60x60.png` |
| imageres_mun | RT_ICON | 2639 (group 8321) | 48x48 | 32 | `icons/icon_8321_3_48x48.png` |
| imageres_mun | RT_ICON | 264 (group 35) | 16x16 | 32 | `icons/icon_35_7_16x16.png` |
| imageres_mun | RT_ICON | 2640 (group 8321) | 40x40 | 32 | `icons/icon_8321_4_40x40.png` |
| imageres_mun | RT_ICON | 2641 (group 8321) | 32x32 | 32 | `icons/icon_8321_5_32x32.png` |
| imageres_mun | RT_ICON | 2642 (group 8321) | 24x24 | 32 | `icons/icon_8321_6_24x24.png` |
| imageres_mun | RT_ICON | 2643 (group 8321) | 20x20 | 32 | `icons/icon_8321_7_20x20.png` |
| imageres_mun | RT_ICON | 2644 (group 8321) | 16x16 | 32 | `icons/icon_8321_8_16x16.png` |
| imageres_mun | RT_ICON | 2645 (group 8322) | 256x256 | 32 | `icons/icon_8322_0_256x256.png` |
| imageres_mun | RT_ICON | 2646 (group 8322) | 96x96 | 32 | `icons/icon_8322_1_96x96.png` |
| imageres_mun | RT_ICON | 2647 (group 8322) | 40x40 | 32 | `icons/icon_8322_2_40x40.png` |
| imageres_mun | RT_ICON | 2648 (group 8322) | 32x32 | 32 | `icons/icon_8322_3_32x32.png` |
| imageres_mun | RT_ICON | 2649 (group 8322) | 24x24 | 32 | `icons/icon_8322_4_24x24.png` |
| imageres_mun | RT_ICON | 265 (group 1035) | 256x256 | 32 | `icons/icon_1035_0_256x256.png` |
| imageres_mun | RT_ICON | 2650 (group 8322) | 20x20 | 32 | `icons/icon_8322_5_20x20.png` |
| imageres_mun | RT_ICON | 2651 (group 8322) | 16x16 | 32 | `icons/icon_8322_6_16x16.png` |
| imageres_mun | RT_ICON | 2652 (group 8336) | 256x256 | 32 | `icons/icon_8336_0_256x256.png` |
| imageres_mun | RT_ICON | 2653 (group 8336) | 64x64 | 32 | `icons/icon_8336_1_64x64.png` |
| imageres_mun | RT_ICON | 2654 (group 8336) | 60x60 | 32 | `icons/icon_8336_2_60x60.png` |
| imageres_mun | RT_ICON | 2655 (group 8336) | 48x48 | 32 | `icons/icon_8336_3_48x48.png` |
| imageres_mun | RT_ICON | 2656 (group 8336) | 40x40 | 32 | `icons/icon_8336_4_40x40.png` |
| imageres_mun | RT_ICON | 2657 (group 8336) | 32x32 | 32 | `icons/icon_8336_5_32x32.png` |
| imageres_mun | RT_ICON | 2658 (group 8336) | 24x24 | 32 | `icons/icon_8336_6_24x24.png` |
| imageres_mun | RT_ICON | 2659 (group 8336) | 20x20 | 32 | `icons/icon_8336_7_20x20.png` |
| imageres_mun | RT_ICON | 266 (group 1035) | 64x64 | 32 | `icons/icon_1035_1_64x64.png` |
| imageres_mun | RT_ICON | 2660 (group 8336) | 16x16 | 32 | `icons/icon_8336_8_16x16.png` |
| imageres_mun | RT_ICON | 2661 (group 8337) | 256x256 | 32 | `icons/icon_8337_0_256x256.png` |
| imageres_mun | RT_ICON | 2662 (group 8337) | 96x96 | 32 | `icons/icon_8337_1_96x96.png` |
| imageres_mun | RT_ICON | 2663 (group 8337) | 48x48 | 32 | `icons/icon_8337_2_48x48.png` |
| imageres_mun | RT_ICON | 2664 (group 8337) | 40x40 | 32 | `icons/icon_8337_3_40x40.png` |
| imageres_mun | RT_ICON | 2665 (group 8337) | 32x32 | 32 | `icons/icon_8337_4_32x32.png` |
| imageres_mun | RT_ICON | 2666 (group 8337) | 24x24 | 32 | `icons/icon_8337_5_24x24.png` |
| imageres_mun | RT_ICON | 2667 (group 8337) | 20x20 | 32 | `icons/icon_8337_6_20x20.png` |
| imageres_mun | RT_ICON | 2668 (group 8337) | 16x16 | 32 | `icons/icon_8337_7_16x16.png` |
| imageres_mun | RT_ICON | 2669 (group 8338) | 256x256 | 32 | `icons/icon_8338_0_256x256.png` |
| imageres_mun | RT_ICON | 267 (group 1035) | 48x48 | 32 | `icons/icon_1035_2_48x48.png` |
| imageres_mun | RT_ICON | 2670 (group 8338) | 64x64 | 32 | `icons/icon_8338_1_64x64.png` |
| imageres_mun | RT_ICON | 2671 (group 8338) | 60x60 | 32 | `icons/icon_8338_2_60x60.png` |
| imageres_mun | RT_ICON | 2672 (group 8338) | 48x48 | 32 | `icons/icon_8338_3_48x48.png` |
| imageres_mun | RT_ICON | 2673 (group 8338) | 40x40 | 32 | `icons/icon_8338_4_40x40.png` |
| imageres_mun | RT_ICON | 2674 (group 8338) | 32x32 | 32 | `icons/icon_8338_5_32x32.png` |
| imageres_mun | RT_ICON | 2675 (group 8338) | 24x24 | 32 | `icons/icon_8338_6_24x24.png` |
| imageres_mun | RT_ICON | 2676 (group 8338) | 20x20 | 32 | `icons/icon_8338_7_20x20.png` |
| imageres_mun | RT_ICON | 2677 (group 8338) | 16x16 | 32 | `icons/icon_8338_8_16x16.png` |
| imageres_mun | RT_ICON | 2678 (group 8339) | 256x256 | 32 | `icons/icon_8339_0_256x256.png` |
| imageres_mun | RT_ICON | 2679 (group 8339) | 96x96 | 32 | `icons/icon_8339_1_96x96.png` |
| imageres_mun | RT_ICON | 268 (group 1035) | 40x40 | 32 | `icons/icon_1035_3_40x40.png` |
| imageres_mun | RT_ICON | 2680 (group 8339) | 48x48 | 32 | `icons/icon_8339_2_48x48.png` |
| imageres_mun | RT_ICON | 2681 (group 8339) | 40x40 | 32 | `icons/icon_8339_3_40x40.png` |
| imageres_mun | RT_ICON | 2682 (group 8339) | 36x36 | 32 | `icons/icon_8339_4_36x36.png` |
| imageres_mun | RT_ICON | 2683 (group 8339) | 24x24 | 32 | `icons/icon_8339_5_24x24.png` |
| imageres_mun | RT_ICON | 2684 (group 8339) | 20x20 | 32 | `icons/icon_8339_6_20x20.png` |
| imageres_mun | RT_ICON | 2685 (group 8339) | 16x16 | 32 | `icons/icon_8339_7_16x16.png` |
| imageres_mun | RT_ICON | 2686 (group 5402) | 64x64 | 32 | `icons/icon_5402_0_64x64.png` |
| imageres_mun | RT_ICON | 2687 (group 5402) | 50x50 | 32 | `icons/icon_5402_1_50x50.png` |
| imageres_mun | RT_ICON | 2688 (group 5402) | 36x36 | 32 | `icons/icon_5402_2_36x36.png` |
| imageres_mun | RT_ICON | 2689 (group 5403) | 64x64 | 32 | `icons/icon_5403_0_64x64.png` |
| imageres_mun | RT_ICON | 269 (group 1035) | 32x32 | 32 | `icons/icon_1035_4_32x32.png` |
| imageres_mun | RT_ICON | 2690 (group 5403) | 50x50 | 32 | `icons/icon_5403_1_50x50.png` |
| imageres_mun | RT_ICON | 2691 (group 5403) | 36x36 | 32 | `icons/icon_5403_2_36x36.png` |
| imageres_mun | RT_ICON | 2692 (group 5404) | 64x64 | 32 | `icons/icon_5404_0_64x64.png` |
| imageres_mun | RT_ICON | 2693 (group 5404) | 50x50 | 32 | `icons/icon_5404_1_50x50.png` |
| imageres_mun | RT_ICON | 2694 (group 5404) | 36x36 | 32 | `icons/icon_5404_2_36x36.png` |
| imageres_mun | RT_ICON | 2695 (group 5405) | 64x64 | 32 | `icons/icon_5405_0_64x64.png` |
| imageres_mun | RT_ICON | 2696 (group 5405) | 50x50 | 32 | `icons/icon_5405_1_50x50.png` |
| imageres_mun | RT_ICON | 2697 (group 5405) | 36x36 | 32 | `icons/icon_5405_2_36x36.png` |
| imageres_mun | RT_ICON | 2698 (group 5406) | 64x64 | 32 | `icons/icon_5406_0_64x64.png` |
| imageres_mun | RT_ICON | 2699 (group 5406) | 50x50 | 32 | `icons/icon_5406_1_50x50.png` |
| imageres_mun | RT_ICON | 27 (group 5) | 48x48 | 32 | `icons/icon_5_2_48x48.png` |
| imageres_mun | RT_ICON | 270 (group 1035) | 24x24 | 32 | `icons/icon_1035_5_24x24.png` |
| imageres_mun | RT_ICON | 2700 (group 5406) | 36x36 | 32 | `icons/icon_5406_2_36x36.png` |
| imageres_mun | RT_ICON | 2701 (group 5407) | 64x64 | 32 | `icons/icon_5407_0_64x64.png` |
| imageres_mun | RT_ICON | 2702 (group 5407) | 50x50 | 32 | `icons/icon_5407_1_50x50.png` |
| imageres_mun | RT_ICON | 2703 (group 5407) | 36x36 | 32 | `icons/icon_5407_2_36x36.png` |
| imageres_mun | RT_ICON | 2704 (group 5408) | 64x64 | 32 | `icons/icon_5408_0_64x64.png` |
| imageres_mun | RT_ICON | 2705 (group 5408) | 50x50 | 32 | `icons/icon_5408_1_50x50.png` |
| imageres_mun | RT_ICON | 2706 (group 5408) | 36x36 | 32 | `icons/icon_5408_2_36x36.png` |
| imageres_mun | RT_ICON | 2707 (group 5409) | 64x64 | 32 | `icons/icon_5409_0_64x64.png` |
| imageres_mun | RT_ICON | 2708 (group 5409) | 50x50 | 32 | `icons/icon_5409_1_50x50.png` |
| imageres_mun | RT_ICON | 2709 (group 5409) | 36x36 | 32 | `icons/icon_5409_2_36x36.png` |
| imageres_mun | RT_ICON | 271 (group 1035) | 20x20 | 32 | `icons/icon_1035_6_20x20.png` |
| imageres_mun | RT_ICON | 2710 (group 5411) | 28x28 | 32 | `icons/icon_5411_0_28x28.png` |
| imageres_mun | RT_ICON | 2711 (group 5411) | 22x22 | 32 | `icons/icon_5411_1_22x22.png` |
| imageres_mun | RT_ICON | 2712 (group 5411) | 16x16 | 32 | `icons/icon_5411_2_16x16.png` |
| imageres_mun | RT_ICON | 2713 (group 5412) | 28x28 | 32 | `icons/icon_5412_0_28x28.png` |
| imageres_mun | RT_ICON | 2714 (group 5412) | 22x22 | 32 | `icons/icon_5412_1_22x22.png` |
| imageres_mun | RT_ICON | 2715 (group 5412) | 16x16 | 32 | `icons/icon_5412_2_16x16.png` |
| imageres_mun | RT_ICON | 2716 (group 1301) | 16x16 | 4 | `icons/icon_1301_0_16x16.png` |
| imageres_mun | RT_ICON | 2717 (group 1302) | 16x16 | 4 | `icons/icon_1302_0_16x16.png` |
| imageres_mun | RT_ICON | 2718 (group 1303) | 16x16 | 4 | `icons/icon_1303_0_16x16.png` |
| imageres_mun | RT_ICON | 2719 (group 1304) | 256x256 | 32 | `icons/icon_1304_0_256x256.png` |
| imageres_mun | RT_ICON | 272 (group 1035) | 16x16 | 32 | `icons/icon_1035_7_16x16.png` |
| imageres_mun | RT_ICON | 2720 (group 1304) | 64x64 | 32 | `icons/icon_1304_1_64x64.png` |
| imageres_mun | RT_ICON | 2721 (group 1304) | 48x48 | 32 | `icons/icon_1304_2_48x48.png` |
| imageres_mun | RT_ICON | 2722 (group 1304) | 40x40 | 32 | `icons/icon_1304_3_40x40.png` |
| imageres_mun | RT_ICON | 2723 (group 1304) | 32x32 | 32 | `icons/icon_1304_4_32x32.png` |
| imageres_mun | RT_ICON | 2724 (group 1304) | 24x24 | 32 | `icons/icon_1304_5_24x24.png` |
| imageres_mun | RT_ICON | 2725 (group 1304) | 20x20 | 32 | `icons/icon_1304_6_20x20.png` |
| imageres_mun | RT_ICON | 2726 (group 1304) | 16x16 | 32 | `icons/icon_1304_7_16x16.png` |
| imageres_mun | RT_ICON | 2727 (group 1306) | 256x256 | 32 | `icons/icon_1306_0_256x256.png` |
| imageres_mun | RT_ICON | 2728 (group 1306) | 64x64 | 32 | `icons/icon_1306_1_64x64.png` |
| imageres_mun | RT_ICON | 2729 (group 1306) | 48x48 | 32 | `icons/icon_1306_2_48x48.png` |
| imageres_mun | RT_ICON | 273 (group 1036) | 256x256 | 32 | `icons/icon_1036_0_256x256.png` |
| imageres_mun | RT_ICON | 2730 (group 1306) | 40x40 | 32 | `icons/icon_1306_3_40x40.png` |
| imageres_mun | RT_ICON | 2731 (group 1306) | 32x32 | 32 | `icons/icon_1306_4_32x32.png` |
| imageres_mun | RT_ICON | 2732 (group 1306) | 24x24 | 32 | `icons/icon_1306_5_24x24.png` |
| imageres_mun | RT_ICON | 2733 (group 1306) | 20x20 | 32 | `icons/icon_1306_6_20x20.png` |
| imageres_mun | RT_ICON | 2734 (group 1306) | 16x16 | 32 | `icons/icon_1306_7_16x16.png` |
| imageres_mun | RT_ICON | 2735 (group 1305) | 256x256 | 32 | `icons/icon_1305_0_256x256.png` |
| imageres_mun | RT_ICON | 2736 (group 1305) | 64x64 | 32 | `icons/icon_1305_1_64x64.png` |
| imageres_mun | RT_ICON | 2737 (group 1305) | 48x48 | 32 | `icons/icon_1305_2_48x48.png` |
| imageres_mun | RT_ICON | 2738 (group 1305) | 40x40 | 32 | `icons/icon_1305_3_40x40.png` |
| imageres_mun | RT_ICON | 2739 (group 1305) | 32x32 | 32 | `icons/icon_1305_4_32x32.png` |
| imageres_mun | RT_ICON | 274 (group 1036) | 64x64 | 32 | `icons/icon_1036_1_64x64.png` |
| imageres_mun | RT_ICON | 2740 (group 1305) | 24x24 | 32 | `icons/icon_1305_5_24x24.png` |
| imageres_mun | RT_ICON | 2741 (group 1305) | 20x20 | 32 | `icons/icon_1305_6_20x20.png` |
| imageres_mun | RT_ICON | 2742 (group 1305) | 16x16 | 32 | `icons/icon_1305_7_16x16.png` |
| imageres_mun | RT_ICON | 2743 (group 1400) | 60x60 | 32 | `icons/icon_1400_0_60x60.png` |
| imageres_mun | RT_ICON | 2744 (group 1400) | 48x48 | 32 | `icons/icon_1400_1_48x48.png` |
| imageres_mun | RT_ICON | 2745 (group 1400) | 36x36 | 32 | `icons/icon_1400_2_36x36.png` |
| imageres_mun | RT_ICON | 2746 (group 1400) | 30x30 | 32 | `icons/icon_1400_3_30x30.png` |
| imageres_mun | RT_ICON | 2747 (group 1400) | 27x27 | 32 | `icons/icon_1400_4_27x27.png` |
| imageres_mun | RT_ICON | 2748 (group 1400) | 24x24 | 32 | `icons/icon_1400_5_24x24.png` |
| imageres_mun | RT_ICON | 2749 (group 1400) | 21x21 | 32 | `icons/icon_1400_6_21x21.png` |
| imageres_mun | RT_ICON | 275 (group 1036) | 48x48 | 32 | `icons/icon_1036_2_48x48.png` |
| imageres_mun | RT_ICON | 2750 (group 1400) | 18x18 | 32 | `icons/icon_1400_7_18x18.png` |
| imageres_mun | RT_ICON | 2751 (group 1400) | 15x15 | 32 | `icons/icon_1400_8_15x15.png` |
| imageres_mun | RT_ICON | 2752 (group 1400) | 12x12 | 32 | `icons/icon_1400_9_12x12.png` |
| imageres_mun | RT_ICON | 2753 (group 1401) | 60x60 | 32 | `icons/icon_1401_0_60x60.png` |
| imageres_mun | RT_ICON | 2754 (group 1401) | 48x48 | 32 | `icons/icon_1401_1_48x48.png` |
| imageres_mun | RT_ICON | 2755 (group 1401) | 36x36 | 32 | `icons/icon_1401_2_36x36.png` |
| imageres_mun | RT_ICON | 2756 (group 1401) | 30x30 | 32 | `icons/icon_1401_3_30x30.png` |
| imageres_mun | RT_ICON | 2757 (group 1401) | 27x27 | 32 | `icons/icon_1401_4_27x27.png` |
| imageres_mun | RT_ICON | 2758 (group 1401) | 24x24 | 32 | `icons/icon_1401_5_24x24.png` |
| imageres_mun | RT_ICON | 2759 (group 1401) | 21x21 | 32 | `icons/icon_1401_6_21x21.png` |
| imageres_mun | RT_ICON | 276 (group 1036) | 40x40 | 32 | `icons/icon_1036_3_40x40.png` |
| imageres_mun | RT_ICON | 2760 (group 1401) | 18x18 | 32 | `icons/icon_1401_7_18x18.png` |
| imageres_mun | RT_ICON | 2761 (group 1401) | 15x15 | 32 | `icons/icon_1401_8_15x15.png` |
| imageres_mun | RT_ICON | 2762 (group 1401) | 12x12 | 32 | `icons/icon_1401_9_12x12.png` |
| imageres_mun | RT_ICON | 2763 (group 1402) | 60x60 | 32 | `icons/icon_1402_0_60x60.png` |
| imageres_mun | RT_ICON | 2764 (group 1402) | 48x48 | 32 | `icons/icon_1402_1_48x48.png` |
| imageres_mun | RT_ICON | 2765 (group 1402) | 36x36 | 32 | `icons/icon_1402_2_36x36.png` |
| imageres_mun | RT_ICON | 2766 (group 1402) | 30x30 | 32 | `icons/icon_1402_3_30x30.png` |
| imageres_mun | RT_ICON | 2767 (group 1402) | 27x27 | 32 | `icons/icon_1402_4_27x27.png` |
| imageres_mun | RT_ICON | 2768 (group 1402) | 24x24 | 32 | `icons/icon_1402_5_24x24.png` |
| imageres_mun | RT_ICON | 2769 (group 1402) | 21x21 | 32 | `icons/icon_1402_6_21x21.png` |
| imageres_mun | RT_ICON | 277 (group 1036) | 32x32 | 32 | `icons/icon_1036_4_32x32.png` |
| imageres_mun | RT_ICON | 2770 (group 1402) | 18x18 | 32 | `icons/icon_1402_7_18x18.png` |
| imageres_mun | RT_ICON | 2771 (group 1402) | 15x15 | 32 | `icons/icon_1402_8_15x15.png` |
| imageres_mun | RT_ICON | 2772 (group 1402) | 12x12 | 32 | `icons/icon_1402_9_12x12.png` |
| imageres_mun | RT_ICON | 2773 (group 1403) | 60x60 | 32 | `icons/icon_1403_0_60x60.png` |
| imageres_mun | RT_ICON | 2774 (group 1403) | 48x48 | 32 | `icons/icon_1403_1_48x48.png` |
| imageres_mun | RT_ICON | 2775 (group 1403) | 36x36 | 32 | `icons/icon_1403_2_36x36.png` |
| imageres_mun | RT_ICON | 2776 (group 1403) | 30x30 | 32 | `icons/icon_1403_3_30x30.png` |
| imageres_mun | RT_ICON | 2777 (group 1403) | 27x27 | 32 | `icons/icon_1403_4_27x27.png` |
| imageres_mun | RT_ICON | 2778 (group 1403) | 24x24 | 32 | `icons/icon_1403_5_24x24.png` |
| imageres_mun | RT_ICON | 2779 (group 1403) | 21x21 | 32 | `icons/icon_1403_6_21x21.png` |
| imageres_mun | RT_ICON | 278 (group 1036) | 24x24 | 32 | `icons/icon_1036_5_24x24.png` |
| imageres_mun | RT_ICON | 2780 (group 1403) | 18x18 | 32 | `icons/icon_1403_7_18x18.png` |
| imageres_mun | RT_ICON | 2781 (group 1403) | 15x15 | 32 | `icons/icon_1403_8_15x15.png` |
| imageres_mun | RT_ICON | 2782 (group 1403) | 12x12 | 32 | `icons/icon_1403_9_12x12.png` |
| imageres_mun | RT_ICON | 2783 (group 1404) | 60x60 | 32 | `icons/icon_1404_0_60x60.png` |
| imageres_mun | RT_ICON | 2784 (group 1404) | 48x48 | 32 | `icons/icon_1404_1_48x48.png` |
| imageres_mun | RT_ICON | 2785 (group 1404) | 36x36 | 32 | `icons/icon_1404_2_36x36.png` |
| imageres_mun | RT_ICON | 2786 (group 1404) | 30x30 | 32 | `icons/icon_1404_3_30x30.png` |
| imageres_mun | RT_ICON | 2787 (group 1404) | 27x27 | 32 | `icons/icon_1404_4_27x27.png` |
| imageres_mun | RT_ICON | 2788 (group 1404) | 24x24 | 32 | `icons/icon_1404_5_24x24.png` |
| imageres_mun | RT_ICON | 2789 (group 1404) | 21x21 | 32 | `icons/icon_1404_6_21x21.png` |
| imageres_mun | RT_ICON | 279 (group 1036) | 20x20 | 32 | `icons/icon_1036_6_20x20.png` |
| imageres_mun | RT_ICON | 2790 (group 1404) | 18x18 | 32 | `icons/icon_1404_7_18x18.png` |
| imageres_mun | RT_ICON | 2791 (group 1404) | 15x15 | 32 | `icons/icon_1404_8_15x15.png` |
| imageres_mun | RT_ICON | 2792 (group 1404) | 12x12 | 32 | `icons/icon_1404_9_12x12.png` |
| imageres_mun | RT_ICON | 2793 (group 1405) | 60x60 | 32 | `icons/icon_1405_0_60x60.png` |
| imageres_mun | RT_ICON | 2794 (group 1405) | 48x48 | 32 | `icons/icon_1405_1_48x48.png` |
| imageres_mun | RT_ICON | 2795 (group 1405) | 36x36 | 32 | `icons/icon_1405_2_36x36.png` |
| imageres_mun | RT_ICON | 2796 (group 1405) | 30x30 | 32 | `icons/icon_1405_3_30x30.png` |
| imageres_mun | RT_ICON | 2797 (group 1405) | 27x27 | 32 | `icons/icon_1405_4_27x27.png` |
| imageres_mun | RT_ICON | 2798 (group 1405) | 24x24 | 32 | `icons/icon_1405_5_24x24.png` |
| imageres_mun | RT_ICON | 2799 (group 1405) | 21x21 | 32 | `icons/icon_1405_6_21x21.png` |
| imageres_mun | RT_ICON | 28 (group 5) | 40x40 | 32 | `icons/icon_5_3_40x40.png` |
| imageres_mun | RT_ICON | 280 (group 1036) | 16x16 | 32 | `icons/icon_1036_7_16x16.png` |
| imageres_mun | RT_ICON | 2800 (group 1405) | 18x18 | 32 | `icons/icon_1405_7_18x18.png` |
| imageres_mun | RT_ICON | 2801 (group 1405) | 15x15 | 32 | `icons/icon_1405_8_15x15.png` |
| imageres_mun | RT_ICON | 2802 (group 1405) | 12x12 | 32 | `icons/icon_1405_9_12x12.png` |
| imageres_mun | RT_ICON | 2803 (group 1028) | 256x256 | 32 | `icons/icon_1028_0_256x256.png` |
| imageres_mun | RT_ICON | 2804 (group 1028) | 64x64 | 32 | `icons/icon_1028_1_64x64.png` |
| imageres_mun | RT_ICON | 2805 (group 1028) | 48x48 | 32 | `icons/icon_1028_2_48x48.png` |
| imageres_mun | RT_ICON | 2806 (group 1028) | 40x40 | 32 | `icons/icon_1028_3_40x40.png` |
| imageres_mun | RT_ICON | 2807 (group 1028) | 32x32 | 32 | `icons/icon_1028_4_32x32.png` |
| imageres_mun | RT_ICON | 2808 (group 1028) | 24x24 | 32 | `icons/icon_1028_5_24x24.png` |
| imageres_mun | RT_ICON | 2809 (group 1028) | 20x20 | 32 | `icons/icon_1028_6_20x20.png` |
| imageres_mun | RT_ICON | 281 (group 1037) | 256x256 | 32 | `icons/icon_1037_0_256x256.png` |
| imageres_mun | RT_ICON | 2810 (group 1028) | 16x16 | 32 | `icons/icon_1028_7_16x16.png` |
| imageres_mun | RT_ICON | 2811 (group 8201) | 256x256 | 32 | `icons/icon_8201_0_256x256.png` |
| imageres_mun | RT_ICON | 2812 (group 8201) | 64x64 | 32 | `icons/icon_8201_1_64x64.png` |
| imageres_mun | RT_ICON | 2813 (group 8201) | 48x48 | 32 | `icons/icon_8201_2_48x48.png` |
| imageres_mun | RT_ICON | 2814 (group 8201) | 40x40 | 32 | `icons/icon_8201_3_40x40.png` |
| imageres_mun | RT_ICON | 2815 (group 8201) | 32x32 | 32 | `icons/icon_8201_4_32x32.png` |
| imageres_mun | RT_ICON | 2816 (group 8201) | 24x24 | 32 | `icons/icon_8201_5_24x24.png` |
| imageres_mun | RT_ICON | 2817 (group 8201) | 20x20 | 32 | `icons/icon_8201_6_20x20.png` |
| imageres_mun | RT_ICON | 2818 (group 8201) | 16x16 | 32 | `icons/icon_8201_7_16x16.png` |
| imageres_mun | RT_ICON | 2819 (group 8205) | 256x256 | 32 | `icons/icon_8205_0_256x256.png` |
| imageres_mun | RT_ICON | 282 (group 1037) | 64x64 | 32 | `icons/icon_1037_1_64x64.png` |
| imageres_mun | RT_ICON | 2820 (group 8205) | 64x64 | 32 | `icons/icon_8205_1_64x64.png` |
| imageres_mun | RT_ICON | 2821 (group 8205) | 48x48 | 32 | `icons/icon_8205_2_48x48.png` |
| imageres_mun | RT_ICON | 2822 (group 8205) | 40x40 | 32 | `icons/icon_8205_3_40x40.png` |
| imageres_mun | RT_ICON | 2823 (group 8205) | 32x32 | 32 | `icons/icon_8205_4_32x32.png` |
| imageres_mun | RT_ICON | 2824 (group 8205) | 24x24 | 32 | `icons/icon_8205_5_24x24.png` |
| imageres_mun | RT_ICON | 2825 (group 8205) | 20x20 | 32 | `icons/icon_8205_6_20x20.png` |
| imageres_mun | RT_ICON | 2826 (group 8205) | 16x16 | 32 | `icons/icon_8205_7_16x16.png` |
| imageres_mun | RT_ICON | 2827 (group 8203) | 48x48 | 32 | `icons/icon_8203_0_48x48.png` |
| imageres_mun | RT_ICON | 2828 (group 8203) | 36x36 | 32 | `icons/icon_8203_1_36x36.png` |
| imageres_mun | RT_ICON | 2829 (group 8203) | 24x24 | 32 | `icons/icon_8203_2_24x24.png` |
| imageres_mun | RT_ICON | 283 (group 1037) | 48x48 | 32 | `icons/icon_1037_2_48x48.png` |
| imageres_mun | RT_ICON | 2830 (group 8203) | 18x18 | 32 | `icons/icon_8203_3_18x18.png` |
| imageres_mun | RT_ICON | 2831 (group 8203) | 16x16 | 32 | `icons/icon_8203_4_16x16.png` |
| imageres_mun | RT_ICON | 2832 (group 8203) | 12x12 | 32 | `icons/icon_8203_5_12x12.png` |
| imageres_mun | RT_ICON | 284 (group 1037) | 40x40 | 32 | `icons/icon_1037_3_40x40.png` |
| imageres_mun | RT_ICON | 285 (group 1037) | 32x32 | 32 | `icons/icon_1037_4_32x32.png` |
| imageres_mun | RT_ICON | 286 (group 1037) | 24x24 | 32 | `icons/icon_1037_5_24x24.png` |
| imageres_mun | RT_ICON | 287 (group 1037) | 20x20 | 32 | `icons/icon_1037_6_20x20.png` |
| imageres_mun | RT_ICON | 288 (group 1037) | 16x16 | 32 | `icons/icon_1037_7_16x16.png` |
| imageres_mun | RT_ICON | 289 (group 1040) | 256x256 | 32 | `icons/icon_1040_0_256x256.png` |
| imageres_mun | RT_ICON | 29 (group 5) | 32x32 | 32 | `icons/icon_5_4_32x32.png` |
| imageres_mun | RT_ICON | 290 (group 1040) | 64x64 | 32 | `icons/icon_1040_1_64x64.png` |
| imageres_mun | RT_ICON | 291 (group 1040) | 48x48 | 32 | `icons/icon_1040_2_48x48.png` |
| imageres_mun | RT_ICON | 292 (group 1040) | 40x40 | 32 | `icons/icon_1040_3_40x40.png` |
| imageres_mun | RT_ICON | 293 (group 1040) | 32x32 | 32 | `icons/icon_1040_4_32x32.png` |
| imageres_mun | RT_ICON | 294 (group 1040) | 24x24 | 32 | `icons/icon_1040_5_24x24.png` |
| imageres_mun | RT_ICON | 295 (group 1040) | 20x20 | 32 | `icons/icon_1040_6_20x20.png` |
| imageres_mun | RT_ICON | 296 (group 1040) | 16x16 | 32 | `icons/icon_1040_7_16x16.png` |
| imageres_mun | RT_ICON | 297 (group 1043) | 256x256 | 32 | `icons/icon_1043_0_256x256.png` |
| imageres_mun | RT_ICON | 298 (group 1043) | 64x64 | 32 | `icons/icon_1043_1_64x64.png` |
| imageres_mun | RT_ICON | 299 (group 1043) | 48x48 | 32 | `icons/icon_1043_2_48x48.png` |
| imageres_mun | RT_ICON | 3 (group 2) | 48x48 | 32 | `icons/icon_2_2_48x48.png` |
| imageres_mun | RT_ICON | 30 (group 5) | 24x24 | 32 | `icons/icon_5_5_24x24.png` |
| imageres_mun | RT_ICON | 300 (group 1043) | 40x40 | 32 | `icons/icon_1043_3_40x40.png` |
| imageres_mun | RT_ICON | 301 (group 1043) | 32x32 | 32 | `icons/icon_1043_4_32x32.png` |
| imageres_mun | RT_ICON | 302 (group 1043) | 24x24 | 32 | `icons/icon_1043_5_24x24.png` |
| imageres_mun | RT_ICON | 303 (group 1043) | 20x20 | 32 | `icons/icon_1043_6_20x20.png` |
| imageres_mun | RT_ICON | 304 (group 1043) | 16x16 | 32 | `icons/icon_1043_7_16x16.png` |
| imageres_mun | RT_ICON | 305 (group 36) | 256x256 | 32 | `icons/icon_36_0_256x256.png` |
| imageres_mun | RT_ICON | 306 (group 36) | 64x64 | 32 | `icons/icon_36_1_64x64.png` |
| imageres_mun | RT_ICON | 307 (group 36) | 48x48 | 32 | `icons/icon_36_2_48x48.png` |
| imageres_mun | RT_ICON | 308 (group 36) | 40x40 | 32 | `icons/icon_36_3_40x40.png` |
| imageres_mun | RT_ICON | 309 (group 36) | 32x32 | 32 | `icons/icon_36_4_32x32.png` |
| imageres_mun | RT_ICON | 31 (group 5) | 20x20 | 32 | `icons/icon_5_6_20x20.png` |
| imageres_mun | RT_ICON | 310 (group 36) | 24x24 | 32 | `icons/icon_36_5_24x24.png` |
| imageres_mun | RT_ICON | 311 (group 36) | 20x20 | 32 | `icons/icon_36_6_20x20.png` |
| imageres_mun | RT_ICON | 312 (group 36) | 16x16 | 32 | `icons/icon_36_7_16x16.png` |
| imageres_mun | RT_ICON | 313 (group 1033) | 256x256 | 32 | `icons/icon_1033_0_256x256.png` |
| imageres_mun | RT_ICON | 314 (group 1033) | 64x64 | 32 | `icons/icon_1033_1_64x64.png` |
| imageres_mun | RT_ICON | 315 (group 1033) | 48x48 | 32 | `icons/icon_1033_2_48x48.png` |
| imageres_mun | RT_ICON | 316 (group 1033) | 40x40 | 32 | `icons/icon_1033_3_40x40.png` |
| imageres_mun | RT_ICON | 317 (group 1033) | 32x32 | 32 | `icons/icon_1033_4_32x32.png` |
| imageres_mun | RT_ICON | 318 (group 1033) | 24x24 | 32 | `icons/icon_1033_5_24x24.png` |
| imageres_mun | RT_ICON | 319 (group 1033) | 20x20 | 32 | `icons/icon_1033_6_20x20.png` |
| imageres_mun | RT_ICON | 32 (group 5) | 16x16 | 32 | `icons/icon_5_7_16x16.png` |
| imageres_mun | RT_ICON | 320 (group 1033) | 16x16 | 32 | `icons/icon_1033_7_16x16.png` |
| imageres_mun | RT_ICON | 321 (group 1034) | 256x256 | 32 | `icons/icon_1034_0_256x256.png` |
| imageres_mun | RT_ICON | 322 (group 1034) | 64x64 | 32 | `icons/icon_1034_1_64x64.png` |
| imageres_mun | RT_ICON | 323 (group 1034) | 48x48 | 32 | `icons/icon_1034_2_48x48.png` |
| imageres_mun | RT_ICON | 324 (group 1034) | 40x40 | 32 | `icons/icon_1034_3_40x40.png` |
| imageres_mun | RT_ICON | 325 (group 1034) | 32x32 | 32 | `icons/icon_1034_4_32x32.png` |
| imageres_mun | RT_ICON | 326 (group 1034) | 24x24 | 32 | `icons/icon_1034_5_24x24.png` |
| imageres_mun | RT_ICON | 327 (group 1034) | 20x20 | 32 | `icons/icon_1034_6_20x20.png` |
| imageres_mun | RT_ICON | 328 (group 1034) | 16x16 | 32 | `icons/icon_1034_7_16x16.png` |
| imageres_mun | RT_ICON | 329 (group 37) | 256x256 | 32 | `icons/icon_37_0_256x256.png` |
| imageres_mun | RT_ICON | 33 (group 6) | 256x256 | 32 | `icons/icon_6_0_256x256.png` |
| imageres_mun | RT_ICON | 330 (group 37) | 64x64 | 32 | `icons/icon_37_1_64x64.png` |
| imageres_mun | RT_ICON | 331 (group 37) | 48x48 | 32 | `icons/icon_37_2_48x48.png` |
| imageres_mun | RT_ICON | 332 (group 37) | 40x40 | 32 | `icons/icon_37_3_40x40.png` |
| imageres_mun | RT_ICON | 333 (group 37) | 32x32 | 32 | `icons/icon_37_4_32x32.png` |
| imageres_mun | RT_ICON | 334 (group 37) | 24x24 | 32 | `icons/icon_37_5_24x24.png` |
| imageres_mun | RT_ICON | 335 (group 37) | 20x20 | 32 | `icons/icon_37_6_20x20.png` |
| imageres_mun | RT_ICON | 336 (group 37) | 16x16 | 32 | `icons/icon_37_7_16x16.png` |
| imageres_mun | RT_ICON | 337 (group 75) | 256x256 | 32 | `icons/icon_75_0_256x256.png` |
| imageres_mun | RT_ICON | 338 (group 75) | 64x64 | 32 | `icons/icon_75_1_64x64.png` |
| imageres_mun | RT_ICON | 339 (group 75) | 48x48 | 32 | `icons/icon_75_2_48x48.png` |
| imageres_mun | RT_ICON | 34 (group 6) | 64x64 | 32 | `icons/icon_6_1_64x64.png` |
| imageres_mun | RT_ICON | 340 (group 75) | 40x40 | 32 | `icons/icon_75_3_40x40.png` |
| imageres_mun | RT_ICON | 341 (group 75) | 32x32 | 32 | `icons/icon_75_4_32x32.png` |
| imageres_mun | RT_ICON | 342 (group 75) | 24x24 | 32 | `icons/icon_75_5_24x24.png` |
| imageres_mun | RT_ICON | 343 (group 75) | 20x20 | 32 | `icons/icon_75_6_20x20.png` |
| imageres_mun | RT_ICON | 344 (group 75) | 16x16 | 32 | `icons/icon_75_7_16x16.png` |
| imageres_mun | RT_ICON | 345 (group 38) | 256x256 | 32 | `icons/icon_38_0_256x256.png` |
| imageres_mun | RT_ICON | 346 (group 38) | 64x64 | 32 | `icons/icon_38_1_64x64.png` |
| imageres_mun | RT_ICON | 347 (group 38) | 48x48 | 32 | `icons/icon_38_2_48x48.png` |
| imageres_mun | RT_ICON | 348 (group 38) | 40x40 | 32 | `icons/icon_38_3_40x40.png` |
| imageres_mun | RT_ICON | 349 (group 38) | 32x32 | 32 | `icons/icon_38_4_32x32.png` |
| imageres_mun | RT_ICON | 35 (group 6) | 48x48 | 32 | `icons/icon_6_2_48x48.png` |
| imageres_mun | RT_ICON | 350 (group 38) | 24x24 | 32 | `icons/icon_38_5_24x24.png` |
| imageres_mun | RT_ICON | 351 (group 38) | 20x20 | 32 | `icons/icon_38_6_20x20.png` |
| imageres_mun | RT_ICON | 352 (group 38) | 16x16 | 32 | `icons/icon_38_7_16x16.png` |
| imageres_mun | RT_ICON | 353 (group 39) | 256x256 | 32 | `icons/icon_39_0_256x256.png` |
| imageres_mun | RT_ICON | 354 (group 39) | 64x64 | 32 | `icons/icon_39_1_64x64.png` |
| imageres_mun | RT_ICON | 355 (group 39) | 48x48 | 32 | `icons/icon_39_2_48x48.png` |
| imageres_mun | RT_ICON | 356 (group 39) | 40x40 | 32 | `icons/icon_39_3_40x40.png` |
| imageres_mun | RT_ICON | 357 (group 39) | 32x32 | 32 | `icons/icon_39_4_32x32.png` |
| imageres_mun | RT_ICON | 358 (group 39) | 24x24 | 32 | `icons/icon_39_5_24x24.png` |
| imageres_mun | RT_ICON | 359 (group 39) | 20x20 | 32 | `icons/icon_39_6_20x20.png` |
| imageres_mun | RT_ICON | 36 (group 6) | 40x40 | 32 | `icons/icon_6_3_40x40.png` |
| imageres_mun | RT_ICON | 360 (group 39) | 16x16 | 32 | `icons/icon_39_7_16x16.png` |
| imageres_mun | RT_ICON | 361 (group 40) | 256x256 | 32 | `icons/icon_40_0_256x256.png` |
| imageres_mun | RT_ICON | 362 (group 40) | 64x64 | 32 | `icons/icon_40_1_64x64.png` |
| imageres_mun | RT_ICON | 363 (group 40) | 48x48 | 32 | `icons/icon_40_2_48x48.png` |
| imageres_mun | RT_ICON | 364 (group 40) | 40x40 | 32 | `icons/icon_40_3_40x40.png` |
| imageres_mun | RT_ICON | 365 (group 40) | 32x32 | 32 | `icons/icon_40_4_32x32.png` |
| imageres_mun | RT_ICON | 366 (group 40) | 24x24 | 32 | `icons/icon_40_5_24x24.png` |
| imageres_mun | RT_ICON | 367 (group 40) | 20x20 | 32 | `icons/icon_40_6_20x20.png` |
| imageres_mun | RT_ICON | 368 (group 40) | 16x16 | 32 | `icons/icon_40_7_16x16.png` |
| imageres_mun | RT_ICON | 369 (group 41) | 256x256 | 32 | `icons/icon_41_0_256x256.png` |
| imageres_mun | RT_ICON | 37 (group 6) | 32x32 | 32 | `icons/icon_6_4_32x32.png` |
| imageres_mun | RT_ICON | 370 (group 41) | 64x64 | 32 | `icons/icon_41_1_64x64.png` |
| imageres_mun | RT_ICON | 371 (group 41) | 48x48 | 32 | `icons/icon_41_2_48x48.png` |
| imageres_mun | RT_ICON | 372 (group 41) | 40x40 | 32 | `icons/icon_41_3_40x40.png` |
| imageres_mun | RT_ICON | 373 (group 41) | 32x32 | 32 | `icons/icon_41_4_32x32.png` |
| imageres_mun | RT_ICON | 374 (group 41) | 24x24 | 32 | `icons/icon_41_5_24x24.png` |
| imageres_mun | RT_ICON | 375 (group 41) | 20x20 | 32 | `icons/icon_41_6_20x20.png` |
| imageres_mun | RT_ICON | 376 (group 41) | 16x16 | 32 | `icons/icon_41_7_16x16.png` |
| imageres_mun | RT_ICON | 377 (group 42) | 256x256 | 32 | `icons/icon_42_0_256x256.png` |
| imageres_mun | RT_ICON | 378 (group 42) | 64x64 | 32 | `icons/icon_42_1_64x64.png` |
| imageres_mun | RT_ICON | 379 (group 42) | 48x48 | 32 | `icons/icon_42_2_48x48.png` |
| imageres_mun | RT_ICON | 38 (group 6) | 24x24 | 32 | `icons/icon_6_5_24x24.png` |
| imageres_mun | RT_ICON | 380 (group 42) | 40x40 | 32 | `icons/icon_42_3_40x40.png` |
| imageres_mun | RT_ICON | 381 (group 42) | 32x32 | 32 | `icons/icon_42_4_32x32.png` |
| imageres_mun | RT_ICON | 382 (group 42) | 24x24 | 32 | `icons/icon_42_5_24x24.png` |
| imageres_mun | RT_ICON | 383 (group 42) | 20x20 | 32 | `icons/icon_42_6_20x20.png` |
| imageres_mun | RT_ICON | 384 (group 42) | 16x16 | 32 | `icons/icon_42_7_16x16.png` |
| imageres_mun | RT_ICON | 385 (group 43) | 256x256 | 32 | `icons/icon_43_0_256x256.png` |
| imageres_mun | RT_ICON | 386 (group 43) | 64x64 | 32 | `icons/icon_43_1_64x64.png` |
| imageres_mun | RT_ICON | 387 (group 43) | 48x48 | 32 | `icons/icon_43_2_48x48.png` |
| imageres_mun | RT_ICON | 388 (group 43) | 40x40 | 32 | `icons/icon_43_3_40x40.png` |
| imageres_mun | RT_ICON | 389 (group 43) | 32x32 | 32 | `icons/icon_43_4_32x32.png` |
| imageres_mun | RT_ICON | 39 (group 6) | 20x20 | 32 | `icons/icon_6_6_20x20.png` |
| imageres_mun | RT_ICON | 390 (group 43) | 24x24 | 32 | `icons/icon_43_5_24x24.png` |
| imageres_mun | RT_ICON | 391 (group 43) | 20x20 | 32 | `icons/icon_43_6_20x20.png` |
| imageres_mun | RT_ICON | 392 (group 43) | 16x16 | 32 | `icons/icon_43_7_16x16.png` |
| imageres_mun | RT_ICON | 393 (group 44) | 256x256 | 32 | `icons/icon_44_0_256x256.png` |
| imageres_mun | RT_ICON | 394 (group 44) | 64x64 | 32 | `icons/icon_44_1_64x64.png` |
| imageres_mun | RT_ICON | 395 (group 44) | 48x48 | 32 | `icons/icon_44_2_48x48.png` |
| imageres_mun | RT_ICON | 396 (group 44) | 40x40 | 32 | `icons/icon_44_3_40x40.png` |
| imageres_mun | RT_ICON | 397 (group 44) | 32x32 | 32 | `icons/icon_44_4_32x32.png` |
| imageres_mun | RT_ICON | 398 (group 44) | 24x24 | 32 | `icons/icon_44_5_24x24.png` |
| imageres_mun | RT_ICON | 399 (group 44) | 20x20 | 32 | `icons/icon_44_6_20x20.png` |
| imageres_mun | RT_ICON | 4 (group 2) | 40x40 | 32 | `icons/icon_2_3_40x40.png` |
| imageres_mun | RT_ICON | 40 (group 6) | 16x16 | 32 | `icons/icon_6_7_16x16.png` |
| imageres_mun | RT_ICON | 400 (group 44) | 16x16 | 32 | `icons/icon_44_7_16x16.png` |
| imageres_mun | RT_ICON | 401 (group 45) | 256x256 | 32 | `icons/icon_45_0_256x256.png` |
| imageres_mun | RT_ICON | 402 (group 45) | 64x64 | 32 | `icons/icon_45_1_64x64.png` |
| imageres_mun | RT_ICON | 403 (group 45) | 48x48 | 32 | `icons/icon_45_2_48x48.png` |
| imageres_mun | RT_ICON | 404 (group 45) | 40x40 | 32 | `icons/icon_45_3_40x40.png` |
| imageres_mun | RT_ICON | 405 (group 45) | 32x32 | 32 | `icons/icon_45_4_32x32.png` |
| imageres_mun | RT_ICON | 406 (group 45) | 24x24 | 32 | `icons/icon_45_5_24x24.png` |
| imageres_mun | RT_ICON | 407 (group 45) | 20x20 | 32 | `icons/icon_45_6_20x20.png` |
| imageres_mun | RT_ICON | 408 (group 45) | 16x16 | 32 | `icons/icon_45_7_16x16.png` |
| imageres_mun | RT_ICON | 409 (group 46) | 256x256 | 32 | `icons/icon_46_0_256x256.png` |
| imageres_mun | RT_ICON | 41 (group 8) | 256x256 | 32 | `icons/icon_8_0_256x256.png` |
| imageres_mun | RT_ICON | 410 (group 46) | 64x64 | 32 | `icons/icon_46_1_64x64.png` |
| imageres_mun | RT_ICON | 411 (group 46) | 48x48 | 32 | `icons/icon_46_2_48x48.png` |
| imageres_mun | RT_ICON | 412 (group 46) | 40x40 | 32 | `icons/icon_46_3_40x40.png` |
| imageres_mun | RT_ICON | 413 (group 46) | 32x32 | 32 | `icons/icon_46_4_32x32.png` |
| imageres_mun | RT_ICON | 414 (group 46) | 24x24 | 32 | `icons/icon_46_5_24x24.png` |
| imageres_mun | RT_ICON | 415 (group 46) | 20x20 | 32 | `icons/icon_46_6_20x20.png` |
| imageres_mun | RT_ICON | 416 (group 46) | 16x16 | 32 | `icons/icon_46_7_16x16.png` |
| imageres_mun | RT_ICON | 417 (group 47) | 256x256 | 32 | `icons/icon_47_0_256x256.png` |
| imageres_mun | RT_ICON | 418 (group 47) | 64x64 | 32 | `icons/icon_47_1_64x64.png` |
| imageres_mun | RT_ICON | 419 (group 47) | 48x48 | 32 | `icons/icon_47_2_48x48.png` |
| imageres_mun | RT_ICON | 42 (group 8) | 64x64 | 32 | `icons/icon_8_1_64x64.png` |
| imageres_mun | RT_ICON | 420 (group 47) | 40x40 | 32 | `icons/icon_47_3_40x40.png` |
| imageres_mun | RT_ICON | 421 (group 47) | 32x32 | 32 | `icons/icon_47_4_32x32.png` |
| imageres_mun | RT_ICON | 422 (group 47) | 24x24 | 32 | `icons/icon_47_5_24x24.png` |
| imageres_mun | RT_ICON | 423 (group 47) | 20x20 | 32 | `icons/icon_47_6_20x20.png` |
| imageres_mun | RT_ICON | 424 (group 47) | 16x16 | 32 | `icons/icon_47_7_16x16.png` |
| imageres_mun | RT_ICON | 425 (group 48) | 256x256 | 32 | `icons/icon_48_0_256x256.png` |
| imageres_mun | RT_ICON | 426 (group 48) | 64x64 | 32 | `icons/icon_48_1_64x64.png` |
| imageres_mun | RT_ICON | 427 (group 48) | 48x48 | 32 | `icons/icon_48_2_48x48.png` |
| imageres_mun | RT_ICON | 428 (group 48) | 40x40 | 32 | `icons/icon_48_3_40x40.png` |
| imageres_mun | RT_ICON | 429 (group 48) | 32x32 | 32 | `icons/icon_48_4_32x32.png` |
| imageres_mun | RT_ICON | 43 (group 8) | 48x48 | 32 | `icons/icon_8_2_48x48.png` |
| imageres_mun | RT_ICON | 430 (group 48) | 24x24 | 32 | `icons/icon_48_5_24x24.png` |
| imageres_mun | RT_ICON | 431 (group 48) | 20x20 | 32 | `icons/icon_48_6_20x20.png` |
| imageres_mun | RT_ICON | 432 (group 48) | 16x16 | 32 | `icons/icon_48_7_16x16.png` |
| imageres_mun | RT_ICON | 433 (group 49) | 256x256 | 32 | `icons/icon_49_0_256x256.png` |
| imageres_mun | RT_ICON | 434 (group 49) | 64x64 | 32 | `icons/icon_49_1_64x64.png` |
| imageres_mun | RT_ICON | 435 (group 49) | 48x48 | 32 | `icons/icon_49_2_48x48.png` |
| imageres_mun | RT_ICON | 436 (group 49) | 40x40 | 32 | `icons/icon_49_3_40x40.png` |
| imageres_mun | RT_ICON | 437 (group 49) | 32x32 | 32 | `icons/icon_49_4_32x32.png` |
| imageres_mun | RT_ICON | 438 (group 49) | 24x24 | 32 | `icons/icon_49_5_24x24.png` |
| imageres_mun | RT_ICON | 439 (group 49) | 20x20 | 32 | `icons/icon_49_6_20x20.png` |
| imageres_mun | RT_ICON | 44 (group 8) | 40x40 | 32 | `icons/icon_8_3_40x40.png` |
| imageres_mun | RT_ICON | 440 (group 49) | 16x16 | 32 | `icons/icon_49_7_16x16.png` |
| imageres_mun | RT_ICON | 441 (group 50) | 256x256 | 32 | `icons/icon_50_0_256x256.png` |
| imageres_mun | RT_ICON | 442 (group 50) | 64x64 | 32 | `icons/icon_50_1_64x64.png` |
| imageres_mun | RT_ICON | 443 (group 50) | 48x48 | 32 | `icons/icon_50_2_48x48.png` |
| imageres_mun | RT_ICON | 444 (group 50) | 40x40 | 32 | `icons/icon_50_3_40x40.png` |
| imageres_mun | RT_ICON | 445 (group 50) | 32x32 | 32 | `icons/icon_50_4_32x32.png` |
| imageres_mun | RT_ICON | 446 (group 50) | 24x24 | 32 | `icons/icon_50_5_24x24.png` |
| imageres_mun | RT_ICON | 447 (group 50) | 20x20 | 32 | `icons/icon_50_6_20x20.png` |
| imageres_mun | RT_ICON | 448 (group 50) | 16x16 | 32 | `icons/icon_50_7_16x16.png` |
| imageres_mun | RT_ICON | 449 (group 51) | 256x256 | 32 | `icons/icon_51_0_256x256.png` |
| imageres_mun | RT_ICON | 45 (group 8) | 32x32 | 32 | `icons/icon_8_4_32x32.png` |
| imageres_mun | RT_ICON | 450 (group 51) | 64x64 | 32 | `icons/icon_51_1_64x64.png` |
| imageres_mun | RT_ICON | 451 (group 51) | 48x48 | 32 | `icons/icon_51_2_48x48.png` |
| imageres_mun | RT_ICON | 452 (group 51) | 40x40 | 32 | `icons/icon_51_3_40x40.png` |
| imageres_mun | RT_ICON | 453 (group 51) | 32x32 | 32 | `icons/icon_51_4_32x32.png` |
| imageres_mun | RT_ICON | 454 (group 51) | 24x24 | 32 | `icons/icon_51_5_24x24.png` |
| imageres_mun | RT_ICON | 455 (group 51) | 20x20 | 32 | `icons/icon_51_6_20x20.png` |
| imageres_mun | RT_ICON | 456 (group 51) | 16x16 | 32 | `icons/icon_51_7_16x16.png` |
| imageres_mun | RT_ICON | 457 (group 52) | 256x256 | 32 | `icons/icon_52_0_256x256.png` |
| imageres_mun | RT_ICON | 458 (group 52) | 64x64 | 32 | `icons/icon_52_1_64x64.png` |
| imageres_mun | RT_ICON | 459 (group 52) | 48x48 | 32 | `icons/icon_52_2_48x48.png` |
| imageres_mun | RT_ICON | 46 (group 8) | 24x24 | 32 | `icons/icon_8_5_24x24.png` |
| imageres_mun | RT_ICON | 460 (group 52) | 40x40 | 32 | `icons/icon_52_3_40x40.png` |
| imageres_mun | RT_ICON | 461 (group 52) | 32x32 | 32 | `icons/icon_52_4_32x32.png` |
| imageres_mun | RT_ICON | 462 (group 52) | 24x24 | 32 | `icons/icon_52_5_24x24.png` |
| imageres_mun | RT_ICON | 463 (group 52) | 20x20 | 32 | `icons/icon_52_6_20x20.png` |
| imageres_mun | RT_ICON | 464 (group 52) | 16x16 | 32 | `icons/icon_52_7_16x16.png` |
| imageres_mun | RT_ICON | 465 (group 53) | 256x256 | 32 | `icons/icon_53_0_256x256.png` |
| imageres_mun | RT_ICON | 466 (group 53) | 64x64 | 32 | `icons/icon_53_1_64x64.png` |
| imageres_mun | RT_ICON | 467 (group 53) | 48x48 | 32 | `icons/icon_53_2_48x48.png` |
| imageres_mun | RT_ICON | 468 (group 53) | 40x40 | 32 | `icons/icon_53_3_40x40.png` |
| imageres_mun | RT_ICON | 469 (group 53) | 32x32 | 32 | `icons/icon_53_4_32x32.png` |
| imageres_mun | RT_ICON | 47 (group 8) | 20x20 | 32 | `icons/icon_8_6_20x20.png` |
| imageres_mun | RT_ICON | 470 (group 53) | 24x24 | 32 | `icons/icon_53_5_24x24.png` |
| imageres_mun | RT_ICON | 471 (group 53) | 20x20 | 32 | `icons/icon_53_6_20x20.png` |
| imageres_mun | RT_ICON | 472 (group 53) | 16x16 | 32 | `icons/icon_53_7_16x16.png` |
| imageres_mun | RT_ICON | 473 (group 54) | 256x256 | 32 | `icons/icon_54_0_256x256.png` |
| imageres_mun | RT_ICON | 474 (group 54) | 64x64 | 32 | `icons/icon_54_1_64x64.png` |
| imageres_mun | RT_ICON | 475 (group 54) | 48x48 | 32 | `icons/icon_54_2_48x48.png` |
| imageres_mun | RT_ICON | 476 (group 54) | 40x40 | 32 | `icons/icon_54_3_40x40.png` |
| imageres_mun | RT_ICON | 477 (group 54) | 32x32 | 32 | `icons/icon_54_4_32x32.png` |
| imageres_mun | RT_ICON | 478 (group 54) | 24x24 | 32 | `icons/icon_54_5_24x24.png` |
| imageres_mun | RT_ICON | 479 (group 54) | 20x20 | 32 | `icons/icon_54_6_20x20.png` |
| imageres_mun | RT_ICON | 48 (group 8) | 16x16 | 32 | `icons/icon_8_7_16x16.png` |
| imageres_mun | RT_ICON | 480 (group 54) | 16x16 | 32 | `icons/icon_54_7_16x16.png` |
| imageres_mun | RT_ICON | 481 (group 55) | 256x256 | 32 | `icons/icon_55_0_256x256.png` |
| imageres_mun | RT_ICON | 482 (group 55) | 64x64 | 32 | `icons/icon_55_1_64x64.png` |
| imageres_mun | RT_ICON | 483 (group 55) | 48x48 | 32 | `icons/icon_55_2_48x48.png` |
| imageres_mun | RT_ICON | 484 (group 55) | 40x40 | 32 | `icons/icon_55_3_40x40.png` |
| imageres_mun | RT_ICON | 485 (group 55) | 32x32 | 32 | `icons/icon_55_4_32x32.png` |
| imageres_mun | RT_ICON | 486 (group 55) | 24x24 | 32 | `icons/icon_55_5_24x24.png` |
| imageres_mun | RT_ICON | 487 (group 55) | 20x20 | 32 | `icons/icon_55_6_20x20.png` |
| imageres_mun | RT_ICON | 488 (group 55) | 16x16 | 32 | `icons/icon_55_7_16x16.png` |
| imageres_mun | RT_ICON | 489 (group 56) | 256x256 | 32 | `icons/icon_56_0_256x256.png` |
| imageres_mun | RT_ICON | 49 (group 9) | 256x256 | 32 | `icons/icon_9_0_256x256.png` |
| imageres_mun | RT_ICON | 490 (group 56) | 64x64 | 32 | `icons/icon_56_1_64x64.png` |
| imageres_mun | RT_ICON | 491 (group 56) | 48x48 | 32 | `icons/icon_56_2_48x48.png` |
| imageres_mun | RT_ICON | 492 (group 56) | 40x40 | 32 | `icons/icon_56_3_40x40.png` |
| imageres_mun | RT_ICON | 493 (group 56) | 32x32 | 32 | `icons/icon_56_4_32x32.png` |
| imageres_mun | RT_ICON | 494 (group 56) | 24x24 | 32 | `icons/icon_56_5_24x24.png` |
| imageres_mun | RT_ICON | 495 (group 56) | 20x20 | 32 | `icons/icon_56_6_20x20.png` |
| imageres_mun | RT_ICON | 496 (group 56) | 16x16 | 32 | `icons/icon_56_7_16x16.png` |
| imageres_mun | RT_ICON | 497 (group 57) | 256x256 | 32 | `icons/icon_57_0_256x256.png` |
| imageres_mun | RT_ICON | 498 (group 57) | 64x64 | 32 | `icons/icon_57_1_64x64.png` |
| imageres_mun | RT_ICON | 499 (group 57) | 48x48 | 32 | `icons/icon_57_2_48x48.png` |
| imageres_mun | RT_ICON | 5 (group 2) | 32x32 | 32 | `icons/icon_2_4_32x32.png` |
| imageres_mun | RT_ICON | 50 (group 9) | 64x64 | 32 | `icons/icon_9_1_64x64.png` |
| imageres_mun | RT_ICON | 500 (group 57) | 40x40 | 32 | `icons/icon_57_3_40x40.png` |
| imageres_mun | RT_ICON | 501 (group 57) | 32x32 | 32 | `icons/icon_57_4_32x32.png` |
| imageres_mun | RT_ICON | 502 (group 57) | 24x24 | 32 | `icons/icon_57_5_24x24.png` |
| imageres_mun | RT_ICON | 503 (group 57) | 20x20 | 32 | `icons/icon_57_6_20x20.png` |
| imageres_mun | RT_ICON | 504 (group 57) | 16x16 | 32 | `icons/icon_57_7_16x16.png` |
| imageres_mun | RT_ICON | 505 (group 58) | 256x256 | 32 | `icons/icon_58_0_256x256.png` |
| imageres_mun | RT_ICON | 506 (group 58) | 64x64 | 32 | `icons/icon_58_1_64x64.png` |
| imageres_mun | RT_ICON | 507 (group 58) | 48x48 | 32 | `icons/icon_58_2_48x48.png` |
| imageres_mun | RT_ICON | 508 (group 58) | 40x40 | 32 | `icons/icon_58_3_40x40.png` |
| imageres_mun | RT_ICON | 509 (group 58) | 32x32 | 32 | `icons/icon_58_4_32x32.png` |
| imageres_mun | RT_ICON | 51 (group 9) | 48x48 | 32 | `icons/icon_9_2_48x48.png` |
| imageres_mun | RT_ICON | 510 (group 58) | 24x24 | 32 | `icons/icon_58_5_24x24.png` |
| imageres_mun | RT_ICON | 511 (group 58) | 20x20 | 32 | `icons/icon_58_6_20x20.png` |
| imageres_mun | RT_ICON | 512 (group 58) | 16x16 | 32 | `icons/icon_58_7_16x16.png` |
| imageres_mun | RT_ICON | 513 (group 59) | 256x256 | 32 | `icons/icon_59_0_256x256.png` |
| imageres_mun | RT_ICON | 514 (group 59) | 64x64 | 32 | `icons/icon_59_1_64x64.png` |
| imageres_mun | RT_ICON | 515 (group 59) | 48x48 | 32 | `icons/icon_59_2_48x48.png` |
| imageres_mun | RT_ICON | 516 (group 59) | 40x40 | 32 | `icons/icon_59_3_40x40.png` |
| imageres_mun | RT_ICON | 517 (group 59) | 32x32 | 32 | `icons/icon_59_4_32x32.png` |
| imageres_mun | RT_ICON | 518 (group 59) | 24x24 | 32 | `icons/icon_59_5_24x24.png` |
| imageres_mun | RT_ICON | 519 (group 59) | 20x20 | 32 | `icons/icon_59_6_20x20.png` |
| imageres_mun | RT_ICON | 52 (group 9) | 40x40 | 32 | `icons/icon_9_3_40x40.png` |
| imageres_mun | RT_ICON | 520 (group 59) | 16x16 | 32 | `icons/icon_59_7_16x16.png` |
| imageres_mun | RT_ICON | 521 (group 60) | 256x256 | 32 | `icons/icon_60_0_256x256.png` |
| imageres_mun | RT_ICON | 522 (group 60) | 64x64 | 32 | `icons/icon_60_1_64x64.png` |
| imageres_mun | RT_ICON | 523 (group 60) | 48x48 | 32 | `icons/icon_60_2_48x48.png` |
| imageres_mun | RT_ICON | 524 (group 60) | 40x40 | 32 | `icons/icon_60_3_40x40.png` |
| imageres_mun | RT_ICON | 525 (group 60) | 32x32 | 32 | `icons/icon_60_4_32x32.png` |
| imageres_mun | RT_ICON | 526 (group 60) | 24x24 | 32 | `icons/icon_60_5_24x24.png` |
| imageres_mun | RT_ICON | 527 (group 60) | 20x20 | 32 | `icons/icon_60_6_20x20.png` |
| imageres_mun | RT_ICON | 528 (group 60) | 16x16 | 32 | `icons/icon_60_7_16x16.png` |
| imageres_mun | RT_ICON | 529 (group 61) | 256x256 | 32 | `icons/icon_61_0_256x256.png` |
| imageres_mun | RT_ICON | 53 (group 9) | 32x32 | 32 | `icons/icon_9_4_32x32.png` |
| imageres_mun | RT_ICON | 530 (group 61) | 64x64 | 32 | `icons/icon_61_1_64x64.png` |
| imageres_mun | RT_ICON | 531 (group 61) | 48x48 | 32 | `icons/icon_61_2_48x48.png` |
| imageres_mun | RT_ICON | 532 (group 61) | 40x40 | 32 | `icons/icon_61_3_40x40.png` |
| imageres_mun | RT_ICON | 533 (group 61) | 32x32 | 32 | `icons/icon_61_4_32x32.png` |
| imageres_mun | RT_ICON | 534 (group 61) | 24x24 | 32 | `icons/icon_61_5_24x24.png` |
| imageres_mun | RT_ICON | 535 (group 61) | 20x20 | 32 | `icons/icon_61_6_20x20.png` |
| imageres_mun | RT_ICON | 536 (group 61) | 16x16 | 32 | `icons/icon_61_7_16x16.png` |
| imageres_mun | RT_ICON | 537 (group 62) | 256x256 | 32 | `icons/icon_62_0_256x256.png` |
| imageres_mun | RT_ICON | 538 (group 62) | 64x64 | 32 | `icons/icon_62_1_64x64.png` |
| imageres_mun | RT_ICON | 539 (group 62) | 48x48 | 32 | `icons/icon_62_2_48x48.png` |
| imageres_mun | RT_ICON | 54 (group 9) | 24x24 | 32 | `icons/icon_9_5_24x24.png` |
| imageres_mun | RT_ICON | 540 (group 62) | 40x40 | 32 | `icons/icon_62_3_40x40.png` |
| imageres_mun | RT_ICON | 541 (group 62) | 32x32 | 32 | `icons/icon_62_4_32x32.png` |
| imageres_mun | RT_ICON | 542 (group 62) | 24x24 | 32 | `icons/icon_62_5_24x24.png` |
| imageres_mun | RT_ICON | 543 (group 62) | 20x20 | 32 | `icons/icon_62_6_20x20.png` |
| imageres_mun | RT_ICON | 544 (group 62) | 16x16 | 32 | `icons/icon_62_7_16x16.png` |
| imageres_mun | RT_ICON | 545 (group 63) | 256x256 | 32 | `icons/icon_63_0_256x256.png` |
| imageres_mun | RT_ICON | 546 (group 63) | 64x64 | 32 | `icons/icon_63_1_64x64.png` |
| imageres_mun | RT_ICON | 547 (group 63) | 48x48 | 32 | `icons/icon_63_2_48x48.png` |
| imageres_mun | RT_ICON | 548 (group 63) | 40x40 | 32 | `icons/icon_63_3_40x40.png` |
| imageres_mun | RT_ICON | 549 (group 63) | 32x32 | 32 | `icons/icon_63_4_32x32.png` |
| imageres_mun | RT_ICON | 55 (group 9) | 20x20 | 32 | `icons/icon_9_6_20x20.png` |
| imageres_mun | RT_ICON | 550 (group 63) | 24x24 | 32 | `icons/icon_63_5_24x24.png` |
| imageres_mun | RT_ICON | 551 (group 63) | 20x20 | 32 | `icons/icon_63_6_20x20.png` |
| imageres_mun | RT_ICON | 552 (group 63) | 16x16 | 32 | `icons/icon_63_7_16x16.png` |
| imageres_mun | RT_ICON | 553 (group 64) | 256x256 | 32 | `icons/icon_64_0_256x256.png` |
| imageres_mun | RT_ICON | 554 (group 64) | 64x64 | 32 | `icons/icon_64_1_64x64.png` |
| imageres_mun | RT_ICON | 555 (group 64) | 48x48 | 32 | `icons/icon_64_2_48x48.png` |
| imageres_mun | RT_ICON | 556 (group 64) | 40x40 | 32 | `icons/icon_64_3_40x40.png` |
| imageres_mun | RT_ICON | 557 (group 64) | 32x32 | 32 | `icons/icon_64_4_32x32.png` |
| imageres_mun | RT_ICON | 558 (group 64) | 24x24 | 32 | `icons/icon_64_5_24x24.png` |
| imageres_mun | RT_ICON | 559 (group 64) | 20x20 | 32 | `icons/icon_64_6_20x20.png` |
| imageres_mun | RT_ICON | 56 (group 9) | 16x16 | 32 | `icons/icon_9_7_16x16.png` |
| imageres_mun | RT_ICON | 560 (group 64) | 16x16 | 32 | `icons/icon_64_7_16x16.png` |
| imageres_mun | RT_ICON | 561 (group 65) | 256x256 | 32 | `icons/icon_65_0_256x256.png` |
| imageres_mun | RT_ICON | 562 (group 65) | 64x64 | 32 | `icons/icon_65_1_64x64.png` |
| imageres_mun | RT_ICON | 563 (group 65) | 48x48 | 32 | `icons/icon_65_2_48x48.png` |
| imageres_mun | RT_ICON | 564 (group 65) | 40x40 | 32 | `icons/icon_65_3_40x40.png` |
| imageres_mun | RT_ICON | 565 (group 65) | 32x32 | 32 | `icons/icon_65_4_32x32.png` |
| imageres_mun | RT_ICON | 566 (group 65) | 24x24 | 32 | `icons/icon_65_5_24x24.png` |
| imageres_mun | RT_ICON | 567 (group 65) | 20x20 | 32 | `icons/icon_65_6_20x20.png` |
| imageres_mun | RT_ICON | 568 (group 65) | 16x16 | 32 | `icons/icon_65_7_16x16.png` |
| imageres_mun | RT_ICON | 569 (group 66) | 256x256 | 32 | `icons/icon_66_0_256x256.png` |
| imageres_mun | RT_ICON | 57 (group 10) | 256x256 | 32 | `icons/icon_10_0_256x256.png` |
| imageres_mun | RT_ICON | 570 (group 66) | 64x64 | 32 | `icons/icon_66_1_64x64.png` |
| imageres_mun | RT_ICON | 571 (group 66) | 48x48 | 32 | `icons/icon_66_2_48x48.png` |
| imageres_mun | RT_ICON | 572 (group 66) | 40x40 | 32 | `icons/icon_66_3_40x40.png` |
| imageres_mun | RT_ICON | 573 (group 66) | 32x32 | 32 | `icons/icon_66_4_32x32.png` |
| imageres_mun | RT_ICON | 574 (group 66) | 24x24 | 32 | `icons/icon_66_5_24x24.png` |
| imageres_mun | RT_ICON | 575 (group 66) | 20x20 | 32 | `icons/icon_66_6_20x20.png` |
| imageres_mun | RT_ICON | 576 (group 66) | 16x16 | 32 | `icons/icon_66_7_16x16.png` |
| imageres_mun | RT_ICON | 577 (group 67) | 256x256 | 32 | `icons/icon_67_0_256x256.png` |
| imageres_mun | RT_ICON | 578 (group 67) | 64x64 | 32 | `icons/icon_67_1_64x64.png` |
| imageres_mun | RT_ICON | 579 (group 67) | 48x48 | 32 | `icons/icon_67_2_48x48.png` |
| imageres_mun | RT_ICON | 58 (group 10) | 64x64 | 32 | `icons/icon_10_1_64x64.png` |
| imageres_mun | RT_ICON | 580 (group 67) | 40x40 | 32 | `icons/icon_67_3_40x40.png` |
| imageres_mun | RT_ICON | 581 (group 67) | 32x32 | 32 | `icons/icon_67_4_32x32.png` |
| imageres_mun | RT_ICON | 582 (group 67) | 24x24 | 32 | `icons/icon_67_5_24x24.png` |
| imageres_mun | RT_ICON | 583 (group 67) | 20x20 | 32 | `icons/icon_67_6_20x20.png` |
| imageres_mun | RT_ICON | 584 (group 67) | 16x16 | 32 | `icons/icon_67_7_16x16.png` |
| imageres_mun | RT_ICON | 585 (group 68) | 256x256 | 32 | `icons/icon_68_0_256x256.png` |
| imageres_mun | RT_ICON | 586 (group 68) | 64x64 | 32 | `icons/icon_68_1_64x64.png` |
| imageres_mun | RT_ICON | 587 (group 68) | 48x48 | 32 | `icons/icon_68_2_48x48.png` |
| imageres_mun | RT_ICON | 588 (group 68) | 40x40 | 32 | `icons/icon_68_3_40x40.png` |
| imageres_mun | RT_ICON | 589 (group 68) | 32x32 | 32 | `icons/icon_68_4_32x32.png` |
| imageres_mun | RT_ICON | 59 (group 10) | 48x48 | 32 | `icons/icon_10_2_48x48.png` |
| imageres_mun | RT_ICON | 590 (group 68) | 24x24 | 32 | `icons/icon_68_5_24x24.png` |
| imageres_mun | RT_ICON | 591 (group 68) | 20x20 | 32 | `icons/icon_68_6_20x20.png` |
| imageres_mun | RT_ICON | 592 (group 68) | 16x16 | 32 | `icons/icon_68_7_16x16.png` |
| imageres_mun | RT_ICON | 593 (group 69) | 256x256 | 32 | `icons/icon_69_0_256x256.png` |
| imageres_mun | RT_ICON | 594 (group 69) | 64x64 | 32 | `icons/icon_69_1_64x64.png` |
| imageres_mun | RT_ICON | 595 (group 69) | 48x48 | 32 | `icons/icon_69_2_48x48.png` |
| imageres_mun | RT_ICON | 596 (group 69) | 40x40 | 32 | `icons/icon_69_3_40x40.png` |
| imageres_mun | RT_ICON | 597 (group 69) | 32x32 | 32 | `icons/icon_69_4_32x32.png` |
| imageres_mun | RT_ICON | 598 (group 69) | 24x24 | 32 | `icons/icon_69_5_24x24.png` |
| imageres_mun | RT_ICON | 599 (group 69) | 20x20 | 32 | `icons/icon_69_6_20x20.png` |
| imageres_mun | RT_ICON | 6 (group 2) | 24x24 | 32 | `icons/icon_2_5_24x24.png` |
| imageres_mun | RT_ICON | 60 (group 10) | 40x40 | 32 | `icons/icon_10_3_40x40.png` |
| imageres_mun | RT_ICON | 600 (group 69) | 16x16 | 32 | `icons/icon_69_7_16x16.png` |
| imageres_mun | RT_ICON | 601 (group 70) | 256x256 | 32 | `icons/icon_70_0_256x256.png` |
| imageres_mun | RT_ICON | 602 (group 70) | 64x64 | 32 | `icons/icon_70_1_64x64.png` |
| imageres_mun | RT_ICON | 603 (group 70) | 48x48 | 32 | `icons/icon_70_2_48x48.png` |
| imageres_mun | RT_ICON | 604 (group 70) | 40x40 | 32 | `icons/icon_70_3_40x40.png` |
| imageres_mun | RT_ICON | 605 (group 70) | 32x32 | 32 | `icons/icon_70_4_32x32.png` |
| imageres_mun | RT_ICON | 606 (group 70) | 24x24 | 32 | `icons/icon_70_5_24x24.png` |
| imageres_mun | RT_ICON | 607 (group 70) | 20x20 | 32 | `icons/icon_70_6_20x20.png` |
| imageres_mun | RT_ICON | 608 (group 70) | 16x16 | 32 | `icons/icon_70_7_16x16.png` |
| imageres_mun | RT_ICON | 609 (group 71) | 256x256 | 32 | `icons/icon_71_0_256x256.png` |
| imageres_mun | RT_ICON | 61 (group 10) | 32x32 | 32 | `icons/icon_10_4_32x32.png` |
| imageres_mun | RT_ICON | 610 (group 71) | 64x64 | 32 | `icons/icon_71_1_64x64.png` |
| imageres_mun | RT_ICON | 611 (group 71) | 48x48 | 32 | `icons/icon_71_2_48x48.png` |
| imageres_mun | RT_ICON | 612 (group 71) | 40x40 | 32 | `icons/icon_71_3_40x40.png` |
| imageres_mun | RT_ICON | 613 (group 71) | 32x32 | 32 | `icons/icon_71_4_32x32.png` |
| imageres_mun | RT_ICON | 614 (group 71) | 24x24 | 32 | `icons/icon_71_5_24x24.png` |
| imageres_mun | RT_ICON | 615 (group 71) | 20x20 | 32 | `icons/icon_71_6_20x20.png` |
| imageres_mun | RT_ICON | 616 (group 71) | 16x16 | 32 | `icons/icon_71_7_16x16.png` |
| imageres_mun | RT_ICON | 617 (group 72) | 256x256 | 32 | `icons/icon_72_0_256x256.png` |
| imageres_mun | RT_ICON | 618 (group 72) | 64x64 | 32 | `icons/icon_72_1_64x64.png` |
| imageres_mun | RT_ICON | 619 (group 72) | 48x48 | 32 | `icons/icon_72_2_48x48.png` |
| imageres_mun | RT_ICON | 62 (group 10) | 24x24 | 32 | `icons/icon_10_5_24x24.png` |
| imageres_mun | RT_ICON | 620 (group 72) | 40x40 | 32 | `icons/icon_72_3_40x40.png` |
| imageres_mun | RT_ICON | 621 (group 72) | 32x32 | 32 | `icons/icon_72_4_32x32.png` |
| imageres_mun | RT_ICON | 622 (group 72) | 24x24 | 32 | `icons/icon_72_5_24x24.png` |
| imageres_mun | RT_ICON | 623 (group 72) | 20x20 | 32 | `icons/icon_72_6_20x20.png` |
| imageres_mun | RT_ICON | 624 (group 72) | 16x16 | 32 | `icons/icon_72_7_16x16.png` |
| imageres_mun | RT_ICON | 625 (group 73) | 256x256 | 32 | `icons/icon_73_0_256x256.png` |
| imageres_mun | RT_ICON | 626 (group 73) | 64x64 | 32 | `icons/icon_73_1_64x64.png` |
| imageres_mun | RT_ICON | 627 (group 73) | 48x48 | 32 | `icons/icon_73_2_48x48.png` |
| imageres_mun | RT_ICON | 628 (group 73) | 40x40 | 32 | `icons/icon_73_3_40x40.png` |
| imageres_mun | RT_ICON | 629 (group 73) | 32x32 | 32 | `icons/icon_73_4_32x32.png` |
| imageres_mun | RT_ICON | 63 (group 10) | 20x20 | 32 | `icons/icon_10_6_20x20.png` |
| imageres_mun | RT_ICON | 630 (group 73) | 24x24 | 32 | `icons/icon_73_5_24x24.png` |
| imageres_mun | RT_ICON | 631 (group 73) | 20x20 | 32 | `icons/icon_73_6_20x20.png` |
| imageres_mun | RT_ICON | 632 (group 73) | 16x16 | 32 | `icons/icon_73_7_16x16.png` |
| imageres_mun | RT_ICON | 633 (group 74) | 256x256 | 32 | `icons/icon_74_0_256x256.png` |
| imageres_mun | RT_ICON | 634 (group 74) | 64x64 | 32 | `icons/icon_74_1_64x64.png` |
| imageres_mun | RT_ICON | 635 (group 74) | 48x48 | 32 | `icons/icon_74_2_48x48.png` |
| imageres_mun | RT_ICON | 636 (group 74) | 40x40 | 32 | `icons/icon_74_3_40x40.png` |
| imageres_mun | RT_ICON | 637 (group 74) | 32x32 | 32 | `icons/icon_74_4_32x32.png` |
| imageres_mun | RT_ICON | 638 (group 74) | 24x24 | 32 | `icons/icon_74_5_24x24.png` |
| imageres_mun | RT_ICON | 639 (group 74) | 20x20 | 32 | `icons/icon_74_6_20x20.png` |
| imageres_mun | RT_ICON | 64 (group 10) | 16x16 | 32 | `icons/icon_10_7_16x16.png` |
| imageres_mun | RT_ICON | 640 (group 74) | 16x16 | 32 | `icons/icon_74_7_16x16.png` |
| imageres_mun | RT_ICON | 641 (group 78) | 256x256 | 32 | `icons/icon_78_0_256x256.png` |
| imageres_mun | RT_ICON | 642 (group 78) | 64x64 | 32 | `icons/icon_78_1_64x64.png` |
| imageres_mun | RT_ICON | 643 (group 78) | 48x48 | 32 | `icons/icon_78_2_48x48.png` |
| imageres_mun | RT_ICON | 644 (group 78) | 40x40 | 32 | `icons/icon_78_3_40x40.png` |
| imageres_mun | RT_ICON | 645 (group 78) | 32x32 | 32 | `icons/icon_78_4_32x32.png` |
| imageres_mun | RT_ICON | 646 (group 78) | 24x24 | 32 | `icons/icon_78_5_24x24.png` |
| imageres_mun | RT_ICON | 647 (group 78) | 20x20 | 32 | `icons/icon_78_6_20x20.png` |
| imageres_mun | RT_ICON | 648 (group 78) | 16x16 | 32 | `icons/icon_78_7_16x16.png` |
| imageres_mun | RT_ICON | 649 (group "BLANK1") | 16x16 | 4 | `icons/icon_BLANK1_0_16x16.png` |
| imageres_mun | RT_ICON | 65 (group 14) | 16x16 | 4 | `icons/icon_14_0_16x16.png` |
| imageres_mun | RT_ICON | 650 (group 104) | 256x256 | 32 | `icons/icon_104_0_256x256.png` |
| imageres_mun | RT_ICON | 651 (group 104) | 64x64 | 32 | `icons/icon_104_1_64x64.png` |
| imageres_mun | RT_ICON | 652 (group 104) | 48x48 | 32 | `icons/icon_104_2_48x48.png` |
| imageres_mun | RT_ICON | 653 (group 104) | 40x40 | 32 | `icons/icon_104_3_40x40.png` |
| imageres_mun | RT_ICON | 654 (group 104) | 32x32 | 32 | `icons/icon_104_4_32x32.png` |
| imageres_mun | RT_ICON | 655 (group 104) | 24x24 | 32 | `icons/icon_104_5_24x24.png` |
| imageres_mun | RT_ICON | 656 (group 104) | 20x20 | 32 | `icons/icon_104_6_20x20.png` |
| imageres_mun | RT_ICON | 657 (group 104) | 16x16 | 32 | `icons/icon_104_7_16x16.png` |
| imageres_mun | RT_ICON | 658 (group 105) | 256x256 | 32 | `icons/icon_105_0_256x256.png` |
| imageres_mun | RT_ICON | 659 (group 105) | 64x64 | 32 | `icons/icon_105_1_64x64.png` |
| imageres_mun | RT_ICON | 66 (group 15) | 256x256 | 32 | `icons/icon_15_0_256x256.png` |
| imageres_mun | RT_ICON | 660 (group 105) | 48x48 | 32 | `icons/icon_105_2_48x48.png` |
| imageres_mun | RT_ICON | 661 (group 105) | 40x40 | 32 | `icons/icon_105_3_40x40.png` |
| imageres_mun | RT_ICON | 662 (group 105) | 32x32 | 32 | `icons/icon_105_4_32x32.png` |
| imageres_mun | RT_ICON | 663 (group 105) | 24x24 | 32 | `icons/icon_105_5_24x24.png` |
| imageres_mun | RT_ICON | 664 (group 105) | 20x20 | 32 | `icons/icon_105_6_20x20.png` |
| imageres_mun | RT_ICON | 665 (group 105) | 16x16 | 32 | `icons/icon_105_7_16x16.png` |
| imageres_mun | RT_ICON | 666 (group 106) | 256x256 | 32 | `icons/icon_106_0_256x256.png` |
| imageres_mun | RT_ICON | 667 (group 106) | 64x64 | 32 | `icons/icon_106_1_64x64.png` |
| imageres_mun | RT_ICON | 668 (group 106) | 48x48 | 32 | `icons/icon_106_2_48x48.png` |
| imageres_mun | RT_ICON | 669 (group 106) | 40x40 | 32 | `icons/icon_106_3_40x40.png` |
| imageres_mun | RT_ICON | 67 (group 15) | 64x64 | 32 | `icons/icon_15_1_64x64.png` |
| imageres_mun | RT_ICON | 670 (group 106) | 32x32 | 32 | `icons/icon_106_4_32x32.png` |
| imageres_mun | RT_ICON | 671 (group 106) | 24x24 | 32 | `icons/icon_106_5_24x24.png` |
| imageres_mun | RT_ICON | 672 (group 106) | 20x20 | 32 | `icons/icon_106_6_20x20.png` |
| imageres_mun | RT_ICON | 673 (group 106) | 16x16 | 32 | `icons/icon_106_7_16x16.png` |
| imageres_mun | RT_ICON | 674 (group 107) | 256x256 | 32 | `icons/icon_107_0_256x256.png` |
| imageres_mun | RT_ICON | 675 (group 107) | 64x64 | 32 | `icons/icon_107_1_64x64.png` |
| imageres_mun | RT_ICON | 676 (group 107) | 48x48 | 32 | `icons/icon_107_2_48x48.png` |
| imageres_mun | RT_ICON | 677 (group 107) | 40x40 | 32 | `icons/icon_107_3_40x40.png` |
| imageres_mun | RT_ICON | 678 (group 107) | 32x32 | 32 | `icons/icon_107_4_32x32.png` |
| imageres_mun | RT_ICON | 679 (group 107) | 24x24 | 32 | `icons/icon_107_5_24x24.png` |
| imageres_mun | RT_ICON | 68 (group 15) | 48x48 | 32 | `icons/icon_15_2_48x48.png` |
| imageres_mun | RT_ICON | 680 (group 107) | 20x20 | 32 | `icons/icon_107_6_20x20.png` |
| imageres_mun | RT_ICON | 681 (group 107) | 16x16 | 32 | `icons/icon_107_7_16x16.png` |
| imageres_mun | RT_ICON | 682 (group 79) | 256x256 | 32 | `icons/icon_79_0_256x256.png` |
| imageres_mun | RT_ICON | 683 (group 79) | 64x64 | 32 | `icons/icon_79_1_64x64.png` |
| imageres_mun | RT_ICON | 684 (group 79) | 60x60 | 32 | `icons/icon_79_2_60x60.png` |
| imageres_mun | RT_ICON | 685 (group 79) | 48x48 | 32 | `icons/icon_79_3_48x48.png` |
| imageres_mun | RT_ICON | 686 (group 79) | 40x40 | 32 | `icons/icon_79_4_40x40.png` |
| imageres_mun | RT_ICON | 687 (group 79) | 32x32 | 32 | `icons/icon_79_5_32x32.png` |
| imageres_mun | RT_ICON | 688 (group 79) | 24x24 | 32 | `icons/icon_79_6_24x24.png` |
| imageres_mun | RT_ICON | 689 (group 79) | 20x20 | 32 | `icons/icon_79_7_20x20.png` |
| imageres_mun | RT_ICON | 69 (group 15) | 40x40 | 32 | `icons/icon_15_3_40x40.png` |
| imageres_mun | RT_ICON | 690 (group 79) | 16x16 | 32 | `icons/icon_79_8_16x16.png` |
| imageres_mun | RT_ICON | 691 (group 98) | 256x256 | 32 | `icons/icon_98_0_256x256.png` |
| imageres_mun | RT_ICON | 692 (group 98) | 64x64 | 32 | `icons/icon_98_1_64x64.png` |
| imageres_mun | RT_ICON | 693 (group 98) | 48x48 | 32 | `icons/icon_98_2_48x48.png` |
| imageres_mun | RT_ICON | 694 (group 98) | 40x40 | 32 | `icons/icon_98_3_40x40.png` |
| imageres_mun | RT_ICON | 695 (group 98) | 32x32 | 32 | `icons/icon_98_4_32x32.png` |
| imageres_mun | RT_ICON | 696 (group 98) | 24x24 | 32 | `icons/icon_98_5_24x24.png` |
| imageres_mun | RT_ICON | 697 (group 98) | 20x20 | 32 | `icons/icon_98_6_20x20.png` |
| imageres_mun | RT_ICON | 698 (group 98) | 16x16 | 32 | `icons/icon_98_7_16x16.png` |
| imageres_mun | RT_ICON | 699 (group 81) | 256x256 | 32 | `icons/icon_81_0_256x256.png` |
| imageres_mun | RT_ICON | 7 (group 2) | 20x20 | 32 | `icons/icon_2_6_20x20.png` |
| imageres_mun | RT_ICON | 70 (group 15) | 32x32 | 32 | `icons/icon_15_4_32x32.png` |
| imageres_mun | RT_ICON | 700 (group 81) | 64x64 | 32 | `icons/icon_81_1_64x64.png` |
| imageres_mun | RT_ICON | 701 (group 81) | 48x48 | 32 | `icons/icon_81_2_48x48.png` |
| imageres_mun | RT_ICON | 702 (group 81) | 40x40 | 32 | `icons/icon_81_3_40x40.png` |
| imageres_mun | RT_ICON | 703 (group 81) | 32x32 | 32 | `icons/icon_81_4_32x32.png` |
| imageres_mun | RT_ICON | 704 (group 81) | 24x24 | 32 | `icons/icon_81_5_24x24.png` |
| imageres_mun | RT_ICON | 705 (group 81) | 20x20 | 32 | `icons/icon_81_6_20x20.png` |
| imageres_mun | RT_ICON | 706 (group 81) | 16x16 | 32 | `icons/icon_81_7_16x16.png` |
| imageres_mun | RT_ICON | 707 (group 82) | 256x256 | 32 | `icons/icon_82_0_256x256.png` |
| imageres_mun | RT_ICON | 708 (group 82) | 64x64 | 32 | `icons/icon_82_1_64x64.png` |
| imageres_mun | RT_ICON | 709 (group 82) | 48x48 | 32 | `icons/icon_82_2_48x48.png` |
| imageres_mun | RT_ICON | 71 (group 15) | 24x24 | 32 | `icons/icon_15_5_24x24.png` |
| imageres_mun | RT_ICON | 710 (group 82) | 40x40 | 32 | `icons/icon_82_3_40x40.png` |
| imageres_mun | RT_ICON | 711 (group 82) | 32x32 | 32 | `icons/icon_82_4_32x32.png` |
| imageres_mun | RT_ICON | 712 (group 82) | 24x24 | 32 | `icons/icon_82_5_24x24.png` |
| imageres_mun | RT_ICON | 713 (group 82) | 20x20 | 32 | `icons/icon_82_6_20x20.png` |
| imageres_mun | RT_ICON | 714 (group 82) | 16x16 | 32 | `icons/icon_82_7_16x16.png` |
| imageres_mun | RT_ICON | 715 (group 83) | 256x256 | 32 | `icons/icon_83_0_256x256.png` |
| imageres_mun | RT_ICON | 716 (group 83) | 64x64 | 32 | `icons/icon_83_1_64x64.png` |
| imageres_mun | RT_ICON | 717 (group 83) | 48x48 | 32 | `icons/icon_83_2_48x48.png` |
| imageres_mun | RT_ICON | 718 (group 83) | 40x40 | 32 | `icons/icon_83_3_40x40.png` |
| imageres_mun | RT_ICON | 719 (group 83) | 32x32 | 32 | `icons/icon_83_4_32x32.png` |
| imageres_mun | RT_ICON | 72 (group 15) | 20x20 | 32 | `icons/icon_15_6_20x20.png` |
| imageres_mun | RT_ICON | 720 (group 83) | 24x24 | 32 | `icons/icon_83_5_24x24.png` |
| imageres_mun | RT_ICON | 721 (group 83) | 20x20 | 32 | `icons/icon_83_6_20x20.png` |
| imageres_mun | RT_ICON | 722 (group 83) | 16x16 | 32 | `icons/icon_83_7_16x16.png` |
| imageres_mun | RT_ICON | 723 (group 84) | 256x256 | 32 | `icons/icon_84_0_256x256.png` |
| imageres_mun | RT_ICON | 724 (group 84) | 64x64 | 32 | `icons/icon_84_1_64x64.png` |
| imageres_mun | RT_ICON | 725 (group 84) | 48x48 | 32 | `icons/icon_84_2_48x48.png` |
| imageres_mun | RT_ICON | 726 (group 84) | 40x40 | 32 | `icons/icon_84_3_40x40.png` |
| imageres_mun | RT_ICON | 727 (group 84) | 32x32 | 32 | `icons/icon_84_4_32x32.png` |
| imageres_mun | RT_ICON | 728 (group 84) | 24x24 | 32 | `icons/icon_84_5_24x24.png` |
| imageres_mun | RT_ICON | 729 (group 84) | 20x20 | 32 | `icons/icon_84_6_20x20.png` |
| imageres_mun | RT_ICON | 73 (group 15) | 16x16 | 32 | `icons/icon_15_7_16x16.png` |
| imageres_mun | RT_ICON | 730 (group 84) | 16x16 | 32 | `icons/icon_84_7_16x16.png` |
| imageres_mun | RT_ICON | 731 (group 85) | 256x256 | 32 | `icons/icon_85_0_256x256.png` |
| imageres_mun | RT_ICON | 732 (group 85) | 64x64 | 32 | `icons/icon_85_1_64x64.png` |
| imageres_mun | RT_ICON | 733 (group 85) | 48x48 | 32 | `icons/icon_85_2_48x48.png` |
| imageres_mun | RT_ICON | 734 (group 85) | 40x40 | 32 | `icons/icon_85_3_40x40.png` |
| imageres_mun | RT_ICON | 735 (group 85) | 32x32 | 32 | `icons/icon_85_4_32x32.png` |
| imageres_mun | RT_ICON | 736 (group 85) | 24x24 | 32 | `icons/icon_85_5_24x24.png` |
| imageres_mun | RT_ICON | 737 (group 85) | 20x20 | 32 | `icons/icon_85_6_20x20.png` |
| imageres_mun | RT_ICON | 738 (group 85) | 16x16 | 32 | `icons/icon_85_7_16x16.png` |
| imageres_mun | RT_ICON | 739 (group 86) | 256x256 | 32 | `icons/icon_86_0_256x256.png` |
| imageres_mun | RT_ICON | 74 (group 5206) | 256x256 | 32 | `icons/icon_5206_0_256x256.png` |
| imageres_mun | RT_ICON | 740 (group 86) | 64x64 | 32 | `icons/icon_86_1_64x64.png` |
| imageres_mun | RT_ICON | 741 (group 86) | 48x48 | 32 | `icons/icon_86_2_48x48.png` |
| imageres_mun | RT_ICON | 742 (group 86) | 40x40 | 32 | `icons/icon_86_3_40x40.png` |
| imageres_mun | RT_ICON | 743 (group 86) | 32x32 | 32 | `icons/icon_86_4_32x32.png` |
| imageres_mun | RT_ICON | 744 (group 86) | 24x24 | 32 | `icons/icon_86_5_24x24.png` |
| imageres_mun | RT_ICON | 745 (group 86) | 20x20 | 32 | `icons/icon_86_6_20x20.png` |
| imageres_mun | RT_ICON | 746 (group 86) | 16x16 | 32 | `icons/icon_86_7_16x16.png` |
| imageres_mun | RT_ICON | 747 (group 87) | 256x256 | 32 | `icons/icon_87_0_256x256.png` |
| imageres_mun | RT_ICON | 748 (group 87) | 64x64 | 32 | `icons/icon_87_1_64x64.png` |
| imageres_mun | RT_ICON | 749 (group 87) | 48x48 | 32 | `icons/icon_87_2_48x48.png` |
| imageres_mun | RT_ICON | 75 (group 5206) | 48x48 | 32 | `icons/icon_5206_1_48x48.png` |
| imageres_mun | RT_ICON | 750 (group 87) | 40x40 | 32 | `icons/icon_87_3_40x40.png` |
| imageres_mun | RT_ICON | 751 (group 87) | 32x32 | 32 | `icons/icon_87_4_32x32.png` |
| imageres_mun | RT_ICON | 752 (group 87) | 24x24 | 32 | `icons/icon_87_5_24x24.png` |
| imageres_mun | RT_ICON | 753 (group 87) | 20x20 | 32 | `icons/icon_87_6_20x20.png` |
| imageres_mun | RT_ICON | 754 (group 87) | 16x16 | 32 | `icons/icon_87_7_16x16.png` |
| imageres_mun | RT_ICON | 755 (group 88) | 256x256 | 32 | `icons/icon_88_0_256x256.png` |
| imageres_mun | RT_ICON | 756 (group 88) | 64x64 | 32 | `icons/icon_88_1_64x64.png` |
| imageres_mun | RT_ICON | 757 (group 88) | 48x48 | 32 | `icons/icon_88_2_48x48.png` |
| imageres_mun | RT_ICON | 758 (group 88) | 40x40 | 32 | `icons/icon_88_3_40x40.png` |
| imageres_mun | RT_ICON | 759 (group 88) | 32x32 | 32 | `icons/icon_88_4_32x32.png` |
| imageres_mun | RT_ICON | 76 (group 5206) | 32x32 | 32 | `icons/icon_5206_2_32x32.png` |
| imageres_mun | RT_ICON | 760 (group 88) | 24x24 | 32 | `icons/icon_88_5_24x24.png` |
| imageres_mun | RT_ICON | 761 (group 88) | 20x20 | 32 | `icons/icon_88_6_20x20.png` |
| imageres_mun | RT_ICON | 762 (group 88) | 16x16 | 32 | `icons/icon_88_7_16x16.png` |
| imageres_mun | RT_ICON | 763 (group 121) | 16x16 | 4 | `icons/icon_121_0_16x16.png` |
| imageres_mun | RT_ICON | 764 (group 197) | 256x256 | 32 | `icons/icon_197_0_256x256.png` |
| imageres_mun | RT_ICON | 765 (group 197) | 64x64 | 32 | `icons/icon_197_1_64x64.png` |
| imageres_mun | RT_ICON | 766 (group 197) | 48x48 | 32 | `icons/icon_197_2_48x48.png` |
| imageres_mun | RT_ICON | 767 (group 197) | 40x40 | 32 | `icons/icon_197_3_40x40.png` |
| imageres_mun | RT_ICON | 768 (group 197) | 32x32 | 32 | `icons/icon_197_4_32x32.png` |
| imageres_mun | RT_ICON | 769 (group 197) | 24x24 | 32 | `icons/icon_197_5_24x24.png` |
| imageres_mun | RT_ICON | 77 (group 5206) | 24x24 | 32 | `icons/icon_5206_3_24x24.png` |
| imageres_mun | RT_ICON | 770 (group 197) | 20x20 | 32 | `icons/icon_197_6_20x20.png` |
| imageres_mun | RT_ICON | 771 (group 197) | 16x16 | 32 | `icons/icon_197_7_16x16.png` |
| imageres_mun | RT_ICON | 772 (group 196) | 256x256 | 32 | `icons/icon_196_0_256x256.png` |
| imageres_mun | RT_ICON | 773 (group 196) | 64x64 | 32 | `icons/icon_196_1_64x64.png` |
| imageres_mun | RT_ICON | 774 (group 196) | 48x48 | 32 | `icons/icon_196_2_48x48.png` |
| imageres_mun | RT_ICON | 775 (group 196) | 40x40 | 32 | `icons/icon_196_3_40x40.png` |
| imageres_mun | RT_ICON | 776 (group 196) | 32x32 | 32 | `icons/icon_196_4_32x32.png` |
| imageres_mun | RT_ICON | 777 (group 196) | 24x24 | 32 | `icons/icon_196_5_24x24.png` |
| imageres_mun | RT_ICON | 778 (group 196) | 20x20 | 32 | `icons/icon_196_6_20x20.png` |
| imageres_mun | RT_ICON | 779 (group 196) | 16x16 | 32 | `icons/icon_196_7_16x16.png` |
| imageres_mun | RT_ICON | 78 (group 5206) | 16x16 | 32 | `icons/icon_5206_4_16x16.png` |
| imageres_mun | RT_ICON | 780 (group 120) | 256x256 | 32 | `icons/icon_120_0_256x256.png` |
| imageres_mun | RT_ICON | 781 (group 120) | 64x64 | 32 | `icons/icon_120_1_64x64.png` |
| imageres_mun | RT_ICON | 782 (group 120) | 48x48 | 32 | `icons/icon_120_2_48x48.png` |
| imageres_mun | RT_ICON | 783 (group 120) | 40x40 | 32 | `icons/icon_120_3_40x40.png` |
| imageres_mun | RT_ICON | 784 (group 120) | 32x32 | 32 | `icons/icon_120_4_32x32.png` |
| imageres_mun | RT_ICON | 785 (group 120) | 24x24 | 32 | `icons/icon_120_5_24x24.png` |
| imageres_mun | RT_ICON | 786 (group 120) | 20x20 | 32 | `icons/icon_120_6_20x20.png` |
| imageres_mun | RT_ICON | 787 (group 120) | 16x16 | 32 | `icons/icon_120_7_16x16.png` |
| imageres_mun | RT_ICON | 788 (group 195) | 256x256 | 32 | `icons/icon_195_0_256x256.png` |
| imageres_mun | RT_ICON | 789 (group 195) | 64x64 | 32 | `icons/icon_195_1_64x64.png` |
| imageres_mun | RT_ICON | 79 (group 17) | 256x256 | 8 | `icons/icon_17_0_256x256.png` |
| imageres_mun | RT_ICON | 790 (group 195) | 48x48 | 32 | `icons/icon_195_2_48x48.png` |
| imageres_mun | RT_ICON | 791 (group 195) | 40x40 | 32 | `icons/icon_195_3_40x40.png` |
| imageres_mun | RT_ICON | 792 (group 195) | 32x32 | 32 | `icons/icon_195_4_32x32.png` |
| imageres_mun | RT_ICON | 793 (group 195) | 24x24 | 32 | `icons/icon_195_5_24x24.png` |
| imageres_mun | RT_ICON | 794 (group 195) | 20x20 | 32 | `icons/icon_195_6_20x20.png` |
| imageres_mun | RT_ICON | 795 (group 195) | 16x16 | 32 | `icons/icon_195_7_16x16.png` |
| imageres_mun | RT_ICON | 796 (group 119) | 16x16 | 4 | `icons/icon_119_0_16x16.png` |
| imageres_mun | RT_ICON | 797 (group 89) | 256x256 | 32 | `icons/icon_89_0_256x256.png` |
| imageres_mun | RT_ICON | 798 (group 89) | 64x64 | 32 | `icons/icon_89_1_64x64.png` |
| imageres_mun | RT_ICON | 799 (group 89) | 48x48 | 32 | `icons/icon_89_2_48x48.png` |
| imageres_mun | RT_ICON | 8 (group 2) | 16x16 | 32 | `icons/icon_2_7_16x16.png` |
| imageres_mun | RT_ICON | 80 (group 17) | 256x256 | 32 | `icons/icon_17_1_256x256.png` |
| imageres_mun | RT_ICON | 800 (group 89) | 40x40 | 32 | `icons/icon_89_3_40x40.png` |
| imageres_mun | RT_ICON | 801 (group 89) | 32x32 | 32 | `icons/icon_89_4_32x32.png` |
| imageres_mun | RT_ICON | 802 (group 89) | 24x24 | 32 | `icons/icon_89_5_24x24.png` |
| imageres_mun | RT_ICON | 803 (group 89) | 20x20 | 32 | `icons/icon_89_6_20x20.png` |
| imageres_mun | RT_ICON | 804 (group 89) | 16x16 | 32 | `icons/icon_89_7_16x16.png` |
| imageres_mun | RT_ICON | 805 (group 90) | 256x256 | 32 | `icons/icon_90_0_256x256.png` |
| imageres_mun | RT_ICON | 806 (group 90) | 64x64 | 32 | `icons/icon_90_1_64x64.png` |
| imageres_mun | RT_ICON | 807 (group 90) | 48x48 | 32 | `icons/icon_90_2_48x48.png` |
| imageres_mun | RT_ICON | 808 (group 90) | 40x40 | 32 | `icons/icon_90_3_40x40.png` |
| imageres_mun | RT_ICON | 809 (group 90) | 32x32 | 32 | `icons/icon_90_4_32x32.png` |
| imageres_mun | RT_ICON | 81 (group 18) | 256x256 | 32 | `icons/icon_18_0_256x256.png` |
| imageres_mun | RT_ICON | 810 (group 90) | 24x24 | 32 | `icons/icon_90_5_24x24.png` |
| imageres_mun | RT_ICON | 811 (group 90) | 20x20 | 32 | `icons/icon_90_6_20x20.png` |
| imageres_mun | RT_ICON | 812 (group 90) | 16x16 | 32 | `icons/icon_90_7_16x16.png` |
| imageres_mun | RT_ICON | 813 (group 91) | 256x256 | 32 | `icons/icon_91_0_256x256.png` |
| imageres_mun | RT_ICON | 814 (group 91) | 64x64 | 32 | `icons/icon_91_1_64x64.png` |
| imageres_mun | RT_ICON | 815 (group 91) | 48x48 | 32 | `icons/icon_91_2_48x48.png` |
| imageres_mun | RT_ICON | 816 (group 91) | 40x40 | 32 | `icons/icon_91_3_40x40.png` |
| imageres_mun | RT_ICON | 817 (group 91) | 32x32 | 32 | `icons/icon_91_4_32x32.png` |
| imageres_mun | RT_ICON | 818 (group 91) | 24x24 | 32 | `icons/icon_91_5_24x24.png` |
| imageres_mun | RT_ICON | 819 (group 91) | 20x20 | 32 | `icons/icon_91_6_20x20.png` |
| imageres_mun | RT_ICON | 82 (group 18) | 64x64 | 32 | `icons/icon_18_1_64x64.png` |
| imageres_mun | RT_ICON | 820 (group 91) | 16x16 | 32 | `icons/icon_91_7_16x16.png` |
| imageres_mun | RT_ICON | 821 (group 92) | 256x256 | 32 | `icons/icon_92_0_256x256.png` |
| imageres_mun | RT_ICON | 822 (group 92) | 64x64 | 32 | `icons/icon_92_1_64x64.png` |
| imageres_mun | RT_ICON | 823 (group 92) | 48x48 | 32 | `icons/icon_92_2_48x48.png` |
| imageres_mun | RT_ICON | 824 (group 92) | 40x40 | 32 | `icons/icon_92_3_40x40.png` |
| imageres_mun | RT_ICON | 825 (group 92) | 32x32 | 32 | `icons/icon_92_4_32x32.png` |
| imageres_mun | RT_ICON | 826 (group 92) | 24x24 | 32 | `icons/icon_92_5_24x24.png` |
| imageres_mun | RT_ICON | 827 (group 92) | 20x20 | 32 | `icons/icon_92_6_20x20.png` |
| imageres_mun | RT_ICON | 828 (group 92) | 16x16 | 32 | `icons/icon_92_7_16x16.png` |
| imageres_mun | RT_ICON | 829 (group 93) | 256x256 | 32 | `icons/icon_93_0_256x256.png` |
| imageres_mun | RT_ICON | 83 (group 18) | 48x48 | 32 | `icons/icon_18_2_48x48.png` |
| imageres_mun | RT_ICON | 830 (group 93) | 64x64 | 32 | `icons/icon_93_1_64x64.png` |
| imageres_mun | RT_ICON | 831 (group 93) | 48x48 | 32 | `icons/icon_93_2_48x48.png` |
| imageres_mun | RT_ICON | 832 (group 93) | 40x40 | 32 | `icons/icon_93_3_40x40.png` |
| imageres_mun | RT_ICON | 833 (group 93) | 32x32 | 32 | `icons/icon_93_4_32x32.png` |
| imageres_mun | RT_ICON | 834 (group 93) | 24x24 | 32 | `icons/icon_93_5_24x24.png` |
| imageres_mun | RT_ICON | 835 (group 93) | 20x20 | 32 | `icons/icon_93_6_20x20.png` |
| imageres_mun | RT_ICON | 836 (group 93) | 16x16 | 32 | `icons/icon_93_7_16x16.png` |
| imageres_mun | RT_ICON | 837 (group 94) | 256x256 | 32 | `icons/icon_94_0_256x256.png` |
| imageres_mun | RT_ICON | 838 (group 94) | 64x64 | 32 | `icons/icon_94_1_64x64.png` |
| imageres_mun | RT_ICON | 839 (group 94) | 48x48 | 32 | `icons/icon_94_2_48x48.png` |
| imageres_mun | RT_ICON | 84 (group 18) | 40x40 | 32 | `icons/icon_18_3_40x40.png` |
| imageres_mun | RT_ICON | 840 (group 94) | 40x40 | 32 | `icons/icon_94_3_40x40.png` |
| imageres_mun | RT_ICON | 841 (group 94) | 32x32 | 32 | `icons/icon_94_4_32x32.png` |
| imageres_mun | RT_ICON | 842 (group 94) | 24x24 | 32 | `icons/icon_94_5_24x24.png` |
| imageres_mun | RT_ICON | 843 (group 94) | 20x20 | 32 | `icons/icon_94_6_20x20.png` |
| imageres_mun | RT_ICON | 844 (group 94) | 16x16 | 32 | `icons/icon_94_7_16x16.png` |
| imageres_mun | RT_ICON | 845 (group 95) | 256x256 | 32 | `icons/icon_95_0_256x256.png` |
| imageres_mun | RT_ICON | 846 (group 95) | 64x64 | 32 | `icons/icon_95_1_64x64.png` |
| imageres_mun | RT_ICON | 847 (group 95) | 48x48 | 32 | `icons/icon_95_2_48x48.png` |
| imageres_mun | RT_ICON | 848 (group 95) | 40x40 | 32 | `icons/icon_95_3_40x40.png` |
| imageres_mun | RT_ICON | 849 (group 95) | 32x32 | 32 | `icons/icon_95_4_32x32.png` |
| imageres_mun | RT_ICON | 85 (group 18) | 32x32 | 32 | `icons/icon_18_4_32x32.png` |
| imageres_mun | RT_ICON | 850 (group 95) | 24x24 | 32 | `icons/icon_95_5_24x24.png` |
| imageres_mun | RT_ICON | 851 (group 95) | 20x20 | 32 | `icons/icon_95_6_20x20.png` |
| imageres_mun | RT_ICON | 852 (group 95) | 16x16 | 32 | `icons/icon_95_7_16x16.png` |
| imageres_mun | RT_ICON | 853 (group 96) | 256x256 | 32 | `icons/icon_96_0_256x256.png` |
| imageres_mun | RT_ICON | 854 (group 96) | 64x64 | 32 | `icons/icon_96_1_64x64.png` |
| imageres_mun | RT_ICON | 855 (group 96) | 48x48 | 32 | `icons/icon_96_2_48x48.png` |
| imageres_mun | RT_ICON | 856 (group 96) | 40x40 | 32 | `icons/icon_96_3_40x40.png` |
| imageres_mun | RT_ICON | 857 (group 96) | 32x32 | 32 | `icons/icon_96_4_32x32.png` |
| imageres_mun | RT_ICON | 858 (group 96) | 24x24 | 32 | `icons/icon_96_5_24x24.png` |
| imageres_mun | RT_ICON | 859 (group 96) | 20x20 | 32 | `icons/icon_96_6_20x20.png` |
| imageres_mun | RT_ICON | 86 (group 18) | 24x24 | 32 | `icons/icon_18_5_24x24.png` |
| imageres_mun | RT_ICON | 860 (group 96) | 16x16 | 32 | `icons/icon_96_7_16x16.png` |
| imageres_mun | RT_ICON | 861 (group 97) | 256x256 | 32 | `icons/icon_97_0_256x256.png` |
| imageres_mun | RT_ICON | 862 (group 97) | 64x64 | 32 | `icons/icon_97_1_64x64.png` |
| imageres_mun | RT_ICON | 863 (group 97) | 48x48 | 32 | `icons/icon_97_2_48x48.png` |
| imageres_mun | RT_ICON | 864 (group 97) | 40x40 | 32 | `icons/icon_97_3_40x40.png` |
| imageres_mun | RT_ICON | 865 (group 97) | 32x32 | 32 | `icons/icon_97_4_32x32.png` |
| imageres_mun | RT_ICON | 866 (group 97) | 24x24 | 32 | `icons/icon_97_5_24x24.png` |
| imageres_mun | RT_ICON | 867 (group 97) | 20x20 | 32 | `icons/icon_97_6_20x20.png` |
| imageres_mun | RT_ICON | 868 (group 97) | 16x16 | 32 | `icons/icon_97_7_16x16.png` |
| imageres_mun | RT_ICON | 869 (group 99) | 256x256 | 32 | `icons/icon_99_0_256x256.png` |
| imageres_mun | RT_ICON | 87 (group 18) | 20x20 | 32 | `icons/icon_18_6_20x20.png` |
| imageres_mun | RT_ICON | 870 (group 99) | 64x64 | 32 | `icons/icon_99_1_64x64.png` |
| imageres_mun | RT_ICON | 871 (group 99) | 48x48 | 32 | `icons/icon_99_2_48x48.png` |
| imageres_mun | RT_ICON | 872 (group 99) | 40x40 | 32 | `icons/icon_99_3_40x40.png` |
| imageres_mun | RT_ICON | 873 (group 99) | 32x32 | 32 | `icons/icon_99_4_32x32.png` |
| imageres_mun | RT_ICON | 874 (group 99) | 24x24 | 32 | `icons/icon_99_5_24x24.png` |
| imageres_mun | RT_ICON | 875 (group 99) | 20x20 | 32 | `icons/icon_99_6_20x20.png` |
| imageres_mun | RT_ICON | 876 (group 99) | 16x16 | 32 | `icons/icon_99_7_16x16.png` |
| imageres_mun | RT_ICON | 877 (group 100) | 256x256 | 32 | `icons/icon_100_0_256x256.png` |
| imageres_mun | RT_ICON | 878 (group 100) | 64x64 | 32 | `icons/icon_100_1_64x64.png` |
| imageres_mun | RT_ICON | 879 (group 100) | 48x48 | 32 | `icons/icon_100_2_48x48.png` |
| imageres_mun | RT_ICON | 88 (group 18) | 16x16 | 32 | `icons/icon_18_7_16x16.png` |
| imageres_mun | RT_ICON | 880 (group 100) | 40x40 | 32 | `icons/icon_100_3_40x40.png` |
| imageres_mun | RT_ICON | 881 (group 100) | 32x32 | 32 | `icons/icon_100_4_32x32.png` |
| imageres_mun | RT_ICON | 882 (group 100) | 24x24 | 32 | `icons/icon_100_5_24x24.png` |
| imageres_mun | RT_ICON | 883 (group 100) | 20x20 | 32 | `icons/icon_100_6_20x20.png` |
| imageres_mun | RT_ICON | 884 (group 100) | 16x16 | 32 | `icons/icon_100_7_16x16.png` |
| imageres_mun | RT_ICON | 885 (group 101) | 256x256 | 32 | `icons/icon_101_0_256x256.png` |
| imageres_mun | RT_ICON | 886 (group 101) | 64x64 | 32 | `icons/icon_101_1_64x64.png` |
| imageres_mun | RT_ICON | 887 (group 101) | 48x48 | 32 | `icons/icon_101_2_48x48.png` |
| imageres_mun | RT_ICON | 888 (group 101) | 40x40 | 32 | `icons/icon_101_3_40x40.png` |
| imageres_mun | RT_ICON | 889 (group 101) | 32x32 | 32 | `icons/icon_101_4_32x32.png` |
| imageres_mun | RT_ICON | 89 (group 19) | 256x256 | 32 | `icons/icon_19_0_256x256.png` |
| imageres_mun | RT_ICON | 890 (group 101) | 24x24 | 32 | `icons/icon_101_5_24x24.png` |
| imageres_mun | RT_ICON | 891 (group 101) | 20x20 | 32 | `icons/icon_101_6_20x20.png` |
| imageres_mun | RT_ICON | 892 (group 101) | 16x16 | 32 | `icons/icon_101_7_16x16.png` |
| imageres_mun | RT_ICON | 893 (group 102) | 256x256 | 32 | `icons/icon_102_0_256x256.png` |
| imageres_mun | RT_ICON | 894 (group 102) | 64x64 | 32 | `icons/icon_102_1_64x64.png` |
| imageres_mun | RT_ICON | 895 (group 102) | 48x48 | 32 | `icons/icon_102_2_48x48.png` |
| imageres_mun | RT_ICON | 896 (group 102) | 40x40 | 32 | `icons/icon_102_3_40x40.png` |
| imageres_mun | RT_ICON | 897 (group 102) | 32x32 | 32 | `icons/icon_102_4_32x32.png` |
| imageres_mun | RT_ICON | 898 (group 102) | 24x24 | 32 | `icons/icon_102_5_24x24.png` |
| imageres_mun | RT_ICON | 899 (group 102) | 20x20 | 32 | `icons/icon_102_6_20x20.png` |
| imageres_mun | RT_ICON | 9 (group 3) | 256x256 | 32 | `icons/icon_3_0_256x256.png` |
| imageres_mun | RT_ICON | 90 (group 19) | 64x64 | 32 | `icons/icon_19_1_64x64.png` |
| imageres_mun | RT_ICON | 900 (group 102) | 16x16 | 32 | `icons/icon_102_7_16x16.png` |
| imageres_mun | RT_ICON | 901 (group 103) | 256x256 | 32 | `icons/icon_103_0_256x256.png` |
| imageres_mun | RT_ICON | 902 (group 103) | 64x64 | 32 | `icons/icon_103_1_64x64.png` |
| imageres_mun | RT_ICON | 903 (group 103) | 48x48 | 32 | `icons/icon_103_2_48x48.png` |
| imageres_mun | RT_ICON | 904 (group 103) | 40x40 | 32 | `icons/icon_103_3_40x40.png` |
| imageres_mun | RT_ICON | 905 (group 103) | 32x32 | 32 | `icons/icon_103_4_32x32.png` |
| imageres_mun | RT_ICON | 906 (group 103) | 24x24 | 32 | `icons/icon_103_5_24x24.png` |
| imageres_mun | RT_ICON | 907 (group 103) | 20x20 | 32 | `icons/icon_103_6_20x20.png` |
| imageres_mun | RT_ICON | 908 (group 103) | 16x16 | 32 | `icons/icon_103_7_16x16.png` |
| imageres_mun | RT_ICON | 909 (group 108) | 256x256 | 32 | `icons/icon_108_0_256x256.png` |
| imageres_mun | RT_ICON | 91 (group 19) | 48x48 | 32 | `icons/icon_19_2_48x48.png` |
| imageres_mun | RT_ICON | 910 (group 108) | 64x64 | 32 | `icons/icon_108_1_64x64.png` |
| imageres_mun | RT_ICON | 911 (group 108) | 48x48 | 32 | `icons/icon_108_2_48x48.png` |
| imageres_mun | RT_ICON | 912 (group 108) | 40x40 | 32 | `icons/icon_108_3_40x40.png` |
| imageres_mun | RT_ICON | 913 (group 108) | 32x32 | 32 | `icons/icon_108_4_32x32.png` |
| imageres_mun | RT_ICON | 914 (group 108) | 24x24 | 32 | `icons/icon_108_5_24x24.png` |
| imageres_mun | RT_ICON | 915 (group 108) | 20x20 | 32 | `icons/icon_108_6_20x20.png` |
| imageres_mun | RT_ICON | 916 (group 108) | 16x16 | 32 | `icons/icon_108_7_16x16.png` |
| imageres_mun | RT_ICON | 917 (group 109) | 256x256 | 32 | `icons/icon_109_0_256x256.png` |
| imageres_mun | RT_ICON | 918 (group 109) | 64x64 | 32 | `icons/icon_109_1_64x64.png` |
| imageres_mun | RT_ICON | 919 (group 109) | 48x48 | 32 | `icons/icon_109_2_48x48.png` |
| imageres_mun | RT_ICON | 92 (group 19) | 40x40 | 32 | `icons/icon_19_3_40x40.png` |
| imageres_mun | RT_ICON | 920 (group 109) | 40x40 | 32 | `icons/icon_109_3_40x40.png` |
| imageres_mun | RT_ICON | 921 (group 109) | 32x32 | 32 | `icons/icon_109_4_32x32.png` |
| imageres_mun | RT_ICON | 922 (group 109) | 24x24 | 32 | `icons/icon_109_5_24x24.png` |
| imageres_mun | RT_ICON | 923 (group 109) | 20x20 | 32 | `icons/icon_109_6_20x20.png` |
| imageres_mun | RT_ICON | 924 (group 109) | 16x16 | 32 | `icons/icon_109_7_16x16.png` |
| imageres_mun | RT_ICON | 925 (group 110) | 256x256 | 32 | `icons/icon_110_0_256x256.png` |
| imageres_mun | RT_ICON | 926 (group 110) | 64x64 | 32 | `icons/icon_110_1_64x64.png` |
| imageres_mun | RT_ICON | 927 (group 110) | 48x48 | 32 | `icons/icon_110_2_48x48.png` |
| imageres_mun | RT_ICON | 928 (group 110) | 40x40 | 32 | `icons/icon_110_3_40x40.png` |
| imageres_mun | RT_ICON | 929 (group 110) | 32x32 | 32 | `icons/icon_110_4_32x32.png` |
| imageres_mun | RT_ICON | 93 (group 19) | 32x32 | 32 | `icons/icon_19_4_32x32.png` |
| imageres_mun | RT_ICON | 930 (group 110) | 24x24 | 32 | `icons/icon_110_5_24x24.png` |
| imageres_mun | RT_ICON | 931 (group 110) | 20x20 | 32 | `icons/icon_110_6_20x20.png` |
| imageres_mun | RT_ICON | 932 (group 110) | 16x16 | 32 | `icons/icon_110_7_16x16.png` |
| imageres_mun | RT_ICON | 933 (group 111) | 256x256 | 32 | `icons/icon_111_0_256x256.png` |
| imageres_mun | RT_ICON | 934 (group 111) | 64x64 | 32 | `icons/icon_111_1_64x64.png` |
| imageres_mun | RT_ICON | 935 (group 111) | 48x48 | 32 | `icons/icon_111_2_48x48.png` |
| imageres_mun | RT_ICON | 936 (group 111) | 40x40 | 32 | `icons/icon_111_3_40x40.png` |
| imageres_mun | RT_ICON | 937 (group 111) | 32x32 | 32 | `icons/icon_111_4_32x32.png` |
| imageres_mun | RT_ICON | 938 (group 111) | 24x24 | 32 | `icons/icon_111_5_24x24.png` |
| imageres_mun | RT_ICON | 939 (group 111) | 20x20 | 32 | `icons/icon_111_6_20x20.png` |
| imageres_mun | RT_ICON | 94 (group 19) | 24x24 | 32 | `icons/icon_19_5_24x24.png` |
| imageres_mun | RT_ICON | 940 (group 111) | 16x16 | 32 | `icons/icon_111_7_16x16.png` |
| imageres_mun | RT_ICON | 941 (group 112) | 256x256 | 32 | `icons/icon_112_0_256x256.png` |
| imageres_mun | RT_ICON | 942 (group 112) | 64x64 | 32 | `icons/icon_112_1_64x64.png` |
| imageres_mun | RT_ICON | 943 (group 112) | 48x48 | 32 | `icons/icon_112_2_48x48.png` |
| imageres_mun | RT_ICON | 944 (group 112) | 40x40 | 32 | `icons/icon_112_3_40x40.png` |
| imageres_mun | RT_ICON | 945 (group 112) | 32x32 | 32 | `icons/icon_112_4_32x32.png` |
| imageres_mun | RT_ICON | 946 (group 112) | 24x24 | 32 | `icons/icon_112_5_24x24.png` |
| imageres_mun | RT_ICON | 947 (group 112) | 20x20 | 32 | `icons/icon_112_6_20x20.png` |
| imageres_mun | RT_ICON | 948 (group 112) | 16x16 | 32 | `icons/icon_112_7_16x16.png` |
| imageres_mun | RT_ICON | 949 (group 113) | 256x256 | 32 | `icons/icon_113_0_256x256.png` |
| imageres_mun | RT_ICON | 95 (group 19) | 20x20 | 32 | `icons/icon_19_6_20x20.png` |
| imageres_mun | RT_ICON | 950 (group 113) | 64x64 | 32 | `icons/icon_113_1_64x64.png` |
| imageres_mun | RT_ICON | 951 (group 113) | 48x48 | 32 | `icons/icon_113_2_48x48.png` |
| imageres_mun | RT_ICON | 952 (group 113) | 40x40 | 32 | `icons/icon_113_3_40x40.png` |
| imageres_mun | RT_ICON | 953 (group 113) | 32x32 | 32 | `icons/icon_113_4_32x32.png` |
| imageres_mun | RT_ICON | 954 (group 113) | 24x24 | 32 | `icons/icon_113_5_24x24.png` |
| imageres_mun | RT_ICON | 955 (group 113) | 20x20 | 32 | `icons/icon_113_6_20x20.png` |
| imageres_mun | RT_ICON | 956 (group 113) | 16x16 | 32 | `icons/icon_113_7_16x16.png` |
| imageres_mun | RT_ICON | 957 (group 189) | 256x256 | 32 | `icons/icon_189_0_256x256.png` |
| imageres_mun | RT_ICON | 958 (group 189) | 64x64 | 32 | `icons/icon_189_1_64x64.png` |
| imageres_mun | RT_ICON | 959 (group 189) | 48x48 | 32 | `icons/icon_189_2_48x48.png` |
| imageres_mun | RT_ICON | 96 (group 19) | 16x16 | 32 | `icons/icon_19_7_16x16.png` |
| imageres_mun | RT_ICON | 960 (group 189) | 40x40 | 32 | `icons/icon_189_3_40x40.png` |
| imageres_mun | RT_ICON | 961 (group 189) | 32x32 | 32 | `icons/icon_189_4_32x32.png` |
| imageres_mun | RT_ICON | 962 (group 189) | 24x24 | 32 | `icons/icon_189_5_24x24.png` |
| imageres_mun | RT_ICON | 963 (group 189) | 20x20 | 32 | `icons/icon_189_6_20x20.png` |
| imageres_mun | RT_ICON | 964 (group 189) | 16x16 | 32 | `icons/icon_189_7_16x16.png` |
| imageres_mun | RT_ICON | 965 (group 114) | 256x256 | 32 | `icons/icon_114_0_256x256.png` |
| imageres_mun | RT_ICON | 966 (group 114) | 64x64 | 32 | `icons/icon_114_1_64x64.png` |
| imageres_mun | RT_ICON | 967 (group 114) | 48x48 | 32 | `icons/icon_114_2_48x48.png` |
| imageres_mun | RT_ICON | 968 (group 114) | 40x40 | 32 | `icons/icon_114_3_40x40.png` |
| imageres_mun | RT_ICON | 969 (group 114) | 32x32 | 32 | `icons/icon_114_4_32x32.png` |
| imageres_mun | RT_ICON | 97 (group 20) | 256x256 | 32 | `icons/icon_20_0_256x256.png` |
| imageres_mun | RT_ICON | 970 (group 114) | 24x24 | 32 | `icons/icon_114_5_24x24.png` |
| imageres_mun | RT_ICON | 971 (group 114) | 20x20 | 32 | `icons/icon_114_6_20x20.png` |
| imageres_mun | RT_ICON | 972 (group 114) | 16x16 | 32 | `icons/icon_114_7_16x16.png` |
| imageres_mun | RT_ICON | 973 (group 115) | 256x256 | 32 | `icons/icon_115_0_256x256.png` |
| imageres_mun | RT_ICON | 974 (group 115) | 64x64 | 32 | `icons/icon_115_1_64x64.png` |
| imageres_mun | RT_ICON | 975 (group 115) | 48x48 | 32 | `icons/icon_115_2_48x48.png` |
| imageres_mun | RT_ICON | 976 (group 115) | 40x40 | 32 | `icons/icon_115_3_40x40.png` |
| imageres_mun | RT_ICON | 977 (group 115) | 32x32 | 32 | `icons/icon_115_4_32x32.png` |
| imageres_mun | RT_ICON | 978 (group 115) | 24x24 | 32 | `icons/icon_115_5_24x24.png` |
| imageres_mun | RT_ICON | 979 (group 115) | 20x20 | 32 | `icons/icon_115_6_20x20.png` |
| imageres_mun | RT_ICON | 98 (group 20) | 64x64 | 32 | `icons/icon_20_1_64x64.png` |
| imageres_mun | RT_ICON | 980 (group 115) | 16x16 | 32 | `icons/icon_115_7_16x16.png` |
| imageres_mun | RT_ICON | 981 (group 116) | 256x256 | 32 | `icons/icon_116_0_256x256.png` |
| imageres_mun | RT_ICON | 982 (group 116) | 64x64 | 32 | `icons/icon_116_1_64x64.png` |
| imageres_mun | RT_ICON | 983 (group 116) | 48x48 | 32 | `icons/icon_116_2_48x48.png` |
| imageres_mun | RT_ICON | 984 (group 116) | 40x40 | 32 | `icons/icon_116_3_40x40.png` |
| imageres_mun | RT_ICON | 985 (group 116) | 32x32 | 32 | `icons/icon_116_4_32x32.png` |
| imageres_mun | RT_ICON | 986 (group 116) | 24x24 | 32 | `icons/icon_116_5_24x24.png` |
| imageres_mun | RT_ICON | 987 (group 116) | 20x20 | 32 | `icons/icon_116_6_20x20.png` |
| imageres_mun | RT_ICON | 988 (group 116) | 16x16 | 32 | `icons/icon_116_7_16x16.png` |
| imageres_mun | RT_ICON | 989 (group 117) | 256x256 | 32 | `icons/icon_117_0_256x256.png` |
| imageres_mun | RT_ICON | 99 (group 20) | 48x48 | 32 | `icons/icon_20_2_48x48.png` |
| imageres_mun | RT_ICON | 990 (group 117) | 64x64 | 32 | `icons/icon_117_1_64x64.png` |
| imageres_mun | RT_ICON | 991 (group 117) | 48x48 | 32 | `icons/icon_117_2_48x48.png` |
| imageres_mun | RT_ICON | 992 (group 117) | 40x40 | 32 | `icons/icon_117_3_40x40.png` |
| imageres_mun | RT_ICON | 993 (group 117) | 32x32 | 32 | `icons/icon_117_4_32x32.png` |
| imageres_mun | RT_ICON | 994 (group 117) | 24x24 | 32 | `icons/icon_117_5_24x24.png` |
| imageres_mun | RT_ICON | 995 (group 117) | 20x20 | 32 | `icons/icon_117_6_20x20.png` |
| imageres_mun | RT_ICON | 996 (group 117) | 16x16 | 32 | `icons/icon_117_7_16x16.png` |
| imageres_mun | RT_ICON | 997 (group 6400) | 256x256 | 32 | `icons/icon_6400_0_256x256.png` |
| imageres_mun | RT_ICON | 998 (group 6400) | 64x64 | 32 | `icons/icon_6400_1_64x64.png` |
| imageres_mun | RT_ICON | 999 (group 6400) | 48x48 | 32 | `icons/icon_6400_2_48x48.png` |
| mmres_mun | RT_GROUP_ICON | 3004 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_3004.ico` |
| mmres_mun | RT_GROUP_ICON | 3010 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_3010.ico` |
| mmres_mun | RT_GROUP_ICON | 3011 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_3011.ico` |
| mmres_mun | RT_GROUP_ICON | 3012 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_3012.ico` |
| mmres_mun | RT_GROUP_ICON | 3013 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_3013.ico` |
| mmres_mun | RT_GROUP_ICON | 3014 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_3014.ico` |
| mmres_mun | RT_GROUP_ICON | 3015 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_3015.ico` |
| mmres_mun | RT_GROUP_ICON | 3016 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_3016.ico` |
| mmres_mun | RT_GROUP_ICON | 3017 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_3017.ico` |
| mmres_mun | RT_GROUP_ICON | 3018 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_3018.ico` |
| mmres_mun | RT_GROUP_ICON | 3019 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_3019.ico` |
| mmres_mun | RT_GROUP_ICON | 3020 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_3020.ico` |
| mmres_mun | RT_GROUP_ICON | 3021 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_3021.ico` |
| mmres_mun | RT_GROUP_ICON | 3030 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_3030.ico` |
| mmres_mun | RT_GROUP_ICON | 3031 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_3031.ico` |
| mmres_mun | RT_GROUP_ICON | 3050 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_3050.ico` |
| mmres_mun | RT_GROUP_ICON | 3051 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_3051.ico` |
| mmres_mun | RT_GROUP_ICON | 3052 | 64x64+48x48+32x32+16x16 | 32,32,32,32 | `icons/groupicon_3052.ico` |
| mmres_mun | RT_ICON | 1 (group 3004) | 256x256 | 32 | `icons/icon_3004_0_256x256.png` |
| mmres_mun | RT_ICON | 10 (group 3010) | 64x64 | 32 | `icons/icon_3010_1_64x64.png` |
| mmres_mun | RT_ICON | 100 (group 3021) | 40x40 | 32 | `icons/icon_3021_3_40x40.png` |
| mmres_mun | RT_ICON | 101 (group 3021) | 32x32 | 32 | `icons/icon_3021_4_32x32.png` |
| mmres_mun | RT_ICON | 102 (group 3021) | 24x24 | 32 | `icons/icon_3021_5_24x24.png` |
| mmres_mun | RT_ICON | 103 (group 3021) | 20x20 | 32 | `icons/icon_3021_6_20x20.png` |
| mmres_mun | RT_ICON | 104 (group 3021) | 16x16 | 32 | `icons/icon_3021_7_16x16.png` |
| mmres_mun | RT_ICON | 105 (group 3030) | 256x256 | 32 | `icons/icon_3030_0_256x256.png` |
| mmres_mun | RT_ICON | 106 (group 3030) | 64x64 | 32 | `icons/icon_3030_1_64x64.png` |
| mmres_mun | RT_ICON | 107 (group 3030) | 48x48 | 32 | `icons/icon_3030_2_48x48.png` |
| mmres_mun | RT_ICON | 108 (group 3030) | 40x40 | 32 | `icons/icon_3030_3_40x40.png` |
| mmres_mun | RT_ICON | 109 (group 3030) | 32x32 | 32 | `icons/icon_3030_4_32x32.png` |
| mmres_mun | RT_ICON | 11 (group 3010) | 48x48 | 32 | `icons/icon_3010_2_48x48.png` |
| mmres_mun | RT_ICON | 110 (group 3030) | 24x24 | 32 | `icons/icon_3030_5_24x24.png` |
| mmres_mun | RT_ICON | 111 (group 3030) | 20x20 | 32 | `icons/icon_3030_6_20x20.png` |
| mmres_mun | RT_ICON | 112 (group 3030) | 16x16 | 32 | `icons/icon_3030_7_16x16.png` |
| mmres_mun | RT_ICON | 113 (group 3031) | 256x256 | 32 | `icons/icon_3031_0_256x256.png` |
| mmres_mun | RT_ICON | 114 (group 3031) | 64x64 | 32 | `icons/icon_3031_1_64x64.png` |
| mmres_mun | RT_ICON | 115 (group 3031) | 48x48 | 32 | `icons/icon_3031_2_48x48.png` |
| mmres_mun | RT_ICON | 116 (group 3031) | 40x40 | 32 | `icons/icon_3031_3_40x40.png` |
| mmres_mun | RT_ICON | 117 (group 3031) | 32x32 | 32 | `icons/icon_3031_4_32x32.png` |
| mmres_mun | RT_ICON | 118 (group 3031) | 24x24 | 32 | `icons/icon_3031_5_24x24.png` |
| mmres_mun | RT_ICON | 119 (group 3031) | 20x20 | 32 | `icons/icon_3031_6_20x20.png` |
| mmres_mun | RT_ICON | 12 (group 3010) | 40x40 | 32 | `icons/icon_3010_3_40x40.png` |
| mmres_mun | RT_ICON | 120 (group 3031) | 16x16 | 32 | `icons/icon_3031_7_16x16.png` |
| mmres_mun | RT_ICON | 121 (group 3050) | 256x256 | 32 | `icons/icon_3050_0_256x256.png` |
| mmres_mun | RT_ICON | 122 (group 3050) | 64x64 | 32 | `icons/icon_3050_1_64x64.png` |
| mmres_mun | RT_ICON | 123 (group 3050) | 48x48 | 32 | `icons/icon_3050_2_48x48.png` |
| mmres_mun | RT_ICON | 124 (group 3050) | 40x40 | 32 | `icons/icon_3050_3_40x40.png` |
| mmres_mun | RT_ICON | 125 (group 3050) | 32x32 | 32 | `icons/icon_3050_4_32x32.png` |
| mmres_mun | RT_ICON | 126 (group 3050) | 24x24 | 32 | `icons/icon_3050_5_24x24.png` |
| mmres_mun | RT_ICON | 127 (group 3050) | 20x20 | 32 | `icons/icon_3050_6_20x20.png` |
| mmres_mun | RT_ICON | 128 (group 3050) | 16x16 | 32 | `icons/icon_3050_7_16x16.png` |
| mmres_mun | RT_ICON | 129 (group 3051) | 256x256 | 32 | `icons/icon_3051_0_256x256.png` |
| mmres_mun | RT_ICON | 13 (group 3010) | 32x32 | 32 | `icons/icon_3010_4_32x32.png` |
| mmres_mun | RT_ICON | 130 (group 3051) | 64x64 | 32 | `icons/icon_3051_1_64x64.png` |
| mmres_mun | RT_ICON | 131 (group 3051) | 48x48 | 32 | `icons/icon_3051_2_48x48.png` |
| mmres_mun | RT_ICON | 132 (group 3051) | 40x40 | 32 | `icons/icon_3051_3_40x40.png` |
| mmres_mun | RT_ICON | 133 (group 3051) | 32x32 | 32 | `icons/icon_3051_4_32x32.png` |
| mmres_mun | RT_ICON | 134 (group 3051) | 24x24 | 32 | `icons/icon_3051_5_24x24.png` |
| mmres_mun | RT_ICON | 135 (group 3051) | 20x20 | 32 | `icons/icon_3051_6_20x20.png` |
| mmres_mun | RT_ICON | 136 (group 3051) | 16x16 | 32 | `icons/icon_3051_7_16x16.png` |
| mmres_mun | RT_ICON | 137 (group 3052) | 64x64 | 32 | `icons/icon_3052_0_64x64.png` |
| mmres_mun | RT_ICON | 138 (group 3052) | 48x48 | 32 | `icons/icon_3052_1_48x48.png` |
| mmres_mun | RT_ICON | 139 (group 3052) | 32x32 | 32 | `icons/icon_3052_2_32x32.png` |
| mmres_mun | RT_ICON | 14 (group 3010) | 24x24 | 32 | `icons/icon_3010_5_24x24.png` |
| mmres_mun | RT_ICON | 140 (group 3052) | 16x16 | 32 | `icons/icon_3052_3_16x16.png` |
| mmres_mun | RT_ICON | 15 (group 3010) | 20x20 | 32 | `icons/icon_3010_6_20x20.png` |
| mmres_mun | RT_ICON | 16 (group 3010) | 16x16 | 32 | `icons/icon_3010_7_16x16.png` |
| mmres_mun | RT_ICON | 17 (group 3011) | 256x256 | 32 | `icons/icon_3011_0_256x256.png` |
| mmres_mun | RT_ICON | 18 (group 3011) | 64x64 | 32 | `icons/icon_3011_1_64x64.png` |
| mmres_mun | RT_ICON | 19 (group 3011) | 48x48 | 32 | `icons/icon_3011_2_48x48.png` |
| mmres_mun | RT_ICON | 2 (group 3004) | 64x64 | 32 | `icons/icon_3004_1_64x64.png` |
| mmres_mun | RT_ICON | 20 (group 3011) | 40x40 | 32 | `icons/icon_3011_3_40x40.png` |
| mmres_mun | RT_ICON | 21 (group 3011) | 32x32 | 32 | `icons/icon_3011_4_32x32.png` |
| mmres_mun | RT_ICON | 22 (group 3011) | 24x24 | 32 | `icons/icon_3011_5_24x24.png` |
| mmres_mun | RT_ICON | 23 (group 3011) | 20x20 | 32 | `icons/icon_3011_6_20x20.png` |
| mmres_mun | RT_ICON | 24 (group 3011) | 16x16 | 32 | `icons/icon_3011_7_16x16.png` |
| mmres_mun | RT_ICON | 25 (group 3012) | 256x256 | 32 | `icons/icon_3012_0_256x256.png` |
| mmres_mun | RT_ICON | 26 (group 3012) | 64x64 | 32 | `icons/icon_3012_1_64x64.png` |
| mmres_mun | RT_ICON | 27 (group 3012) | 48x48 | 32 | `icons/icon_3012_2_48x48.png` |
| mmres_mun | RT_ICON | 28 (group 3012) | 40x40 | 32 | `icons/icon_3012_3_40x40.png` |
| mmres_mun | RT_ICON | 29 (group 3012) | 32x32 | 32 | `icons/icon_3012_4_32x32.png` |
| mmres_mun | RT_ICON | 3 (group 3004) | 48x48 | 32 | `icons/icon_3004_2_48x48.png` |
| mmres_mun | RT_ICON | 30 (group 3012) | 24x24 | 32 | `icons/icon_3012_5_24x24.png` |
| mmres_mun | RT_ICON | 31 (group 3012) | 20x20 | 32 | `icons/icon_3012_6_20x20.png` |
| mmres_mun | RT_ICON | 32 (group 3012) | 16x16 | 32 | `icons/icon_3012_7_16x16.png` |
| mmres_mun | RT_ICON | 33 (group 3013) | 256x256 | 32 | `icons/icon_3013_0_256x256.png` |
| mmres_mun | RT_ICON | 34 (group 3013) | 64x64 | 32 | `icons/icon_3013_1_64x64.png` |
| mmres_mun | RT_ICON | 35 (group 3013) | 48x48 | 32 | `icons/icon_3013_2_48x48.png` |
| mmres_mun | RT_ICON | 36 (group 3013) | 40x40 | 32 | `icons/icon_3013_3_40x40.png` |
| mmres_mun | RT_ICON | 37 (group 3013) | 32x32 | 32 | `icons/icon_3013_4_32x32.png` |
| mmres_mun | RT_ICON | 38 (group 3013) | 24x24 | 32 | `icons/icon_3013_5_24x24.png` |
| mmres_mun | RT_ICON | 39 (group 3013) | 20x20 | 32 | `icons/icon_3013_6_20x20.png` |
| mmres_mun | RT_ICON | 4 (group 3004) | 40x40 | 32 | `icons/icon_3004_3_40x40.png` |
| mmres_mun | RT_ICON | 40 (group 3013) | 16x16 | 32 | `icons/icon_3013_7_16x16.png` |
| mmres_mun | RT_ICON | 41 (group 3014) | 256x256 | 32 | `icons/icon_3014_0_256x256.png` |
| mmres_mun | RT_ICON | 42 (group 3014) | 64x64 | 32 | `icons/icon_3014_1_64x64.png` |
| mmres_mun | RT_ICON | 43 (group 3014) | 48x48 | 32 | `icons/icon_3014_2_48x48.png` |
| mmres_mun | RT_ICON | 44 (group 3014) | 40x40 | 32 | `icons/icon_3014_3_40x40.png` |
| mmres_mun | RT_ICON | 45 (group 3014) | 32x32 | 32 | `icons/icon_3014_4_32x32.png` |
| mmres_mun | RT_ICON | 46 (group 3014) | 24x24 | 32 | `icons/icon_3014_5_24x24.png` |
| mmres_mun | RT_ICON | 47 (group 3014) | 20x20 | 32 | `icons/icon_3014_6_20x20.png` |
| mmres_mun | RT_ICON | 48 (group 3014) | 16x16 | 32 | `icons/icon_3014_7_16x16.png` |
| mmres_mun | RT_ICON | 49 (group 3015) | 256x256 | 32 | `icons/icon_3015_0_256x256.png` |
| mmres_mun | RT_ICON | 5 (group 3004) | 32x32 | 32 | `icons/icon_3004_4_32x32.png` |
| mmres_mun | RT_ICON | 50 (group 3015) | 64x64 | 32 | `icons/icon_3015_1_64x64.png` |
| mmres_mun | RT_ICON | 51 (group 3015) | 48x48 | 32 | `icons/icon_3015_2_48x48.png` |
| mmres_mun | RT_ICON | 52 (group 3015) | 40x40 | 32 | `icons/icon_3015_3_40x40.png` |
| mmres_mun | RT_ICON | 53 (group 3015) | 32x32 | 32 | `icons/icon_3015_4_32x32.png` |
| mmres_mun | RT_ICON | 54 (group 3015) | 24x24 | 32 | `icons/icon_3015_5_24x24.png` |
| mmres_mun | RT_ICON | 55 (group 3015) | 20x20 | 32 | `icons/icon_3015_6_20x20.png` |
| mmres_mun | RT_ICON | 56 (group 3015) | 16x16 | 32 | `icons/icon_3015_7_16x16.png` |
| mmres_mun | RT_ICON | 57 (group 3016) | 256x256 | 32 | `icons/icon_3016_0_256x256.png` |
| mmres_mun | RT_ICON | 58 (group 3016) | 64x64 | 32 | `icons/icon_3016_1_64x64.png` |
| mmres_mun | RT_ICON | 59 (group 3016) | 48x48 | 32 | `icons/icon_3016_2_48x48.png` |
| mmres_mun | RT_ICON | 6 (group 3004) | 24x24 | 32 | `icons/icon_3004_5_24x24.png` |
| mmres_mun | RT_ICON | 60 (group 3016) | 40x40 | 32 | `icons/icon_3016_3_40x40.png` |
| mmres_mun | RT_ICON | 61 (group 3016) | 32x32 | 32 | `icons/icon_3016_4_32x32.png` |
| mmres_mun | RT_ICON | 62 (group 3016) | 24x24 | 32 | `icons/icon_3016_5_24x24.png` |
| mmres_mun | RT_ICON | 63 (group 3016) | 20x20 | 32 | `icons/icon_3016_6_20x20.png` |
| mmres_mun | RT_ICON | 64 (group 3016) | 16x16 | 32 | `icons/icon_3016_7_16x16.png` |
| mmres_mun | RT_ICON | 65 (group 3017) | 256x256 | 32 | `icons/icon_3017_0_256x256.png` |
| mmres_mun | RT_ICON | 66 (group 3017) | 64x64 | 32 | `icons/icon_3017_1_64x64.png` |
| mmres_mun | RT_ICON | 67 (group 3017) | 48x48 | 32 | `icons/icon_3017_2_48x48.png` |
| mmres_mun | RT_ICON | 68 (group 3017) | 40x40 | 32 | `icons/icon_3017_3_40x40.png` |
| mmres_mun | RT_ICON | 69 (group 3017) | 32x32 | 32 | `icons/icon_3017_4_32x32.png` |
| mmres_mun | RT_ICON | 7 (group 3004) | 20x20 | 32 | `icons/icon_3004_6_20x20.png` |
| mmres_mun | RT_ICON | 70 (group 3017) | 24x24 | 32 | `icons/icon_3017_5_24x24.png` |
| mmres_mun | RT_ICON | 71 (group 3017) | 20x20 | 32 | `icons/icon_3017_6_20x20.png` |
| mmres_mun | RT_ICON | 72 (group 3017) | 16x16 | 32 | `icons/icon_3017_7_16x16.png` |
| mmres_mun | RT_ICON | 73 (group 3018) | 256x256 | 32 | `icons/icon_3018_0_256x256.png` |
| mmres_mun | RT_ICON | 74 (group 3018) | 64x64 | 32 | `icons/icon_3018_1_64x64.png` |
| mmres_mun | RT_ICON | 75 (group 3018) | 48x48 | 32 | `icons/icon_3018_2_48x48.png` |
| mmres_mun | RT_ICON | 76 (group 3018) | 40x40 | 32 | `icons/icon_3018_3_40x40.png` |
| mmres_mun | RT_ICON | 77 (group 3018) | 32x32 | 32 | `icons/icon_3018_4_32x32.png` |
| mmres_mun | RT_ICON | 78 (group 3018) | 24x24 | 32 | `icons/icon_3018_5_24x24.png` |
| mmres_mun | RT_ICON | 79 (group 3018) | 20x20 | 32 | `icons/icon_3018_6_20x20.png` |
| mmres_mun | RT_ICON | 8 (group 3004) | 16x16 | 32 | `icons/icon_3004_7_16x16.png` |
| mmres_mun | RT_ICON | 80 (group 3018) | 16x16 | 32 | `icons/icon_3018_7_16x16.png` |
| mmres_mun | RT_ICON | 81 (group 3019) | 256x256 | 32 | `icons/icon_3019_0_256x256.png` |
| mmres_mun | RT_ICON | 82 (group 3019) | 64x64 | 32 | `icons/icon_3019_1_64x64.png` |
| mmres_mun | RT_ICON | 83 (group 3019) | 48x48 | 32 | `icons/icon_3019_2_48x48.png` |
| mmres_mun | RT_ICON | 84 (group 3019) | 40x40 | 32 | `icons/icon_3019_3_40x40.png` |
| mmres_mun | RT_ICON | 85 (group 3019) | 32x32 | 32 | `icons/icon_3019_4_32x32.png` |
| mmres_mun | RT_ICON | 86 (group 3019) | 24x24 | 32 | `icons/icon_3019_5_24x24.png` |
| mmres_mun | RT_ICON | 87 (group 3019) | 20x20 | 32 | `icons/icon_3019_6_20x20.png` |
| mmres_mun | RT_ICON | 88 (group 3019) | 16x16 | 32 | `icons/icon_3019_7_16x16.png` |
| mmres_mun | RT_ICON | 89 (group 3020) | 256x256 | 32 | `icons/icon_3020_0_256x256.png` |
| mmres_mun | RT_ICON | 9 (group 3010) | 256x256 | 32 | `icons/icon_3010_0_256x256.png` |
| mmres_mun | RT_ICON | 90 (group 3020) | 64x64 | 32 | `icons/icon_3020_1_64x64.png` |
| mmres_mun | RT_ICON | 91 (group 3020) | 48x48 | 32 | `icons/icon_3020_2_48x48.png` |
| mmres_mun | RT_ICON | 92 (group 3020) | 40x40 | 32 | `icons/icon_3020_3_40x40.png` |
| mmres_mun | RT_ICON | 93 (group 3020) | 32x32 | 32 | `icons/icon_3020_4_32x32.png` |
| mmres_mun | RT_ICON | 94 (group 3020) | 24x24 | 32 | `icons/icon_3020_5_24x24.png` |
| mmres_mun | RT_ICON | 95 (group 3020) | 20x20 | 32 | `icons/icon_3020_6_20x20.png` |
| mmres_mun | RT_ICON | 96 (group 3020) | 16x16 | 32 | `icons/icon_3020_7_16x16.png` |
| mmres_mun | RT_ICON | 97 (group 3021) | 256x256 | 32 | `icons/icon_3021_0_256x256.png` |
| mmres_mun | RT_ICON | 98 (group 3021) | 64x64 | 32 | `icons/icon_3021_1_64x64.png` |
| mmres_mun | RT_ICON | 99 (group 3021) | 48x48 | 32 | `icons/icon_3021_2_48x48.png` |
| main_cpl_mun | RT_BITMAP | 100 | 83x80 | 24 | `bitmaps/bitmap_100.png` |
| main_cpl_mun | RT_GROUP_ICON | 100 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_100.ico` |
| main_cpl_mun | RT_GROUP_ICON | 104 | 32x32+32x32+32x32 | 4,8,32 | `icons/groupicon_104.ico` |
| main_cpl_mun | RT_GROUP_ICON | 105 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_105.ico` |
| main_cpl_mun | RT_GROUP_ICON | 106 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_106.ico` |
| main_cpl_mun | RT_GROUP_ICON | 108 | 32x32+32x32+32x32 | 4,8,32 | `icons/groupicon_108.ico` |
| main_cpl_mun | RT_GROUP_ICON | 200 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_200.ico` |
| main_cpl_mun | RT_GROUP_ICON | 400 | 48x48+32x32+16x16+48x48+32x32+16x16+256x256+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32,32 | `icons/groupicon_400.ico` |
| main_cpl_mun | RT_GROUP_ICON | 500 | 48x48+32x32+24x24+16x16+48x48+32x32+24x24+16x16+48x48+32x32+24x24+16x16 | 4,4,4,4,8,8,8,8,32,32,32,32 | `icons/groupicon_500.ico` |
| main_cpl_mun | RT_GROUP_ICON | 600 | 32x32+32x32+32x32 | 4,8,32 | `icons/groupicon_600.ico` |
| main_cpl_mun | RT_GROUP_ICON | 601 | 32x32+32x32+32x32 | 4,8,32 | `icons/groupicon_601.ico` |
| main_cpl_mun | RT_GROUP_ICON | 602 | 32x32+32x32+32x32 | 4,8,32 | `icons/groupicon_602.ico` |
| main_cpl_mun | RT_GROUP_ICON | 603 | 64x64+48x48+40x40+32x32+64x64+48x48+40x40+32x32+64x64+48x48+40x40+32x32 | 4,4,4,4,8,8,8,8,32,32,32,32 | `icons/groupicon_603.ico` |
| main_cpl_mun | RT_GROUP_ICON | 605 | 32x32+32x32+32x32 | 4,8,32 | `icons/groupicon_605.ico` |
| main_cpl_mun | RT_GROUP_ICON | 606 | 32x32+32x32+32x32 | 4,8,32 | `icons/groupicon_606.ico` |
| main_cpl_mun | RT_GROUP_ICON | 607 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_607.ico` |
| main_cpl_mun | RT_GROUP_ICON | 608 | 64x64+48x48+40x40+32x32+64x64+48x48+40x40+32x32+64x64+48x48+40x40+32x32 | 4,4,4,4,8,8,8,8,32,32,32,32 | `icons/groupicon_608.ico` |
| main_cpl_mun | RT_ICON | 1 (group 100) | 256x256 | 32 | `icons/icon_100_0_256x256.png` |
| main_cpl_mun | RT_ICON | 10 (group 200) | 64x64 | 32 | `icons/icon_200_1_64x64.png` |
| main_cpl_mun | RT_ICON | 100 (group 106) | 256x256 | 32 | `icons/icon_106_0_256x256.png` |
| main_cpl_mun | RT_ICON | 101 (group 106) | 64x64 | 32 | `icons/icon_106_1_64x64.png` |
| main_cpl_mun | RT_ICON | 102 (group 106) | 48x48 | 32 | `icons/icon_106_2_48x48.png` |
| main_cpl_mun | RT_ICON | 103 (group 106) | 40x40 | 32 | `icons/icon_106_3_40x40.png` |
| main_cpl_mun | RT_ICON | 104 (group 106) | 32x32 | 32 | `icons/icon_106_4_32x32.png` |
| main_cpl_mun | RT_ICON | 105 (group 106) | 24x24 | 32 | `icons/icon_106_5_24x24.png` |
| main_cpl_mun | RT_ICON | 106 (group 106) | 20x20 | 32 | `icons/icon_106_6_20x20.png` |
| main_cpl_mun | RT_ICON | 107 (group 106) | 16x16 | 32 | `icons/icon_106_7_16x16.png` |
| main_cpl_mun | RT_ICON | 11 (group 200) | 48x48 | 32 | `icons/icon_200_2_48x48.png` |
| main_cpl_mun | RT_ICON | 12 (group 200) | 40x40 | 32 | `icons/icon_200_3_40x40.png` |
| main_cpl_mun | RT_ICON | 13 (group 200) | 32x32 | 32 | `icons/icon_200_4_32x32.png` |
| main_cpl_mun | RT_ICON | 14 (group 200) | 24x24 | 32 | `icons/icon_200_5_24x24.png` |
| main_cpl_mun | RT_ICON | 15 (group 200) | 20x20 | 32 | `icons/icon_200_6_20x20.png` |
| main_cpl_mun | RT_ICON | 16 (group 200) | 16x16 | 32 | `icons/icon_200_7_16x16.png` |
| main_cpl_mun | RT_ICON | 17 (group 400) | 48x48 | 4 | `icons/icon_400_0_48x48.png` |
| main_cpl_mun | RT_ICON | 18 (group 400) | 32x32 | 4 | `icons/icon_400_1_32x32.png` |
| main_cpl_mun | RT_ICON | 19 (group 400) | 16x16 | 4 | `icons/icon_400_2_16x16.png` |
| main_cpl_mun | RT_ICON | 2 (group 100) | 64x64 | 32 | `icons/icon_100_1_64x64.png` |
| main_cpl_mun | RT_ICON | 20 (group 400) | 48x48 | 8 | `icons/icon_400_3_48x48.png` |
| main_cpl_mun | RT_ICON | 21 (group 400) | 32x32 | 8 | `icons/icon_400_4_32x32.png` |
| main_cpl_mun | RT_ICON | 22 (group 400) | 16x16 | 8 | `icons/icon_400_5_16x16.png` |
| main_cpl_mun | RT_ICON | 23 (group 400) | 256x256 | 32 | `icons/icon_400_6_256x256.png` |
| main_cpl_mun | RT_ICON | 24 (group 400) | 48x48 | 32 | `icons/icon_400_7_48x48.png` |
| main_cpl_mun | RT_ICON | 25 (group 400) | 32x32 | 32 | `icons/icon_400_8_32x32.png` |
| main_cpl_mun | RT_ICON | 26 (group 400) | 16x16 | 32 | `icons/icon_400_9_16x16.png` |
| main_cpl_mun | RT_ICON | 27 (group 500) | 48x48 | 4 | `icons/icon_500_0_48x48.png` |
| main_cpl_mun | RT_ICON | 28 (group 500) | 32x32 | 4 | `icons/icon_500_1_32x32.png` |
| main_cpl_mun | RT_ICON | 29 (group 500) | 24x24 | 4 | `icons/icon_500_2_24x24.png` |
| main_cpl_mun | RT_ICON | 3 (group 100) | 48x48 | 32 | `icons/icon_100_2_48x48.png` |
| main_cpl_mun | RT_ICON | 30 (group 500) | 16x16 | 4 | `icons/icon_500_3_16x16.png` |
| main_cpl_mun | RT_ICON | 31 (group 500) | 48x48 | 8 | `icons/icon_500_4_48x48.png` |
| main_cpl_mun | RT_ICON | 32 (group 500) | 32x32 | 8 | `icons/icon_500_5_32x32.png` |
| main_cpl_mun | RT_ICON | 33 (group 500) | 24x24 | 8 | `icons/icon_500_6_24x24.png` |
| main_cpl_mun | RT_ICON | 34 (group 500) | 16x16 | 8 | `icons/icon_500_7_16x16.png` |
| main_cpl_mun | RT_ICON | 35 (group 500) | 48x48 | 32 | `icons/icon_500_8_48x48.png` |
| main_cpl_mun | RT_ICON | 36 (group 500) | 32x32 | 32 | `icons/icon_500_9_32x32.png` |
| main_cpl_mun | RT_ICON | 37 (group 500) | 24x24 | 32 | `icons/icon_500_10_24x24.png` |
| main_cpl_mun | RT_ICON | 38 (group 500) | 16x16 | 32 | `icons/icon_500_11_16x16.png` |
| main_cpl_mun | RT_ICON | 39 (group 600) | 32x32 | 4 | `icons/icon_600_0_32x32.png` |
| main_cpl_mun | RT_ICON | 4 (group 100) | 40x40 | 32 | `icons/icon_100_3_40x40.png` |
| main_cpl_mun | RT_ICON | 40 (group 600) | 32x32 | 8 | `icons/icon_600_1_32x32.png` |
| main_cpl_mun | RT_ICON | 41 (group 600) | 32x32 | 32 | `icons/icon_600_2_32x32.png` |
| main_cpl_mun | RT_ICON | 42 (group 108) | 32x32 | 4 | `icons/icon_108_0_32x32.png` |
| main_cpl_mun | RT_ICON | 43 (group 108) | 32x32 | 8 | `icons/icon_108_1_32x32.png` |
| main_cpl_mun | RT_ICON | 44 (group 108) | 32x32 | 32 | `icons/icon_108_2_32x32.png` |
| main_cpl_mun | RT_ICON | 45 (group 104) | 32x32 | 4 | `icons/icon_104_0_32x32.png` |
| main_cpl_mun | RT_ICON | 46 (group 104) | 32x32 | 8 | `icons/icon_104_1_32x32.png` |
| main_cpl_mun | RT_ICON | 47 (group 104) | 32x32 | 32 | `icons/icon_104_2_32x32.png` |
| main_cpl_mun | RT_ICON | 48 (group 601) | 32x32 | 4 | `icons/icon_601_0_32x32.png` |
| main_cpl_mun | RT_ICON | 49 (group 601) | 32x32 | 8 | `icons/icon_601_1_32x32.png` |
| main_cpl_mun | RT_ICON | 5 (group 100) | 32x32 | 32 | `icons/icon_100_4_32x32.png` |
| main_cpl_mun | RT_ICON | 50 (group 601) | 32x32 | 32 | `icons/icon_601_2_32x32.png` |
| main_cpl_mun | RT_ICON | 51 (group 602) | 32x32 | 4 | `icons/icon_602_0_32x32.png` |
| main_cpl_mun | RT_ICON | 52 (group 602) | 32x32 | 8 | `icons/icon_602_1_32x32.png` |
| main_cpl_mun | RT_ICON | 53 (group 602) | 32x32 | 32 | `icons/icon_602_2_32x32.png` |
| main_cpl_mun | RT_ICON | 54 (group 603) | 64x64 | 4 | `icons/icon_603_0_64x64.png` |
| main_cpl_mun | RT_ICON | 55 (group 603) | 48x48 | 4 | `icons/icon_603_1_48x48.png` |
| main_cpl_mun | RT_ICON | 56 (group 603) | 40x40 | 4 | `icons/icon_603_2_40x40.png` |
| main_cpl_mun | RT_ICON | 57 (group 603) | 32x32 | 4 | `icons/icon_603_3_32x32.png` |
| main_cpl_mun | RT_ICON | 58 (group 603) | 64x64 | 8 | `icons/icon_603_4_64x64.png` |
| main_cpl_mun | RT_ICON | 59 (group 603) | 48x48 | 8 | `icons/icon_603_5_48x48.png` |
| main_cpl_mun | RT_ICON | 6 (group 100) | 24x24 | 32 | `icons/icon_100_5_24x24.png` |
| main_cpl_mun | RT_ICON | 60 (group 603) | 40x40 | 8 | `icons/icon_603_6_40x40.png` |
| main_cpl_mun | RT_ICON | 61 (group 603) | 32x32 | 8 | `icons/icon_603_7_32x32.png` |
| main_cpl_mun | RT_ICON | 62 (group 603) | 64x64 | 32 | `icons/icon_603_8_64x64.png` |
| main_cpl_mun | RT_ICON | 63 (group 603) | 48x48 | 32 | `icons/icon_603_9_48x48.png` |
| main_cpl_mun | RT_ICON | 64 (group 603) | 40x40 | 32 | `icons/icon_603_10_40x40.png` |
| main_cpl_mun | RT_ICON | 65 (group 603) | 32x32 | 32 | `icons/icon_603_11_32x32.png` |
| main_cpl_mun | RT_ICON | 66 (group 608) | 64x64 | 4 | `icons/icon_608_0_64x64.png` |
| main_cpl_mun | RT_ICON | 67 (group 608) | 48x48 | 4 | `icons/icon_608_1_48x48.png` |
| main_cpl_mun | RT_ICON | 68 (group 608) | 40x40 | 4 | `icons/icon_608_2_40x40.png` |
| main_cpl_mun | RT_ICON | 69 (group 608) | 32x32 | 4 | `icons/icon_608_3_32x32.png` |
| main_cpl_mun | RT_ICON | 7 (group 100) | 20x20 | 32 | `icons/icon_100_6_20x20.png` |
| main_cpl_mun | RT_ICON | 70 (group 608) | 64x64 | 8 | `icons/icon_608_4_64x64.png` |
| main_cpl_mun | RT_ICON | 71 (group 608) | 48x48 | 8 | `icons/icon_608_5_48x48.png` |
| main_cpl_mun | RT_ICON | 72 (group 608) | 40x40 | 8 | `icons/icon_608_6_40x40.png` |
| main_cpl_mun | RT_ICON | 73 (group 608) | 32x32 | 8 | `icons/icon_608_7_32x32.png` |
| main_cpl_mun | RT_ICON | 74 (group 608) | 64x64 | 32 | `icons/icon_608_8_64x64.png` |
| main_cpl_mun | RT_ICON | 75 (group 608) | 48x48 | 32 | `icons/icon_608_9_48x48.png` |
| main_cpl_mun | RT_ICON | 76 (group 608) | 40x40 | 32 | `icons/icon_608_10_40x40.png` |
| main_cpl_mun | RT_ICON | 77 (group 608) | 32x32 | 32 | `icons/icon_608_11_32x32.png` |
| main_cpl_mun | RT_ICON | 78 (group 605) | 32x32 | 4 | `icons/icon_605_0_32x32.png` |
| main_cpl_mun | RT_ICON | 79 (group 605) | 32x32 | 8 | `icons/icon_605_1_32x32.png` |
| main_cpl_mun | RT_ICON | 8 (group 100) | 16x16 | 32 | `icons/icon_100_7_16x16.png` |
| main_cpl_mun | RT_ICON | 80 (group 605) | 32x32 | 32 | `icons/icon_605_2_32x32.png` |
| main_cpl_mun | RT_ICON | 81 (group 606) | 32x32 | 4 | `icons/icon_606_0_32x32.png` |
| main_cpl_mun | RT_ICON | 82 (group 606) | 32x32 | 8 | `icons/icon_606_1_32x32.png` |
| main_cpl_mun | RT_ICON | 83 (group 606) | 32x32 | 32 | `icons/icon_606_2_32x32.png` |
| main_cpl_mun | RT_ICON | 84 (group 607) | 256x256 | 32 | `icons/icon_607_0_256x256.png` |
| main_cpl_mun | RT_ICON | 85 (group 607) | 64x64 | 32 | `icons/icon_607_1_64x64.png` |
| main_cpl_mun | RT_ICON | 86 (group 607) | 48x48 | 32 | `icons/icon_607_2_48x48.png` |
| main_cpl_mun | RT_ICON | 87 (group 607) | 40x40 | 32 | `icons/icon_607_3_40x40.png` |
| main_cpl_mun | RT_ICON | 88 (group 607) | 32x32 | 32 | `icons/icon_607_4_32x32.png` |
| main_cpl_mun | RT_ICON | 89 (group 607) | 24x24 | 32 | `icons/icon_607_5_24x24.png` |
| main_cpl_mun | RT_ICON | 9 (group 200) | 256x256 | 32 | `icons/icon_200_0_256x256.png` |
| main_cpl_mun | RT_ICON | 90 (group 607) | 20x20 | 32 | `icons/icon_607_6_20x20.png` |
| main_cpl_mun | RT_ICON | 91 (group 607) | 16x16 | 32 | `icons/icon_607_7_16x16.png` |
| main_cpl_mun | RT_ICON | 92 (group 105) | 256x256 | 32 | `icons/icon_105_0_256x256.png` |
| main_cpl_mun | RT_ICON | 93 (group 105) | 64x64 | 32 | `icons/icon_105_1_64x64.png` |
| main_cpl_mun | RT_ICON | 94 (group 105) | 48x48 | 32 | `icons/icon_105_2_48x48.png` |
| main_cpl_mun | RT_ICON | 95 (group 105) | 40x40 | 32 | `icons/icon_105_3_40x40.png` |
| main_cpl_mun | RT_ICON | 96 (group 105) | 32x32 | 32 | `icons/icon_105_4_32x32.png` |
| main_cpl_mun | RT_ICON | 97 (group 105) | 24x24 | 32 | `icons/icon_105_5_24x24.png` |
| main_cpl_mun | RT_ICON | 98 (group 105) | 20x20 | 32 | `icons/icon_105_6_20x20.png` |
| main_cpl_mun | RT_ICON | 99 (group 105) | 16x16 | 32 | `icons/icon_105_7_16x16.png` |
| intl_cpl_mun | RT_GROUP_ICON | 200 | 64x64+48x48+40x40+32x32+24x24+20x20+16x16+64x64+48x48+40x40+32x32+24x24+20x20+16x16+256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 4,4,4,4,4,4,4,8,8,8,8,8,8,8,32,32,32,32,32,32,32,32 | `icons/groupicon_200.ico` |
| intl_cpl_mun | RT_ICON | 1 (group 200) | 64x64 | 4 | `icons/icon_200_0_64x64.png` |
| intl_cpl_mun | RT_ICON | 10 (group 200) | 40x40 | 8 | `icons/icon_200_9_40x40.png` |
| intl_cpl_mun | RT_ICON | 11 (group 200) | 32x32 | 8 | `icons/icon_200_10_32x32.png` |
| intl_cpl_mun | RT_ICON | 12 (group 200) | 24x24 | 8 | `icons/icon_200_11_24x24.png` |
| intl_cpl_mun | RT_ICON | 13 (group 200) | 20x20 | 8 | `icons/icon_200_12_20x20.png` |
| intl_cpl_mun | RT_ICON | 14 (group 200) | 16x16 | 8 | `icons/icon_200_13_16x16.png` |
| intl_cpl_mun | RT_ICON | 15 (group 200) | 256x256 | 32 | `icons/icon_200_14_256x256.png` |
| intl_cpl_mun | RT_ICON | 16 (group 200) | 64x64 | 32 | `icons/icon_200_15_64x64.png` |
| intl_cpl_mun | RT_ICON | 17 (group 200) | 48x48 | 32 | `icons/icon_200_16_48x48.png` |
| intl_cpl_mun | RT_ICON | 18 (group 200) | 40x40 | 32 | `icons/icon_200_17_40x40.png` |
| intl_cpl_mun | RT_ICON | 19 (group 200) | 32x32 | 32 | `icons/icon_200_18_32x32.png` |
| intl_cpl_mun | RT_ICON | 2 (group 200) | 48x48 | 4 | `icons/icon_200_1_48x48.png` |
| intl_cpl_mun | RT_ICON | 20 (group 200) | 24x24 | 32 | `icons/icon_200_19_24x24.png` |
| intl_cpl_mun | RT_ICON | 21 (group 200) | 20x20 | 32 | `icons/icon_200_20_20x20.png` |
| intl_cpl_mun | RT_ICON | 22 (group 200) | 16x16 | 32 | `icons/icon_200_21_16x16.png` |
| intl_cpl_mun | RT_ICON | 3 (group 200) | 40x40 | 4 | `icons/icon_200_2_40x40.png` |
| intl_cpl_mun | RT_ICON | 4 (group 200) | 32x32 | 4 | `icons/icon_200_3_32x32.png` |
| intl_cpl_mun | RT_ICON | 5 (group 200) | 24x24 | 4 | `icons/icon_200_4_24x24.png` |
| intl_cpl_mun | RT_ICON | 6 (group 200) | 20x20 | 4 | `icons/icon_200_5_20x20.png` |
| intl_cpl_mun | RT_ICON | 7 (group 200) | 16x16 | 4 | `icons/icon_200_6_16x16.png` |
| intl_cpl_mun | RT_ICON | 8 (group 200) | 64x64 | 8 | `icons/icon_200_7_64x64.png` |
| intl_cpl_mun | RT_ICON | 9 (group 200) | 48x48 | 8 | `icons/icon_200_8_48x48.png` |
| sysdm_cpl_mun | RT_GROUP_ICON | 1 | 48x48+32x32+16x16+48x48+32x32+16x16+256x256+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32,32 | `icons/groupicon_1.ico` |
| sysdm_cpl_mun | RT_GROUP_ICON | 10 | 48x48+32x32+16x16+48x48+32x32+16x16 | 8,8,8,32,32,32 | `icons/groupicon_10.ico` |
| sysdm_cpl_mun | RT_GROUP_ICON | 2 | 48x48+32x32+24x24+16x16+48x48+32x32+24x24+16x16+256x256+48x48+32x32+24x24+16x16 | 4,4,4,4,8,8,8,8,32,32,32,32,32 | `icons/groupicon_2.ico` |
| sysdm_cpl_mun | RT_ICON | 1 (group 1) | 48x48 | 4 | `icons/icon_1_0_48x48.png` |
| sysdm_cpl_mun | RT_ICON | 10 (group 1) | 16x16 | 32 | `icons/icon_1_9_16x16.png` |
| sysdm_cpl_mun | RT_ICON | 11 (group 2) | 48x48 | 4 | `icons/icon_2_0_48x48.png` |
| sysdm_cpl_mun | RT_ICON | 12 (group 2) | 32x32 | 4 | `icons/icon_2_1_32x32.png` |
| sysdm_cpl_mun | RT_ICON | 13 (group 2) | 24x24 | 4 | `icons/icon_2_2_24x24.png` |
| sysdm_cpl_mun | RT_ICON | 14 (group 2) | 16x16 | 4 | `icons/icon_2_3_16x16.png` |
| sysdm_cpl_mun | RT_ICON | 15 (group 2) | 48x48 | 8 | `icons/icon_2_4_48x48.png` |
| sysdm_cpl_mun | RT_ICON | 16 (group 2) | 32x32 | 8 | `icons/icon_2_5_32x32.png` |
| sysdm_cpl_mun | RT_ICON | 17 (group 2) | 24x24 | 8 | `icons/icon_2_6_24x24.png` |
| sysdm_cpl_mun | RT_ICON | 18 (group 2) | 16x16 | 8 | `icons/icon_2_7_16x16.png` |
| sysdm_cpl_mun | RT_ICON | 19 (group 2) | 256x256 | 32 | `icons/icon_2_8_256x256.png` |
| sysdm_cpl_mun | RT_ICON | 2 (group 1) | 32x32 | 4 | `icons/icon_1_1_32x32.png` |
| sysdm_cpl_mun | RT_ICON | 20 (group 2) | 48x48 | 32 | `icons/icon_2_9_48x48.png` |
| sysdm_cpl_mun | RT_ICON | 21 (group 2) | 32x32 | 32 | `icons/icon_2_10_32x32.png` |
| sysdm_cpl_mun | RT_ICON | 22 (group 2) | 24x24 | 32 | `icons/icon_2_11_24x24.png` |
| sysdm_cpl_mun | RT_ICON | 23 (group 2) | 16x16 | 32 | `icons/icon_2_12_16x16.png` |
| sysdm_cpl_mun | RT_ICON | 24 (group 10) | 48x48 | 8 | `icons/icon_10_0_48x48.png` |
| sysdm_cpl_mun | RT_ICON | 25 (group 10) | 32x32 | 8 | `icons/icon_10_1_32x32.png` |
| sysdm_cpl_mun | RT_ICON | 26 (group 10) | 16x16 | 8 | `icons/icon_10_2_16x16.png` |
| sysdm_cpl_mun | RT_ICON | 27 (group 10) | 48x48 | 32 | `icons/icon_10_3_48x48.png` |
| sysdm_cpl_mun | RT_ICON | 28 (group 10) | 32x32 | 32 | `icons/icon_10_4_32x32.png` |
| sysdm_cpl_mun | RT_ICON | 29 (group 10) | 16x16 | 32 | `icons/icon_10_5_16x16.png` |
| sysdm_cpl_mun | RT_ICON | 3 (group 1) | 16x16 | 4 | `icons/icon_1_2_16x16.png` |
| sysdm_cpl_mun | RT_ICON | 4 (group 1) | 48x48 | 8 | `icons/icon_1_3_48x48.png` |
| sysdm_cpl_mun | RT_ICON | 5 (group 1) | 32x32 | 8 | `icons/icon_1_4_32x32.png` |
| sysdm_cpl_mun | RT_ICON | 6 (group 1) | 16x16 | 8 | `icons/icon_1_5_16x16.png` |
| sysdm_cpl_mun | RT_ICON | 7 (group 1) | 256x256 | 32 | `icons/icon_1_6_256x256.png` |
| sysdm_cpl_mun | RT_ICON | 8 (group 1) | 48x48 | 32 | `icons/icon_1_7_48x48.png` |
| sysdm_cpl_mun | RT_ICON | 9 (group 1) | 32x32 | 32 | `icons/icon_1_8_32x32.png` |
| timedate_cpl_mun | RT_GROUP_ICON | 50 | 48x48+32x32+16x16+48x48+32x32+16x16+256x256+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32,32 | `icons/groupicon_50.ico` |
| timedate_cpl_mun | RT_ICON | 1 (group 50) | 48x48 | 4 | `icons/icon_50_0_48x48.png` |
| timedate_cpl_mun | RT_ICON | 10 (group 50) | 16x16 | 32 | `icons/icon_50_9_16x16.png` |
| timedate_cpl_mun | RT_ICON | 2 (group 50) | 32x32 | 4 | `icons/icon_50_1_32x32.png` |
| timedate_cpl_mun | RT_ICON | 3 (group 50) | 16x16 | 4 | `icons/icon_50_2_16x16.png` |
| timedate_cpl_mun | RT_ICON | 4 (group 50) | 48x48 | 8 | `icons/icon_50_3_48x48.png` |
| timedate_cpl_mun | RT_ICON | 5 (group 50) | 32x32 | 8 | `icons/icon_50_4_32x32.png` |
| timedate_cpl_mun | RT_ICON | 6 (group 50) | 16x16 | 8 | `icons/icon_50_5_16x16.png` |
| timedate_cpl_mun | RT_ICON | 7 (group 50) | 256x256 | 32 | `icons/icon_50_6_256x256.png` |
| timedate_cpl_mun | RT_ICON | 8 (group 50) | 48x48 | 32 | `icons/icon_50_7_48x48.png` |
| timedate_cpl_mun | RT_ICON | 9 (group 50) | 32x32 | 32 | `icons/icon_50_8_32x32.png` |
| themeui_mun | RT_GROUP_ICON | 701 | 48x48+32x32+24x24+16x16+48x48+32x32+24x24+16x16+256x256+48x48+32x32+24x24+16x16 | 4,4,4,4,8,8,8,8,32,32,32,32,32 | `icons/groupicon_701.ico` |
| themeui_mun | RT_GROUP_ICON | 704 | 48x48+32x32+24x24+16x16+48x48+32x32+24x24+16x16+256x256+48x48+32x32+24x24+16x16 | 4,4,4,4,8,8,8,8,32,32,32,32,32 | `icons/groupicon_704.ico` |
| themeui_mun | RT_ICON | 1 (group 701) | 48x48 | 4 | `icons/icon_701_0_48x48.png` |
| themeui_mun | RT_ICON | 10 (group 701) | 48x48 | 32 | `icons/icon_701_9_48x48.png` |
| themeui_mun | RT_ICON | 11 (group 701) | 32x32 | 32 | `icons/icon_701_10_32x32.png` |
| themeui_mun | RT_ICON | 12 (group 701) | 24x24 | 32 | `icons/icon_701_11_24x24.png` |
| themeui_mun | RT_ICON | 13 (group 701) | 16x16 | 32 | `icons/icon_701_12_16x16.png` |
| themeui_mun | RT_ICON | 14 (group 704) | 48x48 | 4 | `icons/icon_704_0_48x48.png` |
| themeui_mun | RT_ICON | 15 (group 704) | 32x32 | 4 | `icons/icon_704_1_32x32.png` |
| themeui_mun | RT_ICON | 16 (group 704) | 24x24 | 4 | `icons/icon_704_2_24x24.png` |
| themeui_mun | RT_ICON | 17 (group 704) | 16x16 | 4 | `icons/icon_704_3_16x16.png` |
| themeui_mun | RT_ICON | 18 (group 704) | 48x48 | 8 | `icons/icon_704_4_48x48.png` |
| themeui_mun | RT_ICON | 19 (group 704) | 32x32 | 8 | `icons/icon_704_5_32x32.png` |
| themeui_mun | RT_ICON | 2 (group 701) | 32x32 | 4 | `icons/icon_701_1_32x32.png` |
| themeui_mun | RT_ICON | 20 (group 704) | 24x24 | 8 | `icons/icon_704_6_24x24.png` |
| themeui_mun | RT_ICON | 21 (group 704) | 16x16 | 8 | `icons/icon_704_7_16x16.png` |
| themeui_mun | RT_ICON | 22 (group 704) | 256x256 | 32 | `icons/icon_704_8_256x256.png` |
| themeui_mun | RT_ICON | 23 (group 704) | 48x48 | 32 | `icons/icon_704_9_48x48.png` |
| themeui_mun | RT_ICON | 24 (group 704) | 32x32 | 32 | `icons/icon_704_10_32x32.png` |
| themeui_mun | RT_ICON | 25 (group 704) | 24x24 | 32 | `icons/icon_704_11_24x24.png` |
| themeui_mun | RT_ICON | 26 (group 704) | 16x16 | 32 | `icons/icon_704_12_16x16.png` |
| themeui_mun | RT_ICON | 3 (group 701) | 24x24 | 4 | `icons/icon_701_2_24x24.png` |
| themeui_mun | RT_ICON | 4 (group 701) | 16x16 | 4 | `icons/icon_701_3_16x16.png` |
| themeui_mun | RT_ICON | 5 (group 701) | 48x48 | 8 | `icons/icon_701_4_48x48.png` |
| themeui_mun | RT_ICON | 6 (group 701) | 32x32 | 8 | `icons/icon_701_5_32x32.png` |
| themeui_mun | RT_ICON | 7 (group 701) | 24x24 | 8 | `icons/icon_701_6_24x24.png` |
| themeui_mun | RT_ICON | 8 (group 701) | 16x16 | 8 | `icons/icon_701_7_16x16.png` |
| themeui_mun | RT_ICON | 9 (group 701) | 256x256 | 32 | `icons/icon_701_8_256x256.png` |
| accessibilitycpl_mun | RT_BITMAP | 309 | 21x19 | 24 | `bitmaps/bitmap_309.png` |
| accessibilitycpl_mun | RT_GROUP_ICON | 1 | 96x96+64x64+48x48+32x32+24x24+16x16+96x96+64x64+48x48+32x32+24x24+16x16+256x256+96x96+64x64+48x48+32x32+24x24+16x16 | 4,4,4,4,4,4,8,8,8,8,8,8,32,32,32,32,32,32,32 | `icons/groupicon_1.ico` |
| accessibilitycpl_mun | RT_GROUP_ICON | 320 | 96x96+72x72+64x64+60x60+48x48+40x40+32x32+24x24+16x16+96x96+72x72+64x64+60x60+48x48+40x40+32x32+24x24+16x16+256x256+96x96+72x72+64x64+60x60+48x48+40x40+32x32+24x24+16x16 | 4,4,4,4,4,4,4,4,4,8,8,8,8,8,8,8,8,8,32,32,32,32,32,32,32,32,32,32 | `icons/groupicon_320.ico` |
| accessibilitycpl_mun | RT_GROUP_ICON | 321 | 96x96+72x72+64x64+60x60+48x48+40x40+32x32+24x24+16x16+96x96+72x72+64x64+60x60+48x48+40x40+32x32+24x24+16x16+256x256+96x96+72x72+64x64+60x60+48x48+40x40+32x32+24x24+16x16 | 4,4,4,4,4,4,4,4,4,8,8,8,8,8,8,8,8,8,32,32,32,32,32,32,32,32,32,32 | `icons/groupicon_321.ico` |
| accessibilitycpl_mun | RT_GROUP_ICON | 322 | 96x96+72x72+64x64+60x60+48x48+40x40+32x32+24x24+16x16+96x96+72x72+64x64+60x60+48x48+40x40+32x32+24x24+16x16+256x256+96x96+72x72+64x64+60x60+48x48+40x40+32x32+24x24+16x16 | 4,4,4,4,4,4,4,4,4,8,8,8,8,8,8,8,8,8,32,32,32,32,32,32,32,32,32,32 | `icons/groupicon_322.ico` |
| accessibilitycpl_mun | RT_GROUP_ICON | 323 | 96x96+72x72+64x64+60x60+48x48+40x40+32x32+24x24+16x16+14x14+10x10+8x8+96x96+72x72+64x64+60x60+48x48+40x40+32x32+24x24+16x16+14x14+10x10+8x8+14x14+10x10+8x8+256x256+96x96+72x72+64x64+60x60+48x48+40x40+32x32+24x24+16x16 | 4,4,4,4,4,4,4,4,4,4,4,4,8,8,8,8,8,8,8,8,8,8,8,8,24,24,24,32,32,32,32,32,32,32,32,32,32 | `icons/groupicon_323.ico` |
| accessibilitycpl_mun | RT_GROUP_ICON | 324 | 96x96+72x72+64x64+60x60+48x48+40x40+32x32+24x24+16x16+14x14+10x10+8x8+96x96+72x72+64x64+60x60+48x48+40x40+32x32+24x24+16x16+256x256+96x96+72x72+64x64+60x60+48x48+40x40+32x32+24x24+16x16 | 4,4,4,4,4,4,4,4,4,4,4,4,8,8,8,8,8,8,8,8,8,32,32,32,32,32,32,32,32,32,32 | `icons/groupicon_324.ico` |
| accessibilitycpl_mun | RT_GROUP_ICON | 325 | 96x96+72x72+64x64+60x60+48x48+40x40+32x32+24x24+16x16+96x96+72x72+64x64+60x60+48x48+40x40+32x32+24x24+16x16+256x256+96x96+72x72+64x64+60x60+48x48+40x40+32x32+24x24+16x16 | 4,4,4,4,4,4,4,4,4,8,8,8,8,8,8,8,8,8,32,32,32,32,32,32,32,32,32,32 | `icons/groupicon_325.ico` |
| accessibilitycpl_mun | RT_GROUP_ICON | 326 | 96x96+72x72+64x64+60x60+48x48+40x40+32x32+24x24+16x16+96x96+72x72+64x64+60x60+48x48+40x40+32x32+24x24+16x16+256x256+96x96+72x72+64x64+60x60+48x48+40x40+32x32+24x24+16x16 | 4,4,4,4,4,4,4,4,4,8,8,8,8,8,8,8,8,8,32,32,32,32,32,32,32,32,32,32 | `icons/groupicon_326.ico` |
| accessibilitycpl_mun | RT_GROUP_ICON | 327 | 96x96+72x72+64x64+60x60+48x48+40x40+32x32+24x24+16x16+96x96+72x72+64x64+60x60+48x48+40x40+32x32+24x24+16x16+256x256+96x96+72x72+64x64+60x60+48x48+40x40+32x32+24x24+16x16 | 4,4,4,4,4,4,4,4,4,8,8,8,8,8,8,8,8,8,32,32,32,32,32,32,32,32,32,32 | `icons/groupicon_327.ico` |
| accessibilitycpl_mun | RT_GROUP_ICON | 328 | 96x96+72x72+64x64+60x60+48x48+40x40+32x32+24x24+16x16+96x96+72x72+64x64+60x60+48x48+40x40+32x32+24x24+16x16+256x256+96x96+72x72+64x64+60x60+48x48+40x40+32x32+24x24+16x16 | 4,4,4,4,4,4,4,4,4,8,8,8,8,8,8,8,8,8,32,32,32,32,32,32,32,32,32,32 | `icons/groupicon_328.ico` |
| accessibilitycpl_mun | RT_GROUP_ICON | 329 | 96x96+72x72+64x64+60x60+48x48+40x40+32x32+24x24+16x16+96x96+72x72+64x64+60x60+48x48+40x40+32x32+24x24+16x16+256x256+96x96+72x72+64x64+60x60+48x48+40x40+32x32+24x24+16x16 | 4,4,4,4,4,4,4,4,4,8,8,8,8,8,8,8,8,8,32,32,32,32,32,32,32,32,32,32 | `icons/groupicon_329.ico` |
| accessibilitycpl_mun | RT_GROUP_ICON | 338 | 96x96+72x72+64x64+60x60+48x48+40x40+32x32+24x24+16x16+96x96+72x72+64x64+60x60+48x48+40x40+32x32+24x24+16x16+256x256+96x96+72x72+64x64+60x60+48x48+40x40+32x32+24x24+16x16 | 4,4,4,4,4,4,4,4,4,8,8,8,8,8,8,8,8,8,32,32,32,32,32,32,32,32,32,32 | `icons/groupicon_338.ico` |
| accessibilitycpl_mun | RT_GROUP_ICON | 340 | 96x96+72x72+64x64+60x60+48x48+40x40+32x32+24x24+16x16+96x96+72x72+64x64+60x60+48x48+40x40+32x32+24x24+16x16+256x256+96x96+72x72+64x64+60x60+48x48+40x40+32x32+24x24+16x16 | 4,4,4,4,4,4,4,4,4,8,8,8,8,8,8,8,8,8,32,32,32,32,32,32,32,32,32,32 | `icons/groupicon_340.ico` |
| accessibilitycpl_mun | RT_GROUP_ICON | 401 | 32x32 | 4 | `icons/groupicon_401.ico` |
| accessibilitycpl_mun | RT_GROUP_ICON | 402 | 32x32 | 4 | `icons/groupicon_402.ico` |
| accessibilitycpl_mun | RT_GROUP_ICON | 403 | 32x32 | 4 | `icons/groupicon_403.ico` |
| accessibilitycpl_mun | RT_GROUP_ICON | 404 | 32x32 | 4 | `icons/groupicon_404.ico` |
| accessibilitycpl_mun | RT_GROUP_ICON | 405 | 32x32 | 4 | `icons/groupicon_405.ico` |
| accessibilitycpl_mun | RT_GROUP_ICON | 406 | 32x32 | 4 | `icons/groupicon_406.ico` |
| accessibilitycpl_mun | RT_GROUP_ICON | 407 | 32x32 | 4 | `icons/groupicon_407.ico` |
| accessibilitycpl_mun | RT_GROUP_ICON | 408 | 32x32 | 4 | `icons/groupicon_408.ico` |
| accessibilitycpl_mun | RT_GROUP_ICON | 409 | 32x32 | 4 | `icons/groupicon_409.ico` |
| accessibilitycpl_mun | RT_ICON | 1 (group 1) | 96x96 | 4 | `icons/icon_1_0_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 10 (group 1) | 32x32 | 8 | `icons/icon_1_9_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 100 (group 322) | 32x32 | 8 | `icons/icon_322_15_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 101 (group 322) | 24x24 | 8 | `icons/icon_322_16_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 102 (group 322) | 16x16 | 8 | `icons/icon_322_17_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 103 (group 322) | 256x256 | 32 | `icons/icon_322_18_256x256.png` |
| accessibilitycpl_mun | RT_ICON | 104 (group 322) | 96x96 | 32 | `icons/icon_322_19_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 105 (group 322) | 72x72 | 32 | `icons/icon_322_20_72x72.png` |
| accessibilitycpl_mun | RT_ICON | 106 (group 322) | 64x64 | 32 | `icons/icon_322_21_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 107 (group 322) | 60x60 | 32 | `icons/icon_322_22_60x60.png` |
| accessibilitycpl_mun | RT_ICON | 108 (group 322) | 48x48 | 32 | `icons/icon_322_23_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 109 (group 322) | 40x40 | 32 | `icons/icon_322_24_40x40.png` |
| accessibilitycpl_mun | RT_ICON | 11 (group 1) | 24x24 | 8 | `icons/icon_1_10_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 110 (group 322) | 32x32 | 32 | `icons/icon_322_25_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 111 (group 322) | 24x24 | 32 | `icons/icon_322_26_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 112 (group 322) | 16x16 | 32 | `icons/icon_322_27_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 113 (group 323) | 96x96 | 4 | `icons/icon_323_0_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 114 (group 323) | 72x72 | 4 | `icons/icon_323_1_72x72.png` |
| accessibilitycpl_mun | RT_ICON | 115 (group 323) | 64x64 | 4 | `icons/icon_323_2_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 116 (group 323) | 60x60 | 4 | `icons/icon_323_3_60x60.png` |
| accessibilitycpl_mun | RT_ICON | 117 (group 323) | 48x48 | 4 | `icons/icon_323_4_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 118 (group 323) | 40x40 | 4 | `icons/icon_323_5_40x40.png` |
| accessibilitycpl_mun | RT_ICON | 119 (group 323) | 32x32 | 4 | `icons/icon_323_6_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 12 (group 1) | 16x16 | 8 | `icons/icon_1_11_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 120 (group 323) | 24x24 | 4 | `icons/icon_323_7_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 121 (group 323) | 16x16 | 4 | `icons/icon_323_8_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 122 (group 323) | 14x14 | 4 | `icons/icon_323_9_14x14.png` |
| accessibilitycpl_mun | RT_ICON | 123 (group 323) | 10x10 | 4 | `icons/icon_323_10_10x10.png` |
| accessibilitycpl_mun | RT_ICON | 124 (group 323) | 8x8 | 4 | `icons/icon_323_11_8x8.png` |
| accessibilitycpl_mun | RT_ICON | 125 (group 323) | 96x96 | 8 | `icons/icon_323_12_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 126 (group 323) | 72x72 | 8 | `icons/icon_323_13_72x72.png` |
| accessibilitycpl_mun | RT_ICON | 127 (group 323) | 64x64 | 8 | `icons/icon_323_14_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 128 (group 323) | 60x60 | 8 | `icons/icon_323_15_60x60.png` |
| accessibilitycpl_mun | RT_ICON | 129 (group 323) | 48x48 | 8 | `icons/icon_323_16_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 13 (group 1) | 256x256 | 32 | `icons/icon_1_12_256x256.png` |
| accessibilitycpl_mun | RT_ICON | 130 (group 323) | 40x40 | 8 | `icons/icon_323_17_40x40.png` |
| accessibilitycpl_mun | RT_ICON | 131 (group 323) | 32x32 | 8 | `icons/icon_323_18_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 132 (group 323) | 24x24 | 8 | `icons/icon_323_19_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 133 (group 323) | 16x16 | 8 | `icons/icon_323_20_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 134 (group 323) | 14x14 | 8 | `icons/icon_323_21_14x14.png` |
| accessibilitycpl_mun | RT_ICON | 135 (group 323) | 10x10 | 8 | `icons/icon_323_22_10x10.png` |
| accessibilitycpl_mun | RT_ICON | 136 (group 323) | 8x8 | 8 | `icons/icon_323_23_8x8.png` |
| accessibilitycpl_mun | RT_ICON | 137 (group 323) | 14x14 | 24 | `icons/icon_323_24_14x14.png` |
| accessibilitycpl_mun | RT_ICON | 138 (group 323) | 10x10 | 24 | `icons/icon_323_25_10x10.png` |
| accessibilitycpl_mun | RT_ICON | 139 (group 323) | 8x8 | 24 | `icons/icon_323_26_8x8.png` |
| accessibilitycpl_mun | RT_ICON | 14 (group 1) | 96x96 | 32 | `icons/icon_1_13_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 140 (group 323) | 256x256 | 32 | `icons/icon_323_27_256x256.png` |
| accessibilitycpl_mun | RT_ICON | 141 (group 323) | 96x96 | 32 | `icons/icon_323_28_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 142 (group 323) | 72x72 | 32 | `icons/icon_323_29_72x72.png` |
| accessibilitycpl_mun | RT_ICON | 143 (group 323) | 64x64 | 32 | `icons/icon_323_30_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 144 (group 323) | 60x60 | 32 | `icons/icon_323_31_60x60.png` |
| accessibilitycpl_mun | RT_ICON | 145 (group 323) | 48x48 | 32 | `icons/icon_323_32_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 146 (group 323) | 40x40 | 32 | `icons/icon_323_33_40x40.png` |
| accessibilitycpl_mun | RT_ICON | 147 (group 323) | 32x32 | 32 | `icons/icon_323_34_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 148 (group 323) | 24x24 | 32 | `icons/icon_323_35_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 149 (group 323) | 16x16 | 32 | `icons/icon_323_36_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 15 (group 1) | 64x64 | 32 | `icons/icon_1_14_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 150 (group 324) | 96x96 | 4 | `icons/icon_324_0_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 151 (group 324) | 72x72 | 4 | `icons/icon_324_1_72x72.png` |
| accessibilitycpl_mun | RT_ICON | 152 (group 324) | 64x64 | 4 | `icons/icon_324_2_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 153 (group 324) | 60x60 | 4 | `icons/icon_324_3_60x60.png` |
| accessibilitycpl_mun | RT_ICON | 154 (group 324) | 48x48 | 4 | `icons/icon_324_4_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 155 (group 324) | 40x40 | 4 | `icons/icon_324_5_40x40.png` |
| accessibilitycpl_mun | RT_ICON | 156 (group 324) | 32x32 | 4 | `icons/icon_324_6_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 157 (group 324) | 24x24 | 4 | `icons/icon_324_7_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 158 (group 324) | 16x16 | 4 | `icons/icon_324_8_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 159 (group 324) | 14x14 | 4 | `icons/icon_324_9_14x14.png` |
| accessibilitycpl_mun | RT_ICON | 16 (group 1) | 48x48 | 32 | `icons/icon_1_15_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 160 (group 324) | 10x10 | 4 | `icons/icon_324_10_10x10.png` |
| accessibilitycpl_mun | RT_ICON | 161 (group 324) | 8x8 | 4 | `icons/icon_324_11_8x8.png` |
| accessibilitycpl_mun | RT_ICON | 162 (group 324) | 96x96 | 8 | `icons/icon_324_12_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 163 (group 324) | 72x72 | 8 | `icons/icon_324_13_72x72.png` |
| accessibilitycpl_mun | RT_ICON | 164 (group 324) | 64x64 | 8 | `icons/icon_324_14_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 165 (group 324) | 60x60 | 8 | `icons/icon_324_15_60x60.png` |
| accessibilitycpl_mun | RT_ICON | 166 (group 324) | 48x48 | 8 | `icons/icon_324_16_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 167 (group 324) | 40x40 | 8 | `icons/icon_324_17_40x40.png` |
| accessibilitycpl_mun | RT_ICON | 168 (group 324) | 32x32 | 8 | `icons/icon_324_18_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 169 (group 324) | 24x24 | 8 | `icons/icon_324_19_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 17 (group 1) | 32x32 | 32 | `icons/icon_1_16_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 170 (group 324) | 16x16 | 8 | `icons/icon_324_20_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 171 (group 324) | 256x256 | 32 | `icons/icon_324_21_256x256.png` |
| accessibilitycpl_mun | RT_ICON | 172 (group 324) | 96x96 | 32 | `icons/icon_324_22_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 173 (group 324) | 72x72 | 32 | `icons/icon_324_23_72x72.png` |
| accessibilitycpl_mun | RT_ICON | 174 (group 324) | 64x64 | 32 | `icons/icon_324_24_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 175 (group 324) | 60x60 | 32 | `icons/icon_324_25_60x60.png` |
| accessibilitycpl_mun | RT_ICON | 176 (group 324) | 48x48 | 32 | `icons/icon_324_26_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 177 (group 324) | 40x40 | 32 | `icons/icon_324_27_40x40.png` |
| accessibilitycpl_mun | RT_ICON | 178 (group 324) | 32x32 | 32 | `icons/icon_324_28_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 179 (group 324) | 24x24 | 32 | `icons/icon_324_29_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 18 (group 1) | 24x24 | 32 | `icons/icon_1_17_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 180 (group 324) | 16x16 | 32 | `icons/icon_324_30_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 181 (group 325) | 96x96 | 4 | `icons/icon_325_0_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 182 (group 325) | 72x72 | 4 | `icons/icon_325_1_72x72.png` |
| accessibilitycpl_mun | RT_ICON | 183 (group 325) | 64x64 | 4 | `icons/icon_325_2_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 184 (group 325) | 60x60 | 4 | `icons/icon_325_3_60x60.png` |
| accessibilitycpl_mun | RT_ICON | 185 (group 325) | 48x48 | 4 | `icons/icon_325_4_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 186 (group 325) | 40x40 | 4 | `icons/icon_325_5_40x40.png` |
| accessibilitycpl_mun | RT_ICON | 187 (group 325) | 32x32 | 4 | `icons/icon_325_6_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 188 (group 325) | 24x24 | 4 | `icons/icon_325_7_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 189 (group 325) | 16x16 | 4 | `icons/icon_325_8_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 19 (group 1) | 16x16 | 32 | `icons/icon_1_18_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 190 (group 325) | 96x96 | 8 | `icons/icon_325_9_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 191 (group 325) | 72x72 | 8 | `icons/icon_325_10_72x72.png` |
| accessibilitycpl_mun | RT_ICON | 192 (group 325) | 64x64 | 8 | `icons/icon_325_11_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 193 (group 325) | 60x60 | 8 | `icons/icon_325_12_60x60.png` |
| accessibilitycpl_mun | RT_ICON | 194 (group 325) | 48x48 | 8 | `icons/icon_325_13_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 195 (group 325) | 40x40 | 8 | `icons/icon_325_14_40x40.png` |
| accessibilitycpl_mun | RT_ICON | 196 (group 325) | 32x32 | 8 | `icons/icon_325_15_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 197 (group 325) | 24x24 | 8 | `icons/icon_325_16_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 198 (group 325) | 16x16 | 8 | `icons/icon_325_17_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 199 (group 325) | 256x256 | 32 | `icons/icon_325_18_256x256.png` |
| accessibilitycpl_mun | RT_ICON | 2 (group 1) | 64x64 | 4 | `icons/icon_1_1_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 20 (group 401) | 32x32 | 4 | `icons/icon_401_0_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 200 (group 325) | 96x96 | 32 | `icons/icon_325_19_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 201 (group 325) | 72x72 | 32 | `icons/icon_325_20_72x72.png` |
| accessibilitycpl_mun | RT_ICON | 202 (group 325) | 64x64 | 32 | `icons/icon_325_21_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 203 (group 325) | 60x60 | 32 | `icons/icon_325_22_60x60.png` |
| accessibilitycpl_mun | RT_ICON | 204 (group 325) | 48x48 | 32 | `icons/icon_325_23_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 205 (group 325) | 40x40 | 32 | `icons/icon_325_24_40x40.png` |
| accessibilitycpl_mun | RT_ICON | 206 (group 325) | 32x32 | 32 | `icons/icon_325_25_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 207 (group 325) | 24x24 | 32 | `icons/icon_325_26_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 208 (group 325) | 16x16 | 32 | `icons/icon_325_27_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 209 (group 326) | 96x96 | 4 | `icons/icon_326_0_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 21 (group 402) | 32x32 | 4 | `icons/icon_402_0_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 210 (group 326) | 72x72 | 4 | `icons/icon_326_1_72x72.png` |
| accessibilitycpl_mun | RT_ICON | 211 (group 326) | 64x64 | 4 | `icons/icon_326_2_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 212 (group 326) | 60x60 | 4 | `icons/icon_326_3_60x60.png` |
| accessibilitycpl_mun | RT_ICON | 213 (group 326) | 48x48 | 4 | `icons/icon_326_4_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 214 (group 326) | 40x40 | 4 | `icons/icon_326_5_40x40.png` |
| accessibilitycpl_mun | RT_ICON | 215 (group 326) | 32x32 | 4 | `icons/icon_326_6_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 216 (group 326) | 24x24 | 4 | `icons/icon_326_7_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 217 (group 326) | 16x16 | 4 | `icons/icon_326_8_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 218 (group 326) | 96x96 | 8 | `icons/icon_326_9_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 219 (group 326) | 72x72 | 8 | `icons/icon_326_10_72x72.png` |
| accessibilitycpl_mun | RT_ICON | 22 (group 403) | 32x32 | 4 | `icons/icon_403_0_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 220 (group 326) | 64x64 | 8 | `icons/icon_326_11_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 221 (group 326) | 60x60 | 8 | `icons/icon_326_12_60x60.png` |
| accessibilitycpl_mun | RT_ICON | 222 (group 326) | 48x48 | 8 | `icons/icon_326_13_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 223 (group 326) | 40x40 | 8 | `icons/icon_326_14_40x40.png` |
| accessibilitycpl_mun | RT_ICON | 224 (group 326) | 32x32 | 8 | `icons/icon_326_15_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 225 (group 326) | 24x24 | 8 | `icons/icon_326_16_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 226 (group 326) | 16x16 | 8 | `icons/icon_326_17_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 227 (group 326) | 256x256 | 32 | `icons/icon_326_18_256x256.png` |
| accessibilitycpl_mun | RT_ICON | 228 (group 326) | 96x96 | 32 | `icons/icon_326_19_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 229 (group 326) | 72x72 | 32 | `icons/icon_326_20_72x72.png` |
| accessibilitycpl_mun | RT_ICON | 23 (group 404) | 32x32 | 4 | `icons/icon_404_0_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 230 (group 326) | 64x64 | 32 | `icons/icon_326_21_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 231 (group 326) | 60x60 | 32 | `icons/icon_326_22_60x60.png` |
| accessibilitycpl_mun | RT_ICON | 232 (group 326) | 48x48 | 32 | `icons/icon_326_23_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 233 (group 326) | 40x40 | 32 | `icons/icon_326_24_40x40.png` |
| accessibilitycpl_mun | RT_ICON | 234 (group 326) | 32x32 | 32 | `icons/icon_326_25_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 235 (group 326) | 24x24 | 32 | `icons/icon_326_26_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 236 (group 326) | 16x16 | 32 | `icons/icon_326_27_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 237 (group 328) | 96x96 | 4 | `icons/icon_328_0_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 238 (group 328) | 72x72 | 4 | `icons/icon_328_1_72x72.png` |
| accessibilitycpl_mun | RT_ICON | 239 (group 328) | 64x64 | 4 | `icons/icon_328_2_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 24 (group 405) | 32x32 | 4 | `icons/icon_405_0_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 240 (group 328) | 60x60 | 4 | `icons/icon_328_3_60x60.png` |
| accessibilitycpl_mun | RT_ICON | 241 (group 328) | 48x48 | 4 | `icons/icon_328_4_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 242 (group 328) | 40x40 | 4 | `icons/icon_328_5_40x40.png` |
| accessibilitycpl_mun | RT_ICON | 243 (group 328) | 32x32 | 4 | `icons/icon_328_6_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 244 (group 328) | 24x24 | 4 | `icons/icon_328_7_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 245 (group 328) | 16x16 | 4 | `icons/icon_328_8_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 246 (group 328) | 96x96 | 8 | `icons/icon_328_9_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 247 (group 328) | 72x72 | 8 | `icons/icon_328_10_72x72.png` |
| accessibilitycpl_mun | RT_ICON | 248 (group 328) | 64x64 | 8 | `icons/icon_328_11_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 249 (group 328) | 60x60 | 8 | `icons/icon_328_12_60x60.png` |
| accessibilitycpl_mun | RT_ICON | 25 (group 406) | 32x32 | 4 | `icons/icon_406_0_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 250 (group 328) | 48x48 | 8 | `icons/icon_328_13_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 251 (group 328) | 40x40 | 8 | `icons/icon_328_14_40x40.png` |
| accessibilitycpl_mun | RT_ICON | 252 (group 328) | 32x32 | 8 | `icons/icon_328_15_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 253 (group 328) | 24x24 | 8 | `icons/icon_328_16_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 254 (group 328) | 16x16 | 8 | `icons/icon_328_17_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 255 (group 328) | 256x256 | 32 | `icons/icon_328_18_256x256.png` |
| accessibilitycpl_mun | RT_ICON | 256 (group 328) | 96x96 | 32 | `icons/icon_328_19_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 257 (group 328) | 72x72 | 32 | `icons/icon_328_20_72x72.png` |
| accessibilitycpl_mun | RT_ICON | 258 (group 328) | 64x64 | 32 | `icons/icon_328_21_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 259 (group 328) | 60x60 | 32 | `icons/icon_328_22_60x60.png` |
| accessibilitycpl_mun | RT_ICON | 26 (group 407) | 32x32 | 4 | `icons/icon_407_0_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 260 (group 328) | 48x48 | 32 | `icons/icon_328_23_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 261 (group 328) | 40x40 | 32 | `icons/icon_328_24_40x40.png` |
| accessibilitycpl_mun | RT_ICON | 262 (group 328) | 32x32 | 32 | `icons/icon_328_25_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 263 (group 328) | 24x24 | 32 | `icons/icon_328_26_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 264 (group 328) | 16x16 | 32 | `icons/icon_328_27_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 265 (group 327) | 96x96 | 4 | `icons/icon_327_0_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 266 (group 327) | 72x72 | 4 | `icons/icon_327_1_72x72.png` |
| accessibilitycpl_mun | RT_ICON | 267 (group 327) | 64x64 | 4 | `icons/icon_327_2_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 268 (group 327) | 60x60 | 4 | `icons/icon_327_3_60x60.png` |
| accessibilitycpl_mun | RT_ICON | 269 (group 327) | 48x48 | 4 | `icons/icon_327_4_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 27 (group 408) | 32x32 | 4 | `icons/icon_408_0_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 270 (group 327) | 40x40 | 4 | `icons/icon_327_5_40x40.png` |
| accessibilitycpl_mun | RT_ICON | 271 (group 327) | 32x32 | 4 | `icons/icon_327_6_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 272 (group 327) | 24x24 | 4 | `icons/icon_327_7_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 273 (group 327) | 16x16 | 4 | `icons/icon_327_8_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 274 (group 327) | 96x96 | 8 | `icons/icon_327_9_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 275 (group 327) | 72x72 | 8 | `icons/icon_327_10_72x72.png` |
| accessibilitycpl_mun | RT_ICON | 276 (group 327) | 64x64 | 8 | `icons/icon_327_11_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 277 (group 327) | 60x60 | 8 | `icons/icon_327_12_60x60.png` |
| accessibilitycpl_mun | RT_ICON | 278 (group 327) | 48x48 | 8 | `icons/icon_327_13_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 279 (group 327) | 40x40 | 8 | `icons/icon_327_14_40x40.png` |
| accessibilitycpl_mun | RT_ICON | 28 (group 409) | 32x32 | 4 | `icons/icon_409_0_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 280 (group 327) | 32x32 | 8 | `icons/icon_327_15_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 281 (group 327) | 24x24 | 8 | `icons/icon_327_16_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 282 (group 327) | 16x16 | 8 | `icons/icon_327_17_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 283 (group 327) | 256x256 | 32 | `icons/icon_327_18_256x256.png` |
| accessibilitycpl_mun | RT_ICON | 284 (group 327) | 96x96 | 32 | `icons/icon_327_19_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 285 (group 327) | 72x72 | 32 | `icons/icon_327_20_72x72.png` |
| accessibilitycpl_mun | RT_ICON | 286 (group 327) | 64x64 | 32 | `icons/icon_327_21_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 287 (group 327) | 60x60 | 32 | `icons/icon_327_22_60x60.png` |
| accessibilitycpl_mun | RT_ICON | 288 (group 327) | 48x48 | 32 | `icons/icon_327_23_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 289 (group 327) | 40x40 | 32 | `icons/icon_327_24_40x40.png` |
| accessibilitycpl_mun | RT_ICON | 29 (group 320) | 96x96 | 4 | `icons/icon_320_0_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 290 (group 327) | 32x32 | 32 | `icons/icon_327_25_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 291 (group 327) | 24x24 | 32 | `icons/icon_327_26_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 292 (group 327) | 16x16 | 32 | `icons/icon_327_27_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 293 (group 329) | 96x96 | 4 | `icons/icon_329_0_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 294 (group 329) | 72x72 | 4 | `icons/icon_329_1_72x72.png` |
| accessibilitycpl_mun | RT_ICON | 295 (group 329) | 64x64 | 4 | `icons/icon_329_2_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 296 (group 329) | 60x60 | 4 | `icons/icon_329_3_60x60.png` |
| accessibilitycpl_mun | RT_ICON | 297 (group 329) | 48x48 | 4 | `icons/icon_329_4_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 298 (group 329) | 40x40 | 4 | `icons/icon_329_5_40x40.png` |
| accessibilitycpl_mun | RT_ICON | 299 (group 329) | 32x32 | 4 | `icons/icon_329_6_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 3 (group 1) | 48x48 | 4 | `icons/icon_1_2_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 30 (group 320) | 72x72 | 4 | `icons/icon_320_1_72x72.png` |
| accessibilitycpl_mun | RT_ICON | 300 (group 329) | 24x24 | 4 | `icons/icon_329_7_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 301 (group 329) | 16x16 | 4 | `icons/icon_329_8_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 302 (group 329) | 96x96 | 8 | `icons/icon_329_9_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 303 (group 329) | 72x72 | 8 | `icons/icon_329_10_72x72.png` |
| accessibilitycpl_mun | RT_ICON | 304 (group 329) | 64x64 | 8 | `icons/icon_329_11_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 305 (group 329) | 60x60 | 8 | `icons/icon_329_12_60x60.png` |
| accessibilitycpl_mun | RT_ICON | 306 (group 329) | 48x48 | 8 | `icons/icon_329_13_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 307 (group 329) | 40x40 | 8 | `icons/icon_329_14_40x40.png` |
| accessibilitycpl_mun | RT_ICON | 308 (group 329) | 32x32 | 8 | `icons/icon_329_15_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 309 (group 329) | 24x24 | 8 | `icons/icon_329_16_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 31 (group 320) | 64x64 | 4 | `icons/icon_320_2_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 310 (group 329) | 16x16 | 8 | `icons/icon_329_17_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 311 (group 329) | 256x256 | 32 | `icons/icon_329_18_256x256.png` |
| accessibilitycpl_mun | RT_ICON | 312 (group 329) | 96x96 | 32 | `icons/icon_329_19_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 313 (group 329) | 72x72 | 32 | `icons/icon_329_20_72x72.png` |
| accessibilitycpl_mun | RT_ICON | 314 (group 329) | 64x64 | 32 | `icons/icon_329_21_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 315 (group 329) | 60x60 | 32 | `icons/icon_329_22_60x60.png` |
| accessibilitycpl_mun | RT_ICON | 316 (group 329) | 48x48 | 32 | `icons/icon_329_23_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 317 (group 329) | 40x40 | 32 | `icons/icon_329_24_40x40.png` |
| accessibilitycpl_mun | RT_ICON | 318 (group 329) | 32x32 | 32 | `icons/icon_329_25_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 319 (group 329) | 24x24 | 32 | `icons/icon_329_26_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 32 (group 320) | 60x60 | 4 | `icons/icon_320_3_60x60.png` |
| accessibilitycpl_mun | RT_ICON | 320 (group 329) | 16x16 | 32 | `icons/icon_329_27_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 321 (group 338) | 96x96 | 4 | `icons/icon_338_0_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 322 (group 338) | 72x72 | 4 | `icons/icon_338_1_72x72.png` |
| accessibilitycpl_mun | RT_ICON | 323 (group 338) | 64x64 | 4 | `icons/icon_338_2_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 324 (group 338) | 60x60 | 4 | `icons/icon_338_3_60x60.png` |
| accessibilitycpl_mun | RT_ICON | 325 (group 338) | 48x48 | 4 | `icons/icon_338_4_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 326 (group 338) | 40x40 | 4 | `icons/icon_338_5_40x40.png` |
| accessibilitycpl_mun | RT_ICON | 327 (group 338) | 32x32 | 4 | `icons/icon_338_6_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 328 (group 338) | 24x24 | 4 | `icons/icon_338_7_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 329 (group 338) | 16x16 | 4 | `icons/icon_338_8_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 33 (group 320) | 48x48 | 4 | `icons/icon_320_4_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 330 (group 338) | 96x96 | 8 | `icons/icon_338_9_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 331 (group 338) | 72x72 | 8 | `icons/icon_338_10_72x72.png` |
| accessibilitycpl_mun | RT_ICON | 332 (group 338) | 64x64 | 8 | `icons/icon_338_11_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 333 (group 338) | 60x60 | 8 | `icons/icon_338_12_60x60.png` |
| accessibilitycpl_mun | RT_ICON | 334 (group 338) | 48x48 | 8 | `icons/icon_338_13_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 335 (group 338) | 40x40 | 8 | `icons/icon_338_14_40x40.png` |
| accessibilitycpl_mun | RT_ICON | 336 (group 338) | 32x32 | 8 | `icons/icon_338_15_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 337 (group 338) | 24x24 | 8 | `icons/icon_338_16_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 338 (group 338) | 16x16 | 8 | `icons/icon_338_17_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 339 (group 338) | 256x256 | 32 | `icons/icon_338_18_256x256.png` |
| accessibilitycpl_mun | RT_ICON | 34 (group 320) | 40x40 | 4 | `icons/icon_320_5_40x40.png` |
| accessibilitycpl_mun | RT_ICON | 340 (group 338) | 96x96 | 32 | `icons/icon_338_19_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 341 (group 338) | 72x72 | 32 | `icons/icon_338_20_72x72.png` |
| accessibilitycpl_mun | RT_ICON | 342 (group 338) | 64x64 | 32 | `icons/icon_338_21_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 343 (group 338) | 60x60 | 32 | `icons/icon_338_22_60x60.png` |
| accessibilitycpl_mun | RT_ICON | 344 (group 338) | 48x48 | 32 | `icons/icon_338_23_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 345 (group 338) | 40x40 | 32 | `icons/icon_338_24_40x40.png` |
| accessibilitycpl_mun | RT_ICON | 346 (group 338) | 32x32 | 32 | `icons/icon_338_25_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 347 (group 338) | 24x24 | 32 | `icons/icon_338_26_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 348 (group 338) | 16x16 | 32 | `icons/icon_338_27_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 349 (group 340) | 96x96 | 4 | `icons/icon_340_0_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 35 (group 320) | 32x32 | 4 | `icons/icon_320_6_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 350 (group 340) | 72x72 | 4 | `icons/icon_340_1_72x72.png` |
| accessibilitycpl_mun | RT_ICON | 351 (group 340) | 64x64 | 4 | `icons/icon_340_2_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 352 (group 340) | 60x60 | 4 | `icons/icon_340_3_60x60.png` |
| accessibilitycpl_mun | RT_ICON | 353 (group 340) | 48x48 | 4 | `icons/icon_340_4_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 354 (group 340) | 40x40 | 4 | `icons/icon_340_5_40x40.png` |
| accessibilitycpl_mun | RT_ICON | 355 (group 340) | 32x32 | 4 | `icons/icon_340_6_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 356 (group 340) | 24x24 | 4 | `icons/icon_340_7_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 357 (group 340) | 16x16 | 4 | `icons/icon_340_8_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 358 (group 340) | 96x96 | 8 | `icons/icon_340_9_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 359 (group 340) | 72x72 | 8 | `icons/icon_340_10_72x72.png` |
| accessibilitycpl_mun | RT_ICON | 36 (group 320) | 24x24 | 4 | `icons/icon_320_7_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 360 (group 340) | 64x64 | 8 | `icons/icon_340_11_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 361 (group 340) | 60x60 | 8 | `icons/icon_340_12_60x60.png` |
| accessibilitycpl_mun | RT_ICON | 362 (group 340) | 48x48 | 8 | `icons/icon_340_13_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 363 (group 340) | 40x40 | 8 | `icons/icon_340_14_40x40.png` |
| accessibilitycpl_mun | RT_ICON | 364 (group 340) | 32x32 | 8 | `icons/icon_340_15_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 365 (group 340) | 24x24 | 8 | `icons/icon_340_16_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 366 (group 340) | 16x16 | 8 | `icons/icon_340_17_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 367 (group 340) | 256x256 | 32 | `icons/icon_340_18_256x256.png` |
| accessibilitycpl_mun | RT_ICON | 368 (group 340) | 96x96 | 32 | `icons/icon_340_19_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 369 (group 340) | 72x72 | 32 | `icons/icon_340_20_72x72.png` |
| accessibilitycpl_mun | RT_ICON | 37 (group 320) | 16x16 | 4 | `icons/icon_320_8_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 370 (group 340) | 64x64 | 32 | `icons/icon_340_21_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 371 (group 340) | 60x60 | 32 | `icons/icon_340_22_60x60.png` |
| accessibilitycpl_mun | RT_ICON | 372 (group 340) | 48x48 | 32 | `icons/icon_340_23_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 373 (group 340) | 40x40 | 32 | `icons/icon_340_24_40x40.png` |
| accessibilitycpl_mun | RT_ICON | 374 (group 340) | 32x32 | 32 | `icons/icon_340_25_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 375 (group 340) | 24x24 | 32 | `icons/icon_340_26_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 376 (group 340) | 16x16 | 32 | `icons/icon_340_27_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 38 (group 320) | 96x96 | 8 | `icons/icon_320_9_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 39 (group 320) | 72x72 | 8 | `icons/icon_320_10_72x72.png` |
| accessibilitycpl_mun | RT_ICON | 4 (group 1) | 32x32 | 4 | `icons/icon_1_3_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 40 (group 320) | 64x64 | 8 | `icons/icon_320_11_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 41 (group 320) | 60x60 | 8 | `icons/icon_320_12_60x60.png` |
| accessibilitycpl_mun | RT_ICON | 42 (group 320) | 48x48 | 8 | `icons/icon_320_13_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 43 (group 320) | 40x40 | 8 | `icons/icon_320_14_40x40.png` |
| accessibilitycpl_mun | RT_ICON | 44 (group 320) | 32x32 | 8 | `icons/icon_320_15_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 45 (group 320) | 24x24 | 8 | `icons/icon_320_16_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 46 (group 320) | 16x16 | 8 | `icons/icon_320_17_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 47 (group 320) | 256x256 | 32 | `icons/icon_320_18_256x256.png` |
| accessibilitycpl_mun | RT_ICON | 48 (group 320) | 96x96 | 32 | `icons/icon_320_19_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 49 (group 320) | 72x72 | 32 | `icons/icon_320_20_72x72.png` |
| accessibilitycpl_mun | RT_ICON | 5 (group 1) | 24x24 | 4 | `icons/icon_1_4_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 50 (group 320) | 64x64 | 32 | `icons/icon_320_21_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 51 (group 320) | 60x60 | 32 | `icons/icon_320_22_60x60.png` |
| accessibilitycpl_mun | RT_ICON | 52 (group 320) | 48x48 | 32 | `icons/icon_320_23_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 53 (group 320) | 40x40 | 32 | `icons/icon_320_24_40x40.png` |
| accessibilitycpl_mun | RT_ICON | 54 (group 320) | 32x32 | 32 | `icons/icon_320_25_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 55 (group 320) | 24x24 | 32 | `icons/icon_320_26_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 56 (group 320) | 16x16 | 32 | `icons/icon_320_27_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 57 (group 321) | 96x96 | 4 | `icons/icon_321_0_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 58 (group 321) | 72x72 | 4 | `icons/icon_321_1_72x72.png` |
| accessibilitycpl_mun | RT_ICON | 59 (group 321) | 64x64 | 4 | `icons/icon_321_2_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 6 (group 1) | 16x16 | 4 | `icons/icon_1_5_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 60 (group 321) | 60x60 | 4 | `icons/icon_321_3_60x60.png` |
| accessibilitycpl_mun | RT_ICON | 61 (group 321) | 48x48 | 4 | `icons/icon_321_4_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 62 (group 321) | 40x40 | 4 | `icons/icon_321_5_40x40.png` |
| accessibilitycpl_mun | RT_ICON | 63 (group 321) | 32x32 | 4 | `icons/icon_321_6_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 64 (group 321) | 24x24 | 4 | `icons/icon_321_7_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 65 (group 321) | 16x16 | 4 | `icons/icon_321_8_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 66 (group 321) | 96x96 | 8 | `icons/icon_321_9_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 67 (group 321) | 72x72 | 8 | `icons/icon_321_10_72x72.png` |
| accessibilitycpl_mun | RT_ICON | 68 (group 321) | 64x64 | 8 | `icons/icon_321_11_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 69 (group 321) | 60x60 | 8 | `icons/icon_321_12_60x60.png` |
| accessibilitycpl_mun | RT_ICON | 7 (group 1) | 96x96 | 8 | `icons/icon_1_6_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 70 (group 321) | 48x48 | 8 | `icons/icon_321_13_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 71 (group 321) | 40x40 | 8 | `icons/icon_321_14_40x40.png` |
| accessibilitycpl_mun | RT_ICON | 72 (group 321) | 32x32 | 8 | `icons/icon_321_15_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 73 (group 321) | 24x24 | 8 | `icons/icon_321_16_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 74 (group 321) | 16x16 | 8 | `icons/icon_321_17_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 75 (group 321) | 256x256 | 32 | `icons/icon_321_18_256x256.png` |
| accessibilitycpl_mun | RT_ICON | 76 (group 321) | 96x96 | 32 | `icons/icon_321_19_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 77 (group 321) | 72x72 | 32 | `icons/icon_321_20_72x72.png` |
| accessibilitycpl_mun | RT_ICON | 78 (group 321) | 64x64 | 32 | `icons/icon_321_21_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 79 (group 321) | 60x60 | 32 | `icons/icon_321_22_60x60.png` |
| accessibilitycpl_mun | RT_ICON | 8 (group 1) | 64x64 | 8 | `icons/icon_1_7_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 80 (group 321) | 48x48 | 32 | `icons/icon_321_23_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 81 (group 321) | 40x40 | 32 | `icons/icon_321_24_40x40.png` |
| accessibilitycpl_mun | RT_ICON | 82 (group 321) | 32x32 | 32 | `icons/icon_321_25_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 83 (group 321) | 24x24 | 32 | `icons/icon_321_26_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 84 (group 321) | 16x16 | 32 | `icons/icon_321_27_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 85 (group 322) | 96x96 | 4 | `icons/icon_322_0_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 86 (group 322) | 72x72 | 4 | `icons/icon_322_1_72x72.png` |
| accessibilitycpl_mun | RT_ICON | 87 (group 322) | 64x64 | 4 | `icons/icon_322_2_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 88 (group 322) | 60x60 | 4 | `icons/icon_322_3_60x60.png` |
| accessibilitycpl_mun | RT_ICON | 89 (group 322) | 48x48 | 4 | `icons/icon_322_4_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 9 (group 1) | 48x48 | 8 | `icons/icon_1_8_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 90 (group 322) | 40x40 | 4 | `icons/icon_322_5_40x40.png` |
| accessibilitycpl_mun | RT_ICON | 91 (group 322) | 32x32 | 4 | `icons/icon_322_6_32x32.png` |
| accessibilitycpl_mun | RT_ICON | 92 (group 322) | 24x24 | 4 | `icons/icon_322_7_24x24.png` |
| accessibilitycpl_mun | RT_ICON | 93 (group 322) | 16x16 | 4 | `icons/icon_322_8_16x16.png` |
| accessibilitycpl_mun | RT_ICON | 94 (group 322) | 96x96 | 8 | `icons/icon_322_9_96x96.png` |
| accessibilitycpl_mun | RT_ICON | 95 (group 322) | 72x72 | 8 | `icons/icon_322_10_72x72.png` |
| accessibilitycpl_mun | RT_ICON | 96 (group 322) | 64x64 | 8 | `icons/icon_322_11_64x64.png` |
| accessibilitycpl_mun | RT_ICON | 97 (group 322) | 60x60 | 8 | `icons/icon_322_12_60x60.png` |
| accessibilitycpl_mun | RT_ICON | 98 (group 322) | 48x48 | 8 | `icons/icon_322_13_48x48.png` |
| accessibilitycpl_mun | RT_ICON | 99 (group 322) | 40x40 | 8 | `icons/icon_322_14_40x40.png` |
| appwiz_cpl_mun | RT_BITMAP | 100 | 12x12 | 24 | `bitmaps/bitmap_100.png` |
| appwiz_cpl_mun | RT_BITMAP | 200 | 17x17 | 32 | `bitmaps/bitmap_200.png` |
| appwiz_cpl_mun | RT_BITMAP | 201 | 17x17 | 32 | `bitmaps/bitmap_201.png` |
| appwiz_cpl_mun | RT_BITMAP | 202 | 17x17 | 32 | `bitmaps/bitmap_202.png` |
| appwiz_cpl_mun | RT_BITMAP | 203 | 17x17 | 32 | `bitmaps/bitmap_203.png` |
| appwiz_cpl_mun | RT_GROUP_ICON | 1002 | 48x48+32x32+16x16+48x48+32x32+16x16+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_1002.ico` |
| appwiz_cpl_mun | RT_GROUP_ICON | 1500 | 48x48+32x32+16x16+48x48+32x32+16x16+256x256+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32,32 | `icons/groupicon_1500.ico` |
| appwiz_cpl_mun | RT_GROUP_ICON | 1506 | 48x48+32x32+16x16+48x48+32x32+16x16+256x256+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32,32 | `icons/groupicon_1506.ico` |
| appwiz_cpl_mun | RT_GROUP_ICON | 1507 | 48x48+32x32+24x24+16x16+64x64+48x48+32x32+24x24+16x16+256x256+64x64+48x48+32x32+24x24+16x16 | 4,4,4,4,8,8,8,8,8,32,32,32,32,32,32 | `icons/groupicon_1507.ico` |
| appwiz_cpl_mun | RT_GROUP_ICON | 250 | 48x48+32x32+16x16+48x48+32x32+16x16+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_250.ico` |
| appwiz_cpl_mun | RT_ICON | 1 (group 1002) | 48x48 | 4 | `icons/icon_1002_0_48x48.png` |
| appwiz_cpl_mun | RT_ICON | 10 (group 1500) | 48x48 | 4 | `icons/icon_1500_0_48x48.png` |
| appwiz_cpl_mun | RT_ICON | 11 (group 1500) | 32x32 | 4 | `icons/icon_1500_1_32x32.png` |
| appwiz_cpl_mun | RT_ICON | 12 (group 1500) | 16x16 | 4 | `icons/icon_1500_2_16x16.png` |
| appwiz_cpl_mun | RT_ICON | 13 (group 1500) | 48x48 | 8 | `icons/icon_1500_3_48x48.png` |
| appwiz_cpl_mun | RT_ICON | 14 (group 1500) | 32x32 | 8 | `icons/icon_1500_4_32x32.png` |
| appwiz_cpl_mun | RT_ICON | 15 (group 1500) | 16x16 | 8 | `icons/icon_1500_5_16x16.png` |
| appwiz_cpl_mun | RT_ICON | 16 (group 1500) | 256x256 | 32 | `icons/icon_1500_6_256x256.png` |
| appwiz_cpl_mun | RT_ICON | 17 (group 1500) | 48x48 | 32 | `icons/icon_1500_7_48x48.png` |
| appwiz_cpl_mun | RT_ICON | 18 (group 1500) | 32x32 | 32 | `icons/icon_1500_8_32x32.png` |
| appwiz_cpl_mun | RT_ICON | 19 (group 1500) | 16x16 | 32 | `icons/icon_1500_9_16x16.png` |
| appwiz_cpl_mun | RT_ICON | 2 (group 1002) | 32x32 | 4 | `icons/icon_1002_1_32x32.png` |
| appwiz_cpl_mun | RT_ICON | 20 (group 250) | 48x48 | 4 | `icons/icon_250_0_48x48.png` |
| appwiz_cpl_mun | RT_ICON | 21 (group 250) | 32x32 | 4 | `icons/icon_250_1_32x32.png` |
| appwiz_cpl_mun | RT_ICON | 22 (group 250) | 16x16 | 4 | `icons/icon_250_2_16x16.png` |
| appwiz_cpl_mun | RT_ICON | 23 (group 250) | 48x48 | 8 | `icons/icon_250_3_48x48.png` |
| appwiz_cpl_mun | RT_ICON | 24 (group 250) | 32x32 | 8 | `icons/icon_250_4_32x32.png` |
| appwiz_cpl_mun | RT_ICON | 25 (group 250) | 16x16 | 8 | `icons/icon_250_5_16x16.png` |
| appwiz_cpl_mun | RT_ICON | 26 (group 250) | 48x48 | 32 | `icons/icon_250_6_48x48.png` |
| appwiz_cpl_mun | RT_ICON | 27 (group 250) | 32x32 | 32 | `icons/icon_250_7_32x32.png` |
| appwiz_cpl_mun | RT_ICON | 28 (group 250) | 16x16 | 32 | `icons/icon_250_8_16x16.png` |
| appwiz_cpl_mun | RT_ICON | 29 (group 1506) | 48x48 | 4 | `icons/icon_1506_0_48x48.png` |
| appwiz_cpl_mun | RT_ICON | 3 (group 1002) | 16x16 | 4 | `icons/icon_1002_2_16x16.png` |
| appwiz_cpl_mun | RT_ICON | 30 (group 1506) | 32x32 | 4 | `icons/icon_1506_1_32x32.png` |
| appwiz_cpl_mun | RT_ICON | 31 (group 1506) | 16x16 | 4 | `icons/icon_1506_2_16x16.png` |
| appwiz_cpl_mun | RT_ICON | 32 (group 1506) | 48x48 | 8 | `icons/icon_1506_3_48x48.png` |
| appwiz_cpl_mun | RT_ICON | 33 (group 1506) | 32x32 | 8 | `icons/icon_1506_4_32x32.png` |
| appwiz_cpl_mun | RT_ICON | 34 (group 1506) | 16x16 | 8 | `icons/icon_1506_5_16x16.png` |
| appwiz_cpl_mun | RT_ICON | 35 (group 1506) | 256x256 | 32 | `icons/icon_1506_6_256x256.png` |
| appwiz_cpl_mun | RT_ICON | 36 (group 1506) | 48x48 | 32 | `icons/icon_1506_7_48x48.png` |
| appwiz_cpl_mun | RT_ICON | 37 (group 1506) | 32x32 | 32 | `icons/icon_1506_8_32x32.png` |
| appwiz_cpl_mun | RT_ICON | 38 (group 1506) | 16x16 | 32 | `icons/icon_1506_9_16x16.png` |
| appwiz_cpl_mun | RT_ICON | 39 (group 1507) | 48x48 | 4 | `icons/icon_1507_0_48x48.png` |
| appwiz_cpl_mun | RT_ICON | 4 (group 1002) | 48x48 | 8 | `icons/icon_1002_3_48x48.png` |
| appwiz_cpl_mun | RT_ICON | 40 (group 1507) | 32x32 | 4 | `icons/icon_1507_1_32x32.png` |
| appwiz_cpl_mun | RT_ICON | 41 (group 1507) | 24x24 | 4 | `icons/icon_1507_2_24x24.png` |
| appwiz_cpl_mun | RT_ICON | 42 (group 1507) | 16x16 | 4 | `icons/icon_1507_3_16x16.png` |
| appwiz_cpl_mun | RT_ICON | 43 (group 1507) | 64x64 | 8 | `icons/icon_1507_4_64x64.png` |
| appwiz_cpl_mun | RT_ICON | 44 (group 1507) | 48x48 | 8 | `icons/icon_1507_5_48x48.png` |
| appwiz_cpl_mun | RT_ICON | 45 (group 1507) | 32x32 | 8 | `icons/icon_1507_6_32x32.png` |
| appwiz_cpl_mun | RT_ICON | 46 (group 1507) | 24x24 | 8 | `icons/icon_1507_7_24x24.png` |
| appwiz_cpl_mun | RT_ICON | 47 (group 1507) | 16x16 | 8 | `icons/icon_1507_8_16x16.png` |
| appwiz_cpl_mun | RT_ICON | 48 (group 1507) | 256x256 | 32 | `icons/icon_1507_9_256x256.png` |
| appwiz_cpl_mun | RT_ICON | 49 (group 1507) | 64x64 | 32 | `icons/icon_1507_10_64x64.png` |
| appwiz_cpl_mun | RT_ICON | 5 (group 1002) | 32x32 | 8 | `icons/icon_1002_4_32x32.png` |
| appwiz_cpl_mun | RT_ICON | 50 (group 1507) | 48x48 | 32 | `icons/icon_1507_11_48x48.png` |
| appwiz_cpl_mun | RT_ICON | 51 (group 1507) | 32x32 | 32 | `icons/icon_1507_12_32x32.png` |
| appwiz_cpl_mun | RT_ICON | 52 (group 1507) | 24x24 | 32 | `icons/icon_1507_13_24x24.png` |
| appwiz_cpl_mun | RT_ICON | 53 (group 1507) | 16x16 | 32 | `icons/icon_1507_14_16x16.png` |
| appwiz_cpl_mun | RT_ICON | 6 (group 1002) | 16x16 | 8 | `icons/icon_1002_5_16x16.png` |
| appwiz_cpl_mun | RT_ICON | 7 (group 1002) | 48x48 | 32 | `icons/icon_1002_6_48x48.png` |
| appwiz_cpl_mun | RT_ICON | 8 (group 1002) | 32x32 | 32 | `icons/icon_1002_7_32x32.png` |
| appwiz_cpl_mun | RT_ICON | 9 (group 1002) | 16x16 | 32 | `icons/icon_1002_8_16x16.png` |
| inetcpl_cpl_mun | RT_BITMAP | 4474 | 80x16 | 4 | `bitmaps/bitmap_4474.png` |
| inetcpl_cpl_mun | RT_BITMAP | 7200 | 64x64 | 24 | `bitmaps/bitmap_7200.png` |
| inetcpl_cpl_mun | RT_BITMAP | 7201 | 64x64 | 24 | `bitmaps/bitmap_7201.png` |
| inetcpl_cpl_mun | RT_BITMAP | 7202 | 64x64 | 24 | `bitmaps/bitmap_7202.png` |
| inetcpl_cpl_mun | RT_BITMAP | 7203 | 64x64 | 24 | `bitmaps/bitmap_7203.png` |
| inetcpl_cpl_mun | RT_BITMAP | 7204 | 64x64 | 24 | `bitmaps/bitmap_7204.png` |
| inetcpl_cpl_mun | RT_BITMAP | 7205 | 64x64 | 24 | `bitmaps/bitmap_7205.png` |
| inetcpl_cpl_mun | RT_BITMAP | 7206 | 80x80 | 24 | `bitmaps/bitmap_7206.png` |
| inetcpl_cpl_mun | RT_BITMAP | 7207 | 80x80 | 24 | `bitmaps/bitmap_7207.png` |
| inetcpl_cpl_mun | RT_BITMAP | 7208 | 80x80 | 24 | `bitmaps/bitmap_7208.png` |
| inetcpl_cpl_mun | RT_BITMAP | 7209 | 80x80 | 24 | `bitmaps/bitmap_7209.png` |
| inetcpl_cpl_mun | RT_BITMAP | 7210 | 80x80 | 24 | `bitmaps/bitmap_7210.png` |
| inetcpl_cpl_mun | RT_BITMAP | 7211 | 80x80 | 24 | `bitmaps/bitmap_7211.png` |
| inetcpl_cpl_mun | RT_BITMAP | 7212 | 128x128 | 24 | `bitmaps/bitmap_7212.png` |
| inetcpl_cpl_mun | RT_BITMAP | 7213 | 128x128 | 24 | `bitmaps/bitmap_7213.png` |
| inetcpl_cpl_mun | RT_BITMAP | 7214 | 128x128 | 24 | `bitmaps/bitmap_7214.png` |
| inetcpl_cpl_mun | RT_BITMAP | 7215 | 128x128 | 24 | `bitmaps/bitmap_7215.png` |
| inetcpl_cpl_mun | RT_BITMAP | 7216 | 128x128 | 24 | `bitmaps/bitmap_7216.png` |
| inetcpl_cpl_mun | RT_BITMAP | 7217 | 128x128 | 24 | `bitmaps/bitmap_7217.png` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 1301 | 48x48+32x32+16x16+48x48+32x32+16x16+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_1301.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 1303 | 48x48+32x32+16x16+48x48+32x32+16x16+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_1303.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 1307 | 48x48+32x32+48x48+32x32+48x48+32x32 | 4,4,8,8,32,32 | `icons/groupicon_1307.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 1310 | 48x48+32x32+48x48+32x32+48x48+32x32 | 4,4,8,8,32,32 | `icons/groupicon_1310.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 1312 | 16x16+16x16+16x16 | 4,8,32 | `icons/groupicon_1312.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 1313 | 48x48+32x32+16x16+48x48+32x32+16x16+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_1313.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 1315 | 32x32+32x32+32x32 | 4,8,32 | `icons/groupicon_1315.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 1318 | 48x48+32x32+48x48+32x32+48x48+32x32 | 4,4,8,8,32,32 | `icons/groupicon_1318.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 1321 | 16x16+16x16+16x16 | 4,8,32 | `icons/groupicon_1321.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 1322 | 48x48+32x32+48x48+32x32+48x48+32x32 | 4,4,8,8,32,32 | `icons/groupicon_1322.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 1323 | 24x24+16x16+24x24+16x16+24x24+16x16 | 4,4,8,8,32,32 | `icons/groupicon_1323.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 1326 | 16x16+16x16+16x16 | 4,8,32 | `icons/groupicon_1326.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 1327 | 16x16+16x16+16x16 | 4,8,32 | `icons/groupicon_1327.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 1328 | 48x48+32x32+16x16+48x48+32x32+16x16+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_1328.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 4441 | 32x32+16x16+32x32+16x16+32x32+16x16 | 4,4,8,8,32,32 | `icons/groupicon_4441.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 4442 | 24x24+16x16+24x24+16x16+24x24+16x16 | 4,4,8,8,32,32 | `icons/groupicon_4442.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 4443 | 48x48+32x32+24x24+16x16+48x48+32x32+24x24+16x16+256x256+48x48+32x32+24x24+16x16 | 4,4,4,4,8,8,8,8,32,32,32,32,32 | `icons/groupicon_4443.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 4447 | 24x24+16x16+24x24+16x16+24x24+16x16 | 4,4,8,8,32,32 | `icons/groupicon_4447.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 4460 | 48x48+32x32+48x48+32x32+48x48+32x32 | 4,4,8,8,32,32 | `icons/groupicon_4460.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 4478 | 32x32+32x32+32x32 | 4,8,32 | `icons/groupicon_4478.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 4480 | 48x48+32x32+16x16+48x48+32x32+16x16+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_4480.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 4481 | 48x48+32x32+16x16+48x48+32x32+16x16+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_4481.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 4482 | 24x24+16x16+24x24+16x16+24x24+16x16 | 4,4,8,8,32,32 | `icons/groupicon_4482.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 4483 | 48x48+32x32+24x24+16x16+48x48+32x32+24x24+16x16+48x48+32x32+24x24+16x16 | 4,4,4,4,8,8,8,8,32,32,32,32 | `icons/groupicon_4483.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 4485 | 16x16+16x16+16x16 | 4,8,32 | `icons/groupicon_4485.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 4486 | 24x24+16x16+24x24+16x16+24x24+16x16 | 4,4,8,8,32,32 | `icons/groupicon_4486.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 4487 | 48x48+32x32+16x16+48x48+32x32+16x16+256x256+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32,32 | `icons/groupicon_4487.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 4490 | 48x48+32x32+48x48+32x32+48x48+32x32 | 4,4,8,8,32,32 | `icons/groupicon_4490.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 4497 | 24x24+16x16+24x24+16x16+24x24+16x16 | 4,4,8,8,32,32 | `icons/groupicon_4497.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 4498 | 48x48+32x32+16x16+48x48+32x32+16x16+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_4498.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 4651 | 32x32+32x32+32x32 | 4,8,32 | `icons/groupicon_4651.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 4652 | 32x32+32x32+32x32 | 4,8,32 | `icons/groupicon_4652.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 5390 | 48x48+32x32+16x16+48x48+32x32+16x16+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_5390.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 5393 | 48x48+32x32+24x24+16x16+48x48+32x32+24x24+16x16+48x48+32x32+24x24+16x16 | 4,4,4,4,8,8,8,8,32,32,32,32 | `icons/groupicon_5393.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 5410 | 48x48+32x32+16x16+48x48+32x32+16x16+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_5410.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 5411 | 48x48+32x32+16x16+48x48+32x32+16x16+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_5411.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 5419 | 48x48+32x32+16x16+48x48+32x32+16x16+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_5419.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 5422 | 48x48+32x32+16x16+48x48+32x32+16x16+256x256+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32,32 | `icons/groupicon_5422.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 5423 | 48x48+32x32+16x16+48x48+32x32+16x16+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_5423.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 5424 | 32x32+16x16+32x32+16x16+32x32+16x16 | 4,4,8,8,32,32 | `icons/groupicon_5424.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 5425 | 32x32+16x16+32x32+16x16+32x32+16x16 | 4,4,8,8,32,32 | `icons/groupicon_5425.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 5426 | 32x32+32x32+32x32 | 4,8,32 | `icons/groupicon_5426.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 5603 | 32x32 | 8 | `icons/groupicon_5603.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 5604 | 32x32 | 4 | `icons/groupicon_5604.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 5605 | 48x48+32x32+48x48+32x32+48x48+32x32 | 4,4,8,8,32,32 | `icons/groupicon_5605.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 5606 | 48x48+32x32+48x48+32x32+48x48+32x32 | 4,4,8,8,32,32 | `icons/groupicon_5606.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 5901 | 48x48+32x32+16x16+48x48+32x32+16x16+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_5901.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 5905 | 48x48+32x32+24x24+16x16+48x48+32x32+24x24+16x16+256x256+48x48+32x32+24x24+16x16 | 4,4,4,4,8,8,8,8,32,32,32,32,32 | `icons/groupicon_5905.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 5906 | 48x48+32x32+24x24+16x16+48x48+32x32+24x24+16x16+48x48+32x32+24x24+16x16 | 4,4,4,4,8,8,8,8,32,32,32,32 | `icons/groupicon_5906.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 6601 | 32x32 | 4 | `icons/groupicon_6601.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 6609 | 16x16 | 32 | `icons/groupicon_6609.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 6610 | 16x16 | 32 | `icons/groupicon_6610.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 6611 | 16x16 | 32 | `icons/groupicon_6611.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 6623 | 48x48+32x32+48x48+32x32+48x48+32x32 | 4,4,8,8,32,32 | `icons/groupicon_6623.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 6627 | 32x32+4x4 | 4,8 | `icons/groupicon_6627.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 6628 | 32x32+4x4 | 4,8 | `icons/groupicon_6628.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 6629 | 32x32+4x4 | 4,8 | `icons/groupicon_6629.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 6630 | 32x32+4x4 | 4,8 | `icons/groupicon_6630.ico` |
| inetcpl_cpl_mun | RT_GROUP_ICON | 6657 | 32x32+24x24+16x16+32x32+24x24+16x16+32x32+24x24+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_6657.ico` |
| inetcpl_cpl_mun | RT_ICON | 1 (group 1321) | 16x16 | 4 | `icons/icon_1321_0_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 10 (group 4487) | 256x256 | 32 | `icons/icon_4487_6_256x256.png` |
| inetcpl_cpl_mun | RT_ICON | 100 (group 4486) | 24x24 | 32 | `icons/icon_4486_4_24x24.png` |
| inetcpl_cpl_mun | RT_ICON | 101 (group 4486) | 16x16 | 32 | `icons/icon_4486_5_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 102 (group 1313) | 48x48 | 4 | `icons/icon_1313_0_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 103 (group 1313) | 32x32 | 4 | `icons/icon_1313_1_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 104 (group 1313) | 16x16 | 4 | `icons/icon_1313_2_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 105 (group 1313) | 48x48 | 8 | `icons/icon_1313_3_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 106 (group 1313) | 32x32 | 8 | `icons/icon_1313_4_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 107 (group 1313) | 16x16 | 8 | `icons/icon_1313_5_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 108 (group 1313) | 48x48 | 32 | `icons/icon_1313_6_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 109 (group 1313) | 32x32 | 32 | `icons/icon_1313_7_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 11 (group 4487) | 48x48 | 32 | `icons/icon_4487_7_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 110 (group 1313) | 16x16 | 32 | `icons/icon_1313_8_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 111 (group 4497) | 24x24 | 4 | `icons/icon_4497_0_24x24.png` |
| inetcpl_cpl_mun | RT_ICON | 112 (group 4497) | 16x16 | 4 | `icons/icon_4497_1_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 113 (group 4497) | 24x24 | 8 | `icons/icon_4497_2_24x24.png` |
| inetcpl_cpl_mun | RT_ICON | 114 (group 4497) | 16x16 | 8 | `icons/icon_4497_3_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 115 (group 4497) | 24x24 | 32 | `icons/icon_4497_4_24x24.png` |
| inetcpl_cpl_mun | RT_ICON | 116 (group 4497) | 16x16 | 32 | `icons/icon_4497_5_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 117 (group 4498) | 48x48 | 4 | `icons/icon_4498_0_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 118 (group 4498) | 32x32 | 4 | `icons/icon_4498_1_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 119 (group 4498) | 16x16 | 4 | `icons/icon_4498_2_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 12 (group 4487) | 32x32 | 32 | `icons/icon_4487_8_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 120 (group 4498) | 48x48 | 8 | `icons/icon_4498_3_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 121 (group 4498) | 32x32 | 8 | `icons/icon_4498_4_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 122 (group 4498) | 16x16 | 8 | `icons/icon_4498_5_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 123 (group 4498) | 48x48 | 32 | `icons/icon_4498_6_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 124 (group 4498) | 32x32 | 32 | `icons/icon_4498_7_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 125 (group 4498) | 16x16 | 32 | `icons/icon_4498_8_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 126 (group 4652) | 32x32 | 4 | `icons/icon_4652_0_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 127 (group 4652) | 32x32 | 8 | `icons/icon_4652_1_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 128 (group 4652) | 32x32 | 32 | `icons/icon_4652_2_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 129 (group 4651) | 32x32 | 4 | `icons/icon_4651_0_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 13 (group 4487) | 16x16 | 32 | `icons/icon_4487_9_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 130 (group 4651) | 32x32 | 8 | `icons/icon_4651_1_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 131 (group 4651) | 32x32 | 32 | `icons/icon_4651_2_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 132 (group 1315) | 32x32 | 4 | `icons/icon_1315_0_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 133 (group 1315) | 32x32 | 8 | `icons/icon_1315_1_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 134 (group 1315) | 32x32 | 32 | `icons/icon_1315_2_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 135 (group 1310) | 48x48 | 4 | `icons/icon_1310_0_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 136 (group 1310) | 32x32 | 4 | `icons/icon_1310_1_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 137 (group 1310) | 48x48 | 8 | `icons/icon_1310_2_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 138 (group 1310) | 32x32 | 8 | `icons/icon_1310_3_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 139 (group 1310) | 48x48 | 32 | `icons/icon_1310_4_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 14 (group 1312) | 16x16 | 4 | `icons/icon_1312_0_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 140 (group 1310) | 32x32 | 32 | `icons/icon_1310_5_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 141 (group 1301) | 48x48 | 4 | `icons/icon_1301_0_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 142 (group 1301) | 32x32 | 4 | `icons/icon_1301_1_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 143 (group 1301) | 16x16 | 4 | `icons/icon_1301_2_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 144 (group 1301) | 48x48 | 8 | `icons/icon_1301_3_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 145 (group 1301) | 32x32 | 8 | `icons/icon_1301_4_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 146 (group 1301) | 16x16 | 8 | `icons/icon_1301_5_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 147 (group 1301) | 48x48 | 32 | `icons/icon_1301_6_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 148 (group 1301) | 32x32 | 32 | `icons/icon_1301_7_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 149 (group 1301) | 16x16 | 32 | `icons/icon_1301_8_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 15 (group 1312) | 16x16 | 8 | `icons/icon_1312_1_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 150 (group 1303) | 48x48 | 4 | `icons/icon_1303_0_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 151 (group 1303) | 32x32 | 4 | `icons/icon_1303_1_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 152 (group 1303) | 16x16 | 4 | `icons/icon_1303_2_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 153 (group 1303) | 48x48 | 8 | `icons/icon_1303_3_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 154 (group 1303) | 32x32 | 8 | `icons/icon_1303_4_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 155 (group 1303) | 16x16 | 8 | `icons/icon_1303_5_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 156 (group 1303) | 48x48 | 32 | `icons/icon_1303_6_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 157 (group 1303) | 32x32 | 32 | `icons/icon_1303_7_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 158 (group 1303) | 16x16 | 32 | `icons/icon_1303_8_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 159 (group 1328) | 48x48 | 4 | `icons/icon_1328_0_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 16 (group 1312) | 16x16 | 32 | `icons/icon_1312_2_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 160 (group 1328) | 32x32 | 4 | `icons/icon_1328_1_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 161 (group 1328) | 16x16 | 4 | `icons/icon_1328_2_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 162 (group 1328) | 48x48 | 8 | `icons/icon_1328_3_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 163 (group 1328) | 32x32 | 8 | `icons/icon_1328_4_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 164 (group 1328) | 16x16 | 8 | `icons/icon_1328_5_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 165 (group 1328) | 48x48 | 32 | `icons/icon_1328_6_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 166 (group 1328) | 32x32 | 32 | `icons/icon_1328_7_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 167 (group 1328) | 16x16 | 32 | `icons/icon_1328_8_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 168 (group 4490) | 48x48 | 4 | `icons/icon_4490_0_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 169 (group 4490) | 32x32 | 4 | `icons/icon_4490_1_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 17 (group 1307) | 48x48 | 4 | `icons/icon_1307_0_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 170 (group 4490) | 48x48 | 8 | `icons/icon_4490_2_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 171 (group 4490) | 32x32 | 8 | `icons/icon_4490_3_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 172 (group 4490) | 48x48 | 32 | `icons/icon_4490_4_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 173 (group 4490) | 32x32 | 32 | `icons/icon_4490_5_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 174 (group 1318) | 48x48 | 4 | `icons/icon_1318_0_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 175 (group 1318) | 32x32 | 4 | `icons/icon_1318_1_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 176 (group 1318) | 48x48 | 8 | `icons/icon_1318_2_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 177 (group 1318) | 32x32 | 8 | `icons/icon_1318_3_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 178 (group 1318) | 48x48 | 32 | `icons/icon_1318_4_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 179 (group 1318) | 32x32 | 32 | `icons/icon_1318_5_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 18 (group 1307) | 32x32 | 4 | `icons/icon_1307_1_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 180 (group 4460) | 48x48 | 4 | `icons/icon_4460_0_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 181 (group 4460) | 32x32 | 4 | `icons/icon_4460_1_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 182 (group 4460) | 48x48 | 8 | `icons/icon_4460_2_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 183 (group 4460) | 32x32 | 8 | `icons/icon_4460_3_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 184 (group 4460) | 48x48 | 32 | `icons/icon_4460_4_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 185 (group 4460) | 32x32 | 32 | `icons/icon_4460_5_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 186 (group 1322) | 48x48 | 4 | `icons/icon_1322_0_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 187 (group 1322) | 32x32 | 4 | `icons/icon_1322_1_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 188 (group 1322) | 48x48 | 8 | `icons/icon_1322_2_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 189 (group 1322) | 32x32 | 8 | `icons/icon_1322_3_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 19 (group 1307) | 48x48 | 8 | `icons/icon_1307_2_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 190 (group 1322) | 48x48 | 32 | `icons/icon_1322_4_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 191 (group 1322) | 32x32 | 32 | `icons/icon_1322_5_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 192 (group 5390) | 48x48 | 4 | `icons/icon_5390_0_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 193 (group 5390) | 32x32 | 4 | `icons/icon_5390_1_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 194 (group 5390) | 16x16 | 4 | `icons/icon_5390_2_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 195 (group 5390) | 48x48 | 8 | `icons/icon_5390_3_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 196 (group 5390) | 32x32 | 8 | `icons/icon_5390_4_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 197 (group 5390) | 16x16 | 8 | `icons/icon_5390_5_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 198 (group 5390) | 48x48 | 32 | `icons/icon_5390_6_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 199 (group 5390) | 32x32 | 32 | `icons/icon_5390_7_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 2 (group 1321) | 16x16 | 8 | `icons/icon_1321_1_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 20 (group 1307) | 32x32 | 8 | `icons/icon_1307_3_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 200 (group 5390) | 16x16 | 32 | `icons/icon_5390_8_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 201 (group 5410) | 48x48 | 4 | `icons/icon_5410_0_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 202 (group 5410) | 32x32 | 4 | `icons/icon_5410_1_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 203 (group 5410) | 16x16 | 4 | `icons/icon_5410_2_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 204 (group 5410) | 48x48 | 8 | `icons/icon_5410_3_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 205 (group 5410) | 32x32 | 8 | `icons/icon_5410_4_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 206 (group 5410) | 16x16 | 8 | `icons/icon_5410_5_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 207 (group 5410) | 48x48 | 32 | `icons/icon_5410_6_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 208 (group 5410) | 32x32 | 32 | `icons/icon_5410_7_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 209 (group 5410) | 16x16 | 32 | `icons/icon_5410_8_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 21 (group 1307) | 48x48 | 32 | `icons/icon_1307_4_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 210 (group 5411) | 48x48 | 4 | `icons/icon_5411_0_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 211 (group 5411) | 32x32 | 4 | `icons/icon_5411_1_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 212 (group 5411) | 16x16 | 4 | `icons/icon_5411_2_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 213 (group 5411) | 48x48 | 8 | `icons/icon_5411_3_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 214 (group 5411) | 32x32 | 8 | `icons/icon_5411_4_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 215 (group 5411) | 16x16 | 8 | `icons/icon_5411_5_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 216 (group 5411) | 48x48 | 32 | `icons/icon_5411_6_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 217 (group 5411) | 32x32 | 32 | `icons/icon_5411_7_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 218 (group 5411) | 16x16 | 32 | `icons/icon_5411_8_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 219 (group 5422) | 48x48 | 4 | `icons/icon_5422_0_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 22 (group 1307) | 32x32 | 32 | `icons/icon_1307_5_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 220 (group 5422) | 32x32 | 4 | `icons/icon_5422_1_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 221 (group 5422) | 16x16 | 4 | `icons/icon_5422_2_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 222 (group 5422) | 48x48 | 8 | `icons/icon_5422_3_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 223 (group 5422) | 32x32 | 8 | `icons/icon_5422_4_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 224 (group 5422) | 16x16 | 8 | `icons/icon_5422_5_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 225 (group 5422) | 256x256 | 32 | `icons/icon_5422_6_256x256.png` |
| inetcpl_cpl_mun | RT_ICON | 226 (group 5422) | 48x48 | 32 | `icons/icon_5422_7_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 227 (group 5422) | 32x32 | 32 | `icons/icon_5422_8_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 228 (group 5422) | 16x16 | 32 | `icons/icon_5422_9_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 229 (group 5423) | 48x48 | 4 | `icons/icon_5423_0_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 23 (group 4442) | 24x24 | 4 | `icons/icon_4442_0_24x24.png` |
| inetcpl_cpl_mun | RT_ICON | 230 (group 5423) | 32x32 | 4 | `icons/icon_5423_1_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 231 (group 5423) | 16x16 | 4 | `icons/icon_5423_2_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 232 (group 5423) | 48x48 | 8 | `icons/icon_5423_3_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 233 (group 5423) | 32x32 | 8 | `icons/icon_5423_4_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 234 (group 5423) | 16x16 | 8 | `icons/icon_5423_5_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 235 (group 5423) | 48x48 | 32 | `icons/icon_5423_6_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 236 (group 5423) | 32x32 | 32 | `icons/icon_5423_7_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 237 (group 5423) | 16x16 | 32 | `icons/icon_5423_8_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 238 (group 5425) | 32x32 | 4 | `icons/icon_5425_0_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 239 (group 5425) | 16x16 | 4 | `icons/icon_5425_1_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 24 (group 4442) | 16x16 | 4 | `icons/icon_4442_1_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 240 (group 5425) | 32x32 | 8 | `icons/icon_5425_2_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 241 (group 5425) | 16x16 | 8 | `icons/icon_5425_3_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 242 (group 5425) | 32x32 | 32 | `icons/icon_5425_4_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 243 (group 5425) | 16x16 | 32 | `icons/icon_5425_5_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 244 (group 5426) | 32x32 | 4 | `icons/icon_5426_0_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 245 (group 5426) | 32x32 | 8 | `icons/icon_5426_1_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 246 (group 5426) | 32x32 | 32 | `icons/icon_5426_2_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 247 (group 5424) | 32x32 | 4 | `icons/icon_5424_0_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 248 (group 5424) | 16x16 | 4 | `icons/icon_5424_1_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 249 (group 5424) | 32x32 | 8 | `icons/icon_5424_2_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 25 (group 4442) | 24x24 | 8 | `icons/icon_4442_2_24x24.png` |
| inetcpl_cpl_mun | RT_ICON | 250 (group 5424) | 16x16 | 8 | `icons/icon_5424_3_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 251 (group 5424) | 32x32 | 32 | `icons/icon_5424_4_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 252 (group 5424) | 16x16 | 32 | `icons/icon_5424_5_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 253 (group 1326) | 16x16 | 4 | `icons/icon_1326_0_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 254 (group 1326) | 16x16 | 8 | `icons/icon_1326_1_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 255 (group 1326) | 16x16 | 32 | `icons/icon_1326_2_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 256 (group 1327) | 16x16 | 4 | `icons/icon_1327_0_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 257 (group 1327) | 16x16 | 8 | `icons/icon_1327_1_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 258 (group 1327) | 16x16 | 32 | `icons/icon_1327_2_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 259 (group 5901) | 48x48 | 4 | `icons/icon_5901_0_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 26 (group 4442) | 16x16 | 8 | `icons/icon_4442_3_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 260 (group 5901) | 32x32 | 4 | `icons/icon_5901_1_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 261 (group 5901) | 16x16 | 4 | `icons/icon_5901_2_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 262 (group 5901) | 48x48 | 8 | `icons/icon_5901_3_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 263 (group 5901) | 32x32 | 8 | `icons/icon_5901_4_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 264 (group 5901) | 16x16 | 8 | `icons/icon_5901_5_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 265 (group 5901) | 48x48 | 32 | `icons/icon_5901_6_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 266 (group 5901) | 32x32 | 32 | `icons/icon_5901_7_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 267 (group 5901) | 16x16 | 32 | `icons/icon_5901_8_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 268 (group 5603) | 32x32 | 8 | `icons/icon_5603_0_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 269 (group 5604) | 32x32 | 4 | `icons/icon_5604_0_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 27 (group 4442) | 24x24 | 32 | `icons/icon_4442_4_24x24.png` |
| inetcpl_cpl_mun | RT_ICON | 270 (group 5605) | 48x48 | 4 | `icons/icon_5605_0_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 271 (group 5605) | 32x32 | 4 | `icons/icon_5605_1_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 272 (group 5605) | 48x48 | 8 | `icons/icon_5605_2_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 273 (group 5605) | 32x32 | 8 | `icons/icon_5605_3_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 274 (group 5605) | 48x48 | 32 | `icons/icon_5605_4_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 275 (group 5605) | 32x32 | 32 | `icons/icon_5605_5_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 276 (group 5606) | 48x48 | 4 | `icons/icon_5606_0_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 277 (group 5606) | 32x32 | 4 | `icons/icon_5606_1_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 278 (group 5606) | 48x48 | 8 | `icons/icon_5606_2_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 279 (group 5606) | 32x32 | 8 | `icons/icon_5606_3_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 28 (group 4442) | 16x16 | 32 | `icons/icon_4442_5_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 280 (group 5606) | 48x48 | 32 | `icons/icon_5606_4_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 281 (group 5606) | 32x32 | 32 | `icons/icon_5606_5_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 282 (group 5419) | 48x48 | 4 | `icons/icon_5419_0_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 283 (group 5419) | 32x32 | 4 | `icons/icon_5419_1_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 284 (group 5419) | 16x16 | 4 | `icons/icon_5419_2_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 285 (group 5419) | 48x48 | 8 | `icons/icon_5419_3_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 286 (group 5419) | 32x32 | 8 | `icons/icon_5419_4_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 287 (group 5419) | 16x16 | 8 | `icons/icon_5419_5_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 288 (group 5419) | 48x48 | 32 | `icons/icon_5419_6_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 289 (group 5419) | 32x32 | 32 | `icons/icon_5419_7_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 29 (group 4443) | 48x48 | 4 | `icons/icon_4443_0_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 290 (group 5419) | 16x16 | 32 | `icons/icon_5419_8_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 291 (group 1323) | 24x24 | 4 | `icons/icon_1323_0_24x24.png` |
| inetcpl_cpl_mun | RT_ICON | 292 (group 1323) | 16x16 | 4 | `icons/icon_1323_1_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 293 (group 1323) | 24x24 | 8 | `icons/icon_1323_2_24x24.png` |
| inetcpl_cpl_mun | RT_ICON | 294 (group 1323) | 16x16 | 8 | `icons/icon_1323_3_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 295 (group 1323) | 24x24 | 32 | `icons/icon_1323_4_24x24.png` |
| inetcpl_cpl_mun | RT_ICON | 296 (group 1323) | 16x16 | 32 | `icons/icon_1323_5_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 297 (group 5905) | 48x48 | 4 | `icons/icon_5905_0_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 298 (group 5905) | 32x32 | 4 | `icons/icon_5905_1_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 299 (group 5905) | 24x24 | 4 | `icons/icon_5905_2_24x24.png` |
| inetcpl_cpl_mun | RT_ICON | 3 (group 1321) | 16x16 | 32 | `icons/icon_1321_2_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 30 (group 4443) | 32x32 | 4 | `icons/icon_4443_1_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 300 (group 5905) | 16x16 | 4 | `icons/icon_5905_3_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 301 (group 5905) | 48x48 | 8 | `icons/icon_5905_4_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 302 (group 5905) | 32x32 | 8 | `icons/icon_5905_5_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 303 (group 5905) | 24x24 | 8 | `icons/icon_5905_6_24x24.png` |
| inetcpl_cpl_mun | RT_ICON | 304 (group 5905) | 16x16 | 8 | `icons/icon_5905_7_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 305 (group 5905) | 256x256 | 32 | `icons/icon_5905_8_256x256.png` |
| inetcpl_cpl_mun | RT_ICON | 306 (group 5905) | 48x48 | 32 | `icons/icon_5905_9_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 307 (group 5905) | 32x32 | 32 | `icons/icon_5905_10_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 308 (group 5905) | 24x24 | 32 | `icons/icon_5905_11_24x24.png` |
| inetcpl_cpl_mun | RT_ICON | 309 (group 5905) | 16x16 | 32 | `icons/icon_5905_12_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 31 (group 4443) | 24x24 | 4 | `icons/icon_4443_2_24x24.png` |
| inetcpl_cpl_mun | RT_ICON | 310 (group 5906) | 48x48 | 4 | `icons/icon_5906_0_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 311 (group 5906) | 32x32 | 4 | `icons/icon_5906_1_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 312 (group 5906) | 24x24 | 4 | `icons/icon_5906_2_24x24.png` |
| inetcpl_cpl_mun | RT_ICON | 313 (group 5906) | 16x16 | 4 | `icons/icon_5906_3_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 314 (group 5906) | 48x48 | 8 | `icons/icon_5906_4_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 315 (group 5906) | 32x32 | 8 | `icons/icon_5906_5_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 316 (group 5906) | 24x24 | 8 | `icons/icon_5906_6_24x24.png` |
| inetcpl_cpl_mun | RT_ICON | 317 (group 5906) | 16x16 | 8 | `icons/icon_5906_7_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 318 (group 5906) | 48x48 | 32 | `icons/icon_5906_8_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 319 (group 5906) | 32x32 | 32 | `icons/icon_5906_9_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 32 (group 4443) | 16x16 | 4 | `icons/icon_4443_3_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 320 (group 5906) | 24x24 | 32 | `icons/icon_5906_10_24x24.png` |
| inetcpl_cpl_mun | RT_ICON | 321 (group 5906) | 16x16 | 32 | `icons/icon_5906_11_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 322 (group 6601) | 32x32 | 4 | `icons/icon_6601_0_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 323 (group 6609) | 16x16 | 32 | `icons/icon_6609_0_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 324 (group 6610) | 16x16 | 32 | `icons/icon_6610_0_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 325 (group 6611) | 16x16 | 32 | `icons/icon_6611_0_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 326 (group 6623) | 48x48 | 4 | `icons/icon_6623_0_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 327 (group 6623) | 32x32 | 4 | `icons/icon_6623_1_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 328 (group 6623) | 48x48 | 8 | `icons/icon_6623_2_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 329 (group 6623) | 32x32 | 8 | `icons/icon_6623_3_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 33 (group 4443) | 48x48 | 8 | `icons/icon_4443_4_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 330 (group 6623) | 48x48 | 32 | `icons/icon_6623_4_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 331 (group 6623) | 32x32 | 32 | `icons/icon_6623_5_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 332 (group 6627) | 32x32 | 4 | `icons/icon_6627_0_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 333 (group 6627) | 4x4 | 8 | `icons/icon_6627_1_4x4.png` |
| inetcpl_cpl_mun | RT_ICON | 334 (group 6628) | 32x32 | 4 | `icons/icon_6628_0_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 335 (group 6628) | 4x4 | 8 | `icons/icon_6628_1_4x4.png` |
| inetcpl_cpl_mun | RT_ICON | 336 (group 6629) | 32x32 | 4 | `icons/icon_6629_0_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 337 (group 6629) | 4x4 | 8 | `icons/icon_6629_1_4x4.png` |
| inetcpl_cpl_mun | RT_ICON | 338 (group 6630) | 32x32 | 4 | `icons/icon_6630_0_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 339 (group 6630) | 4x4 | 8 | `icons/icon_6630_1_4x4.png` |
| inetcpl_cpl_mun | RT_ICON | 34 (group 4443) | 32x32 | 8 | `icons/icon_4443_5_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 340 (group 5393) | 48x48 | 4 | `icons/icon_5393_0_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 341 (group 5393) | 32x32 | 4 | `icons/icon_5393_1_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 342 (group 5393) | 24x24 | 4 | `icons/icon_5393_2_24x24.png` |
| inetcpl_cpl_mun | RT_ICON | 343 (group 5393) | 16x16 | 4 | `icons/icon_5393_3_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 344 (group 5393) | 48x48 | 8 | `icons/icon_5393_4_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 345 (group 5393) | 32x32 | 8 | `icons/icon_5393_5_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 346 (group 5393) | 24x24 | 8 | `icons/icon_5393_6_24x24.png` |
| inetcpl_cpl_mun | RT_ICON | 347 (group 5393) | 16x16 | 8 | `icons/icon_5393_7_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 348 (group 5393) | 48x48 | 32 | `icons/icon_5393_8_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 349 (group 5393) | 32x32 | 32 | `icons/icon_5393_9_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 35 (group 4443) | 24x24 | 8 | `icons/icon_4443_6_24x24.png` |
| inetcpl_cpl_mun | RT_ICON | 350 (group 5393) | 24x24 | 32 | `icons/icon_5393_10_24x24.png` |
| inetcpl_cpl_mun | RT_ICON | 351 (group 5393) | 16x16 | 32 | `icons/icon_5393_11_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 352 (group 6657) | 32x32 | 4 | `icons/icon_6657_0_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 353 (group 6657) | 24x24 | 4 | `icons/icon_6657_1_24x24.png` |
| inetcpl_cpl_mun | RT_ICON | 354 (group 6657) | 16x16 | 4 | `icons/icon_6657_2_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 355 (group 6657) | 32x32 | 8 | `icons/icon_6657_3_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 356 (group 6657) | 24x24 | 8 | `icons/icon_6657_4_24x24.png` |
| inetcpl_cpl_mun | RT_ICON | 357 (group 6657) | 16x16 | 8 | `icons/icon_6657_5_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 358 (group 6657) | 32x32 | 32 | `icons/icon_6657_6_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 359 (group 6657) | 24x24 | 32 | `icons/icon_6657_7_24x24.png` |
| inetcpl_cpl_mun | RT_ICON | 36 (group 4443) | 16x16 | 8 | `icons/icon_4443_7_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 360 (group 6657) | 16x16 | 32 | `icons/icon_6657_8_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 37 (group 4443) | 256x256 | 32 | `icons/icon_4443_8_256x256.png` |
| inetcpl_cpl_mun | RT_ICON | 38 (group 4443) | 48x48 | 32 | `icons/icon_4443_9_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 39 (group 4443) | 32x32 | 32 | `icons/icon_4443_10_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 4 (group 4487) | 48x48 | 4 | `icons/icon_4487_0_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 40 (group 4443) | 24x24 | 32 | `icons/icon_4443_11_24x24.png` |
| inetcpl_cpl_mun | RT_ICON | 41 (group 4443) | 16x16 | 32 | `icons/icon_4443_12_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 42 (group 4441) | 32x32 | 4 | `icons/icon_4441_0_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 43 (group 4441) | 16x16 | 4 | `icons/icon_4441_1_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 44 (group 4441) | 32x32 | 8 | `icons/icon_4441_2_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 45 (group 4441) | 16x16 | 8 | `icons/icon_4441_3_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 46 (group 4441) | 32x32 | 32 | `icons/icon_4441_4_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 47 (group 4441) | 16x16 | 32 | `icons/icon_4441_5_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 48 (group 4447) | 24x24 | 4 | `icons/icon_4447_0_24x24.png` |
| inetcpl_cpl_mun | RT_ICON | 49 (group 4447) | 16x16 | 4 | `icons/icon_4447_1_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 5 (group 4487) | 32x32 | 4 | `icons/icon_4487_1_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 50 (group 4447) | 24x24 | 8 | `icons/icon_4447_2_24x24.png` |
| inetcpl_cpl_mun | RT_ICON | 51 (group 4447) | 16x16 | 8 | `icons/icon_4447_3_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 52 (group 4447) | 24x24 | 32 | `icons/icon_4447_4_24x24.png` |
| inetcpl_cpl_mun | RT_ICON | 53 (group 4447) | 16x16 | 32 | `icons/icon_4447_5_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 54 (group 4478) | 32x32 | 4 | `icons/icon_4478_0_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 55 (group 4478) | 32x32 | 8 | `icons/icon_4478_1_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 56 (group 4478) | 32x32 | 32 | `icons/icon_4478_2_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 57 (group 4480) | 48x48 | 4 | `icons/icon_4480_0_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 58 (group 4480) | 32x32 | 4 | `icons/icon_4480_1_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 59 (group 4480) | 16x16 | 4 | `icons/icon_4480_2_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 6 (group 4487) | 16x16 | 4 | `icons/icon_4487_2_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 60 (group 4480) | 48x48 | 8 | `icons/icon_4480_3_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 61 (group 4480) | 32x32 | 8 | `icons/icon_4480_4_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 62 (group 4480) | 16x16 | 8 | `icons/icon_4480_5_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 63 (group 4480) | 48x48 | 32 | `icons/icon_4480_6_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 64 (group 4480) | 32x32 | 32 | `icons/icon_4480_7_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 65 (group 4480) | 16x16 | 32 | `icons/icon_4480_8_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 66 (group 4481) | 48x48 | 4 | `icons/icon_4481_0_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 67 (group 4481) | 32x32 | 4 | `icons/icon_4481_1_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 68 (group 4481) | 16x16 | 4 | `icons/icon_4481_2_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 69 (group 4481) | 48x48 | 8 | `icons/icon_4481_3_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 7 (group 4487) | 48x48 | 8 | `icons/icon_4487_3_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 70 (group 4481) | 32x32 | 8 | `icons/icon_4481_4_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 71 (group 4481) | 16x16 | 8 | `icons/icon_4481_5_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 72 (group 4481) | 48x48 | 32 | `icons/icon_4481_6_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 73 (group 4481) | 32x32 | 32 | `icons/icon_4481_7_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 74 (group 4481) | 16x16 | 32 | `icons/icon_4481_8_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 75 (group 4482) | 24x24 | 4 | `icons/icon_4482_0_24x24.png` |
| inetcpl_cpl_mun | RT_ICON | 76 (group 4482) | 16x16 | 4 | `icons/icon_4482_1_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 77 (group 4482) | 24x24 | 8 | `icons/icon_4482_2_24x24.png` |
| inetcpl_cpl_mun | RT_ICON | 78 (group 4482) | 16x16 | 8 | `icons/icon_4482_3_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 79 (group 4482) | 24x24 | 32 | `icons/icon_4482_4_24x24.png` |
| inetcpl_cpl_mun | RT_ICON | 8 (group 4487) | 32x32 | 8 | `icons/icon_4487_4_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 80 (group 4482) | 16x16 | 32 | `icons/icon_4482_5_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 81 (group 4483) | 48x48 | 4 | `icons/icon_4483_0_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 82 (group 4483) | 32x32 | 4 | `icons/icon_4483_1_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 83 (group 4483) | 24x24 | 4 | `icons/icon_4483_2_24x24.png` |
| inetcpl_cpl_mun | RT_ICON | 84 (group 4483) | 16x16 | 4 | `icons/icon_4483_3_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 85 (group 4483) | 48x48 | 8 | `icons/icon_4483_4_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 86 (group 4483) | 32x32 | 8 | `icons/icon_4483_5_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 87 (group 4483) | 24x24 | 8 | `icons/icon_4483_6_24x24.png` |
| inetcpl_cpl_mun | RT_ICON | 88 (group 4483) | 16x16 | 8 | `icons/icon_4483_7_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 89 (group 4483) | 48x48 | 32 | `icons/icon_4483_8_48x48.png` |
| inetcpl_cpl_mun | RT_ICON | 9 (group 4487) | 16x16 | 8 | `icons/icon_4487_5_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 90 (group 4483) | 32x32 | 32 | `icons/icon_4483_9_32x32.png` |
| inetcpl_cpl_mun | RT_ICON | 91 (group 4483) | 24x24 | 32 | `icons/icon_4483_10_24x24.png` |
| inetcpl_cpl_mun | RT_ICON | 92 (group 4483) | 16x16 | 32 | `icons/icon_4483_11_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 93 (group 4485) | 16x16 | 4 | `icons/icon_4485_0_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 94 (group 4485) | 16x16 | 8 | `icons/icon_4485_1_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 95 (group 4485) | 16x16 | 32 | `icons/icon_4485_2_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 96 (group 4486) | 24x24 | 4 | `icons/icon_4486_0_24x24.png` |
| inetcpl_cpl_mun | RT_ICON | 97 (group 4486) | 16x16 | 4 | `icons/icon_4486_1_16x16.png` |
| inetcpl_cpl_mun | RT_ICON | 98 (group 4486) | 24x24 | 8 | `icons/icon_4486_2_24x24.png` |
| inetcpl_cpl_mun | RT_ICON | 99 (group 4486) | 16x16 | 8 | `icons/icon_4486_3_16x16.png` |
| mmsys_cpl_mun | RT_BITMAP | 221 | 80x16 | 24 | `bitmaps/bitmap_221.png` |
| mmsys_cpl_mun | RT_BITMAP | 223 | 22x33 | 24 | `bitmaps/bitmap_223.png` |
| mmsys_cpl_mun | RT_BITMAP | 225 | 60x20 | 24 | `bitmaps/bitmap_225.png` |
| mmsys_cpl_mun | RT_BITMAP | 250 | 122x23 | 4 | `bitmaps/bitmap_250.png` |
| mmsys_cpl_mun | RT_GROUP_ICON | 100 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_100.ico` |
| mmsys_cpl_mun | RT_GROUP_ICON | 110 | 32x32+32x32+32x32 | 4,8,32 | `icons/groupicon_110.ico` |
| mmsys_cpl_mun | RT_GROUP_ICON | 111 | 32x32+32x32+32x32 | 4,8,32 | `icons/groupicon_111.ico` |
| mmsys_cpl_mun | RT_GROUP_ICON | 112 | 16x16+24x24+20x20 | 8,32,32 | `icons/groupicon_112.ico` |
| mmsys_cpl_mun | RT_GROUP_ICON | 113 | 16x16+24x24+20x20 | 8,32,32 | `icons/groupicon_113.ico` |
| mmsys_cpl_mun | RT_GROUP_ICON | 114 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_114.ico` |
| mmsys_cpl_mun | RT_GROUP_ICON | 115 | 48x48+40x40+32x32+24x24 | 32,32,32,32 | `icons/groupicon_115.ico` |
| mmsys_cpl_mun | RT_GROUP_ICON | 3004 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_3004.ico` |
| mmsys_cpl_mun | RT_GROUP_ICON | 4000 | 16x16+32x32 | 4,4 | `icons/groupicon_4000.ico` |
| mmsys_cpl_mun | RT_ICON | 1 (group 100) | 256x256 | 32 | `icons/icon_100_0_256x256.png` |
| mmsys_cpl_mun | RT_ICON | 10 (group 110) | 32x32 | 8 | `icons/icon_110_1_32x32.png` |
| mmsys_cpl_mun | RT_ICON | 11 (group 110) | 32x32 | 32 | `icons/icon_110_2_32x32.png` |
| mmsys_cpl_mun | RT_ICON | 12 (group 111) | 32x32 | 4 | `icons/icon_111_0_32x32.png` |
| mmsys_cpl_mun | RT_ICON | 13 (group 111) | 32x32 | 8 | `icons/icon_111_1_32x32.png` |
| mmsys_cpl_mun | RT_ICON | 14 (group 111) | 32x32 | 32 | `icons/icon_111_2_32x32.png` |
| mmsys_cpl_mun | RT_ICON | 15 (group 112) | 16x16 | 8 | `icons/icon_112_0_16x16.png` |
| mmsys_cpl_mun | RT_ICON | 16 (group 112) | 24x24 | 32 | `icons/icon_112_1_24x24.png` |
| mmsys_cpl_mun | RT_ICON | 17 (group 112) | 20x20 | 32 | `icons/icon_112_2_20x20.png` |
| mmsys_cpl_mun | RT_ICON | 18 (group 113) | 16x16 | 8 | `icons/icon_113_0_16x16.png` |
| mmsys_cpl_mun | RT_ICON | 19 (group 113) | 24x24 | 32 | `icons/icon_113_1_24x24.png` |
| mmsys_cpl_mun | RT_ICON | 2 (group 100) | 64x64 | 32 | `icons/icon_100_1_64x64.png` |
| mmsys_cpl_mun | RT_ICON | 20 (group 113) | 20x20 | 32 | `icons/icon_113_2_20x20.png` |
| mmsys_cpl_mun | RT_ICON | 21 (group 114) | 256x256 | 32 | `icons/icon_114_0_256x256.png` |
| mmsys_cpl_mun | RT_ICON | 22 (group 114) | 64x64 | 32 | `icons/icon_114_1_64x64.png` |
| mmsys_cpl_mun | RT_ICON | 23 (group 114) | 48x48 | 32 | `icons/icon_114_2_48x48.png` |
| mmsys_cpl_mun | RT_ICON | 24 (group 114) | 40x40 | 32 | `icons/icon_114_3_40x40.png` |
| mmsys_cpl_mun | RT_ICON | 25 (group 114) | 32x32 | 32 | `icons/icon_114_4_32x32.png` |
| mmsys_cpl_mun | RT_ICON | 26 (group 114) | 24x24 | 32 | `icons/icon_114_5_24x24.png` |
| mmsys_cpl_mun | RT_ICON | 27 (group 114) | 20x20 | 32 | `icons/icon_114_6_20x20.png` |
| mmsys_cpl_mun | RT_ICON | 28 (group 114) | 16x16 | 32 | `icons/icon_114_7_16x16.png` |
| mmsys_cpl_mun | RT_ICON | 29 (group 4000) | 16x16 | 4 | `icons/icon_4000_0_16x16.png` |
| mmsys_cpl_mun | RT_ICON | 3 (group 100) | 48x48 | 32 | `icons/icon_100_2_48x48.png` |
| mmsys_cpl_mun | RT_ICON | 30 (group 4000) | 32x32 | 4 | `icons/icon_4000_1_32x32.png` |
| mmsys_cpl_mun | RT_ICON | 31 (group 115) | 48x48 | 32 | `icons/icon_115_0_48x48.png` |
| mmsys_cpl_mun | RT_ICON | 32 (group 115) | 40x40 | 32 | `icons/icon_115_1_40x40.png` |
| mmsys_cpl_mun | RT_ICON | 33 (group 115) | 32x32 | 32 | `icons/icon_115_2_32x32.png` |
| mmsys_cpl_mun | RT_ICON | 34 (group 115) | 24x24 | 32 | `icons/icon_115_3_24x24.png` |
| mmsys_cpl_mun | RT_ICON | 35 (group 3004) | 256x256 | 32 | `icons/icon_3004_0_256x256.png` |
| mmsys_cpl_mun | RT_ICON | 36 (group 3004) | 64x64 | 32 | `icons/icon_3004_1_64x64.png` |
| mmsys_cpl_mun | RT_ICON | 37 (group 3004) | 48x48 | 32 | `icons/icon_3004_2_48x48.png` |
| mmsys_cpl_mun | RT_ICON | 38 (group 3004) | 40x40 | 32 | `icons/icon_3004_3_40x40.png` |
| mmsys_cpl_mun | RT_ICON | 39 (group 3004) | 32x32 | 32 | `icons/icon_3004_4_32x32.png` |
| mmsys_cpl_mun | RT_ICON | 4 (group 100) | 40x40 | 32 | `icons/icon_100_3_40x40.png` |
| mmsys_cpl_mun | RT_ICON | 40 (group 3004) | 24x24 | 32 | `icons/icon_3004_5_24x24.png` |
| mmsys_cpl_mun | RT_ICON | 41 (group 3004) | 20x20 | 32 | `icons/icon_3004_6_20x20.png` |
| mmsys_cpl_mun | RT_ICON | 42 (group 3004) | 16x16 | 32 | `icons/icon_3004_7_16x16.png` |
| mmsys_cpl_mun | RT_ICON | 5 (group 100) | 32x32 | 32 | `icons/icon_100_4_32x32.png` |
| mmsys_cpl_mun | RT_ICON | 6 (group 100) | 24x24 | 32 | `icons/icon_100_5_24x24.png` |
| mmsys_cpl_mun | RT_ICON | 7 (group 100) | 20x20 | 32 | `icons/icon_100_6_20x20.png` |
| mmsys_cpl_mun | RT_ICON | 8 (group 100) | 16x16 | 32 | `icons/icon_100_7_16x16.png` |
| mmsys_cpl_mun | RT_ICON | 9 (group 110) | 32x32 | 4 | `icons/icon_110_0_32x32.png` |
| printui_mun | RT_BITMAP | 426 | 164x628 | 8 | `bitmaps/bitmap_426.png` |
| printui_mun | RT_BITMAP | 427 | 49x49 | 4 | `bitmaps/bitmap_427.png` |
| printui_mun | RT_BITMAP | 429 | 120x226 | 8 | `bitmaps/bitmap_429.png` |
| printui_mun | RT_GROUP_ICON | 132 | 48x48+32x32+16x16+48x48+32x32+16x16+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_132.ico` |
| printui_mun | RT_GROUP_ICON | 420 | 48x48+32x32+16x16+48x48+32x32+16x16+256x256+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32,32 | `icons/groupicon_420.ico` |
| printui_mun | RT_GROUP_ICON | 421 | 32x32+32x32 | 4,1 | `icons/groupicon_421.ico` |
| printui_mun | RT_GROUP_ICON | 422 | 32x32+32x32 | 4,1 | `icons/groupicon_422.ico` |
| printui_mun | RT_GROUP_ICON | 423 | 32x32+32x32 | 4,1 | `icons/groupicon_423.ico` |
| printui_mun | RT_GROUP_ICON | 424 | 32x32+16x16+48x48+16x16+32x32+48x48 | 4,4,4,8,8,8 | `icons/groupicon_424.ico` |
| printui_mun | RT_GROUP_ICON | 425 | 32x32+16x16+32x32+16x16+32x32+16x16 | 4,4,8,8,32,32 | `icons/groupicon_425.ico` |
| printui_mun | RT_GROUP_ICON | 433 | 32x32 | 4 | `icons/groupicon_433.ico` |
| printui_mun | RT_GROUP_ICON | 434 | 48x48+32x32+16x16+48x48+32x32+16x16+256x256+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32,32 | `icons/groupicon_434.ico` |
| printui_mun | RT_ICON | 1 (group 132) | 48x48 | 4 | `icons/icon_132_0_48x48.png` |
| printui_mun | RT_ICON | 10 (group 420) | 48x48 | 4 | `icons/icon_420_0_48x48.png` |
| printui_mun | RT_ICON | 11 (group 420) | 32x32 | 4 | `icons/icon_420_1_32x32.png` |
| printui_mun | RT_ICON | 12 (group 420) | 16x16 | 4 | `icons/icon_420_2_16x16.png` |
| printui_mun | RT_ICON | 13 (group 420) | 48x48 | 8 | `icons/icon_420_3_48x48.png` |
| printui_mun | RT_ICON | 14 (group 420) | 32x32 | 8 | `icons/icon_420_4_32x32.png` |
| printui_mun | RT_ICON | 15 (group 420) | 16x16 | 8 | `icons/icon_420_5_16x16.png` |
| printui_mun | RT_ICON | 16 (group 420) | 256x256 | 32 | `icons/icon_420_6_256x256.png` |
| printui_mun | RT_ICON | 17 (group 420) | 48x48 | 32 | `icons/icon_420_7_48x48.png` |
| printui_mun | RT_ICON | 18 (group 420) | 32x32 | 32 | `icons/icon_420_8_32x32.png` |
| printui_mun | RT_ICON | 19 (group 420) | 16x16 | 32 | `icons/icon_420_9_16x16.png` |
| printui_mun | RT_ICON | 2 (group 132) | 32x32 | 4 | `icons/icon_132_1_32x32.png` |
| printui_mun | RT_ICON | 20 (group 421) | 32x32 | 4 | `icons/icon_421_0_32x32.png` |
| printui_mun | RT_ICON | 21 (group 421) | 32x32 | 1 | `icons/icon_421_1_32x32.png` |
| printui_mun | RT_ICON | 22 (group 422) | 32x32 | 4 | `icons/icon_422_0_32x32.png` |
| printui_mun | RT_ICON | 23 (group 422) | 32x32 | 1 | `icons/icon_422_1_32x32.png` |
| printui_mun | RT_ICON | 24 (group 423) | 32x32 | 4 | `icons/icon_423_0_32x32.png` |
| printui_mun | RT_ICON | 25 (group 423) | 32x32 | 1 | `icons/icon_423_1_32x32.png` |
| printui_mun | RT_ICON | 26 (group 424) | 32x32 | 4 | `icons/icon_424_0_32x32.png` |
| printui_mun | RT_ICON | 27 (group 424) | 16x16 | 4 | `icons/icon_424_1_16x16.png` |
| printui_mun | RT_ICON | 28 (group 424) | 48x48 | 4 | `icons/icon_424_2_48x48.png` |
| printui_mun | RT_ICON | 29 (group 424) | 16x16 | 8 | `icons/icon_424_3_16x16.png` |
| printui_mun | RT_ICON | 3 (group 132) | 16x16 | 4 | `icons/icon_132_2_16x16.png` |
| printui_mun | RT_ICON | 30 (group 424) | 32x32 | 8 | `icons/icon_424_4_32x32.png` |
| printui_mun | RT_ICON | 31 (group 424) | 48x48 | 8 | `icons/icon_424_5_48x48.png` |
| printui_mun | RT_ICON | 32 (group 425) | 32x32 | 4 | `icons/icon_425_0_32x32.png` |
| printui_mun | RT_ICON | 33 (group 425) | 16x16 | 4 | `icons/icon_425_1_16x16.png` |
| printui_mun | RT_ICON | 34 (group 425) | 32x32 | 8 | `icons/icon_425_2_32x32.png` |
| printui_mun | RT_ICON | 35 (group 425) | 16x16 | 8 | `icons/icon_425_3_16x16.png` |
| printui_mun | RT_ICON | 36 (group 425) | 32x32 | 32 | `icons/icon_425_4_32x32.png` |
| printui_mun | RT_ICON | 37 (group 425) | 16x16 | 32 | `icons/icon_425_5_16x16.png` |
| printui_mun | RT_ICON | 38 (group 433) | 32x32 | 4 | `icons/icon_433_0_32x32.png` |
| printui_mun | RT_ICON | 39 (group 434) | 48x48 | 4 | `icons/icon_434_0_48x48.png` |
| printui_mun | RT_ICON | 4 (group 132) | 48x48 | 8 | `icons/icon_132_3_48x48.png` |
| printui_mun | RT_ICON | 40 (group 434) | 32x32 | 4 | `icons/icon_434_1_32x32.png` |
| printui_mun | RT_ICON | 41 (group 434) | 16x16 | 4 | `icons/icon_434_2_16x16.png` |
| printui_mun | RT_ICON | 42 (group 434) | 48x48 | 8 | `icons/icon_434_3_48x48.png` |
| printui_mun | RT_ICON | 43 (group 434) | 32x32 | 8 | `icons/icon_434_4_32x32.png` |
| printui_mun | RT_ICON | 44 (group 434) | 16x16 | 8 | `icons/icon_434_5_16x16.png` |
| printui_mun | RT_ICON | 45 (group 434) | 256x256 | 32 | `icons/icon_434_6_256x256.png` |
| printui_mun | RT_ICON | 46 (group 434) | 48x48 | 32 | `icons/icon_434_7_48x48.png` |
| printui_mun | RT_ICON | 47 (group 434) | 32x32 | 32 | `icons/icon_434_8_32x32.png` |
| printui_mun | RT_ICON | 48 (group 434) | 16x16 | 32 | `icons/icon_434_9_16x16.png` |
| printui_mun | RT_ICON | 5 (group 132) | 32x32 | 8 | `icons/icon_132_4_32x32.png` |
| printui_mun | RT_ICON | 6 (group 132) | 16x16 | 8 | `icons/icon_132_5_16x16.png` |
| printui_mun | RT_ICON | 7 (group 132) | 48x48 | 32 | `icons/icon_132_6_48x48.png` |
| printui_mun | RT_ICON | 8 (group 132) | 32x32 | 32 | `icons/icon_132_7_32x32.png` |
| printui_mun | RT_ICON | 9 (group 132) | 16x16 | 32 | `icons/icon_132_8_16x16.png` |
| wscui_cpl_mun | RT_GROUP_ICON | 5000 | 128x128+96x96+64x64+48x48+32x32+24x24+16x16+128x128+96x96+64x64+48x48+32x32+24x24+16x16+256x256+128x128+96x96+64x64+48x48+32x32+24x24+16x16 | 4,4,4,4,4,4,4,8,8,8,8,8,8,8,32,32,32,32,32,32,32,32 | `icons/groupicon_5000.ico` |
| wscui_cpl_mun | RT_GROUP_ICON | 5001 | 128x128+96x96+64x64+48x48+32x32+24x24+16x16+128x128+96x96+64x64+48x48+32x32+24x24+16x16+256x256+128x128+96x96+64x64+48x48+32x32+24x24+16x16 | 4,4,4,4,4,4,4,8,8,8,8,8,8,8,32,32,32,32,32,32,32,32 | `icons/groupicon_5001.ico` |
| wscui_cpl_mun | RT_GROUP_ICON | 5002 | 48x48+32x32+24x24+16x16+48x48+32x32+24x24+16x16+256x256+48x48+32x32+24x24+16x16 | 4,4,4,4,8,8,8,8,32,32,32,32,32 | `icons/groupicon_5002.ico` |
| wscui_cpl_mun | RT_GROUP_ICON | 5003 | 48x48+32x32+16x16+48x48+32x32+16x16+256x256+48x48+32x32+16x16 | 4,4,4,8,8,8,32,32,32,32 | `icons/groupicon_5003.ico` |
| wscui_cpl_mun | RT_GROUP_ICON | 5004 | 128x128+96x96+80x80+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,24,24,24,24,24,24,24,24,24 | `icons/groupicon_5004.ico` |
| wscui_cpl_mun | RT_GROUP_ICON | 5005 | 256x256+128x128+48x48+32x32+24x24+16x16 | 32,32,32,32,32,32 | `icons/groupicon_5005.ico` |
| wscui_cpl_mun | RT_ICON | 1 (group 5000) | 128x128 | 4 | `icons/icon_5000_0_128x128.png` |
| wscui_cpl_mun | RT_ICON | 10 (group 5000) | 64x64 | 8 | `icons/icon_5000_9_64x64.png` |
| wscui_cpl_mun | RT_ICON | 11 (group 5000) | 48x48 | 8 | `icons/icon_5000_10_48x48.png` |
| wscui_cpl_mun | RT_ICON | 12 (group 5000) | 32x32 | 8 | `icons/icon_5000_11_32x32.png` |
| wscui_cpl_mun | RT_ICON | 13 (group 5000) | 24x24 | 8 | `icons/icon_5000_12_24x24.png` |
| wscui_cpl_mun | RT_ICON | 14 (group 5000) | 16x16 | 8 | `icons/icon_5000_13_16x16.png` |
| wscui_cpl_mun | RT_ICON | 15 (group 5000) | 256x256 | 32 | `icons/icon_5000_14_256x256.png` |
| wscui_cpl_mun | RT_ICON | 16 (group 5000) | 128x128 | 32 | `icons/icon_5000_15_128x128.png` |
| wscui_cpl_mun | RT_ICON | 17 (group 5000) | 96x96 | 32 | `icons/icon_5000_16_96x96.png` |
| wscui_cpl_mun | RT_ICON | 18 (group 5000) | 64x64 | 32 | `icons/icon_5000_17_64x64.png` |
| wscui_cpl_mun | RT_ICON | 19 (group 5000) | 48x48 | 32 | `icons/icon_5000_18_48x48.png` |
| wscui_cpl_mun | RT_ICON | 2 (group 5000) | 96x96 | 4 | `icons/icon_5000_1_96x96.png` |
| wscui_cpl_mun | RT_ICON | 20 (group 5000) | 32x32 | 32 | `icons/icon_5000_19_32x32.png` |
| wscui_cpl_mun | RT_ICON | 21 (group 5000) | 24x24 | 32 | `icons/icon_5000_20_24x24.png` |
| wscui_cpl_mun | RT_ICON | 22 (group 5000) | 16x16 | 32 | `icons/icon_5000_21_16x16.png` |
| wscui_cpl_mun | RT_ICON | 23 (group 5001) | 128x128 | 4 | `icons/icon_5001_0_128x128.png` |
| wscui_cpl_mun | RT_ICON | 24 (group 5001) | 96x96 | 4 | `icons/icon_5001_1_96x96.png` |
| wscui_cpl_mun | RT_ICON | 25 (group 5001) | 64x64 | 4 | `icons/icon_5001_2_64x64.png` |
| wscui_cpl_mun | RT_ICON | 26 (group 5001) | 48x48 | 4 | `icons/icon_5001_3_48x48.png` |
| wscui_cpl_mun | RT_ICON | 27 (group 5001) | 32x32 | 4 | `icons/icon_5001_4_32x32.png` |
| wscui_cpl_mun | RT_ICON | 28 (group 5001) | 24x24 | 4 | `icons/icon_5001_5_24x24.png` |
| wscui_cpl_mun | RT_ICON | 29 (group 5001) | 16x16 | 4 | `icons/icon_5001_6_16x16.png` |
| wscui_cpl_mun | RT_ICON | 3 (group 5000) | 64x64 | 4 | `icons/icon_5000_2_64x64.png` |
| wscui_cpl_mun | RT_ICON | 30 (group 5001) | 128x128 | 8 | `icons/icon_5001_7_128x128.png` |
| wscui_cpl_mun | RT_ICON | 31 (group 5001) | 96x96 | 8 | `icons/icon_5001_8_96x96.png` |
| wscui_cpl_mun | RT_ICON | 32 (group 5001) | 64x64 | 8 | `icons/icon_5001_9_64x64.png` |
| wscui_cpl_mun | RT_ICON | 33 (group 5001) | 48x48 | 8 | `icons/icon_5001_10_48x48.png` |
| wscui_cpl_mun | RT_ICON | 34 (group 5001) | 32x32 | 8 | `icons/icon_5001_11_32x32.png` |
| wscui_cpl_mun | RT_ICON | 35 (group 5001) | 24x24 | 8 | `icons/icon_5001_12_24x24.png` |
| wscui_cpl_mun | RT_ICON | 36 (group 5001) | 16x16 | 8 | `icons/icon_5001_13_16x16.png` |
| wscui_cpl_mun | RT_ICON | 37 (group 5001) | 256x256 | 32 | `icons/icon_5001_14_256x256.png` |
| wscui_cpl_mun | RT_ICON | 38 (group 5001) | 128x128 | 32 | `icons/icon_5001_15_128x128.png` |
| wscui_cpl_mun | RT_ICON | 39 (group 5001) | 96x96 | 32 | `icons/icon_5001_16_96x96.png` |
| wscui_cpl_mun | RT_ICON | 4 (group 5000) | 48x48 | 4 | `icons/icon_5000_3_48x48.png` |
| wscui_cpl_mun | RT_ICON | 40 (group 5001) | 64x64 | 32 | `icons/icon_5001_17_64x64.png` |
| wscui_cpl_mun | RT_ICON | 41 (group 5001) | 48x48 | 32 | `icons/icon_5001_18_48x48.png` |
| wscui_cpl_mun | RT_ICON | 42 (group 5001) | 32x32 | 32 | `icons/icon_5001_19_32x32.png` |
| wscui_cpl_mun | RT_ICON | 43 (group 5001) | 24x24 | 32 | `icons/icon_5001_20_24x24.png` |
| wscui_cpl_mun | RT_ICON | 44 (group 5001) | 16x16 | 32 | `icons/icon_5001_21_16x16.png` |
| wscui_cpl_mun | RT_ICON | 45 (group 5002) | 48x48 | 4 | `icons/icon_5002_0_48x48.png` |
| wscui_cpl_mun | RT_ICON | 46 (group 5002) | 32x32 | 4 | `icons/icon_5002_1_32x32.png` |
| wscui_cpl_mun | RT_ICON | 47 (group 5002) | 24x24 | 4 | `icons/icon_5002_2_24x24.png` |
| wscui_cpl_mun | RT_ICON | 48 (group 5002) | 16x16 | 4 | `icons/icon_5002_3_16x16.png` |
| wscui_cpl_mun | RT_ICON | 49 (group 5002) | 48x48 | 8 | `icons/icon_5002_4_48x48.png` |
| wscui_cpl_mun | RT_ICON | 5 (group 5000) | 32x32 | 4 | `icons/icon_5000_4_32x32.png` |
| wscui_cpl_mun | RT_ICON | 50 (group 5002) | 32x32 | 8 | `icons/icon_5002_5_32x32.png` |
| wscui_cpl_mun | RT_ICON | 51 (group 5002) | 24x24 | 8 | `icons/icon_5002_6_24x24.png` |
| wscui_cpl_mun | RT_ICON | 52 (group 5002) | 16x16 | 8 | `icons/icon_5002_7_16x16.png` |
| wscui_cpl_mun | RT_ICON | 53 (group 5002) | 256x256 | 32 | `icons/icon_5002_8_256x256.png` |
| wscui_cpl_mun | RT_ICON | 54 (group 5002) | 48x48 | 32 | `icons/icon_5002_9_48x48.png` |
| wscui_cpl_mun | RT_ICON | 55 (group 5002) | 32x32 | 32 | `icons/icon_5002_10_32x32.png` |
| wscui_cpl_mun | RT_ICON | 56 (group 5002) | 24x24 | 32 | `icons/icon_5002_11_24x24.png` |
| wscui_cpl_mun | RT_ICON | 57 (group 5002) | 16x16 | 32 | `icons/icon_5002_12_16x16.png` |
| wscui_cpl_mun | RT_ICON | 58 (group 5003) | 48x48 | 4 | `icons/icon_5003_0_48x48.png` |
| wscui_cpl_mun | RT_ICON | 59 (group 5003) | 32x32 | 4 | `icons/icon_5003_1_32x32.png` |
| wscui_cpl_mun | RT_ICON | 6 (group 5000) | 24x24 | 4 | `icons/icon_5000_5_24x24.png` |
| wscui_cpl_mun | RT_ICON | 60 (group 5003) | 16x16 | 4 | `icons/icon_5003_2_16x16.png` |
| wscui_cpl_mun | RT_ICON | 61 (group 5003) | 48x48 | 8 | `icons/icon_5003_3_48x48.png` |
| wscui_cpl_mun | RT_ICON | 62 (group 5003) | 32x32 | 8 | `icons/icon_5003_4_32x32.png` |
| wscui_cpl_mun | RT_ICON | 63 (group 5003) | 16x16 | 8 | `icons/icon_5003_5_16x16.png` |
| wscui_cpl_mun | RT_ICON | 64 (group 5003) | 256x256 | 32 | `icons/icon_5003_6_256x256.png` |
| wscui_cpl_mun | RT_ICON | 65 (group 5003) | 48x48 | 32 | `icons/icon_5003_7_48x48.png` |
| wscui_cpl_mun | RT_ICON | 66 (group 5003) | 32x32 | 32 | `icons/icon_5003_8_32x32.png` |
| wscui_cpl_mun | RT_ICON | 67 (group 5003) | 16x16 | 32 | `icons/icon_5003_9_16x16.png` |
| wscui_cpl_mun | RT_ICON | 68 (group 5004) | 128x128 | 32 | `icons/icon_5004_0_128x128.png` |
| wscui_cpl_mun | RT_ICON | 69 (group 5004) | 96x96 | 24 | `icons/icon_5004_1_96x96.png` |
| wscui_cpl_mun | RT_ICON | 7 (group 5000) | 16x16 | 4 | `icons/icon_5000_6_16x16.png` |
| wscui_cpl_mun | RT_ICON | 70 (group 5004) | 80x80 | 24 | `icons/icon_5004_2_80x80.png` |
| wscui_cpl_mun | RT_ICON | 71 (group 5004) | 64x64 | 24 | `icons/icon_5004_3_64x64.png` |
| wscui_cpl_mun | RT_ICON | 72 (group 5004) | 48x48 | 24 | `icons/icon_5004_4_48x48.png` |
| wscui_cpl_mun | RT_ICON | 73 (group 5004) | 40x40 | 24 | `icons/icon_5004_5_40x40.png` |
| wscui_cpl_mun | RT_ICON | 74 (group 5004) | 32x32 | 24 | `icons/icon_5004_6_32x32.png` |
| wscui_cpl_mun | RT_ICON | 75 (group 5004) | 24x24 | 24 | `icons/icon_5004_7_24x24.png` |
| wscui_cpl_mun | RT_ICON | 76 (group 5004) | 20x20 | 24 | `icons/icon_5004_8_20x20.png` |
| wscui_cpl_mun | RT_ICON | 77 (group 5004) | 16x16 | 24 | `icons/icon_5004_9_16x16.png` |
| wscui_cpl_mun | RT_ICON | 78 (group 5005) | 256x256 | 32 | `icons/icon_5005_0_256x256.png` |
| wscui_cpl_mun | RT_ICON | 79 (group 5005) | 128x128 | 32 | `icons/icon_5005_1_128x128.png` |
| wscui_cpl_mun | RT_ICON | 8 (group 5000) | 128x128 | 8 | `icons/icon_5000_7_128x128.png` |
| wscui_cpl_mun | RT_ICON | 80 (group 5005) | 48x48 | 32 | `icons/icon_5005_2_48x48.png` |
| wscui_cpl_mun | RT_ICON | 81 (group 5005) | 32x32 | 32 | `icons/icon_5005_3_32x32.png` |
| wscui_cpl_mun | RT_ICON | 82 (group 5005) | 24x24 | 32 | `icons/icon_5005_4_24x24.png` |
| wscui_cpl_mun | RT_ICON | 83 (group 5005) | 16x16 | 32 | `icons/icon_5005_5_16x16.png` |
| wscui_cpl_mun | RT_ICON | 9 (group 5000) | 96x96 | 8 | `icons/icon_5000_8_96x96.png` |
| themecpl_mun | RT_GROUP_ICON | 1 | 256x256+64x64+48x48+40x40+32x32+24x24+20x20+16x16 | 32,32,32,32,32,32,32,32 | `icons/groupicon_1.ico` |
| themecpl_mun | RT_GROUP_ICON | 2 | 48x48+32x32+48x48+32x32+48x48+32x32 | 4,4,8,8,32,32 | `icons/groupicon_2.ico` |
| themecpl_mun | RT_ICON | 1 (group 1) | 256x256 | 32 | `icons/icon_1_0_256x256.png` |
| themecpl_mun | RT_ICON | 10 (group 2) | 32x32 | 4 | `icons/icon_2_1_32x32.png` |
| themecpl_mun | RT_ICON | 11 (group 2) | 48x48 | 8 | `icons/icon_2_2_48x48.png` |
| themecpl_mun | RT_ICON | 12 (group 2) | 32x32 | 8 | `icons/icon_2_3_32x32.png` |
| themecpl_mun | RT_ICON | 13 (group 2) | 48x48 | 32 | `icons/icon_2_4_48x48.png` |
| themecpl_mun | RT_ICON | 14 (group 2) | 32x32 | 32 | `icons/icon_2_5_32x32.png` |
| themecpl_mun | RT_ICON | 2 (group 1) | 64x64 | 32 | `icons/icon_1_1_64x64.png` |
| themecpl_mun | RT_ICON | 3 (group 1) | 48x48 | 32 | `icons/icon_1_2_48x48.png` |
| themecpl_mun | RT_ICON | 4 (group 1) | 40x40 | 32 | `icons/icon_1_3_40x40.png` |
| themecpl_mun | RT_ICON | 5 (group 1) | 32x32 | 32 | `icons/icon_1_4_32x32.png` |
| themecpl_mun | RT_ICON | 6 (group 1) | 24x24 | 32 | `icons/icon_1_5_24x24.png` |
| themecpl_mun | RT_ICON | 7 (group 1) | 20x20 | 32 | `icons/icon_1_6_20x20.png` |
| themecpl_mun | RT_ICON | 8 (group 1) | 16x16 | 32 | `icons/icon_1_7_16x16.png` |
| themecpl_mun | RT_ICON | 9 (group 2) | 48x48 | 4 | `icons/icon_2_0_48x48.png` |
| mspaint_exe_mun | RT_BITMAP | 102 | 186x15 | 4 | `bitmaps/bitmap_102.png` |
| mspaint_exe_mun | RT_BITMAP | 107 | 18x12 | 4 | `bitmaps/bitmap_107.png` |
| mspaint_exe_mun | RT_BITMAP | 111 | 18x12 | 4 | `bitmaps/bitmap_111.png` |
| mspaint_exe_mun | RT_BITMAP | 137 | 20x12 | 4 | `bitmaps/bitmap_137.png` |
| mspaint_exe_mun | RT_BITMAP | 138 | 20x12 | 4 | `bitmaps/bitmap_138.png` |
| mspaint_exe_mun | RT_BITMAP | 139 | 20x12 | 4 | `bitmaps/bitmap_139.png` |
| mspaint_exe_mun | RT_BITMAP | 3550 | 15x15 | 1 | `bitmaps/bitmap_3550.png` |
| mspaint_exe_mun | RT_BITMAP | 3551 | 15x15 | 1 | `bitmaps/bitmap_3551.png` |
| mspaint_exe_mun | RT_BITMAP | 38001 | 16x16 | 32 | `bitmaps/bitmap_38001.png` |
| mspaint_exe_mun | RT_BITMAP | 38002 | 20x20 | 32 | `bitmaps/bitmap_38002.png` |
| mspaint_exe_mun | RT_BITMAP | 38003 | 24x24 | 32 | `bitmaps/bitmap_38003.png` |
| mspaint_exe_mun | RT_BITMAP | 38004 | 32x32 | 32 | `bitmaps/bitmap_38004.png` |
| mspaint_exe_mun | RT_BITMAP | 38005 | 16x16 | 32 | `bitmaps/bitmap_38005.png` |
| mspaint_exe_mun | RT_BITMAP | 38006 | 20x20 | 32 | `bitmaps/bitmap_38006.png` |
| mspaint_exe_mun | RT_BITMAP | 38007 | 24x24 | 32 | `bitmaps/bitmap_38007.png` |
| mspaint_exe_mun | RT_BITMAP | 38008 | 32x32 | 32 | `bitmaps/bitmap_38008.png` |
| mspaint_exe_mun | RT_BITMAP | 38009 | 16x16 | 4 | `bitmaps/bitmap_38009.png` |
| mspaint_exe_mun | RT_BITMAP | 38010 | 16x16 | 4 | `bitmaps/bitmap_38010.png` |
| mspaint_exe_mun | RT_BITMAP | 38011 | 16x16 | 4 | `bitmaps/bitmap_38011.png` |
| mspaint_exe_mun | RT_BITMAP | 8021 | 5x5 | 1 | `bitmaps/bitmap_8021.png` |
| mspaint_exe_mun | RT_BITMAP | 8022 | 5x5 | 1 | `bitmaps/bitmap_8022.png` |
| mspaint_exe_mun | RT_BITMAP | 851 | 1x1 | 1 | `bitmaps/bitmap_851.png` |
| mspaint_exe_mun | RT_BITMAP | 852 | 2x2 | 1 | `bitmaps/bitmap_852.png` |
| mspaint_exe_mun | RT_BITMAP | 853 | 3x3 | 1 | `bitmaps/bitmap_853.png` |
| mspaint_exe_mun | RT_BITMAP | 854 | 4x4 | 1 | `bitmaps/bitmap_854.png` |
| mspaint_exe_mun | RT_BITMAP | 855 | 5x5 | 1 | `bitmaps/bitmap_855.png` |
| mspaint_exe_mun | RT_BITMAP | 856 | 6x6 | 1 | `bitmaps/bitmap_856.png` |
| mspaint_exe_mun | RT_BITMAP | 857 | 7x7 | 1 | `bitmaps/bitmap_857.png` |
| mspaint_exe_mun | RT_BITMAP | 858 | 8x8 | 1 | `bitmaps/bitmap_858.png` |
| mspaint_exe_mun | RT_BITMAP | 859 | 624x16 | 24 | `bitmaps/bitmap_859.png` |
| mspaint_exe_mun | RT_BITMAP | 863 | 72x23 | 8 | `bitmaps/bitmap_863.png` |
| mspaint_exe_mun | RT_BITMAP | 864 | 72x24 | 1 | `bitmaps/bitmap_864.png` |
| mspaint_exe_mun | RT_BITMAP | 9248 | 14x59 | 24 | `bitmaps/bitmap_9248.png` |
| mspaint_exe_mun | RT_GROUP_ICON | 114 | 32x32 | 4 | `icons/groupicon_114.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 115 | 32x32 | 4 | `icons/groupicon_115.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 116 | 32x32 | 4 | `icons/groupicon_116.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 117 | 32x32 | 4 | `icons/groupicon_117.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 2 | 64x64+48x48+32x32+24x24+16x16+64x64+48x48+32x32+24x24+16x16+256x256+64x64+48x48+32x32+24x24+16x16 | 4,4,4,4,4,8,8,8,8,8,32,32,32,32,32,32 | `icons/groupicon_2.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 3 | 32x32+48x48+16x16+32x32+48x48+16x16 | 4,4,4,8,8,8 | `icons/groupicon_3.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 50134 | 32x32+24x24+16x16+32x32+24x24+16x16+32x32+24x24+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_50134.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 50135 | 32x32+24x24+16x16+32x32+24x24+16x16+32x32+24x24+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_50135.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 50136 | 32x32+24x24+16x16+32x32+24x24+16x16+32x32+24x24+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_50136.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 50137 | 32x32+24x24+16x16+32x32+24x24+16x16+32x32+24x24+16x16 | 4,4,4,8,8,8,32,32,32 | `icons/groupicon_50137.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 6401 | 24x24+20x20+16x16 | 32,32,32 | `icons/groupicon_6401.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 6402 | 24x24+20x20+16x16 | 32,32,32 | `icons/groupicon_6402.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 6403 | 24x24+20x20+16x16 | 32,32,32 | `icons/groupicon_6403.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 6404 | 24x24+20x20+16x16 | 32,32,32 | `icons/groupicon_6404.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 6405 | 24x24+20x20+16x16 | 32,32,32 | `icons/groupicon_6405.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 6406 | 24x24+20x20+16x16 | 32,32,32 | `icons/groupicon_6406.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 6407 | 24x24+20x20+16x16 | 32,32,32 | `icons/groupicon_6407.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 6408 | 24x24+20x20+16x16 | 32,32,32 | `icons/groupicon_6408.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 6409 | 24x24+20x20+16x16 | 32,32,32 | `icons/groupicon_6409.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 6410 | 24x24+20x20+16x16 | 32,32,32 | `icons/groupicon_6410.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 6441 | 24x24+20x20+16x16 | 32,32,32 | `icons/groupicon_6441.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 6442 | 24x24+20x20+16x16 | 32,32,32 | `icons/groupicon_6442.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 6443 | 24x24+20x20+16x16 | 32,32,32 | `icons/groupicon_6443.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 6444 | 24x24+20x20+16x16 | 32,32,32 | `icons/groupicon_6444.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 6445 | 24x24+20x20+16x16 | 32,32,32 | `icons/groupicon_6445.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 6446 | 24x24+20x20+16x16 | 32,32,32 | `icons/groupicon_6446.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 6447 | 24x24+20x20+16x16 | 32,32,32 | `icons/groupicon_6447.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 6448 | 24x24+20x20+16x16 | 32,32,32 | `icons/groupicon_6448.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 6449 | 24x24+20x20+16x16 | 32,32,32 | `icons/groupicon_6449.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 6450 | 24x24+20x20+16x16 | 32,32,32 | `icons/groupicon_6450.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 6471 | 24x24+20x20+16x16 | 32,32,32 | `icons/groupicon_6471.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 6472 | 24x24+20x20+16x16 | 32,32,32 | `icons/groupicon_6472.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 6473 | 24x24+20x20+16x16 | 32,32,32 | `icons/groupicon_6473.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 6474 | 24x24+20x20+16x16 | 32,32,32 | `icons/groupicon_6474.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 6475 | 24x24+20x20+16x16 | 32,32,32 | `icons/groupicon_6475.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 6476 | 24x24+20x20+16x16 | 32,32,32 | `icons/groupicon_6476.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 6477 | 24x24+20x20+16x16 | 32,32,32 | `icons/groupicon_6477.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 6478 | 24x24+20x20+16x16 | 32,32,32 | `icons/groupicon_6478.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 6479 | 24x24+20x20+16x16 | 32,32,32 | `icons/groupicon_6479.ico` |
| mspaint_exe_mun | RT_GROUP_ICON | 6480 | 24x24+20x20+16x16 | 32,32,32 | `icons/groupicon_6480.ico` |
| mspaint_exe_mun | RT_ICON | 1 (group 2) | 64x64 | 4 | `icons/icon_2_0_64x64.png` |
| mspaint_exe_mun | RT_ICON | 10 (group 2) | 16x16 | 8 | `icons/icon_2_9_16x16.png` |
| mspaint_exe_mun | RT_ICON | 100 (group 6442) | 20x20 | 32 | `icons/icon_6442_1_20x20.png` |
| mspaint_exe_mun | RT_ICON | 101 (group 6442) | 16x16 | 32 | `icons/icon_6442_2_16x16.png` |
| mspaint_exe_mun | RT_ICON | 102 (group 6444) | 24x24 | 32 | `icons/icon_6444_0_24x24.png` |
| mspaint_exe_mun | RT_ICON | 103 (group 6444) | 20x20 | 32 | `icons/icon_6444_1_20x20.png` |
| mspaint_exe_mun | RT_ICON | 104 (group 6444) | 16x16 | 32 | `icons/icon_6444_2_16x16.png` |
| mspaint_exe_mun | RT_ICON | 105 (group 6443) | 24x24 | 32 | `icons/icon_6443_0_24x24.png` |
| mspaint_exe_mun | RT_ICON | 106 (group 6443) | 20x20 | 32 | `icons/icon_6443_1_20x20.png` |
| mspaint_exe_mun | RT_ICON | 107 (group 6443) | 16x16 | 32 | `icons/icon_6443_2_16x16.png` |
| mspaint_exe_mun | RT_ICON | 108 (group 6446) | 24x24 | 32 | `icons/icon_6446_0_24x24.png` |
| mspaint_exe_mun | RT_ICON | 109 (group 6446) | 20x20 | 32 | `icons/icon_6446_1_20x20.png` |
| mspaint_exe_mun | RT_ICON | 11 (group 2) | 256x256 | 32 | `icons/icon_2_10_256x256.png` |
| mspaint_exe_mun | RT_ICON | 110 (group 6446) | 16x16 | 32 | `icons/icon_6446_2_16x16.png` |
| mspaint_exe_mun | RT_ICON | 111 (group 6450) | 24x24 | 32 | `icons/icon_6450_0_24x24.png` |
| mspaint_exe_mun | RT_ICON | 112 (group 6450) | 20x20 | 32 | `icons/icon_6450_1_20x20.png` |
| mspaint_exe_mun | RT_ICON | 113 (group 6450) | 16x16 | 32 | `icons/icon_6450_2_16x16.png` |
| mspaint_exe_mun | RT_ICON | 114 (group 6447) | 24x24 | 32 | `icons/icon_6447_0_24x24.png` |
| mspaint_exe_mun | RT_ICON | 115 (group 6447) | 20x20 | 32 | `icons/icon_6447_1_20x20.png` |
| mspaint_exe_mun | RT_ICON | 116 (group 6447) | 16x16 | 32 | `icons/icon_6447_2_16x16.png` |
| mspaint_exe_mun | RT_ICON | 117 (group 6449) | 24x24 | 32 | `icons/icon_6449_0_24x24.png` |
| mspaint_exe_mun | RT_ICON | 118 (group 6449) | 20x20 | 32 | `icons/icon_6449_1_20x20.png` |
| mspaint_exe_mun | RT_ICON | 119 (group 6449) | 16x16 | 32 | `icons/icon_6449_2_16x16.png` |
| mspaint_exe_mun | RT_ICON | 12 (group 2) | 64x64 | 32 | `icons/icon_2_11_64x64.png` |
| mspaint_exe_mun | RT_ICON | 120 (group 6448) | 24x24 | 32 | `icons/icon_6448_0_24x24.png` |
| mspaint_exe_mun | RT_ICON | 121 (group 6448) | 20x20 | 32 | `icons/icon_6448_1_20x20.png` |
| mspaint_exe_mun | RT_ICON | 122 (group 6448) | 16x16 | 32 | `icons/icon_6448_2_16x16.png` |
| mspaint_exe_mun | RT_ICON | 123 (group 6471) | 24x24 | 32 | `icons/icon_6471_0_24x24.png` |
| mspaint_exe_mun | RT_ICON | 124 (group 6471) | 20x20 | 32 | `icons/icon_6471_1_20x20.png` |
| mspaint_exe_mun | RT_ICON | 125 (group 6471) | 16x16 | 32 | `icons/icon_6471_2_16x16.png` |
| mspaint_exe_mun | RT_ICON | 126 (group 6475) | 24x24 | 32 | `icons/icon_6475_0_24x24.png` |
| mspaint_exe_mun | RT_ICON | 127 (group 6475) | 20x20 | 32 | `icons/icon_6475_1_20x20.png` |
| mspaint_exe_mun | RT_ICON | 128 (group 6475) | 16x16 | 32 | `icons/icon_6475_2_16x16.png` |
| mspaint_exe_mun | RT_ICON | 129 (group 6472) | 24x24 | 32 | `icons/icon_6472_0_24x24.png` |
| mspaint_exe_mun | RT_ICON | 13 (group 2) | 48x48 | 32 | `icons/icon_2_12_48x48.png` |
| mspaint_exe_mun | RT_ICON | 130 (group 6472) | 20x20 | 32 | `icons/icon_6472_1_20x20.png` |
| mspaint_exe_mun | RT_ICON | 131 (group 6472) | 16x16 | 32 | `icons/icon_6472_2_16x16.png` |
| mspaint_exe_mun | RT_ICON | 132 (group 6474) | 24x24 | 32 | `icons/icon_6474_0_24x24.png` |
| mspaint_exe_mun | RT_ICON | 133 (group 6474) | 20x20 | 32 | `icons/icon_6474_1_20x20.png` |
| mspaint_exe_mun | RT_ICON | 134 (group 6474) | 16x16 | 32 | `icons/icon_6474_2_16x16.png` |
| mspaint_exe_mun | RT_ICON | 135 (group 6473) | 24x24 | 32 | `icons/icon_6473_0_24x24.png` |
| mspaint_exe_mun | RT_ICON | 136 (group 6473) | 20x20 | 32 | `icons/icon_6473_1_20x20.png` |
| mspaint_exe_mun | RT_ICON | 137 (group 6473) | 16x16 | 32 | `icons/icon_6473_2_16x16.png` |
| mspaint_exe_mun | RT_ICON | 138 (group 6476) | 24x24 | 32 | `icons/icon_6476_0_24x24.png` |
| mspaint_exe_mun | RT_ICON | 139 (group 6476) | 20x20 | 32 | `icons/icon_6476_1_20x20.png` |
| mspaint_exe_mun | RT_ICON | 14 (group 2) | 32x32 | 32 | `icons/icon_2_13_32x32.png` |
| mspaint_exe_mun | RT_ICON | 140 (group 6476) | 16x16 | 32 | `icons/icon_6476_2_16x16.png` |
| mspaint_exe_mun | RT_ICON | 141 (group 6480) | 24x24 | 32 | `icons/icon_6480_0_24x24.png` |
| mspaint_exe_mun | RT_ICON | 142 (group 6480) | 20x20 | 32 | `icons/icon_6480_1_20x20.png` |
| mspaint_exe_mun | RT_ICON | 143 (group 6480) | 16x16 | 32 | `icons/icon_6480_2_16x16.png` |
| mspaint_exe_mun | RT_ICON | 144 (group 6477) | 24x24 | 32 | `icons/icon_6477_0_24x24.png` |
| mspaint_exe_mun | RT_ICON | 145 (group 6477) | 20x20 | 32 | `icons/icon_6477_1_20x20.png` |
| mspaint_exe_mun | RT_ICON | 146 (group 6477) | 16x16 | 32 | `icons/icon_6477_2_16x16.png` |
| mspaint_exe_mun | RT_ICON | 147 (group 6479) | 24x24 | 32 | `icons/icon_6479_0_24x24.png` |
| mspaint_exe_mun | RT_ICON | 148 (group 6479) | 20x20 | 32 | `icons/icon_6479_1_20x20.png` |
| mspaint_exe_mun | RT_ICON | 149 (group 6479) | 16x16 | 32 | `icons/icon_6479_2_16x16.png` |
| mspaint_exe_mun | RT_ICON | 15 (group 2) | 24x24 | 32 | `icons/icon_2_14_24x24.png` |
| mspaint_exe_mun | RT_ICON | 150 (group 6478) | 24x24 | 32 | `icons/icon_6478_0_24x24.png` |
| mspaint_exe_mun | RT_ICON | 151 (group 6478) | 20x20 | 32 | `icons/icon_6478_1_20x20.png` |
| mspaint_exe_mun | RT_ICON | 152 (group 6478) | 16x16 | 32 | `icons/icon_6478_2_16x16.png` |
| mspaint_exe_mun | RT_ICON | 16 (group 2) | 16x16 | 32 | `icons/icon_2_15_16x16.png` |
| mspaint_exe_mun | RT_ICON | 17 (group 3) | 32x32 | 4 | `icons/icon_3_0_32x32.png` |
| mspaint_exe_mun | RT_ICON | 18 (group 3) | 48x48 | 4 | `icons/icon_3_1_48x48.png` |
| mspaint_exe_mun | RT_ICON | 19 (group 3) | 16x16 | 4 | `icons/icon_3_2_16x16.png` |
| mspaint_exe_mun | RT_ICON | 2 (group 2) | 48x48 | 4 | `icons/icon_2_1_48x48.png` |
| mspaint_exe_mun | RT_ICON | 20 (group 3) | 32x32 | 8 | `icons/icon_3_3_32x32.png` |
| mspaint_exe_mun | RT_ICON | 21 (group 3) | 48x48 | 8 | `icons/icon_3_4_48x48.png` |
| mspaint_exe_mun | RT_ICON | 22 (group 3) | 16x16 | 8 | `icons/icon_3_5_16x16.png` |
| mspaint_exe_mun | RT_ICON | 23 (group 114) | 32x32 | 4 | `icons/icon_114_0_32x32.png` |
| mspaint_exe_mun | RT_ICON | 24 (group 115) | 32x32 | 4 | `icons/icon_115_0_32x32.png` |
| mspaint_exe_mun | RT_ICON | 25 (group 116) | 32x32 | 4 | `icons/icon_116_0_32x32.png` |
| mspaint_exe_mun | RT_ICON | 26 (group 117) | 32x32 | 4 | `icons/icon_117_0_32x32.png` |
| mspaint_exe_mun | RT_ICON | 27 (group 50134) | 32x32 | 4 | `icons/icon_50134_0_32x32.png` |
| mspaint_exe_mun | RT_ICON | 28 (group 50134) | 24x24 | 4 | `icons/icon_50134_1_24x24.png` |
| mspaint_exe_mun | RT_ICON | 29 (group 50134) | 16x16 | 4 | `icons/icon_50134_2_16x16.png` |
| mspaint_exe_mun | RT_ICON | 3 (group 2) | 32x32 | 4 | `icons/icon_2_2_32x32.png` |
| mspaint_exe_mun | RT_ICON | 30 (group 50134) | 32x32 | 8 | `icons/icon_50134_3_32x32.png` |
| mspaint_exe_mun | RT_ICON | 31 (group 50134) | 24x24 | 8 | `icons/icon_50134_4_24x24.png` |
| mspaint_exe_mun | RT_ICON | 32 (group 50134) | 16x16 | 8 | `icons/icon_50134_5_16x16.png` |
| mspaint_exe_mun | RT_ICON | 33 (group 50134) | 32x32 | 32 | `icons/icon_50134_6_32x32.png` |
| mspaint_exe_mun | RT_ICON | 34 (group 50134) | 24x24 | 32 | `icons/icon_50134_7_24x24.png` |
| mspaint_exe_mun | RT_ICON | 35 (group 50134) | 16x16 | 32 | `icons/icon_50134_8_16x16.png` |
| mspaint_exe_mun | RT_ICON | 36 (group 50135) | 32x32 | 4 | `icons/icon_50135_0_32x32.png` |
| mspaint_exe_mun | RT_ICON | 37 (group 50135) | 24x24 | 4 | `icons/icon_50135_1_24x24.png` |
| mspaint_exe_mun | RT_ICON | 38 (group 50135) | 16x16 | 4 | `icons/icon_50135_2_16x16.png` |
| mspaint_exe_mun | RT_ICON | 39 (group 50135) | 32x32 | 8 | `icons/icon_50135_3_32x32.png` |
| mspaint_exe_mun | RT_ICON | 4 (group 2) | 24x24 | 4 | `icons/icon_2_3_24x24.png` |
| mspaint_exe_mun | RT_ICON | 40 (group 50135) | 24x24 | 8 | `icons/icon_50135_4_24x24.png` |
| mspaint_exe_mun | RT_ICON | 41 (group 50135) | 16x16 | 8 | `icons/icon_50135_5_16x16.png` |
| mspaint_exe_mun | RT_ICON | 42 (group 50135) | 32x32 | 32 | `icons/icon_50135_6_32x32.png` |
| mspaint_exe_mun | RT_ICON | 43 (group 50135) | 24x24 | 32 | `icons/icon_50135_7_24x24.png` |
| mspaint_exe_mun | RT_ICON | 44 (group 50135) | 16x16 | 32 | `icons/icon_50135_8_16x16.png` |
| mspaint_exe_mun | RT_ICON | 45 (group 50136) | 32x32 | 4 | `icons/icon_50136_0_32x32.png` |
| mspaint_exe_mun | RT_ICON | 46 (group 50136) | 24x24 | 4 | `icons/icon_50136_1_24x24.png` |
| mspaint_exe_mun | RT_ICON | 47 (group 50136) | 16x16 | 4 | `icons/icon_50136_2_16x16.png` |
| mspaint_exe_mun | RT_ICON | 48 (group 50136) | 32x32 | 8 | `icons/icon_50136_3_32x32.png` |
| mspaint_exe_mun | RT_ICON | 49 (group 50136) | 24x24 | 8 | `icons/icon_50136_4_24x24.png` |
| mspaint_exe_mun | RT_ICON | 5 (group 2) | 16x16 | 4 | `icons/icon_2_4_16x16.png` |
| mspaint_exe_mun | RT_ICON | 50 (group 50136) | 16x16 | 8 | `icons/icon_50136_5_16x16.png` |
| mspaint_exe_mun | RT_ICON | 51 (group 50136) | 32x32 | 32 | `icons/icon_50136_6_32x32.png` |
| mspaint_exe_mun | RT_ICON | 52 (group 50136) | 24x24 | 32 | `icons/icon_50136_7_24x24.png` |
| mspaint_exe_mun | RT_ICON | 53 (group 50136) | 16x16 | 32 | `icons/icon_50136_8_16x16.png` |
| mspaint_exe_mun | RT_ICON | 54 (group 50137) | 32x32 | 4 | `icons/icon_50137_0_32x32.png` |
| mspaint_exe_mun | RT_ICON | 55 (group 50137) | 24x24 | 4 | `icons/icon_50137_1_24x24.png` |
| mspaint_exe_mun | RT_ICON | 56 (group 50137) | 16x16 | 4 | `icons/icon_50137_2_16x16.png` |
| mspaint_exe_mun | RT_ICON | 57 (group 50137) | 32x32 | 8 | `icons/icon_50137_3_32x32.png` |
| mspaint_exe_mun | RT_ICON | 58 (group 50137) | 24x24 | 8 | `icons/icon_50137_4_24x24.png` |
| mspaint_exe_mun | RT_ICON | 59 (group 50137) | 16x16 | 8 | `icons/icon_50137_5_16x16.png` |
| mspaint_exe_mun | RT_ICON | 6 (group 2) | 64x64 | 8 | `icons/icon_2_5_64x64.png` |
| mspaint_exe_mun | RT_ICON | 60 (group 50137) | 32x32 | 32 | `icons/icon_50137_6_32x32.png` |
| mspaint_exe_mun | RT_ICON | 61 (group 50137) | 24x24 | 32 | `icons/icon_50137_7_24x24.png` |
| mspaint_exe_mun | RT_ICON | 62 (group 50137) | 16x16 | 32 | `icons/icon_50137_8_16x16.png` |
| mspaint_exe_mun | RT_ICON | 63 (group 6401) | 24x24 | 32 | `icons/icon_6401_0_24x24.png` |
| mspaint_exe_mun | RT_ICON | 64 (group 6401) | 20x20 | 32 | `icons/icon_6401_1_20x20.png` |
| mspaint_exe_mun | RT_ICON | 65 (group 6401) | 16x16 | 32 | `icons/icon_6401_2_16x16.png` |
| mspaint_exe_mun | RT_ICON | 66 (group 6405) | 24x24 | 32 | `icons/icon_6405_0_24x24.png` |
| mspaint_exe_mun | RT_ICON | 67 (group 6405) | 20x20 | 32 | `icons/icon_6405_1_20x20.png` |
| mspaint_exe_mun | RT_ICON | 68 (group 6405) | 16x16 | 32 | `icons/icon_6405_2_16x16.png` |
| mspaint_exe_mun | RT_ICON | 69 (group 6402) | 24x24 | 32 | `icons/icon_6402_0_24x24.png` |
| mspaint_exe_mun | RT_ICON | 7 (group 2) | 48x48 | 8 | `icons/icon_2_6_48x48.png` |
| mspaint_exe_mun | RT_ICON | 70 (group 6402) | 20x20 | 32 | `icons/icon_6402_1_20x20.png` |
| mspaint_exe_mun | RT_ICON | 71 (group 6402) | 16x16 | 32 | `icons/icon_6402_2_16x16.png` |
| mspaint_exe_mun | RT_ICON | 72 (group 6404) | 24x24 | 32 | `icons/icon_6404_0_24x24.png` |
| mspaint_exe_mun | RT_ICON | 73 (group 6404) | 20x20 | 32 | `icons/icon_6404_1_20x20.png` |
| mspaint_exe_mun | RT_ICON | 74 (group 6404) | 16x16 | 32 | `icons/icon_6404_2_16x16.png` |
| mspaint_exe_mun | RT_ICON | 75 (group 6403) | 24x24 | 32 | `icons/icon_6403_0_24x24.png` |
| mspaint_exe_mun | RT_ICON | 76 (group 6403) | 20x20 | 32 | `icons/icon_6403_1_20x20.png` |
| mspaint_exe_mun | RT_ICON | 77 (group 6403) | 16x16 | 32 | `icons/icon_6403_2_16x16.png` |
| mspaint_exe_mun | RT_ICON | 78 (group 6406) | 24x24 | 32 | `icons/icon_6406_0_24x24.png` |
| mspaint_exe_mun | RT_ICON | 79 (group 6406) | 20x20 | 32 | `icons/icon_6406_1_20x20.png` |
| mspaint_exe_mun | RT_ICON | 8 (group 2) | 32x32 | 8 | `icons/icon_2_7_32x32.png` |
| mspaint_exe_mun | RT_ICON | 80 (group 6406) | 16x16 | 32 | `icons/icon_6406_2_16x16.png` |
| mspaint_exe_mun | RT_ICON | 81 (group 6410) | 24x24 | 32 | `icons/icon_6410_0_24x24.png` |
| mspaint_exe_mun | RT_ICON | 82 (group 6410) | 20x20 | 32 | `icons/icon_6410_1_20x20.png` |
| mspaint_exe_mun | RT_ICON | 83 (group 6410) | 16x16 | 32 | `icons/icon_6410_2_16x16.png` |
| mspaint_exe_mun | RT_ICON | 84 (group 6407) | 24x24 | 32 | `icons/icon_6407_0_24x24.png` |
| mspaint_exe_mun | RT_ICON | 85 (group 6407) | 20x20 | 32 | `icons/icon_6407_1_20x20.png` |
| mspaint_exe_mun | RT_ICON | 86 (group 6407) | 16x16 | 32 | `icons/icon_6407_2_16x16.png` |
| mspaint_exe_mun | RT_ICON | 87 (group 6409) | 24x24 | 32 | `icons/icon_6409_0_24x24.png` |
| mspaint_exe_mun | RT_ICON | 88 (group 6409) | 20x20 | 32 | `icons/icon_6409_1_20x20.png` |
| mspaint_exe_mun | RT_ICON | 89 (group 6409) | 16x16 | 32 | `icons/icon_6409_2_16x16.png` |
| mspaint_exe_mun | RT_ICON | 9 (group 2) | 24x24 | 8 | `icons/icon_2_8_24x24.png` |
| mspaint_exe_mun | RT_ICON | 90 (group 6408) | 24x24 | 32 | `icons/icon_6408_0_24x24.png` |
| mspaint_exe_mun | RT_ICON | 91 (group 6408) | 20x20 | 32 | `icons/icon_6408_1_20x20.png` |
| mspaint_exe_mun | RT_ICON | 92 (group 6408) | 16x16 | 32 | `icons/icon_6408_2_16x16.png` |
| mspaint_exe_mun | RT_ICON | 93 (group 6441) | 24x24 | 32 | `icons/icon_6441_0_24x24.png` |
| mspaint_exe_mun | RT_ICON | 94 (group 6441) | 20x20 | 32 | `icons/icon_6441_1_20x20.png` |
| mspaint_exe_mun | RT_ICON | 95 (group 6441) | 16x16 | 32 | `icons/icon_6441_2_16x16.png` |
| mspaint_exe_mun | RT_ICON | 96 (group 6445) | 24x24 | 32 | `icons/icon_6445_0_24x24.png` |
| mspaint_exe_mun | RT_ICON | 97 (group 6445) | 20x20 | 32 | `icons/icon_6445_1_20x20.png` |
| mspaint_exe_mun | RT_ICON | 98 (group 6445) | 16x16 | 32 | `icons/icon_6445_2_16x16.png` |
| mspaint_exe_mun | RT_ICON | 99 (group 6442) | 24x24 | 32 | `icons/icon_6442_0_24x24.png` |
| mspaint_exe_mun | RT_GROUP_CURSOR | 9263 | multi | - | `cursors/groupcursor_9263.cur` |
| mspaint_exe_mun | RT_GROUP_CURSOR | 9264 | multi | - | `cursors/groupcursor_9264.cur` |
| mspaint_exe_mun | RT_GROUP_CURSOR | 9265 | multi | - | `cursors/groupcursor_9265.cur` |
| mspaint_exe_mun | RT_GROUP_CURSOR | 9266 | multi | - | `cursors/groupcursor_9266.cur` |
| mspaint_exe_mun | RT_GROUP_CURSOR | 9267 | multi | - | `cursors/groupcursor_9267.cur` |
| mspaint_exe_mun | RT_GROUP_CURSOR | 9268 | multi | - | `cursors/groupcursor_9268.cur` |
| mspaint_exe_mun | RT_GROUP_CURSOR | 9269 | multi | - | `cursors/groupcursor_9269.cur` |
| mspaint_exe_mun | RT_GROUP_CURSOR | 9270 | multi | - | `cursors/groupcursor_9270.cur` |
| mspaint_exe_mun | RT_GROUP_CURSOR | 9271 | multi | - | `cursors/groupcursor_9271.cur` |
| mspaint_exe_mun | RT_GROUP_CURSOR | 9272 | multi | - | `cursors/groupcursor_9272.cur` |
| mspaint_exe_mun | RT_GROUP_CURSOR | 9273 | multi | - | `cursors/groupcursor_9273.cur` |
| mspaint_exe_mun | RT_GROUP_CURSOR | 9274 | multi | - | `cursors/groupcursor_9274.cur` |
| mspaint_exe_mun | RT_GROUP_CURSOR | 9275 | multi | - | `cursors/groupcursor_9275.cur` |
| mspaint_exe_mun | RT_GROUP_CURSOR | 9276 | multi | - | `cursors/groupcursor_9276.cur` |
| mspaint_exe_mun | RT_GROUP_CURSOR | 9277 | multi | - | `cursors/groupcursor_9277.cur` |
| mspaint_exe_mun | RT_GROUP_CURSOR | 9278 | multi | - | `cursors/groupcursor_9278.cur` |
| mspaint_exe_mun | RT_GROUP_CURSOR | 9279 | multi | - | `cursors/groupcursor_9279.cur` |
| mspaint_exe_mun | RT_GROUP_CURSOR | 9280 | multi | - | `cursors/groupcursor_9280.cur` |
| mspaint_exe_mun | RT_GROUP_CURSOR | 9281 | multi | - | `cursors/groupcursor_9281.cur` |
| mspaint_exe_mun | RT_GROUP_CURSOR | 9282 | multi | - | `cursors/groupcursor_9282.cur` |
| mspaint_exe_mun | RT_GROUP_CURSOR | 9283 | multi | - | `cursors/groupcursor_9283.cur` |
| mspaint_exe_mun | RT_GROUP_CURSOR | 9284 | multi | - | `cursors/groupcursor_9284.cur` |
| mspaint_exe_mun | RT_GROUP_CURSOR | 9285 | multi | - | `cursors/groupcursor_9285.cur` |
| mspaint_exe_mun | RT_GROUP_CURSOR | 9286 | multi | - | `cursors/groupcursor_9286.cur` |
| mspaint_exe_mun | RT_GROUP_CURSOR | 9287 | multi | - | `cursors/groupcursor_9287.cur` |
| mspaint_exe_mun | RT_GROUP_CURSOR | 9288 | multi | - | `cursors/groupcursor_9288.cur` |
| mspaint_exe_mun | RT_GROUP_CURSOR | 9289 | multi | - | `cursors/groupcursor_9289.cur` |
| mspaint_exe_mun | RT_GROUP_CURSOR | 9290 | multi | - | `cursors/groupcursor_9290.cur` |
| mspaint_exe_mun | RT_GROUP_CURSOR | 9291 | multi | - | `cursors/groupcursor_9291.cur` |
| mspaint_exe_mun | RT_GROUP_CURSOR | 9292 | multi | - | `cursors/groupcursor_9292.cur` |
| mspaint_exe_mun | RT_GROUP_CURSOR | 9293 | multi | - | `cursors/groupcursor_9293.cur` |
| mspaint_exe_mun | RT_GROUP_CURSOR | 9294 | multi | - | `cursors/groupcursor_9294.cur` |
| mspaint_exe_mun | RT_GROUP_CURSOR | 9295 | multi | - | `cursors/groupcursor_9295.cur` |
| mspaint_exe_mun | RT_GROUP_CURSOR | 9296 | multi | - | `cursors/groupcursor_9296.cur` |
| mspaint_exe_mun | RT_GROUP_CURSOR | 9297 | multi | - | `cursors/groupcursor_9297.cur` |
| mspaint_exe_mun | RT_GROUP_CURSOR | 9298 | multi | - | `cursors/groupcursor_9298.cur` |
| mspaint_exe_mun | RT_CURSOR | 153 | 32x32 | 32 | `icons/cursor_9263_153_32x32.png` |
| mspaint_exe_mun | RT_CURSOR | 154 | 32x32 | 32 | `icons/cursor_9265_154_32x32.png` |
| mspaint_exe_mun | RT_CURSOR | 155 | 32x32 | 32 | `icons/cursor_9267_155_32x32.png` |
| mspaint_exe_mun | RT_CURSOR | 156 | 32x32 | 32 | `icons/cursor_9269_156_32x32.png` |
| mspaint_exe_mun | RT_CURSOR | 157 | 32x32 | 32 | `icons/cursor_9271_157_32x32.png` |
| mspaint_exe_mun | RT_CURSOR | 158 | 32x32 | 32 | `icons/cursor_9273_158_32x32.png` |
| mspaint_exe_mun | RT_CURSOR | 159 | 32x32 | 32 | `icons/cursor_9275_159_32x32.png` |
| mspaint_exe_mun | RT_CURSOR | 160 | 32x32 | 32 | `icons/cursor_9277_160_32x32.png` |
| mspaint_exe_mun | RT_CURSOR | 161 | 32x32 | 32 | `icons/cursor_9279_161_32x32.png` |
| mspaint_exe_mun | RT_CURSOR | 162 | 32x32 | 32 | `icons/cursor_9281_162_32x32.png` |
| mspaint_exe_mun | RT_CURSOR | 163 | 32x32 | 32 | `icons/cursor_9283_163_32x32.png` |
| mspaint_exe_mun | RT_CURSOR | 164 | 32x32 | 32 | `icons/cursor_9285_164_32x32.png` |
| mspaint_exe_mun | RT_CURSOR | 165 | 32x32 | 32 | `icons/cursor_9287_165_32x32.png` |
| mspaint_exe_mun | RT_CURSOR | 166 | 32x32 | 32 | `icons/cursor_9289_166_32x32.png` |
| mspaint_exe_mun | RT_CURSOR | 167 | 32x32 | 32 | `icons/cursor_9291_167_32x32.png` |
| mspaint_exe_mun | RT_CURSOR | 168 | 32x32 | 32 | `icons/cursor_9293_168_32x32.png` |
| mspaint_exe_mun | RT_CURSOR | 169 | 32x32 | 32 | `icons/cursor_9295_169_32x32.png` |
| mspaint_exe_mun | RT_CURSOR | 170 | 32x32 | 32 | `icons/cursor_9297_170_32x32.png` |
| mspaint_exe_mun | RT_CURSOR | 171 | 48x48 | 32 | `icons/cursor_9264_171_48x48.png` |
| mspaint_exe_mun | RT_CURSOR | 172 | 48x48 | 32 | `icons/cursor_9266_172_48x48.png` |
| mspaint_exe_mun | RT_CURSOR | 173 | 48x48 | 32 | `icons/cursor_9268_173_48x48.png` |
| mspaint_exe_mun | RT_CURSOR | 174 | 48x48 | 32 | `icons/cursor_9270_174_48x48.png` |
| mspaint_exe_mun | RT_CURSOR | 175 | 48x48 | 32 | `icons/cursor_9272_175_48x48.png` |
| mspaint_exe_mun | RT_CURSOR | 176 | 48x48 | 32 | `icons/cursor_9274_176_48x48.png` |
| mspaint_exe_mun | RT_CURSOR | 177 | 48x48 | 32 | `icons/cursor_9276_177_48x48.png` |
| mspaint_exe_mun | RT_CURSOR | 178 | 48x48 | 32 | `icons/cursor_9278_178_48x48.png` |
| mspaint_exe_mun | RT_CURSOR | 179 | 48x48 | 32 | `icons/cursor_9280_179_48x48.png` |
| mspaint_exe_mun | RT_CURSOR | 180 | 48x48 | 32 | `icons/cursor_9282_180_48x48.png` |
| mspaint_exe_mun | RT_CURSOR | 181 | 48x48 | 32 | `icons/cursor_9284_181_48x48.png` |
| mspaint_exe_mun | RT_CURSOR | 182 | 32x32 | 32 | `icons/cursor_9286_182_32x32.png` |
| mspaint_exe_mun | RT_CURSOR | 183 | 48x48 | 32 | `icons/cursor_9288_183_48x48.png` |
| mspaint_exe_mun | RT_CURSOR | 184 | 48x48 | 32 | `icons/cursor_9290_184_48x48.png` |
| mspaint_exe_mun | RT_CURSOR | 185 | 48x48 | 32 | `icons/cursor_9292_185_48x48.png` |
| mspaint_exe_mun | RT_CURSOR | 186 | 48x48 | 32 | `icons/cursor_9294_186_48x48.png` |
| mspaint_exe_mun | RT_CURSOR | 187 | 48x48 | 32 | `icons/cursor_9296_187_48x48.png` |
| mspaint_exe_mun | RT_CURSOR | 188 | 48x48 | 32 | `icons/cursor_9298_188_48x48.png` |
| mstsc_exe_mun | RT_GROUP_ICON | 101 | 256x256+16x16+20x20+24x24+32x32+40x40+48x48+64x64+128x128 | 32,32,32,32,32,32,32,32,32 | `icons/groupicon_101.ico` |
| mstsc_exe_mun | RT_GROUP_ICON | 13000 | 32x32+48x48+32x32+48x48 | 8,8,32,32 | `icons/groupicon_13000.ico` |
| mstsc_exe_mun | RT_GROUP_ICON | 13001 | 16x16+32x32+40x40+48x48 | 32,32,32,32 | `icons/groupicon_13001.ico` |
| mstsc_exe_mun | RT_GROUP_ICON | 13002 | 32x32+48x48+32x32+48x48 | 8,8,32,32 | `icons/groupicon_13002.ico` |
| mstsc_exe_mun | RT_GROUP_ICON | 13003 | 32x32+48x48+32x32+48x48 | 8,8,32,32 | `icons/groupicon_13003.ico` |
| mstsc_exe_mun | RT_GROUP_ICON | 13004 | 32x32+48x48+32x32+48x48 | 8,8,32,32 | `icons/groupicon_13004.ico` |
| mstsc_exe_mun | RT_GROUP_ICON | 13005 | 32x32+48x48+32x32+48x48 | 8,8,32,32 | `icons/groupicon_13005.ico` |
| mstsc_exe_mun | RT_GROUP_ICON | 13006 | 32x32+48x48+32x32+48x48 | 8,8,32,32 | `icons/groupicon_13006.ico` |
| mstsc_exe_mun | RT_GROUP_ICON | 13007 | 32x32+48x48+32x32+48x48 | 8,8,32,32 | `icons/groupicon_13007.ico` |
| mstsc_exe_mun | RT_GROUP_ICON | 13008 | 32x32+48x48+32x32+48x48 | 8,8,32,32 | `icons/groupicon_13008.ico` |
| mstsc_exe_mun | RT_GROUP_ICON | 13009 | 32x32+48x48+32x32+48x48 | 8,8,32,32 | `icons/groupicon_13009.ico` |
| mstsc_exe_mun | RT_GROUP_ICON | 13011 | 32x32+48x48+32x32+48x48 | 8,8,32,32 | `icons/groupicon_13011.ico` |
| mstsc_exe_mun | RT_GROUP_ICON | 13012 | 32x32+48x48+32x32+48x48 | 8,8,32,32 | `icons/groupicon_13012.ico` |
| mstsc_exe_mun | RT_GROUP_ICON | 13013 | 32x32+48x48+32x32+48x48 | 8,8,32,32 | `icons/groupicon_13013.ico` |
| mstsc_exe_mun | RT_GROUP_ICON | 13014 | 32x32+16x16+32x32+16x16+32x32+16x16 | 4,4,8,8,32,32 | `icons/groupicon_13014.ico` |
| mstsc_exe_mun | RT_GROUP_ICON | 20008 | 32x32 | 32 | `icons/groupicon_20008.ico` |
| mstsc_exe_mun | RT_GROUP_ICON | 20012 | 16x16 | 32 | `icons/groupicon_20012.ico` |
| mstsc_exe_mun | RT_GROUP_ICON | 20022 | 48x48 | 32 | `icons/groupicon_20022.ico` |
| mstsc_exe_mun | RT_GROUP_ICON | 382 | 16x16+16x16+16x16 | 4,8,32 | `icons/groupicon_382.ico` |
| mstsc_exe_mun | RT_GROUP_ICON | 383 | 16x16+16x16 | 8,32 | `icons/groupicon_383.ico` |
| mstsc_exe_mun | RT_ICON | 1 (group 101) | 256x256 | 32 | `icons/icon_101_0_256x256.png` |
| mstsc_exe_mun | RT_ICON | 10 (group 382) | 16x16 | 4 | `icons/icon_382_0_16x16.png` |
| mstsc_exe_mun | RT_ICON | 11 (group 382) | 16x16 | 8 | `icons/icon_382_1_16x16.png` |
| mstsc_exe_mun | RT_ICON | 12 (group 382) | 16x16 | 32 | `icons/icon_382_2_16x16.png` |
| mstsc_exe_mun | RT_ICON | 13 (group 383) | 16x16 | 8 | `icons/icon_383_0_16x16.png` |
| mstsc_exe_mun | RT_ICON | 14 (group 383) | 16x16 | 32 | `icons/icon_383_1_16x16.png` |
| mstsc_exe_mun | RT_ICON | 15 (group 13000) | 32x32 | 8 | `icons/icon_13000_0_32x32.png` |
| mstsc_exe_mun | RT_ICON | 16 (group 13000) | 48x48 | 8 | `icons/icon_13000_1_48x48.png` |
| mstsc_exe_mun | RT_ICON | 17 (group 13000) | 32x32 | 32 | `icons/icon_13000_2_32x32.png` |
| mstsc_exe_mun | RT_ICON | 18 (group 13000) | 48x48 | 32 | `icons/icon_13000_3_48x48.png` |
| mstsc_exe_mun | RT_ICON | 19 (group 13001) | 16x16 | 32 | `icons/icon_13001_0_16x16.png` |
| mstsc_exe_mun | RT_ICON | 2 (group 101) | 16x16 | 32 | `icons/icon_101_1_16x16.png` |
| mstsc_exe_mun | RT_ICON | 20 (group 13001) | 32x32 | 32 | `icons/icon_13001_1_32x32.png` |
| mstsc_exe_mun | RT_ICON | 21 (group 13001) | 40x40 | 32 | `icons/icon_13001_2_40x40.png` |
| mstsc_exe_mun | RT_ICON | 22 (group 13001) | 48x48 | 32 | `icons/icon_13001_3_48x48.png` |
| mstsc_exe_mun | RT_ICON | 23 (group 13002) | 32x32 | 8 | `icons/icon_13002_0_32x32.png` |
| mstsc_exe_mun | RT_ICON | 24 (group 13002) | 48x48 | 8 | `icons/icon_13002_1_48x48.png` |
| mstsc_exe_mun | RT_ICON | 25 (group 13002) | 32x32 | 32 | `icons/icon_13002_2_32x32.png` |
| mstsc_exe_mun | RT_ICON | 26 (group 13002) | 48x48 | 32 | `icons/icon_13002_3_48x48.png` |
| mstsc_exe_mun | RT_ICON | 27 (group 13003) | 32x32 | 8 | `icons/icon_13003_0_32x32.png` |
| mstsc_exe_mun | RT_ICON | 28 (group 13003) | 48x48 | 8 | `icons/icon_13003_1_48x48.png` |
| mstsc_exe_mun | RT_ICON | 29 (group 13003) | 32x32 | 32 | `icons/icon_13003_2_32x32.png` |
| mstsc_exe_mun | RT_ICON | 3 (group 101) | 20x20 | 32 | `icons/icon_101_2_20x20.png` |
| mstsc_exe_mun | RT_ICON | 30 (group 13003) | 48x48 | 32 | `icons/icon_13003_3_48x48.png` |
| mstsc_exe_mun | RT_ICON | 31 (group 13004) | 32x32 | 8 | `icons/icon_13004_0_32x32.png` |
| mstsc_exe_mun | RT_ICON | 32 (group 13004) | 48x48 | 8 | `icons/icon_13004_1_48x48.png` |
| mstsc_exe_mun | RT_ICON | 33 (group 13004) | 32x32 | 32 | `icons/icon_13004_2_32x32.png` |
| mstsc_exe_mun | RT_ICON | 34 (group 13004) | 48x48 | 32 | `icons/icon_13004_3_48x48.png` |
| mstsc_exe_mun | RT_ICON | 35 (group 13013) | 32x32 | 8 | `icons/icon_13013_0_32x32.png` |
| mstsc_exe_mun | RT_ICON | 36 (group 13013) | 48x48 | 8 | `icons/icon_13013_1_48x48.png` |
| mstsc_exe_mun | RT_ICON | 37 (group 13013) | 32x32 | 32 | `icons/icon_13013_2_32x32.png` |
| mstsc_exe_mun | RT_ICON | 38 (group 13013) | 48x48 | 32 | `icons/icon_13013_3_48x48.png` |
| mstsc_exe_mun | RT_ICON | 39 (group 13005) | 32x32 | 8 | `icons/icon_13005_0_32x32.png` |
| mstsc_exe_mun | RT_ICON | 4 (group 101) | 24x24 | 32 | `icons/icon_101_3_24x24.png` |
| mstsc_exe_mun | RT_ICON | 40 (group 13005) | 48x48 | 8 | `icons/icon_13005_1_48x48.png` |
| mstsc_exe_mun | RT_ICON | 41 (group 13005) | 32x32 | 32 | `icons/icon_13005_2_32x32.png` |
| mstsc_exe_mun | RT_ICON | 42 (group 13005) | 48x48 | 32 | `icons/icon_13005_3_48x48.png` |
| mstsc_exe_mun | RT_ICON | 43 (group 13006) | 32x32 | 8 | `icons/icon_13006_0_32x32.png` |
| mstsc_exe_mun | RT_ICON | 44 (group 13006) | 48x48 | 8 | `icons/icon_13006_1_48x48.png` |
| mstsc_exe_mun | RT_ICON | 45 (group 13006) | 32x32 | 32 | `icons/icon_13006_2_32x32.png` |
| mstsc_exe_mun | RT_ICON | 46 (group 13006) | 48x48 | 32 | `icons/icon_13006_3_48x48.png` |
| mstsc_exe_mun | RT_ICON | 47 (group 13007) | 32x32 | 8 | `icons/icon_13007_0_32x32.png` |
| mstsc_exe_mun | RT_ICON | 48 (group 13007) | 48x48 | 8 | `icons/icon_13007_1_48x48.png` |
| mstsc_exe_mun | RT_ICON | 49 (group 13007) | 32x32 | 32 | `icons/icon_13007_2_32x32.png` |
| mstsc_exe_mun | RT_ICON | 5 (group 101) | 32x32 | 32 | `icons/icon_101_4_32x32.png` |
| mstsc_exe_mun | RT_ICON | 50 (group 13007) | 48x48 | 32 | `icons/icon_13007_3_48x48.png` |
| mstsc_exe_mun | RT_ICON | 51 (group 13008) | 32x32 | 8 | `icons/icon_13008_0_32x32.png` |
| mstsc_exe_mun | RT_ICON | 52 (group 13008) | 48x48 | 8 | `icons/icon_13008_1_48x48.png` |
| mstsc_exe_mun | RT_ICON | 53 (group 13008) | 32x32 | 32 | `icons/icon_13008_2_32x32.png` |
| mstsc_exe_mun | RT_ICON | 54 (group 13008) | 48x48 | 32 | `icons/icon_13008_3_48x48.png` |
| mstsc_exe_mun | RT_ICON | 55 (group 13009) | 32x32 | 8 | `icons/icon_13009_0_32x32.png` |
| mstsc_exe_mun | RT_ICON | 56 (group 13009) | 48x48 | 8 | `icons/icon_13009_1_48x48.png` |
| mstsc_exe_mun | RT_ICON | 57 (group 13009) | 32x32 | 32 | `icons/icon_13009_2_32x32.png` |
| mstsc_exe_mun | RT_ICON | 58 (group 13009) | 48x48 | 32 | `icons/icon_13009_3_48x48.png` |
| mstsc_exe_mun | RT_ICON | 59 (group 13014) | 32x32 | 4 | `icons/icon_13014_0_32x32.png` |
| mstsc_exe_mun | RT_ICON | 6 (group 101) | 40x40 | 32 | `icons/icon_101_5_40x40.png` |
| mstsc_exe_mun | RT_ICON | 60 (group 13014) | 16x16 | 4 | `icons/icon_13014_1_16x16.png` |
| mstsc_exe_mun | RT_ICON | 61 (group 13014) | 32x32 | 8 | `icons/icon_13014_2_32x32.png` |
| mstsc_exe_mun | RT_ICON | 62 (group 13014) | 16x16 | 8 | `icons/icon_13014_3_16x16.png` |
| mstsc_exe_mun | RT_ICON | 63 (group 13014) | 32x32 | 32 | `icons/icon_13014_4_32x32.png` |
| mstsc_exe_mun | RT_ICON | 64 (group 13014) | 16x16 | 32 | `icons/icon_13014_5_16x16.png` |
| mstsc_exe_mun | RT_ICON | 65 (group 13011) | 32x32 | 8 | `icons/icon_13011_0_32x32.png` |
| mstsc_exe_mun | RT_ICON | 66 (group 13011) | 48x48 | 8 | `icons/icon_13011_1_48x48.png` |
| mstsc_exe_mun | RT_ICON | 67 (group 13011) | 32x32 | 32 | `icons/icon_13011_2_32x32.png` |
| mstsc_exe_mun | RT_ICON | 68 (group 13011) | 48x48 | 32 | `icons/icon_13011_3_48x48.png` |
| mstsc_exe_mun | RT_ICON | 69 (group 13012) | 32x32 | 8 | `icons/icon_13012_0_32x32.png` |
| mstsc_exe_mun | RT_ICON | 7 (group 101) | 48x48 | 32 | `icons/icon_101_6_48x48.png` |
| mstsc_exe_mun | RT_ICON | 70 (group 13012) | 48x48 | 8 | `icons/icon_13012_1_48x48.png` |
| mstsc_exe_mun | RT_ICON | 71 (group 13012) | 32x32 | 32 | `icons/icon_13012_2_32x32.png` |
| mstsc_exe_mun | RT_ICON | 72 (group 13012) | 48x48 | 32 | `icons/icon_13012_3_48x48.png` |
| mstsc_exe_mun | RT_ICON | 73 (group 20008) | 32x32 | 32 | `icons/icon_20008_0_32x32.png` |
| mstsc_exe_mun | RT_ICON | 74 (group 20022) | 48x48 | 32 | `icons/icon_20022_0_48x48.png` |
| mstsc_exe_mun | RT_ICON | 75 (group 20012) | 16x16 | 32 | `icons/icon_20012_0_16x16.png` |
| mstsc_exe_mun | RT_ICON | 8 (group 101) | 64x64 | 32 | `icons/icon_101_7_64x64.png` |
| mstsc_exe_mun | RT_ICON | 9 (group 101) | 128x128 | 32 | `icons/icon_101_8_128x128.png` |

*8992 inventory rows.*

---

## Appendix A - commands run and key output

Interpreter (bundled, Pillow 12.3.0, nothing installed):

```
C:\Users\zlihome\.dsh\dsh-runtimes\dsh-primary-runtime\dependencies\python\python.exe
python 3.12.14 (main, Sep  1 2026) [MSC v.1944 64-bit (AMD64)]
Pillow 12.3.0
```

### A.1 Locating the real resource carriers

```
PS> Get-ChildItem C:\Windows\WinSxS -Recurse -Filter comctl32.dll
```
Key output (14 hits; the two that matter):

```
C:\Windows\WinSxS\amd64_microsoft.windows.common-controls_6595b64144ccf1df_5.82.26100.8328_none_87eede957a2ccddd\comctl32.dll   742904   5.82 (WinBuild.160101.0800)
C:\Windows\WinSxS\amd64_microsoft.windows.common-controls_6595b64144ccf1df_6.0.26100.9278_none_3e0d1ba8e3303201\comctl32.dll  2684408   5.82 (WinBuild.160101.0800)
```

```
PS> Get-ChildItem C:\Windows\SystemResources\*.mun
```
174 `.mun` files. Relevant ones:

```
accessibilitycpl.dll.mun  3129344     imageres.dll.mun   24280064    printui.dll.mun     520192
appwiz.cpl.mun             332800     inetcpl.cpl.mun     1598464    shell32.dll.mun   18143744
DDORes.dll.mun           16382976     intl.cpl.mun         263680    sysdm.cpl.mun       204288
main.cpl.mun               577024     mmres.dll.mun       9568256    themecpl.dll.mun    450048
mmsys.cpl.mun              423936     moricons.dll.mun     185344    themeui.dll.mun     229888
mspaint.exe.mun           2485248     mstsc.exe.mun        550400    timedate.cpl.mun    282624
wscui.cpl.mun             1102336
```

### A.2 Listing resources

```
python tools\pe_extract.py C:\Windows\System32\comctl32.dll --list
```

Key output (tail):

```
--------------------------------------------------------------------------
total resources: 77
  MUI                1
  RT_BITMAP          10
  RT_CURSOR          33
  RT_GROUP_CURSOR    18
  RT_GROUP_ICON      4
  RT_ICON            10
  RT_VERSION         1
```

```
python tools\pe_extract.py C:\Windows\System32\user32.dll --list
  RT_BITMAP          7          <- the complete OBM search space; see 4.2
```

```
python tools\pe_extract.py C:\Windows\System32\shell32.dll --list
  4 resources: MUI, MUI, RT_VERSION(16), RT_MANIFEST(24)   -> 0 bitmaps, 0 icons
```

```
python tools\pe_extract.py C:\Windows\SystemResources\moricons.dll.mun --list
total resources: 351
  RT_GROUP_ICON      113
  RT_ICON            237
  RT_VERSION         1
```

### A.3 Extracting

One invocation per source (the exact loop that produced everything):

```powershell
$py = "C:\Users\zlihome\.dsh\dsh-runtimes\dsh-primary-runtime\dependencies\python\python.exe"
$ws = "C:\Users\zlihome\Desktop\keyODO"
foreach ($k in $map.Keys) {
  & $py "$ws\tools\pe_extract.py" $map[$k] --out "$ws\assets\extracted\$k" `
        --types bitmap,icon,cursor --contact-sheet --label $k 2> "$ws\assets\logs\$k.log"
}
```

Key output (one representative line per source class):

```
[pe_extract]   bitmap 120         240x16    4bpp -> bitmap_120.png
[pe_extract]   bitmap 121         360x24    4bpp -> bitmap_121.png
[pe_extract]   group icon 150      1 image(s) -> groupicon_150.ico
[pe_extract]   icon  23           32x32    1bpp (orphan)
[pe_extract] comctl32: 75 record(s), 0 error(s)
[pe_extract] contact sheet: ...\assets\extracted\comctl32\_contact_sheet.png (53 cells)
```

Errors encountered on the first pass and fixed in the tool (recorded because they are the reason
the extractor has RLE and PNG-icon support at all):

```
[pe_extract] ! skip RT_BITMAP id=120 ...: DIBError: unsupported compression 2 (RLE not supported)
[pe_extract] ! icon 70 in group 1: cannot determine icon DIB header size (biSize=1196314761)
[pe_extract] ! skip RT_GROUP_ICON id="BLANK0" ...: OSError: [Errno 22] Invalid argument
```

1. `compression 2` = **BI_RLE4** - the comctl32 toolbar strips are RLE4-compressed. RLE4/RLE8
   decoding was added; without it *none* of the classic toolbar strips can be extracted.
2. `biSize=1196314761` = `0x474E5089` = the PNG signature - these are Vista+ **PNG-compressed
   RT_ICON** payloads. Support added; they are embedded verbatim into the `.ico`.
3. `[Errno 22]` - some MUI resource **names literally contain double-quote characters**
   (e.g. `"BLANK0"`), which are illegal in Windows filenames. All output names now go through
   `sanitize()`.

### A.4 Final validation of every produced file

```python
import os, glob, collections
from PIL import Image
files = []
for ext in ("png","ico","cur"):
    files += glob.glob(os.path.join(root,"**","*."+ext), recursive=True)
```

Output:

```
files checked: 9049 {'.png': 7816, '.ico': 1104, '.cur': 129}
ico multi-image size-count histogram: {1: 240, 2: 58, 3: 108, 4: 41, 5: 14, 6: 6, 7: 6, 8: 591, 9: 20, 10: 17, 11: 1, 13: 2}
FAILURES: 0
```

### A.5 Cross-checks quoted in section 4

```
PS> Get-FileHash ...\user32\bitmaps\bitmap_32559.png ; Get-FileHash ...\comctl32\bitmaps\bitmap_133.png
user32/32559 vs comctl32/133 : IDENTICAL  5CB5A806C334A3E1...
user32/32660 vs comctl32/134 : IDENTICAL  E4CB1FEC52E1312E...
user32/32661 vs comctl32/132 : IDENTICAL  D8B65CFCA30FE121...

comctl32 System32 vs WinSxS v5, bitmap_120.png : True  (identical)
comctl32 v5 vs v6,            bitmap_120.png : True  (identical)
comctl32 v5 vs v6,            bitmap_124.png : False (re-authored to 32 bpp in v6)
```

### A.6 Header dump that proved the RLE4 and bitmap-core facts

```
BMP 120  size=1810   biSize=40 240x16 planes=1 bpp=4 comp=2 sizeImage=1706 clrUsed=0 clrImp=0
BMP 121  size=2760   biSize=40 360x24 planes=1 bpp=4 comp=2 sizeImage=2656 clrUsed=0 clrImp=0
BMP 124  size=1374   biSize=40 192x16 planes=1 bpp=4 comp=2 sizeImage=1270 clrUsed=0 clrImp=0
BMP 130  size=626    biSize=40  80x16 planes=1 bpp=4 comp=2 sizeImage=522  clrUsed=0 clrImp=0
BMP 140  size=200    biSize=40  13x12 planes=1 bpp=4 comp=0 sizeImage=96   clrUsed=0 clrImp=0
```

Corner pixel of `bitmap_120.png` is `(192,192,192)` = `COLOR_BTNFACE`, i.e. the classic 3D-face
grey, which confirms the 16-entry colour table was decoded correctly.

```
GRPCURSOR 102 bytes: 000002000100
   as BYTE/BYTE : (32, 0, 64, 0, 1, 1, 308, 1)
   as WORD/WORD : (32, 64, 1, 1, 308, 1)
```

This is why the extractor derives cursor geometry from the DIB rather than from the group entry:
`GRPCURSORDIRENTRY` height is the *doubled* DIB height, and the DIB itself is preceded by a 4-byte
hotspot (detected because `data[4:8]` parses as a valid `biSize` of 40).

---

## Appendix B - per-source manifests

Each source directory contains a machine-readable `_manifest.json`:

```
assets/extracted/<label>/_manifest.json
```

with the fields `source` (absolute path of the PE file), `label`, `machine`, `pe32_plus`,
`types_requested`, `records` (every extracted resource: kind, type, id, lang, width, height, bpp,
output file, resource size) and `errors`. These files are the raw evidence behind every table above.
Contact sheets are at `assets/extracted/<label>/_contact_sheet.png`
(`_contact_sheet_2.png`, `_3.png`, ... for sources with more than 240 assets).

Extraction logs (raw stderr from every run) are in `assets/logs/<label>.log`, and
`assets/logs/sources.json` records the resolved path, byte size and file version of all 54 sources.
