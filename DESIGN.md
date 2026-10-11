# KeyODO 界面设计说明书

**键盘里程表 · Windows 98 SE 原生风格 · Win32 + GDI**

| 项目 | 值 |
|---|---|
| 目标平台 | Windows 98 SE（强制兼容），同时可在 NT 系上以经典外观运行 |
| 工具链 | Win32 API + GDI（纯 API，无 MFC / 无 GDI+ / 无主题 API） |
| 窗口基准 | **480 × 320 @ 96 DPI**（外框尺寸） |
| 客户区 | **474 × 296** |
| 界面语言 | 简体中文（Win98SE 中文版惯例），字符串表可整体替换为英文 |
| 本阶段范围 | **仅界面**。不设计统计逻辑、键盘钩子、注册表持久化 |
| 视觉复核 | [`mockup/out/keyodo_480x320.png`](mockup/out/keyodo_480x320.png)（1:1）· [`mockup/out/keyodo_2x.png`](mockup/out/keyodo_2x.png)（2 倍放大） |

---

## 0. 本文档的证据基础

界面里每一个像素都应能追溯到证据，而不是"我觉得 Win98 好像是这样"。本项目在设计阶段就已经把可验证的部分实测完了：

| 证据 | 产出 | 用途 |
|---|---|---|
| 调用真实 `user32!DrawEdge` / `DrawFrameControl`，逐像素读回 | [`reports/gdi_edge_probe.txt`](reports/gdi_edge_probe.txt) · [`reports/gdi_edge_probe.png`](reports/gdi_edge_probe.png) | §4 配色环配方、§7 全部立体边框画法 |
| 本机 `GetSysColor` 全表 | 同上 | §4 对照"现代配色 vs Win98 标准配色" |
| `MARLETT.TTF` 字形转储 | [`reports/marlett_map.txt`](reports/marlett_map.txt) | §7.4 关于符号字体的结论 |
| 系统 DLL/CPL 资源提取（54 个 PE，8992 条资源） | [`assets/ASSETS.md`](assets/ASSETS.md) | §7.5 真实可用位图/图标清单 |

**结论先行**：Win98 的界面外观**不是位图拼出来的**，而是 `DrawEdge` / `DrawFrameControl` / `DrawStatusText` 三个 API 现场生成的。所以"直接使用电脑内置的 Win98 艺术资产"这句话，在 KeyODO 里的正确落地方式是——**让 GDI 自己画，并把配色钉死在 Windows 标准方案上**，而不是去网上找皮肤图片。§7 给出全部细节。

---

## 1. 主窗口布局方案

### 1.1 窗口类型与样式

```c
DWORD dwStyle   = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN;
DWORD dwExStyle = WS_EX_TOPMOST;      /* 里程表要一直看得见；可选 */
```

* **不要 `WS_THICKFRAME`**：仪表盘按固定尺寸设计，放大缩小会让 480×320 的构图失效。因此标题栏只有「最小化」和「关闭」，没有「最大化」——这也是 1999 年小工具的常见做法。
* **不要 `WS_EX_TOOLWINDOW`**：在 Win98SE 上会让标题栏变矮、字体变小，破坏基准尺寸。
* `WS_EX_TOPMOST` 建议做成菜单项「查看 → 总在最前」，默认开启。

### 1.2 尺寸推导（这是硬性要求，不能硬编码外框）

Win98SE 在 96 DPI + "Windows 标准" 配色下的窗口度量：

```
SM_CXDLGFRAME = 3      SM_CYDLGFRAME = 3      SM_CYCAPTION = 18
外框宽 = 客户区宽 + 3 + 3          = 474 + 6  = 480
外框高 = 客户区高 + 3 + 18 + 3     = 296 + 24 = 320
```

**正确做法**：先按客户区设计（474 × 296），启动时用 `AdjustWindowRectEx()` 反推外框，再 `SetWindowPos()`。这样在大字体 / 小字体 / 其它配色方案下都不会错位。

> **现代 Windows 上的注意点**：Win10/11 的 `AdjustWindowRectEx` 会额外算上不可见的 8 px 拖拽边框（`SM_CXSIZEFRAME + SM_CXPADDEDBORDER`），本机实测同一份客户区会得到 488×335。要在现代系统上保持 480×320 的观感，需要处理 `WM_NCCALCSIZE` 把这段不可见边框裁掉。**Win98SE 上没有这个问题**，不需要任何特殊处理。

### 1.3 三段式客户区

```
客户区 474 × 296
├── 菜单栏        20 px    ← 用真正的 HMENU（SetMenu），不要自绘
├── 内容区       256 px
│   ├── 速度表 GroupBox   (4,27)  252×244
│   │   ├── 方形仪表板 + 圆形表盘
│   │   ├── 盘内嵌读数（WPM/KPM 可切换）
│   │   └── 盘下合并读数：`D 62`（档位 + 数值同一块面板）
│   ├── 里程表 GroupBox   (260,27) 210×114   → ODO 滚轮 + TRIP 滚轮 + 清零
│   └── 速率 GroupBox     (260,151) 210×120  → KPM + MAX + 换挡显示 + 复位
└── 状态栏        20 px    ← 3 格，1 px 下凹边框
```

### 1.4 自绘 / 原生控件分工

| 元素 | 实现方式 | 理由 |
|---|---|---|
| 标题栏、边框 | 系统自绘 | Win98SE 上是原生经典外观，最真 |
| 菜单栏 | **原生 `HMENU` + `SetMenu`** | 高度、快捷键下划线、弹出菜单全部由系统保证 |
| 状态栏 | 可用 `STATUSCLASSNAME`，或自绘 | 自绘更可控（本设计给的是自绘配方，见 §5.6） |
| GroupBox | 自绘 `EDGE_ETCHED` 1 px + 标题断口 | 系统 `BS_GROUPBOX` 也可以，但自绘才能精确控制标题位置 |
| 数字滚轮 / 读数窗 / 档位闸 / 按钮 | **原生控件类**（`BUTTON`）或自绘配方 | 原生 = 绝对正宗；本设计两套配方都给 |
| **WPM 仪表盘（含盘内嵌读数）** | **100% 自绘 GDI** | 无对应原生控件 |
| **档位读数 / 换挡显示** | **100% 自绘 GDI** | 无对应原生控件 |

### 1.5 键盘可访问性

* 所有可聚焦控件按 `WS_GROUP` / `WS_TABSTOP` 排好 Tab 顺序：清零 → 复位 → （若有）其它按钮。
* 菜单助记符：`文件(F) 编辑(E) 查看(V) 统计(S) 帮助(H)`，`&` 前缀写在 .rc 里。
* 仪表盘**不进 Tab 顺序**，用 `WS_EX_TRANSPARENT` 或不建窗口（直接画在父窗口上）。

---

## 2. ASCII 界面草图

### 2.1 整窗结构（1 字符 ≈ 1 px 的示意，非等比）

