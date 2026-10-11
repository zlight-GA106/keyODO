# -*- coding: utf-8 -*-
"""
to_cp936.py -- re-encode a resource script from UTF-8 to code page 936 (GBK).

Windows 98 SE is an ANSI platform: .rc string literals must be stored as GBK or
the menus come out as mojibake on a real Simplified-Chinese Windows 98 SE.
This repository keeps res/keyodo.rc in UTF-8 so it can be reviewed and diffed
anywhere; the build converts it into build/ before invoking rc.exe.

    python tools/to_cp936.py res/keyodo.rc --out build/keyodo_cp936.rc
    python tools/to_cp936.py res/keyodo.rc --check      # report only

Keeping the conversion out of the source tree means the file under version
control is always the readable UTF-8 one.
"""
import os
import sys

TARGET_CP = "gbk"          # code page 936


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2

    path = argv[1]
    check_only = "--check" in argv
    out_path = None
    if "--out" in argv:
        i = argv.index("--out")
        if i + 1 >= len(argv):
            print("--out needs a path")
            return 2
        out_path = argv[i + 1]

    if not os.path.exists(path):
        print("not found: %s" % path)
        return 1

    raw = open(path, "rb").read()
    try:
        text = raw.decode("utf-8")
        src = "utf-8"
    except UnicodeDecodeError:
        try:
            text = raw.decode(TARGET_CP)
            print("%s is already %s; copying unchanged." % (path, TARGET_CP))
            if out_path and not check_only:
                d = os.path.dirname(out_path)
                if d:
                    os.makedirs(d, exist_ok=True)
                open(out_path, "wb").write(raw)
            return 0
        except UnicodeDecodeError as e:
            print("cannot decode %s as utf-8 or %s: %s" % (path, TARGET_CP, e))
            return 1

    try:
        out = text.encode(TARGET_CP)
    except UnicodeEncodeError as e:
        print("ERROR: a character cannot be represented in %s: %s" % (TARGET_CP, e))
        print("       Fix the source string before shipping to Windows 98 SE.")
        return 1

    non_ascii = sum(1 for ch in text if ord(ch) > 127)
    print("%s: %s, %d non-ASCII chars, %d -> %d bytes"
          % (path, src, non_ascii, len(raw), len(out)))

    if check_only:
        print("--check: no write performed.")
        return 0

    target = out_path if out_path else path
    d = os.path.dirname(target)
    if d:
        os.makedirs(d, exist_ok=True)
    with open(target, "wb") as f:
        f.write(out)
    print("wrote %s as %s." % (target, TARGET_CP))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
