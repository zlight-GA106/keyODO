# KeyODO 构建说明

## 1. 一键构建

```cmd
build.cmd
```

产物：

| 文件 | 说明 |
|---|---|
| `build\keyodo.exe` | 32 位（x86）原生 Win32 + GDI 程序，静态 CRT |
| `dist\KeyODO_Setup.exe` | NSIS 安装程序 |

其他目标：

```cmd
build.cmd app      :: 只编译 keyodo.exe
build.cmd setup    :: 只打包安装程序（需要 keyodo.exe 已存在）
build.cmd clean    :: 删除 build\ 与 dist\
```

## 2. 环境要求

| 组件 | 用途 | 说明 |
|---|---|---|
| Visual Studio（含 C++ 桌面工作负载） | 编译器与链接器 | 通过 `vswhere.exe` 自动定位最新安装，并调用 `vcvars32.bat` 取得 x86 环境 |
| NSIS 3.x（`makensis.exe`） | 生成安装程序 | 依次在 `KEYODO_NSIS` 环境变量、`PATH`、`%ProgramFiles(x86)%\NSIS`、`%ProgramFiles%\NSIS`、`%USERPROFILE%\.keyodo-tools\nsis` 中查找 |
| Python 3 | 构建期的资源编码转换 | 仅用标准库，无需安装任何包 |

设置 `KEYODO_NSIS` 可以指定 `makensis.exe` 的完整路径：

```cmd
set KEYODO_NSIS=D:\tools\nsis\makensis.exe
build.cmd
```

## 3. 构建都做了什么

1. **定位 32 位工具链**：`vswhere -latest -products * -property installationPath`，然后 `call vcvars32.bat`。
2. **编译**：`cl /c /TC /W3 /O2 /MT /GS- /I include /I res`，14 个源文件。
   `/MT` 静态链接 CRT，因此 `keyodo.exe` 不依赖任何 VC 运行库，可独立分发。
3. **资源编码转换**：`res/keyodo.rc` 在仓库里是 UTF-8；Windows 98 SE 是 ANSI 平台，资源必须是 CP936(GBK)，所以先由 `tools/to_cp936.py` 转换到 `build\keyodo_cp936.rc` 再交给 `rc.exe`。
4. **链接**：`link /SUBSYSTEM:WINDOWS /MACHINE:X86`，只依赖 `user32` `gdi32` `shell32`。
   **故意不嵌入清单**：一旦带上 comctl32 v6 清单，按钮和菜单会变成现代主题外观，整套 Win98 观感当场作废。
5. **安装程序编码转换**：`installer/keyodo.nsi` 同样在仓库里保持无 BOM 的 UTF-8；NSIS 3.x 只有看到 UTF-8 BOM 才会按 UTF-8 解析脚本，所以由 `tools/to_utf8bom.py` 生成 `build\keyodo_utf8.nsi` 后再调用 `makensis`。

这一步让构建结果**不依赖执行构建的机器使用什么代码页**。

## 4. 安装程序

`dist\KeyODO_Setup.exe`：

* 安装到 `%ProgramFiles(x86)%\KeyODO`（32 位进程的 `$PROGRAMFILES` 视图）
* 创建开始菜单快捷方式（KeyODO / 卸载 KeyODO）
* 可选创建桌面快捷方式（组件页可取消勾选）
* 写入标准「添加或删除程序」信息（显示名、版本、发布者、安装位置、大小、卸载命令）
* 自带 `uninstall.exe`
* 安装完成页可勾选立即运行 KeyODO

### 命令行

| 命令 | 效果 |
|---|---|
| `KeyODO_Setup.exe` | 交互式安装 |
| `KeyODO_Setup.exe /S` | 静默安装，全部默认组件 |
| `KeyODO_Setup.exe /S /D=C:\KeyODO` | 静默安装到指定目录（`/D` 必须是最后一个参数，且路径不要加引号） |
| `KeyODO_Setup.exe /S /COMPONENTS="main,startmenu"` | 静默安装且不创建桌面快捷方式 |
| `uninstall.exe /S` | 静默卸载 |