```
┌──────────────────────────────────────────────────────────────────────────────┐
│[▒]KeyODO                                                      [ _ ] [ X ]   │ ← 18px 标题栏
│                                                    #000080 ──► #A6CAF0 渐变  │
├──────────────────────────────────────────────────────────────────────────────┤
│ 文件(F)  编辑(E)  查看(V)  统计(S)  帮助(H)                                  │ ← 20px 菜单栏
├──────────────────────────────────────────────────────────────────────────────┤
│┌─ 速度表 ────────────────────┐┌─ 里程表 ──────────────────────────────────┐ │
││┌──────────────────────────┐││ ODO:                                       │ │
│││        ╱‾‾‾‾60‾‾‾‾╲        │││┌────────────────────────────────────────┐ │ │
│││    40 ╱           ╲ 80    ││││ 1  2  3  4  5  6  7                    │ │ │
│││  20  │      ┃      │  100 │││└────────────────────────────────────────┘ │ │
│││   │  │      ●      │   ▓  │││ TRIP:                                      │ │
│││   0  │             │ 120  │││┌────────┐ ┌──────────────────────┐         │ │
│││      │    WPM      │      ││││ 清零(C)│ │ 4  8  1  2           │         │ │
│││      │ ┌─────────┐ │      │││└────────┘ └──────────────────────┘         │ │
│││      │ │   62    │ │      ││└────────────────────────────────────────────┘ │
│││       ╲└─────────┘╱       ││┌─ 速率 ────────────────────────────────────┐ │
││└──────────────────────────┘││ KPM:      ┌───────────────┐                │ │
││ ┌────────────────────────┐ ││           │      310      │                │ │
││ │        D 62            │ ││ MAX:      └───────────────┘                │ │
││ └────────────────────────┘ ││           ┌───────────────┐                │ │
│└────────────────────────────┘│           │       88      │                │ │
│                               │ ┌───────────────────────────────────────┐ │ │
│                               │ │    P     R     N     D                │ │ │
│                               │ └───────────────────────────────────────┘ │ │
│                               │ ┌────────────────────┐                     │ │
│                               │ │     复位(R)        │                     │ │
│                               │ └────────────────────┘                     │ │
│                               └────────────────────────────────────────────┘ │
├──────────────────────────────────────────────────────────────────────────────┤
│ 就绪                     │ 00:12:34               │ 62 WPM                  │ ← 20px 状态栏
└──────────────────────────────────────────────────────────────────────────────┘
   └──────────────────────── 480 px ────────────────────────┘
```

盘下读数为**一块面板内的合并显示**：档位字符 + 数值，读作 `D 62`。
档位闸按当前变速模式只显示相关档位：自动挡 `P R N D`，手动挡 `R N M1 M2 M3`。
当前档在闸中反白（深蓝底白字），**R 档**在读数与闸中均为红色。

### 2.2 仪表盘放大示意（半径关系）

```
                R_FACE   = 78    ← 白色盘面（⌀156）
                R_BEZEL  = 86    ← 圆形凸起表圈（左上高光 / 右下阴影）
                R_PANEL  = 92    ← 方形凹陷仪表板 184×184

        角度换算  ang(w) = 210° − 2° × w      （0° = 3 点方向，逆时针为正）

                        w=60 在 90°  ┌─ 12 点
                                    │
       w=0 在 210°  ╭────── 0 ──────┼───── 100 ──────╮  w=120 在 −30°
        8 点        │      ╲        │        ╱   ▓▓   │  4 点
                    │       ╲       │       ╱    ▓▓   │
                    │        ╲      │      ╱     ▓▓   │
                    │         ╲     │     ╱            │
                    │          ╲    │    ╱             │
                    │           ╲   │   ╱              │
                    │            ╲  │  ╱               │
                    │             ╲ │ ╱                │
                    │              ╲│╱                 │
                    │               ●  = 轴心          │
                    │              WPM  = 单位          │
                    │            ┌──────────┐           │
                    │            │    62    │  = 盘内嵌读数
                    ╰────────────└──────────╯───────────╯
                      ↑ 红刻度线（峰值记忆）画在 ang(最高WPM)

  半径分层（自外向内）：
    74 ─ 78   红区环带（Polygon 近似环扇形，100 → 120 WPM）
    64 ─ 74   主刻度（每 20 WPM，2 px 宽，黑）
    67 ─ 74   中刻度（每 10 WPM，2 px 宽，黑）
    70 ─ 74   次刻度（每  5 WPM，1 px 宽，#808080）
    52        数字 0 / 20 / … / 120（Arial Bold 8pt，黑）
    27        单位字样 WPM / KPM（Arial Bold 8pt，#808080，正下方 270°）
    ─ 62      指针尖端
    −14 ─ 0   指针配重尾
    0 ─ 7     轴心（#C0C0C0 圆 + 黑边 + r=2 黑点心）

  盘内嵌读数（客户区 88,171 → 172,193，84×22）：
    2 px 下凹边框 + 白底 + Arial Bold 17px 黑字，居中。
    它在指针扫掠范围之下 —— 240° 弧从 8 点顺时针到 4 点，指针永不正指正下方 ——
    因此任何时候都不会被指针遮住。
```

---

## 3. 控件尺寸与坐标表

**坐标系**：全部为**客户区坐标**，原点在客户区左上角，单位 px（96 DPI）。矩形写作 `(left, top, right, bottom)`，`right`/`bottom` 为**开区间**（与 Win32 `RECT` 一致）。

### 3.1 骨架

| 元素 | 矩形 | 宽×高 |
|---|---|---|
| 菜单栏 | `(0, 0, 474, 20)` | 474 × 20 |
| 内容区 | `(0, 20, 474, 276)` | 474 × 256 |
| 状态栏 | `(0, 276, 474, 296)` | 474 × 20 |

### 3.2 GroupBox

| 元素 | 矩形 | 宽×高 | 标题 |
|---|---|---|---|
| 速度表 | `(4, 27, 256, 271)` | 252 × 244 | `速度表` |
| 里程表 | `(260, 27, 470, 141)` | 210 × 114 | `里程表` |
| 速率 | `(260, 151, 470, 271)` | 210 × 120 | `速率` |

> GroupBox 标题画在**上边框线上**，即 `(left+9, top−5)`，字高 11 px。这条规则决定了盒子顶部至少要离菜单栏 7 px，否则标题会压到菜单栏——本设计的 `top = 27` 正是为此。

### 3.3 速度表内部

| 元素 | 矩形 / 中心 | 尺寸 |
|---|---|---|
| 方形仪表板 | `(38, 40, 222, 224)` | 184 × 184 |
| 表盘中心 | `(130, 132)` | — |
| 圆形表圈 | 以中心为心，R = 86 | ⌀ 172 |
| 白色盘面 | R = 78 | ⌀ 156 |
| 红区环带 | R 74 → 78，ang(100) → ang(120) | 4 px 宽 |
| 主刻度 | R 64 → 74，每 20 WPM | 2 px 宽 |
| 中刻度 | R 67 → 74，每 10 WPM | 2 px 宽 |
| 次刻度 | R 70 → 74，每 5 WPM | 1 px 宽 |
| 数字环 | 中心半径 R = 52 | Arial Bold 8pt |
| 单位字样 | R = 27，正下方（270°） | Arial Bold 8pt |
| 指针 | 尖端 R = 62，尾部 R = −14 | 半宽 3.2 / 2.4 |
| 轴心 | R = 7 + 中心点 R = 2 | — |
| 峰值记忆标 | R 62 → 76，红色 3 px | 位于 ang(最高WPM) |
| **盘内嵌读数** | `(88, 171, 172, 193)` | 84 × 22 |
| **盘下合并读数（档位+数值）** | `(72, 228, 188, 264)` | 116 × 36 |

盘下合并读数居中于表盘中心（116 宽，72 → 188，中心 130）。
盘内嵌读数的下沿（193）距盘面下沿（210）留 17 px。

合并读数内部两个锚点（距面板左边）：

| 内容 | 锚点 | 字体 | 颜色 |
|---|---|---|---|
| 档位字符 | `+18` | Arial Bold 29px（`M1`–`M3` 用 20px） | R 档红，其余黑 |
| 数值 | `+50` | Arial Bold 29px | 黑 |

两者都**左对齐固定锚点**：数值位数变化（`8` → `62` → `120`）时不会左右跳动，读数区自己占住位置。

### 3.4 里程表内部

