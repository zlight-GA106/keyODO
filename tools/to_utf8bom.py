# -*- coding: utf-8 -*-
"""
to_utf8bom.py -- copy a text file, guaranteeing a UTF-8 byte order mark.

NSIS 3.x reads a script as the system ANSI code page unless it starts with a
UTF-8 BOM. installer/keyodo.nsi contains Simplified-Chinese strings, so it is
kept BOM-less in the repository (clean diffs, no editor surprises) and given a
BOM into build/ at build time. That also makes the build independent of the
code page of whatever machine runs it.

    python tools/to_utf8bom.py installer/keyodo.nsi --out build/keyodo_utf8.nsi
    python tools/to_utf8bom.py installer/keyodo.nsi --check
"""
import os
import sys

BOM = b"\xef\xbb\xbf"


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
        raw.decode("utf-8-sig")
    except UnicodeDecodeError as e:
        print("ERROR: %s is not valid UTF-8: %s" % (path, e))
        return 1

    had_bom = raw.startswith(BOM)
    body = raw[len(BOM):] if had_bom else raw
    out = BOM + body

    print("%s: %s, %d bytes -> %d bytes with BOM"
          % (path, "already had a BOM" if had_bom else "no BOM", len(raw), len(out)))

    if check_only:
        print("--check: no write performed.")
        return 0

    target = out_path if out_path else path
    d = os.path.dirname(target)
    if d:
        os.makedirs(d, exist_ok=True)
    with open(target, "wb") as f:
        f.write(out)
    print("wrote %s" % target)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
