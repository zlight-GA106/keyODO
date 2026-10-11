# KeyODO

键盘里程表 —— Windows 98 界面的原生 Win32 + GDI 桌面小工具。

![KeyODO 0.1](mockup/out/keyodo_480x320.png)

统计按键次数与输入速度，界面严格复刻 `mockup/out/` 里的设计稿：经典灰色、3D 凹凸边框、
老式菜单栏、状态栏、GroupBox、汽车仪表盘。没有圆角、没有卡片、没有主题 API。

**只统计按键次数，不记录任何输入内容。**

---

## 快速开始

```cmd
build.cmd
```

产出：

* `build\keyodo.exe` —— 32 位原生程序（静态 CRT，无需任何运行库）
* `dist\KeyODO_Setup.exe` —— 安装程序

安装：

```cmd
dist\KeyODO_Setup.exe              :: 交互式安装（需要管理员权限）
dist\KeyODO_Setup.exe /S           :: 静默安装
dist\KeyODO_Setup.exe /S /D=C:\KeyODO
```

详见 [构建说明](docs/BUILD.md)。

---

## 功能（0.1）

| 项目 | 说明 |
|---|---|
| **ODO** | 累计真实按键次数，7 位前导零，退出时保存、下次启动恢复 |
| **TRIP** | 本次按键次数，可清零 |
| **WPM / KPM** | 实时速度，默认 10 秒滑动窗口；菜单「统计」可切换 5 / 10 / 30 秒 |
| **MAX** | 最高 WPM，同时以红色刻度线画在表盘上（峰值记忆） |
| **仪表盘** | 指针带指数平滑，不会一跳一跳 |
| **档位** | 自动挡 `P R N D` / 手动挡 `R N M1 M2 M3`，只显示当前变速模式相关的档位 |
| **读数模式** | 表盘内嵌读数与盘下读数可在 WPM / KPM 间切换（`Ctrl+K`） |
| **换挡显示** | 档位闸，当前档反白；R 档红色 |
| **总在最前** | 可选 |

读数模式、变速模式、窗口位置、统计窗口长度都会随统计数据一起保存。

---

## 隐私

程序只接触**虚拟键码与扫描码**，不做键盘布局转换，不产生字符，不记录、不保存、不发送
任何输入内容。保存的 `keyodo.ini` 里只有计数器和显示设置。

---

## 架构

四个模块，互不感知，由 `src/app.c` 组装：

```
input/     键盘事件源        →  Raw Input（当前）/ 低级钩子（可切换）
stats/     计数与速率        →  ODO / TRIP / WPM / KPM / MAX / 档位
ui/        渲染              →  只画一份数据快照，不知道数据从哪来
storage/   持久化            →  只读写一条纯数据记录
```

**输入后端是可替换的**：`input/input.h` 定义了 `InputBackend` 接口，
`input.c` 负责选择。切换方式：

```cmd
keyodo.exe --input=hook        :: 或改 keyodo.ini 里的 [Input] Backend
```

两个后端都在 `tools/smoke_test.py` 下验证通过。将来要接 Windows 98 的
`WH_KEYBOARD` + DLL 方案，只需新增一个文件并在选择表里加一项，其它模块完全不动。

**界面模块不知道键盘存在**，统计模块不知道有窗口，存储模块不知道统计是什么。

详见 [构建说明 §6](docs/BUILD.md)。

---

## Windows 98 SE

源码是 Win98SE 兼容的：只用 ANSI API、不用 `GetWindowLongPtr` / 分层窗口 / 主题 API /
GDI+、不用 C99 特性、中文全部放在资源字符串表里（`.c`/`.h` 没有一个非 ASCII 字节）。

不过 **VS2022 编译出来的这个二进制跑不了 Win98SE**——现代 CRT 会引入 Win98 不存在的内核
导入。要出 Win98SE 的二进制，用当年的工具链重编即可，代码不用改：

```
MSVC 6.0 + Windows 98 SDK
```

细节见 [构建说明 §5](docs/BUILD.md)。

---

## 验证

```cmd
python tools\smoke_test.py --keys 20      :: 端到端：注入按键 → 计数 → 退出 → 检查持久化
python tools\capture_window.py            :: 抓取运行中的窗口
python tools\compare_mockup.py            :: 与设计稿逐像素比对静态部分
python tools\probe_rawinput.py            :: 独立验证 Raw Input 能否收到注入按键
```

`compare_mockup.py` 当前结果：外框、标题栏、菜单栏、三个 GroupBox 边框、表盘面板、
状态栏边框、两个按钮 —— 全部 0.00%，最大偏差 0.16%。

---

## 目录

| 路径 | 内容 |
|---|---|
| `src/` | 程序源码 |
| `res/` | 资源脚本、资源 ID、图标 |
| `include/keyodo_layout.h` | 布局常量（与设计稿同一组数字） |
| `installer/keyodo.nsi` | NSIS 安装脚本 |
| `build.cmd` | 一键构建 |
| `docs/BUILD.md` | 构建与安装说明 |
| `DESIGN.md` | 界面设计说明书（视觉规范） |
| `docs/COLOR.md` | 配色取值的证据与推导 |
| `mockup/` | 设计稿渲染器与输出 |
| `reports/` | 真实 GDI 图元的逐像素实测记录 |
| `tools/` | 构建期编码转换、验证与测试工具 |

---

## 界面设计是规范，不是产物

`DESIGN.md` 与 `mockup/out/` 是视觉规范，程序逻辑去适配界面，而不是反过来改界面。
`tools/compare_mockup.py` 就是这条约束的自动化检查。