| 元素 | 矩形 | 尺寸 | 说明 |
|---|---|---|---|
| 标题 `ODO:` | `(270, 39)` | 11 px 高 | 宋体 9pt |
| ODO 滚轮 | `(270, 53, 460, 83)` | 190 × 30 | **7 位**，格宽 26 |
| 标题 `TRIP:` | `(270, 89)` | 11 px 高 | |
| TRIP 滚轮 | `(348, 103, 460, 133)` | 112 × 30 | **4 位**，右对齐到 460 |
| 按钮 `清零(C)` | `(270, 105, 340, 131)` | 70 × 26 | 原生 BUTTON |

滚轮内部结构：`EDGE_SUNKEN` 2 px 边框 + 2 px 内边距 → 内容区 182 × 22 → 7 格 × 26 px。格与格之间用 1 px `#DFDFDF` + 1 px `#FFFFFF` 竖向凹线分隔，形成"每个数字一个窗口"的机械计数器观感。

### 3.5 速率内部

| 元素 | 矩形 | 尺寸 |
|---|---|---|
| 标题 `KPM:` | `(270, 166)` | 11 px 高 |
| KPM 读数窗 | `(368, 160, 460, 184)` | 92 × 24 |
| 标题 `MAX:` | `(270, 194)` | 11 px 高 |
| MAX 读数窗 | `(368, 188, 460, 212)` | 92 × 24 |
| **换挡显示（档位闸）** | `(270, 216, 460, 238)` | 190 × 22 |
| 按钮 `复位(R)` | `(270, 241, 382, 267)` | 112 × 26 |

档位闸按变速模式显示 4 槽（自动挡 `P R N D`）或 5 槽（手动挡 `R N M1 M2 M3`），整组居中。

### 3.6 状态栏分格

| 格 | x 范围 | 宽 | 内容 |
|---|---|---|---|
| 1（弹性） | 1 → 235 | 235 | `就绪` / 提示文字 |
| 2 | 236 → 353 | 118 | `已运行 00:12:34` |
| 3 | 354 → 472 | 119 | `最高 88 WPM` |

分格线：每格左边界前 2 px 画 1 px `#808080` + 1 px `#FFFFFF`（凹刻竖线）。
文字统一在 `(格左 + 3, 状态栏顶 + 4)`。

### 3.7 字体清单

| 用途 | 字体 | 字高 | 字重 |
|---|---|---|---|
| 菜单、GroupBox 标题、静态文字、按钮 | **宋体** | 12 px（9pt） | 常规 |
| 标题栏文字 | 宋体 | 12 px | 粗 |
| 表盘数字 / 单位字样 | Arial | 11 px（8pt） | 粗 |
| 里程滚轮数字 | Arial | 20 px（15pt） | 粗 |
| KPM / MAX / 盘内嵌读数 | Arial | 17 px | 粗 |
| 大号读数、单字档位 | Arial | 29 px（22pt） | 粗 |
| `M1` / `M2` / `M3` 档位 | Arial | 20 px | 粗 |
| 档位闸字母 | Arial | 14 px | 粗 |
> **为什么中文用宋体 9pt**：Win98SE 简体中文版的系统默认 GUI 字体就是宋体 9pt（12 px 点阵）。这个尺寸下有内嵌点阵字形，渲染出来是**没有抗锯齿的硬边像素字**——正是"1999 年真的存在过的软件"的字感。**不要**用微软雅黑 / 思源黑体 / Segoe UI。
>
> 正确取值方式：`SystemParametersInfo(SPI_GETNONCLIENTMETRICS, …, &ncm, 0)` 取 `ncm.lfMessageFont` 作为对话框字体；仪表数字再单独 `CreateFont()` 建 Arial Bold。

---

## 4. Win98 风格配色建议

### 4.1 "Windows 标准"方案（钉死值）

| 系统角色 | RGB | HEX | 用途 |
|---|---|---|---|
| `COLOR_BTNFACE` / `3DFACE` | 192,192,192 | `#C0C0C0` | **界面主底色**、凹槽内底、按钮面 |
| `COLOR_BTNSHADOW` / `3DSHADOW` | 128,128,128 | `#808080` | 右下阴影、下凹边框、次要文字 |
| `COLOR_BTNHIGHLIGHT` / `3DHILIGHT` | 255,255,255 | `#FFFFFF` | 左上高光 |
| `COLOR_3DLIGHT` | 223,223,223 | `#DFDFDF` | 内层高光（凸起）/ 内层亮线（下凹） |
| `COLOR_3DDKSHADOW` | 128,128,128 | `#808080` | 最外层暗边 |
| `COLOR_WINDOW` | 255,255,255 | `#FFFFFF` | 表盘面、数字窗、滚轮底 |
| `COLOR_WINDOWTEXT` / `BTNTEXT` | 0,0,0 | `#000000` | 正文、数字、指针 |
| `COLOR_GRAYTEXT` | 128,128,128 | `#808080` | 次刻度、单位字样、禁用文字 |
| `COLOR_MENU` | 192,192,192 | `#C0C0C0` | 菜单栏底 |
| `COLOR_ACTIVECAPTION` | 0,0,128 | `#000080` | 标题栏左端 |
| `COLOR_GRADIENTACTIVECAPTION` | 166,202,240 | `#A6CAF0` | 标题栏右端（**Win98 标志性渐变**） |
| `COLOR_CAPTIONTEXT` | 255,255,255 | `#FFFFFF` | 标题文字 |
| `COLOR_HIGHLIGHT` | 0,0,128 | `#000080` | 进度条填充块 |
| `COLOR_INFOBK` | 255,255,225 | `#FFFFE1` | 工具提示底 |
| `COLOR_WINDOWFRAME` | 0,0,0 | `#000000` | 窗口最外 1 px 黑边 |

仪表盘额外用到的两种墨色，都在 Win98 16 色安全集内：

| 名称 | RGB | 用途 |
|---|---|---|
| 红 | 255,0,0 | 红区环带、峰值记忆标 |
| 深红 | 128,0,0 | （备用）指针可选配色 |

### 4.2 标题栏渐变

Win98 默认的标题栏是**横向渐变**，不是纯色（这是 Win98 区别于 Win95 的视觉签名）：

```
x = left … right
f = (x − left) / (right − left)
color = ACTIVECAPTION + (GRADIENTACTIVECAPTION − ACTIVECAPTION) × f
        #000080 ──────────────────────────────► #A6CAF0
```

实现：自绘标题栏时用 `for` 循环逐列 `SetPixel`/`FillRect`；或者建 `HMENU` 之外的纯色替代。**注意**：如果让系统画标题栏（推荐，Win98SE 上最正宗），渐变由系统负责，代码里什么都不用做。

### 4.3 ⚠️ 关键工程决定：钉死 vs 读系统

**本机实测**：这台 Windows 11 的 `HKCU\Control Panel\Colors` 给的是**现代配色**，不是 Win98 配色：

```
ButtonFace = 240,240,240   ← 现代（Win98 应为 192,192,192）
ButtonShadow = 160,160,160 ← 现代（Win98 应为 128,128,128）
ButtonDkShadow = 105,105,105
ActiveTitle = 153,180,209  ← 现代蓝色（Win98 应为 0,0,128）
```

**所以设计上必须二选一，并明确写下来：**

| 方案 | 做法 | 适用 |
|---|---|---|
| **A. 尊重用户方案（推荐，Win98SE 上正确）** | 全部颜色走 `GetSysColor()`，不写死任何 RGB | 真机 Win98SE、或任何用户自定义了配色的机器 |
| **B. 钉死经典配色** | 用 §4.1 的常量，完全不调 `GetSysColor` | 在现代系统上要"演出" Win98 观感时 |