### 两个前置检查

* **需要管理员权限。** 安装到 `%ProgramFiles%` 和写 `HKLM` 都需要提权。安装程序用一次真实的注册表写入探测权限，失败时明确报错退出（退出码 5），而不是"看起来装好了、其实注册表和快捷方式全没写"。
* **KeyODO 正在运行时会拒绝安装/卸载**，因为 `keyodo.exe` 被占用。退出码 2。

卸载会删除程序目录、快捷方式和注册表项，但**保留 `%APPDATA%\KeyODO\keyodo.ini`**，因为那里面是用户累计的 ODO 统计数据；静默卸载不会弹提示，交互式卸载会说明这一点。

## 5. Windows 98 SE 说明（重要）

**源码是 Windows 98 SE 兼容的**：

* 只用 ANSI API（`…A` 系列），不定义 `UNICODE`
* 不使用 `GetWindowLongPtr`、`LONG_PTR`、分层窗口、主题 API、GDI+、`AlphaBlend`
* 不使用 C99 特性（复合字面量、变长数组、混合声明）；64 位整数只通过 `KeyU64`，映射到 MSVC 6 就已支持的 `__int64`
* 所有 3D 边框、按钮、表盘都由 GDI 图元自绘，不依赖任何现代控件
* 中文全部放在 `res/keyodo.rc` 字符串表里，`.c` / `.h` 文件里**没有一个非 ASCII 字节**，因此不会被编辑器按错误编码破坏

**但用 VS2022 编译出来的这个二进制不能跑在 Win98SE 上**：现代 CRT 会引入 `InitializeCriticalSectionEx`、`GetSystemTimePreciseAsFileTime` 等 Win98 不存在的内核导入。

要产出真正能在 Win98SE 上跑的二进制，需要用当年的工具链重新编译：

```
MSVC 6.0 + Windows 98 SDK  →  nmake /f makefile.msvc
```

代码本身不需要改动——这正是"结构上为 Win98SE 留出空间"的含义。`build.cmd` 用的是现代工具链，只是为了在当前（Windows 11）开发机上立刻得到可安装可运行的产品。

## 6. 源码结构

```
src/
  main.c              进程入口、消息循环、加速键表
  app.c / app.h       组装层：唯一同时认识四个模块的地方
  common.c / .h       KeyU64 格式化等小工具
  platform/           唯一知道"现在跑在哪个 Windows 上"的模块
  ui/                 界面：窗口、自绘 Win98 chrome、表盘、档位、控件
    ui.c              主窗口：窗口过程、布局、命中测试、菜单
    metrics.c / .h    把 96 DPI 规格按显示器 DPI 换算成实际尺寸与字体
    theme.c / .h      钉死的 Win98 配色 + 实测 3D 边框配方
    widgets.c / .h    按钮、分组框、滚轮、读数窗、文字排版
    gauge.c / .h      WPM 速度表（在自己的坐标系里绘制）
    gear.c / .h       档位读数与换挡显示
    widget.c / .h     桌面仪表：只含表盘的独立悬浮窗口
  stats/              按键统计：ODO / TRIP / WPM / KPM / MAX / 档位
  input/              输入后端
    input.h           后端接口
    input.c           后端选择
    input_raw.c       Raw Input 实现（当前默认）
    input_hook.c      低级键盘钩子实现（演示可替换性）
  storage/            INI 读写
res/
  keyodo.rc           菜单 / 字符串表 / 版本信息（UTF-8，构建时转 CP936）
  resource.h          资源与命令 ID、版本号
  keyodo.ico          16×16 + 32×32，4bpp，16 色
include/
  keyodo_layout.h     布局常量（与设计稿同一组数字）
installer/
  keyodo.nsi          安装脚本
tools/                设计稿渲染、实测探针、编码转换、安装测试
mockup/               界面设计稿与其渲染器
```

