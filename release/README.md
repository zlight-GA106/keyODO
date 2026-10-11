# KeyODO 0.1 — 构建产物

这个目录**故意**签入到仓库里，这样不用装工具链也能直接拿到可运行的程序。
`build/` 和 `dist/` 里的其它中间文件仍然不进版本库。

| 文件 | 说明 | 大小 |
|---|---|---|
| `keyodo.exe` | 32 位原生 Win32 + GDI 程序，静态链接 CRT，不需要任何运行库 | ~235 KB |
| `KeyODO_Setup.exe` | NSIS 安装程序，内含上面那个 exe | ~220 KB |

## 校验

```cmd
certutil -hashfile keyodo.exe SHA256
certutil -hashfile KeyODO_Setup.exe SHA256
```

摘要见 `SHA256SUMS.txt`。

## 运行

`keyodo.exe` 可以直接双击运行，不会写注册表、不需要安装。

`KeyODO_Setup.exe` 需要管理员权限（装到 `Program Files`、写卸载信息）。静默安装：

```cmd
KeyODO_Setup.exe /S
KeyODO_Setup.exe /S /D=C:\KeyODO
```

## 从源码重建

```cmd
build.cmd
```

需要 Visual Studio（含 C++ 桌面工作负载）、NSIS 3.x、Python 3。详见
[`docs/BUILD.md`](../docs/BUILD.md)。

## 这些二进制对应哪次提交

见 `BUILD-INFO.txt`。