**建议实现**：主路径用 A（这是 1999 年软件的正确行为），另加一个隐藏开关（如 `KeyODO.ini` 里的 `ForceClassicScheme=1`）切到 B。仪表盘里的红/深红不受配色方案影响，始终写死。

`COLOR_3DLIGHT` 取 `#DFDFDF` 而不是某些注册表转储里的 `192,192,192`：理由见 [`docs/COLOR.md`](docs/COLOR.md)——只有 `#DFDFDF` 才能重现经典下凹输入框"右下角一条 `#DFDFDF` 亮线压在白色之外"的特征外观。

---

## 5. WPM 仪表盘样式（主视觉）

这是整个界面唯一没有原生控件对应物的部分，也是"像不像 1999 年软件"的关键。**全部用 GDI 图元绘制，不使用任何位图、不使用任何抗锯齿、不使用任何渐变。**

### 5.1 角度映射

```
ang(w) = 210° − 240° × (w / 120)      度数，0° = 3 点方向，逆时针为正

w =   0  →  210°   （8 点方向，左下）
w =  30  →  150°   （10 点方向）
w =  60  →   90°   （12 点方向，正上）
w =  90  →   30°   （2 点方向）
w = 120  →  −30°   （4 点方向，右下）
```

即经典的**顺时针 240° 扫角**，起点在左下、终点在右下——和真车速度表完全一致。

屏幕坐标换算：

```c
x = cx + (int)(r * cos(ang * PI / 180.0));
y = cy - (int)(r * sin(ang * PI / 180.0));   /* y 轴翻转 */
```

### 5.2 绘制顺序（严格按此次序，否则会互相覆盖）

| # | 元素 | GDI 调用 | 要点 |
|---|---|---|---|
| 1 | 方形仪表板 | `DrawEdge(EDGE_SUNKEN)` + `FillRect(BTNFACE)` | 2 px 下凹，形成"嵌进面板的仪表" |
| 2 | 圆形表圈 | `Ellipse` ×3 + `Arc` ×4 | 见 5.3 |
| 3 | 白色盘面 | `Ellipse`（`WINDOW` 刷 + `BTNSHADOW` 1 px 笔） | |
| 4 | 红区环带 | `Polygon`（环扇形，见 5.4） | 4 px 宽，100→120 WPM |
| 5 | 刻度 | `Polygon`（细长四边形）× 49 | 每 5 WPM 一根 |
| 6 | 数字 0…120 | `TextOut`（`SetBkMode(TRANSPARENT)`） | 每 20 WPM 一个 |
| 7 | 单位字样 | `TextOut` | R=27，正下方；跟随所选模式显示 `WPM` 或 `KPM` |
| 8 | 峰值记忆标 | `Polygon`（红色 3 px 径向条） | 位于最高 WPM 角度 |
| 9 | 指针 | `Polygon`（5 点，带配重尾） | |
| 10 | 轴心 | `Ellipse` + `Ellipse` | |
| **11** | **盘内嵌读数** | `DrawEdge(EDGE_SUNKEN)` + `TextOut` | **盘面内，单位字样正下方，84×22** |
| **12** | **盘下合并读数** | 同上，Arial Bold 22pt | 档位字符 + 数值同一块面板，读作 `D 62` |
| **13** | **换挡显示** | 同上 | 在速率箱，按变速模式只画相关档位 |

第 11 步能在盘内安全落位，是因为 240° 扫角不含正下方（见 5.1），指针的扫掠区域到不了那里。

### 5.2.1 读数模式切换（WPM ⇄ KPM）

一个模式变量同时驱动三处，保证永远自洽：

| 位置 | 受什么影响 |
|---|---|
| 盘面单位字样 | 直接打印 `WPM` / `KPM` |
| 盘内嵌读数 | 显示当前模式的值 |
| 盘下合并读数 | 显示当前模式的值 |
| 状态栏第 3 格 | `62 WPM` / `310 KPM` |
| 读数模式 / 变速模式 | WPM ⇄ KPM、自动挡 ⇄ 手动挡 |
| **指针** | **始终显示速度**：选 KPM 时用 `KPM / 5` 驱动，指针不会跳档 |

切换入口：菜单「查看 → 显示 KPM(&K)」，快捷键 `Ctrl+K`。

### 5.3 圆形表圈的立体做法（GDI 没有 `DrawEdge` 的圆形版本）

用**沿 45°/225° 对角线劈开的两段圆弧**模拟左上打光：

```c
/* 外圈：左上高光、右下阴影 */
SelectObject(hdc, pen(BTNHIGHLIGHT, 1));
Arc(hdc, ..., pt(45°), pt(225°));       /* 逆时针 45°→90°→180°→225° */
SelectObject(hdc, pen(BTNSHADOW, 1));
Arc(hdc, ..., pt(225°), pt(45°));       /* 另一半 */

/* 内圈：方向相反，形成"凸起环" */
SelectObject(hdc, pen(3DDKSHADOW, 1));  Arc(..., pt(225°), pt(45°));
SelectObject(hdc, pen(3DLIGHT,    1));  Arc(..., pt(45°),  pt(225°));
```

`Arc` 的角度约定：0° = 3 点，逆时针递增，**从起点逆时针画到终点**。所以"上半圈"是 `Arc(45° → 225°)`。

### 5.4 环扇形（红区）的画法

Win98SE 无 `GradientFill` 可靠保证，也最好不要依赖路径 API。**最稳的做法是用一个 `Polygon` 近似环扇形**：

```c
POINT pts[2 * N];
/* 外弧 ang(100) → ang(120)，N 个点 */
for (i = 0; i < N; i++)  pts[i]     = polar(cx, cy, R_OUT, lerp(a0, a1, i/(N-1)));
/* 内弧反向回来 */
for (i = 0; i < N; i++)  pts[N + i] = polar(cx, cy, R_IN,  lerp(a1, a0, i/(N-1)));
Polygon(hdc, pts, 2 * N);      /* N = 16 时肉眼已完全平滑 */
```

同一招也用于刻度线和指针——**全项目只用 `Polygon` 一种填充图元**，兼容性最稳。

### 5.5 机械滚轮（ODO / TRIP）

```
外层：DrawEdge(EDGE_SUNKEN, BF_RECT) + FillRect(WINDOW)
内边距 2 px  →  内容区 7 格 × 26 px 宽 × 22 px 高
每格：数字居中（Arial Bold 15pt 黑），格间 1 px #DFDFDF + 1 px #FFFFFF 竖线
```

ODO 固定 **7 位**前导零（`0001234`），TRIP 固定 **4 位**（`0000`）右对齐——和真车的总里程表 / 小里程表布局一致。前导零是刻意的：机械滚轮不会隐藏零位。

### 5.6 档位显示（合并读数 + 换挡显示）

原「负载条」位置改为换挡显示。档位在两处出现，两处都只显示当前变速模式下的档位。

```
档位读数（速度箱，72,228 → 188,264，116×36）  → 一块面板内 "D 62"
换挡显示（速率箱，270,216 → 460,238，190×22） → 档位闸，当前档反白
```

**变速模式**（菜单：查看 → 自动挡 / 手动挡，`Ctrl+T` 切手动）：

| 模式 | 档位集 | 闸中槽位数 |
|---|---|---|
| 自动挡 `AUTO` | `P R N D` | 4 |
| 手动挡 `MANUAL` | `R N M1 M2 M3` | 5 |

自动挡只显示 `P R N D`，手动挡只显示 `R N M1 M2 M3` —— 两个集合互不混淆。
档位名用**字符串**而不是下标，两个集合就不必共享编号，切模式时也不会串档。