## 7. 验证工具

```cmd
:: 端到端：启动、注入按键、检查计数、退出后检查持久化
python tools\smoke_test.py --keys 20

:: 抓取运行中的窗口为 PNG（用于与设计稿比对）；--widget 抓桌面仪表
python tools\capture_window.py --out build\capture.png
python tools\capture_window.py --widget --out build\widget.png

:: 与设计稿做结构比对（静态部分应逐像素一致）
python tools\compare_mockup.py --shot build\capture.png

:: 缩放是否真的等比：窗口尺寸、结构线位置、文字是否重新栅格化
python tools\verify_scaling.py

:: 1080p / 2K / 4K x 16:9 / 16:10 x 各缩放级别是否放得下
python tools\verify_resolution_matrix.py

:: 桌面仪表的显示、尺寸、位置恢复、越界钳制与持久化
python tools\widget_test.py

:: 独立验证 Raw Input 在本机是否真的能收到注入按键
python tools\probe_rawinput.py
```

### 指定缩放（调试与偏好）

```cmd
keyodo.exe --dpi=96        :: 在 200% 的屏幕上强制按 100% 布局渲染
keyodo.exe --dpi=144
```

等价于 `keyodo.ini` 里的 `[Window] Dpi`（0 = 跟随显示器）。这个开关让
`tools\verify_scaling.py` 能在一台机器上跑完 100/125/150/200% 四种缩放。

### 输入后端切换

`%APPDATA%\KeyODO\keyodo.ini` 的 `[Input] Backend` 可设为 `raw` 或 `hook`，也可以用命令行覆盖：

```cmd
keyodo.exe --input=hook
```

两个后端走的是同一套 `InputBackend` 接口，`stats` / `ui` / `storage` 完全不感知差异。这就是为将来替换成 Windows 98 的 `WH_KEYBOARD` + DLL 方案预留的位置：新增一个文件、在 `input.c` 的选择表里加一项即可。

## 8. 常见问题

| 现象 | 原因与处理 |
|---|---|
| `ERROR: vswhere.exe not found` | 未安装 Visual Studio 的 C++ 工作负载 |
| `ERROR: makensis.exe not found` | 安装 NSIS，或设置 `KEYODO_NSIS` |
| `Bad text encoding: keyodo.nsi` | 直接把 `installer\keyodo.nsi` 交给 `makensis` 了；请用 `build.cmd`，它会先补 UTF-8 BOM |
| 安装程序退出码 5 | 没有管理员权限；右键→以管理员身份运行 |
| 安装程序退出码 2 | KeyODO 正在运行，先退出 |
| 菜单显示成乱码 | 资源没有转成 CP936；用 `build.cmd` 而不是手工调用 `rc.exe` |
| 界面在高分屏上显得小 | 见下 |

### 高分屏（DPI）

KeyODO 按「每显示器 DPI 感知 v2」申请（逐级回退到 v1 → 系统 DPI 感知 → Windows 98 SE
上无此 API 则停在 96 DPI），然后把 `include/keyodo_layout.h` 里的 96 DPI 规格按显示器
DPI 换算成实际尺寸与字体，**不做位图拉伸**。因此在 200% 的 4K 屏上界面是 960×640 且文字
按 2 倍重新栅格化，锐利而不是糊的；1 px 的立体边框也会跟着变粗，保持 Win98 的厚重感。

实测 100% / 125% / 150% / 200% 分别为 480×320、600×400、720×480、960×640，
`tools\verify_scaling.py` 会逐个验证。跨显示器拖动时 `WM_DPICHANGED` 会触发整体重算；
桌面仪表与主窗口共用一套缩放，所以要变一起变。

Windows 98 SE 没有多显示器 DPI 概念，行为与当年一致（恒为 96 DPI）。
