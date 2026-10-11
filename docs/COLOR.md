# 配色取值的证据与推导

本文说明 [`DESIGN.md` §4](../DESIGN.md) 里那张配色表的每个数字是怎么来的。设计文档中的颜色**不是凭记忆写的**，能实测的都实测了。

---

## 1. 结构（哪个环是什么颜色）——实测，无歧义

用真实 `user32!DrawEdge` / `user32!DrawFrameControl` 在一个 32 位 DIB 上渲染，再用 `GetPixel` 逐像素读回，并把每个像素的 RGB 反查成系统颜色角色名。

原始记录：[`reports/gdi_edge_probe.txt`](../reports/gdi_edge_probe.txt)（27 组图元）
可视化：[`reports/gdi_edge_probe.png`](../reports/gdi_edge_probe.png)

实测输出（节选）：

```
DrawEdge EDGE_RAISED (BDR_RAISEDOUTER|BDR_RAISEDINNER) BF_RECT
    top    : y=0 COLOR_3DLIGHT  y=1 COLOR_WINDOW  y=2 (fill)
    bottom : y=21 COLOR_3DDKSHADOW  y=20 COLOR_BTNSHADOW  y=19 (fill)
    left   : x=0 COLOR_3DLIGHT  x=1 COLOR_WINDOW
    right  : x=21 COLOR_3DDKSHADOW  x=20 COLOR_BTNSHADOW

DrawFrameControl DFC_BUTTON/DFCS_BUTTONPUSH (40x22)
    top    : y=0 COLOR_WINDOW  y=1 COLOR_3DLIGHT  y=2 COLOR_MENU
    bottom : y=21 COLOR_3DDKSHADOW  y=20 COLOR_BTNSHADOW  y=19 COLOR_MENU
```

由此得到结论：

* `BDR_RAISEDOUTER` = 左上 `3DLIGHT` / 右下 `3DDKSHADOW`
* `BDR_RAISEDINNER` = 左上 `BTNHIGHLIGHT` / 右下 `BTNSHADOW`
* `BDR_SUNKENOUTER` = 左上 `BTNSHADOW` / 右下 `BTNHIGHLIGHT`
* `BDR_SUNKENINNER` = 左上 `3DDKSHADOW` / 右下 `3DLIGHT`
* `DFCS_BUTTONPUSH` 用的是**反过来**的组合：最外层是 `BTNHIGHLIGHT`（白），往里才是 `3DLIGHT`；右下最外层是 `3DDKSHADOW`。也就是说按钮的凸起层次是"白 → 浅灰 → 面 → 深灰 → 最深"。

> 注意 `EDGE_RAISED` 和 `DFCS_BUTTONPUSH` 的最外环**方向相反**，这不是笔误，是两个不同代码路径的真实行为。设计里要分别对待，不能"统一成一种凸起画法"。

### 1.1 一处无法区分的地方（诚实标注）

本机实测中 `COLOR_WINDOW` = `#FFFFFF`，而 Win98 标准方案里 `COLOR_BTNHIGHLIGHT` 也是 `#FFFFFF`；同理本机 `COLOR_MENU` = `COLOR_BTNFACE` = `#F0F0F0`，Win98 里两者也都是 `#C0C0C0`。**在任何一个标准配色方案下这两组角色取值相同，探针无法区分，也没有区分的必要。** 只有当用户自定义出一个 `Menu ≠ ButtonFace` 的奇怪方案时才会露出差异——那种情况下 `DrawFrameControl` 由系统自己处理，不需要我们关心。

---

## 2. 数值（每个角色到底是多少）——部分实测、部分推导

`reports/gdi_edge_probe.txt` 里的"live"一组是**本机（Windows 11）**的值，**不是 Win98 的值**：

```
COLOR_BTNFACE/3DFACE        = #F0F0F0      ← 现代
COLOR_BTNSHADOW/3DSHADOW    = #A0A0A0      ← 现代
COLOR_3DLIGHT               = #E3E3E3      ← 现代
COLOR_3DDKSHADOW            = #696969      ← 现代
COLOR_ACTIVECAPTION         = #99B4D1      ← 现代
```

Win98 SE "Windows 标准"方案的值必须单独确定。推导依据如下。

### 2.1 确定的（多方一致，无争议）

| 角色 | 值 | 依据 |
|---|---|---|
| `BTNFACE` / `3DFACE` | `#C0C0C0` | 16 色 VGA 调色板的中灰，所有 Win9x 方案表一致 |
| `BTNSHADOW` / `3DSHADOW` | `#808080` | 同上 |
| `BTNHIGHLIGHT` / `3DHILIGHT` | `#FFFFFF` | 同上 |
| `WINDOW` | `#FFFFFF` | 同上 |
| `WINDOWTEXT` / `BTNTEXT` | `#000000` | 同上 |
| `MENU` / `SCROLLBAR` | `#C0C0C0` | 同上 |
| `ACTIVECAPTION` | `#000080` | Win95/98 默认标题栏深蓝 |
| `GRADIENTACTIVECAPTION` | `#A6CAF0` | Win98 引入标题栏渐变，右端色 |
| `CAPTIONTEXT` | `#FFFFFF` | |
| `HIGHLIGHT` | `#000080` | 选中项、进度条填充块 |
| `INFOBK` | `#FFFFE1` | 工具提示淡黄 |
| `GRAYTEXT` | `#808080` | 禁用文字 |
| `WINDOWFRAME` | `#000000` | 窗口最外 1 px |