**档位读数**：`draw_gear_value()`。档位字符 Arial Bold 29px（`M1`–`M3` 用 20px），数值 Arial Bold 29px，两者都左对齐固定锚点（`+18` / `+50`），中间自然留出一个空格宽度的间隔，读起来就是 `D 62`。
**R 档红色 `#FF0000`**，其余档黑色 —— 倒挡用红色是汽车惯例，也让"刚按了退格"在余光里就能看见。

**档位闸**：2 px 下凹边框 + 白底，按当前模式的档位集等分槽位。

| 状态 | 底 | 字 |
|---|---|---|
| 当前档 | `COLOR_HIGHLIGHT` `#000080` 反白块 | `COLOR_HIGHLIGHTTEXT` `#FFFFFF` |
| 其它档 | `COLOR_WINDOW` `#FFFFFF` | `COLOR_GRAYTEXT` `#808080` |

槽宽 = `min(34, 可用宽度 / 档位数)`，整组居中 —— 自动挡 4 槽、手动挡 5 槽都居中排布，不会因为槽少而拉散。
数字用 Arial Bold 14px 居中。

档位语义（本阶段只定义显示，不定义判定逻辑）：

| 档 | 含义 | 触发键 |
|---|---|---|
| `P` | 停车 | 无输入活动 |
| `R` | 倒挡 | Backspace |
| `N` | 空挡 | Space |
| `D` | 前进 | 普通字符键 |
| `M1` `M2` `M3` | 手动挡三档 | 自动显示 |

档位闸的位置是原来的负载条，尺寸未变（270,216 → 460,238），只是把「下凹槽 + 8/2 分块进度条」换成了「下凹槽 + 槽位闸」。

**一致性约束**：当前档必须属于当前模式的档位集。后端在切换模式时负责把档位收敛到新集合内；界面不显示集合外的档位。

### 5.7 重绘与闪烁

指针每秒都在动，必须双缓冲，否则 1999 年的机器上会闪成灾难：

```c
case WM_ERASEBKGND:  return TRUE;                    /* 阻止背景擦除 */
case WM_PAINT:
    hdc = BeginPaint(hwnd, &ps);
    memDC  = CreateCompatibleDC(hdc);
    memBmp = CreateCompatibleBitmap(hdc, 474, 296);  /* 或只建表盘那块 */
    SelectObject(memDC, memBmp);
    /* …全部绘制… */
    BitBlt(hdc, 0, 0, 474, 296, memDC, 0, 0, SRCCOPY);
    DeleteObject(memBmp); DeleteDC(memDC);
    EndPaint(hwnd, &ps);
    return 0;
```

**只重画表盘区域**（`(36,38)-(224,226)` 外扩 2 px）而不是整个客户区，是让老机器跑得动的关键。

---

## 6. 需要准备的图标 / 位图资源

**好消息：这个界面几乎不需要位图。** 表盘、滚轮、档位闸、边框全部是 GDI 程序化绘制，这既是最正宗的 Win98 做法，也免去了所有 DPI 适配问题。

| # | 资源 | 尺寸 | 格式 | 说明 |
|---|---|---|---|---|
| 1 | 应用图标 | **16×16** | `.ICO`，**16 色** | 标题栏 + 任务栏。**必须**是 16 色、无 alpha 通道的 BMP-in-ICO；32 位带 alpha 的图标在 Win98SE 上会显示成黑块 |
| 2 | 应用图标（大） | **32×32** | `.ICO`，16 色 | `Alt+Tab` 切换器、资源管理器 |
| 3 | —— | —— | —— | 不需要 48/256 px：Win98SE 用不到，加了只会让人一眼看出是现代软件重打包 |
| 4 | 光标 | 系统标准 | `IDC_ARROW` / `IDC_WAIT` | 不要自造光标 |
| 5 | 工具提示 | 无位图 | —— | `COLOR_INFOBK` 纯色底 + 1 px 黑边 |

**明确不要的资源**（这些是"现代复古主题"的味道来源）：

* ❌ 任何 PNG 资源（Win98SE 的 GDI 不支持 PNG）
* ❌ 任何 32 位带 alpha 的位图
* ❌ 任何 256×256 或更大的图标
* ❌ 任何渐变 / 阴影 / 噪点贴图
* ❌ 任何"复古材质"皮肤图

图标设计建议：16×16 的**键盘侧视 + 一排小滚轮**，或者干脆沿用 Windows 自带的经典键盘图标（见 §7.5 的实际可用资产清单）。手工画 16 色的 16×16 图标是这个项目里**唯一值得花时间的美术工作**。

---

## 7. 直接使用电脑内置的 Win98 艺术资产

这一节回答"怎么才算真的用了系统自带资产"。分三层，从最重要到最次要。

### 7.1 第一层（最重要）：Win98 的界面是**程序化生成**的

Win98 的立体感不是图片，是 `DrawEdge()` / `DrawFrameControl()` / `DrawStatusText()` 三个 API 现算的。**你把这三个调用的参数配对，就等于拿到了全套原生皮肤。** 下面这张表是把真实 GDI 渲染结果逐像素读回来后整理出的**实测配方**（原始数据见 [`reports/gdi_edge_probe.txt`](reports/gdi_edge_probe.txt)）：

| `DrawEdge` 组合 | 左上·外 | 左上·内 | 右下·内 | 右下·外 |
|---|---|---|---|---|
| `EDGE_RAISED` | `3DLIGHT` | `BTNHIGHLIGHT` | `BTNSHADOW` | `3DDKSHADOW` |
| `EDGE_SUNKEN` | `BTNSHADOW` | `3DDKSHADOW` | `3DLIGHT` | `BTNHIGHLIGHT` |
| `EDGE_ETCHED` | `BTNSHADOW` | `BTNHIGHLIGHT` | `BTNSHADOW` | `BTNHIGHLIGHT` |
| `EDGE_BUMP` | `3DLIGHT` | `3DDKSHADOW` | `3DLIGHT` | `3DDKSHADOW` |
| `BDR_SUNKENOUTER`（单线） | `BTNSHADOW` | — | — | `BTNHIGHLIGHT` |
| `BDR_SUNKENINNER`（单线） | `3DDKSHADOW` | — | — | `3DLIGHT` |

`DrawFrameControl` 实测（同样是读回的像素）：

| 控件 | 左上·外 | 左上·内 | 面 | 右下·内 | 右下·外 |
|---|---|---|---|---|---|
| 按钮（常态） | `BTNHIGHLIGHT` | `3DLIGHT` | `BTNFACE` | `BTNSHADOW` | `3DDKSHADOW` |
| 按钮（按下） | `3DDKSHADOW` | `BTNSHADOW` | `BTNFACE` | `3DLIGHT` | `BTNHIGHLIGHT` |
| 复选框 | `BTNSHADOW` | `3DDKSHADOW` | `WINDOW` | `3DLIGHT` | `BTNHIGHLIGHT` |
| 标题栏按钮 / 滚动箭头 | 同"按钮（常态）" | | | | |
| **工具栏按钮·选中**（`DFCS_FLAT\|DFCS_CHECKED`） | 1 px `BTNSHADOW` 包边 + 内部 **50% 棋盘抖动**（`BTNFACE` 与 `WINDOW` 交错，逐行错位） | | | | |

最后一行是本次实测最有价值的发现之一：Win98 工具栏按钮的"按下"态不是纯色，而是**抖动网点**。这是老软件的标志性纹理，值得在 KeyODO 的按钮/切换项上复用。

`DrawStatusText(NULL 边框)` 实测：**1 px**，左上 `BTNSHADOW`、右下 `WINDOW`。这就是状态栏边框的全部秘密。

> 直接 `#include <windows.h>` 调这些 API，一行皮肤代码都不用写。**前提是把配色钉在 Windows 标准方案上**（§4.3），否则现代系统上会画出 `#F0F0F0` 的"Win98 形状、Win11 颜色"四不像。

### 7.2 第二层：本机现存的真实 Win98 字体资产（已实测存在）

| 文件 | 是什么 | 怎么用 |
|---|---|---|
| `C:\Windows\Fonts\sserife.fon` | **MS Sans Serif 8/10/12/…** —— Win98 英文版系统字体本体，点阵 | `CreateFont(11, …)` 或直接 `SYSTEM_FONT`；英文界面用 |
| `C:\Windows\Fonts\vgasys.fon` | System 点阵字体（Win3.x 遗物） | 需要极致复古时用 |
| `C:\Windows\Fonts\dosapp.fon` | Terminal / 终端点阵字体 | 数字读数窗的备选 |
| `C:\Windows\Fonts\marlett.ttf` | **MARLETT** —— Win95/98 界面字形字体（滚动箭头、菜单对勾、标题栏按钮、尺寸柄） | 见 §7.4 的重要限制 |
| `C:\Windows\Fonts\lucon.ttf` | Lucida Console | 等宽数字备选 |

**中文部分**：Win98SE 简体中文版的系统字体是 **宋体 9pt**（`simsun.ttc` 在 96 DPI 下用内嵌 12 px 点阵）。直接用，效果就是硬边像素中文。

### 7.3 第三层：`DrawFrameControl` 免费提供的字形

不需要找任何图标资源，下面这些全部由系统现画，且在 96 DPI 下逐像素正确：

```c
DrawFrameControl(hdc, &rc, DFC_CAPTION, DFCS_CAPTIONMIN | DFCS_CAPTIONMAX | DFCS_CAPTIONCLOSE);
DrawFrameControl(hdc, &rc, DFC_BUTTON,  DFCS_BUTTONCHECK | DFCS_CHECKED);
DrawFrameControl(hdc, &rc, DFC_BUTTON,  DFCS_BUTTONRADIO | DFCS_CHECKED);
DrawFrameControl(hdc, &rc, DFC_SCROLL,  DFCS_SCROLLUP | DFCS_SCROLLDOWN);
DrawFrameControl(hdc, &rc, DFC_SCROLL,  DFCS_SCROLLCOMBOBOX);
DrawFrameControl(hdc, &rc, DFC_SCROLL,  DFCS_SCROLLSIZEGRIP);
DrawFrameControl(hdc, &rc, DFC_MENU,    DFCS_MENUCHECK | DFCS_MENUARROW);
```

### 7.4 ⚠️ 关于 MARLETT 的实测结论（重要，避免踩坑）

`MARLETT.TTF` 确实还在本机，但**它的 cmap 只有 `(3,0)` 符号表**（段 `0xF030–0xF039`、`0xF057`、`0xF061–0xF079`…），**没有 Unicode 子表**。

实测结果：

* 用 FreeType / Pillow 按字符码 `0x20–0xFF` 渲染 → **224 个槽位全部返回同一个 7×11 方框**（`.notdef`），一个真字形都拿不到。
* 用 `0xF0xx` 符号码渲染 → 同样全是 `.notdef`。

**结论**：

* ✅ **在真正的 GDI 里可用**：`SelectObject(hFont)` 选入 MARLETT 后 `TextOut(hdc, x, y, "r", 1)`，GDI 会按符号字体规则把 `0x20–0xFF` 映射到 `0xF020–0xF0FF`，字形正常。这是 1999 年程序的标准用法。
* ❌ **不要试图用第三方渲染库或工具（FreeType、Pillow、在线字体预览）去"提取"MARLETT 的字形**，拿不到。
* 💡 **更省事的替代**：§7.3 的 `DrawFrameControl` 能画出同样这些字形，而且连颜色都按系统方案走。KeyODO 里**优先用 `DrawFrameControl`**，只有在需要"系统画不出来"的特定箭头时才 `TextOut` MARLETT。

### 7.5 已提取的系统内置位图/图标（实测清单）

本机对 54 个 PE 文件做了完整资源提取（8992 条资源，0 错误），完整清单见 [`assets/ASSETS.md`](assets/ASSETS.md)。与 KeyODO 相关度最高的：

| 资产 | 来源 | 资源 ID | 尺寸 | 备注 |
|---|---|---|---|---|
| 经典工具栏条（小） | `comctl32.dll` v5.82 | **RT_BITMAP 120** | 240×16，15 个 16×16 | `IDB_STD_SMALL_COLOR`，4bpp **RLE4** 压缩 |
| 经典工具栏条（大） | 同上 | **121** | 360×24，15 个 24×24 | `IDB_STD_LARGE_COLOR` |
| 视图工具栏条 | 同上 | 124 / 125 | 192×16 / 288×24 | `IDB_VIEW_*`，**仅 v5 构建有此 4bpp 原版** |
| 历史工具栏条 | 同上 | 130 / 131 | 80×16 / 120×24 | `IDB_HIST_*` |
| 标题栏按钮字形位图 | 同上 | 132 / 133 / 134 | 28×28，4bpp RLE4 | 与 user32 的 32661/32559/32660 **SHA-256 完全一致** |
| 资源管理器工具栏条 | `shell32.dll.mun` | 命名资源 `IDB_TB_SH_DEF_16` | 768×16，48 个 16×16 | 经典 IE4 风格 |
| Windows 3.x PIF 图标（含飞窗旗） | `pifmgr.dll` | 38 个 `RT_GROUP_ICON`（id 1–9, 11–39） | 32×32，4bpp | 极复古，适合当"关于"对话框插图 |
| MS-DOS/Win3.x 应用图标库 | `moricons.dll.mun` | 113 个 `RT_GROUP_ICON` | 32×32，4bpp+1bpp | Word/Excel/WordPerfect/Lotus… |
| 经典 OLE 横幅 | `oledlg.dll` | 10 / 11 / 12 | 480×30 / 480×40 / 720×60 | 4bpp |
| 经典光标 | `user32.dll` | 34 组，1bpp | — | 包括 I 型、十字、沙漏 |

### 7.6 两个必须知道的环境事实（实测，不是猜测）

**① 现代 Windows 上，真正的资源不在 `System32`，而在 `C:\Windows\SystemResources\<name>.mun`。**

本机实测：`shell32.dll`、`moricons.dll`、`imageres.dll`、`mmres.dll`、`main.cpl`、`intl.cpl`、`sysdm.cpl`、`timedate.cpl`、`themeui.dll` 在 `System32` 下**全是 MUI 空壳**，提取到 0 条资源；同一份代码改从对应的 `.mun` 读，`shell32.dll.mun` 得到 **2865** 条、`moricons.dll.mun` 得到 **350** 条。`browseui.dll` 则连 `.mun` 都不存在。

**② `OBM_*` 家族已经彻底消失。**

经典文档里常引用的 `OBM_CHECK(32760)`、`OBM_CHECKBOXES(32759)`、`OBM_RADIOIMAGE(32764)`、`OBM_BTNCORNERS(32758)`、`OBM_OLD_*` 等 22 个 ID，在**三个不同的 user32 二进制**（System32 / SysWOW64 / WinSxS 26100.1591）里**全部不存在**。现存的 user32 只有 7 个 `RT_BITMAP`：36、136、236、32559、32660、32661、32731。原因是 Windows 现在**在运行时合成**复选框 / 单选钮 / 箭头字形。

> **对设计的直接影响**：如果 KeyODO 打算"从 user32 里 `LoadBitmap(OBM_CHECKBOXES)` 拿经典勾选框"，这条路在现代系统上是断的。**正确做法**是 §7.3 的 `DrawFrameControl`——它在 Win98SE 和现代系统上都能画出正确结果，且不依赖任何资源 ID。这也是本次设计全程坚持"程序化绘制"的原因之一。