### 2.2 `COLOR_3DLIGHT` 为什么取 `#DFDFDF` 而不是 `#C0C0C0`

一些流传甚广的 "Windows 标准方案" 注册表转储里写的是：

```
ButtonLight = 192 192 192     ← 与 ButtonFace 相同
```

**但如果真是这样，经典的下凹输入框/状态栏看起来就不对了。**

看实测数据：`EDGE_SUNKEN` 的右下角是

```
bottom : y=21 COLOR_WINDOW(白)   y=20 COLOR_3DLIGHT
```

即**右下角内层是一条 `3DLIGHT` 亮线，压在白色之外**。Win98 的下凹输入框、状态栏、资源管理器地址栏右下角确实都有这条"灰白双线"效果——这条线必须**明显比底色的 `#C0C0C0` 亮**才看得出来。若 `3DLIGHT = #C0C0C0`，这条线就会完全消失在底色里，经典外观当场丢失。

同理，`DFCS_BUTTONPUSH` 的左上内环是 `3DLIGHT`（实测第 1 行 = `COLOR_3DLIGHT`）。若它与面色相同，经典的"白 → 浅灰 → 面"三层凸起会退化成"白 → 面"两层，按钮明显变扁——而 Win98 按钮恰恰以**厚实**著称。

因此本设计取：

```
COLOR_3DLIGHT = #DFDFDF
```

`#DFDFDF` 是 Windows 半色调调色板里的标准浅灰，也是 Win9x 界面各处（滚动条箭头底、刻度等）实际大量使用的值。

> **工程结论**：正因为这个值存在争议，**不要把它写死在程序里**。正式实现必须走 `GetSysColor(COLOR_3DLIGHT)`，让 Win98SE 自己给出真值；`#DFDFDF` 只用于两处——(1) 设计稿渲染，(2) 在现代系统上"演出" Win98 观感时的可选钉死模式（见 DESIGN.md §4.3）。

### 2.3 `COLOR_3DDKSHADOW` 为什么取 `#808080`

同样的转储里 `ButtonDkShadow = 128 128 128`，与 `ButtonShadow` 相同。这与外观观察一致：

* Win98 的按钮右下最外环是**中灰**，不是黑。整个 Win9x 时代的按钮、任务栏按钮、开始按钮都**没有黑色描边**。
* 若取 `#000000`，所有按钮都会出现一圈黑边，那是 Windows 2000/XP "Windows 经典"方案的观感，不是 Win98 的。

因此：

```
COLOR_3DDKSHADOW = #808080
```

副作用：`EDGE_RAISED` 的右下角会变成**连续 2 px 的 `#808080`**（内环 `BTNSHADOW` + 外环 `3DDKSHADOW`）。这正是经典 Win9x 凸起边框"右下一条粗灰边"的来源，是对的。

> 同样地，程序里应走 `GetSysColor(COLOR_3DDKSHADOW)`。

---

## 3. 最终两张表

### 3.1 钉死值（设计稿 / 强制经典模式）

见 [`DESIGN.md` §4.1](../DESIGN.md)，也可直接 `#include "keyodo_layout.h"` 用 `W98_*` 宏。

### 3.2 运行时（推荐主路径）

```c
#define CLR(role) GetSysColor(role)

/* 立体边框一律交给系统，不要自己算颜色 */
DrawEdge(hdc, &rc, EDGE_RAISED, BF_RECT);
DrawEdge(hdc, &rc, EDGE_SUNKEN, BF_RECT);
DrawFrameControl(hdc, &rc, DFC_BUTTON, DFCS_BUTTONPUSH);

/* 只有仪表盘自绘部分需要显式取色 */
HBRUSH hFace  = CreateSolidBrush(GetSysColor(COLOR_3DFACE));
HPEN   hLight = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DHILIGHT));
HPEN   hShad  = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));
```

判断当前是否确实是经典方案，可比较 `GetSysColor(COLOR_3DFACE)` 是否等于 `RGB(192,192,192)`。

---

## 4. 仪表盘专用墨色（不跟随配色方案）

| 名称 | 值 | 理由 |
|---|---|---|
| 红区 / 峰值标 | `#FF0000` | 16 色安全集内的纯红；仪表警示色 |
| R 档 | `#FF0000` | 倒挡红色是汽车惯例，也是唯一按档位变色的元素 |
| 档位闸反白块 | `#000080` 底 + `#FFFFFF` 字 | `COLOR_HIGHLIGHT` / `COLOR_HIGHLIGHTTEXT`，与列表选中项同款 |
| 档位闸未选档 | `#808080` | 与 `GRAYTEXT` 同值，退到背景层 |
| 指针 / 其它档位 | `#000000` | 白盘面上对比度最高；仪表惯例 |
| 次刻度 | `#808080` | 与 `GRAYTEXT` 同值，视觉上退到背景层 |
| 盘面 | `#FFFFFF` | 真车速度表是白底黑字，也符合"汽车仪表盘"需求 |

仪表内的颜色**刻意不跟系统方案联动**：真车的仪表配色不会因为你换了 Windows 主题就变。这是一个刻意的设计决定，不是遗漏。

---

## 5. 复现方法

```bash
# 重新生成逐像素实测记录
python tools/probe_gdi_edge.py        # -> reports/gdi_edge_probe.txt / .png

# 重新生成设计稿
python mockup/keyodo_mockup.py        # -> mockup/out/keyodo_480x320.png 等

# 检查资源脚本能否安全转成 CP936
python tools/to_cp936.py res/keyodo.rc --check
```