**③ 同样不存在的**：`comctl32` 的 122/123/126/127/128/129（无 `IDB_BROWSER_*` 家族）；`pifmgr.dll` 的图标组 id 10。这些在 `ASSETS.md` 里都标注为"不存在"，而不是悄悄跳过。

### 7.7 §7 小结：KeyODO 到底该"用"什么

| 优先级 | 做法 | 覆盖范围 |
|---|---|---|
| ★★★ | 直接调 `DrawEdge` / `DrawFrameControl` / `DrawStatusText`，配色走 §4.1 | 全部边框、按钮、勾选、箭头、标题栏按钮、状态栏 |
| ★★★ | 用系统字体：宋体 9pt（中文）+ Arial（仪表数字） | 全部文字 |
| ★★☆ | `Polygon` / `Ellipse` / `Arc` 程序化画仪表盘与档位 | 表盘、档位读数、换挡显示 |
| ★☆☆ | 从 `.mun` 提取的经典位图（§7.5） | 仅"关于"对话框插图、可选工具栏 |
| —— | 自定义 16×16 16 色应用图标 | 唯一需要手工绘制的资源 |

**一句话**：KeyODO 不需要任何皮肤文件，它需要的是**正确的 API 调用参数**。这就是"使用电脑内置 Win98 艺术资产"的完整答案。

---

## 8. Win98SE 兼容性硬性要求

### 8.1 字符集：用 ANSI，不要 Unicode

Win98SE 的 Win32 是 **ANSI 内核**。虽然提供 `…W` 函数，但内部要转换，且**资源不能是 UTF-16**。

* ✅ 全部用 `…A` 系列（`TextOutA`、`CreateFontA`、`CreateWindowExA`）或直接写 `TEXT()` 宏并**不定义 `UNICODE`**。
* ✅ `.rc` 用 `LANGUAGE LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED`，字符串按 **CP936（GBK）** 编码保存，不要 UTF-8 BOM。
* ✅ 编译选项加 `/D_UNICODE` 的反面：确保 `UNICODE` 和 `_UNICODE` **未定义**。

### 8.2 必须避免的 API（Win98SE 上不存在或行为不同）

| 禁止 | 原因 | 替代 |
|---|---|---|
| `GetWindowLongPtr` / `SetWindowLongPtr` / `LONG_PTR` | Win2000+ 才有 | `GetWindowLong` / `SetWindowLong` / `LONG` |
| `SetLayeredWindowAttributes`、分层窗口 | Win2000+ | 不用 |
| `AlphaBlend`、`GradientFill`（msimg32） | Win98SE 有 msimg32 但支持不完整 | 逐列 `FillRect` 手工渐变 |
| `DrawThemeBackground` / `OpenThemeData` / uxtheme | 不存在 | `DrawFrameControl` |
| `SetWindowCompositionAttribute`、DWM 系列 | 不存在 | 不用 |
| `MENUINFO` / `SetMenuInfo` | Win2000+ | 标准菜单即可 |
| `LoadIconMetric` | Vista+ | `LoadIcon` / `LoadImage` |
| GDI+（`GdiplusStartup`…） | Win98SE 无 gdiplus.dll | 纯 GDI |
| `SHGetKnownFolderPath` | Vista+ | `SHGetSpecialFolderPath` |
| `_snprintf` 之外的 C99 设施 | MSVC 6 无 | `wsprintf` / `_snprintf` |
| `//` 以外的 C99（变长数组、`long long`、`stdint.h`） | MSVC 6 无 | 用 `__int64`、自定义 typedef |

### 8.3 必须避免的 API（现代系统上会破坏经典外观）

| 禁止 | 原因 |
|---|---|
| **comctl32 v6 清单**（`Microsoft.Windows.Common-Controls` 6.0.0.0） | 一旦启用，按钮/进度条变成主题化外观，**整套 Win98 观感当场作废**。**故意不加清单文件**，让程序在现代系统上也拿到经典（无主题）控件。 |
| `SetProcessDPIAware` | Win98SE 无此 API；且 96 DPI 基准设计不需要 |

### 8.4 编译与目标宏

```c
#define WINVER        0x0410      /* Windows 98 */
#define _WIN32_WINDOWS 0x0410
#define _WIN32_IE     0x0400      /* IE4 级别的 shell API，Win98SE 安全 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
```

* 目标编译器：**MSVC 6.0**（1999 年工具链）或 MinGW-w64 + `-m32`。
* 必须 32 位（`x86`）。Win98SE 没有 64 位版本。
* `InitCommonControlsEx(ICC_BAR_CLASSES)` —— Win98SE 自带 comctl32 4.7x+。
  本设计不用 `ICC_PROGRESS_CLASS`：原来的负载条已改为自绘档位闸。

### 8.5 运行期自检（启动时做一次，写进状态栏或"关于"）

| 检查 | 方法 | 失败时 |
|---|---|---|
| 客户区尺寸是否符合设计 | `AdjustWindowRectEx` + `GetClientRect` | 若偏差 > 8 px，整体按 `MulDiv` 缩放布局常量 |
| 配色方案是否 Windows 标准 | 比较 `GetSysColor(BTNFACE)` 与 `#C0C0C0` | 决定是否切到"钉死经典配色"模式 |
| 是否 96 DPI | `GetDeviceCaps(hdc, LOGPIXELSX)` | ≠ 96 时按比例缩放 |

---

## 9. 明确禁止的视觉手法（反模式清单）

| ❌ 禁止 | 为什么 |
|---|---|
| 圆角（`RoundRect`、CSS 式圆角） | Win98 只有直角矩形和正圆/椭圆 |
| 卡片化布局、浮动面板、投影 | 1999 年没有这个概念 |
| Material / Fluent / 现代 Dashboard | 与目标完全相反 |
| 任何抗锯齿的文字或图形 | 表盘刻度、数字必须硬边 |
| 半透明、alpha 混合、毛玻璃 | Win98SE 无此能力 |
| 渐变（除标题栏那一处系统自带的） | 按钮、面板一律纯色 |
| 悬停高亮 / 平滑动画 / 缓动 | 1999 年是即时、无动画的 |
| 深色模式 | 不存在 |
| Segoe UI / 微软雅黑 / 思源黑体 | 现代字体，一眼假 |
| 自定义皮肤位图 | 见 §6 |
| 超过 16 色的自定义图标 | Win98SE 显示会劣化 |
| 高 DPI 缩放 / 矢量图标 | 目标只有 96 DPI |

---

## 10. 本阶段不做的事（逻辑后端）

按需求，本阶段**只做界面**。以下全部留待下一阶段，当前设计只需保证界面**有能力显示它们**：

* 全局键盘钩子（`WH_KEYBOARD_LL`）与按键计数
* WPM / KPM 的滑动窗口计算
* ODO / TRIP / 最高 WPM 的会话状态与 INI 持久化
* TRIP 清零、峰值复位的实际行为
* 系统托盘图标、开机自启
* 采样定时器（界面已按"每秒重绘一次表盘区域"设计，见 §5.7）

界面对后端的唯一契约：

| 方向 | 内容 |
|---|---|
| 后端 → 界面 | `当前WPM`、`最高WPM`、`ODO`、`TRIP`、`KPM`、`运行时长` |
| 后端 → 界面 | `档位`（必须是当前变速模式档位集里的字符串） |
| 界面 → 后端 | 清零 TRIP、复位最高值、切换读数模式、切换变速模式、显示/隐藏档位闸 |

读数模式、变速模式与档位闸的显示开关属于界面状态，后端只需要保证"档位 ∈ 当前模式的档位集"。

---

## 11. 0.1 实现阶段增补：缩放与桌面仪表

§1–§10 是 **96 DPI 基准下的视觉规范**，一个字都没有改：程序去适配它，而不是反过来。
但 0.1 需要真的跑在 1080p / 2K / 4K 上，因此补两件事。

### 11.1 整套规格按显示器 DPI 等比换算

`include/keyodo_layout.h` 里那组数字仍然是唯一规范。`src/ui/metrics.c` 把它们乘以
`显示器DPI / 96` 得到实际尺寸，字体也按同一比例重新创建。

有两点是刻意的：

* **不做位图拉伸。** 200% 下文字是按 2 倍重新栅格化的，而不是把 96 DPI 的位图放大——
  后者在中文点阵字上会糊成一团。相应地，1 px 的立体边框也按比例加粗（200% 下为 2 px），
  否则放大的界面会变得"发丝一样细"，失掉 Win98 的厚重感。
* **窗口尺寸按整体缩放后反推客户区**，而不是分别缩放客户区与外框。后者会四舍五入两次，
  在 125% / 150% 下多出 1 px（600×400 变成 601×401）。

100% 时每个换算值都等于规范里的常量，所以与设计稿的逐像素比对仍然成立。

### 11.2 桌面仪表（只含表盘）

菜单「查看 → 桌面仪表」把速度表组件单独抽成一个无边框悬浮窗口。

* **只画表盘**：表圈、表盘面、刻度、数字、指针、峰值刻度线、表盘内嵌读数。
  §1 里的分组框、标题、灰色方形面板、档位读数、状态栏全部不带。
* 表盘本身在**自己的坐标系**里绘制，主窗口用 `OffsetViewportOrgEx` 把它摆到面板位置，
  桌面仪表则直接画在原点。同一个 `Gauge_Draw` 服务两处，靠 `GaugeSpec.bare` 切换要不要
  那层方形面板，不存在两份画法。
* 窗口用 `SetWindowRgn` 裁成正圆：四角真正透明且点击穿透。不用分层窗口（Windows 2000+），
  也不用颜色键（会跟系统主题撞色）。
* 它是 `WS_POPUP` + `WS_EX_TOOLWINDOW` + 置顶：不进任务栏、不进 Alt+Tab。
* 位置与开关随统计数据一起保存在 `%APPDATA%\KeyODO\keyodo.ini` 的 `[Widget]`，
  越界位置会被拉回工作区。

颜色、边框配方、字体与主窗口完全共用（`theme.c` / `metrics.c`），所以桌面仪表和主窗口里
的表盘逐像素一致——`tools/compare_mockup.py` 与 `tools/widget_test.py` 一起守着这条。

### 11.3 指针的自然回落（§5.7 的修正）

§5.7 只规定了指针的平滑，没有规定**停手之后**的行为。滑动窗口的语义是"最近 N 秒内的
按键数"，因此停手后读数会平着保持整个窗口长度（默认 10 秒），然后垂直掉下来——指针
看起来像卡住了。真车的表针不是这样。

现在：键盘安静超过 600 ms 之后，速率本身乘以一个时间常数为 900 ms 的指数衰减因子。
宽限期不能省——正常打字的间隔是 100–300 ms，短暂思考不能被误判成停手。

指针的平滑常数也改成不对称：上升 140 ms、下落 260 ms，下落更"重"，读起来像带阻尼的表针。
衰减作用在速率上而不是只作用在指针上，所以指针、表盘内嵌读数、KPM 面板、状态栏四个
地方永远一致。

这条曲线由 `tools/needle_test.py` 从截图里量指针角度来验证，实测 43.5 WPM 在 2.9 秒内
平滑回落到 0，半程用时 0.95 秒。

### 11.4 自绘下拉菜单（§1 的补充）

§1 只规定到菜单栏，没有规定下拉菜单。0.1 一开始用系统 `TrackPopupMenu`，在 Windows 11 上
弹出来的是圆角、带阴影、现代字体的菜单，落在一整片钉死配色的 Win98 界面上显得像 bug。

现在由 `src/ui/menu.c` 自绘，与主窗口共用同一套 `theme.c` / `metrics.c`：

* 2px 突起边框（`TK_RAISED`），`COLOR_MENU` 底色，与分组框、按钮同一套配方
* 选中项 `COLOR_HIGHLIGHT` 底 + `COLOR_HIGHLIGHTTEXT` 字，与列表选中一致
* 勾选标记与单选圆点用 8×7 的位掩码手绘（不用 `DrawFrameControl`，那会被主题化）
* 助记符下划线、分隔线（阴影 + 高光两像素）、右对齐的快捷键列，都按 Win98 的画法
* 高亮项上勾选标记转为白色，这是 Win98 的实际行为

菜单文字和快捷键全部放在字符串表里（`IDS_MENU_*`，`\t` 分隔两列，`&` 标记助记符），
`.rc` 里**故意不再有 MENU 资源**——两份菜单文字迟早会不一致。

`tools/menu_test.py` 用投递消息的方式驱动（不依赖光标和可见桌面）：方向键走到
「换挡显示」→ 回车 → 退出后读 ini 确认设置真的翻转了；再验 Esc 不改动任何东西、
点击空白处能关掉。`tools/menu_capture.py` 用 `PrintWindow` 把每个菜单抓成 PNG。

---

## 附：交付物索引

| 文件 | 内容 |
|---|---|
| [`DESIGN.md`](DESIGN.md) | 本文件——完整界面设计说明书 |
| [`mockup/out/keyodo_480x320.png`](mockup/out/keyodo_480x320.png) | **1:1 视觉复核图**（480×320，WPM 模式，D 档） |
| [`mockup/out/keyodo_2x.png`](mockup/out/keyodo_2x.png) | 2 倍放大（看像素细节） |
| [`mockup/out/keyodo_client_2x.png`](mockup/out/keyodo_client_2x.png) | 客户区 2 倍放大（不含标题栏/边框） |
| [`mockup/out/keyodo_kpm_r.png`](mockup/out/keyodo_kpm_r.png) | KPM 模式 + 自动挡 R 档 |
| [`mockup/out/keyodo_park_p.png`](mockup/out/keyodo_park_p.png) | WPM 模式 + 自动挡 P 档 |
| [`mockup/out/keyodo_manual_m2.png`](mockup/out/keyodo_manual_m2.png) | 手动挡 M2 档（档位闸只显示 `R N M1 M2 M3`） |
| [`mockup/out/keyodo_states.png`](mockup/out/keyodo_states.png) | 合并读数示例 + 两种变速模式的全部档位 |
| [`mockup/keyodo_mockup.py`](mockup/keyodo_mockup.py) | **可执行的布局规范**：本文第 3 节每个坐标都在这里，且与将来的 C 代码一一对应 |
| [`mockup/win98kit.py`](mockup/win98kit.py) | Win98 渲染工具：配色表、实测边框配方、真实 GDI 控件重上色、真实 GDI 文字 |
| [`include/keyodo_layout.h`](include/keyodo_layout.h) | 布局常量头文件（直接 `#include` 进 C 工程） |
| [`res/keyodo.rc`](res/keyodo.rc) · [`res/resource.h`](res/resource.h) | 资源脚本（菜单 / 字符串表 / 图标 / 版本）与资源 ID —— 已是 0.1 程序实际使用的文件 |
| [`docs/COLOR.md`](docs/COLOR.md) | 配色取值的证据与推导 |
| [`reports/gdi_edge_probe.txt`](reports/gdi_edge_probe.txt) | 27 组立体边框图元的逐像素实测记录 |
| [`assets/ASSETS.md`](assets/ASSETS.md) | 系统内置 Win98 美术资产清单（54 个 PE，8992 条资源） |
