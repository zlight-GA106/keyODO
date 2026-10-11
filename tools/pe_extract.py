#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
pe_extract.py -- pure-Python Win32 PE resource extractor.

Extracts RT_BITMAP / RT_ICON / RT_GROUP_ICON / RT_CURSOR / RT_GROUP_CURSOR
resources out of PE files (DLL / CPL / EXE) into PNG and ICO files.

Dependencies: Python standard library (`struct`, `os`, `sys`, `json`, `argparse`)
              plus Pillow for PNG output.  No third-party PE libraries.

CLI
---
  pe_extract.py <pe-path> --out <dir> --types bitmap,icon
  pe_extract.py <pe-path> --list
  pe_extract.py <pe-path> --out <dir> --types bitmap,icon --contact-sheet
  pe_extract.py <pe-path> --out <dir> --sheet-only        # regenerate sheet+manifest
                                                          # from an existing out dir

Extra options:
  --label NAME     override folder/label name used in file names & manifests
  --max-cell N     contact-sheet cell content box in px (default 96)
  --quiet

Structure of the output directory (created automatically):

  <out>/bitmaps/bitmap_<id>.png
  <out>/icons/groupicon_<id>.ico              (real multi-image .ico)
  <out>/icons/icon_<groupid>_<idx>_<w>x<h>.png
  <out>/icons/icon_res<id>_<w>x<h>.png        (RT_ICON not referenced by a group)
  <out>/icons/icon_res<id>.ico
  <out>/cursors/groupcursor_<id>.cur
  <out>/cursors/cursor_<gid>_<idx>_<w>x<h>.png
  <out>/_manifest.json
  <out>/_contact_sheet.png (+ _contact_sheet_2.png ...)

Every malformed resource is logged and skipped; one bad resource never aborts
the run.
"""

from __future__ import annotations

import argparse
import io
import json
import os
import re
import struct
import sys
import traceback

try:
    from PIL import Image, ImageDraw, ImageFont
except Exception as exc:  # pragma: no cover
    sys.stderr.write("FATAL: Pillow is required for PNG output: %r\n" % (exc,))
    raise

# --------------------------------------------------------------------------
# constants
# --------------------------------------------------------------------------

RT_CURSOR = 1
RT_BITMAP = 2
RT_ICON = 3
RT_GROUP_CURSOR = 12
RT_GROUP_ICON = 14

RT_NAMES = {
    1: "RT_CURSOR",
    2: "RT_BITMAP",
    3: "RT_ICON",
    4: "RT_MENU",
    5: "RT_DIALOG",
    6: "RT_STRING",
    7: "RT_FONTDIR",
    8: "RT_FONT",
    9: "RT_ACCELERATOR",
    10: "RT_RCDATA",
    11: "RT_MESSAGETABLE",
    12: "RT_GROUP_CURSOR",
    14: "RT_GROUP_ICON",
    16: "RT_VERSION",
    17: "RT_DLGINCLUDE",
    19: "RT_PLUGPLAY",
    20: "RT_VXD",
    21: "RT_ANICURSOR",
    22: "RT_ANIICON",
    23: "RT_HTML",
    24: "RT_MANIFEST",
}

TYPE_KEYS = {
    "bitmap": RT_BITMAP,
    "bitmaps": RT_BITMAP,
    "icon": RT_ICON,
    "icons": RT_ICON,
    "group_icon": RT_GROUP_ICON,
    "groupicon": RT_GROUP_ICON,
    "cursor": RT_CURSOR,
    "cursors": RT_CURSOR,
    "group_cursor": RT_GROUP_CURSOR,
    "groupcursor": RT_GROUP_CURSOR,
}

LOG_PREFIX = "[pe_extract]"


def log(msg: str, quiet: bool = False) -> None:
    if not quiet:
        sys.stderr.write("%s %s\n" % (LOG_PREFIX, msg))
        sys.stderr.flush()


def sanitize(name: str) -> str:
    out = re.sub(r"[^A-Za-z0-9._-]+", "_", str(name)).strip("_")
    return out or "unnamed"


# --------------------------------------------------------------------------
# PE parsing
# --------------------------------------------------------------------------

class PEError(Exception):
    pass


class PEFile(object):
    """Minimal PE reader: DOS header -> PE sig -> COFF -> optional header ->
    data directories -> sections -> resource directory tree."""

    def __init__(self, path):
        self.path = path
        with open(path, "rb") as fh:
            self.data = fh.read()
        self.is_pe32_plus = False
        self.sections = []          # list of dicts
        self.resource_rva = 0
        self.resource_size = 0
        self.machine = 0
        self.timestamp = 0
        self._parse()

    # -- low level helpers --------------------------------------------------
    def _u8(self, off):
        return self.data[off]

    def _u16(self, off):
        return struct.unpack_from("<H", self.data, off)[0]

    def _u32(self, off):
        return struct.unpack_from("<I", self.data, off)[0]

    def _i32(self, off):
        return struct.unpack_from("<i", self.data, off)[0]

    def _cstr(self, off, maxlen=64):
        end = self.data.find(b"\x00", off, off + maxlen)
        if end < 0:
            end = off + maxlen
        return self.data[off:end].decode("latin-1", "replace")

    # -- parsing ------------------------------------------------------------
    def _parse(self):
        d = self.data
        if len(d) < 0x40 or d[0:2] != b"MZ":
            raise PEError("not an MZ executable")
        e_lfanew = self._u32(0x3C)
        if e_lfanew + 24 > len(d) or d[e_lfanew:e_lfanew + 4] != b"PE\0\0":
            raise PEError("no PE signature at e_lfanew=0x%X" % e_lfanew)
        coff = e_lfanew + 4
        self.machine = self._u16(coff)
        num_sections = self._u16(coff + 2)
        self.timestamp = self._u32(coff + 4)
        size_opt = self._u16(coff + 16)
        opt = coff + 20
        if size_opt < 2:
            raise PEError("empty optional header")
        magic = self._u16(opt)
        if magic == 0x10B:
            self.is_pe32_plus = False
            dd_off = opt + 96
            num_dd_off = opt + 92
        elif magic == 0x20B:
            self.is_pe32_plus = True
            dd_off = opt + 112
            num_dd_off = opt + 108
        else:
            raise PEError("unknown optional header magic 0x%04X" % magic)

        num_dd = self._u32(num_dd_off) if num_dd_off + 4 <= len(d) else 0
        if num_dd < 3:
            raise PEError("no resource data directory (NumberOfRvaAndSizes=%d)" % num_dd)
        self.resource_rva = self._u32(dd_off + 2 * 8)
        self.resource_size = self._u32(dd_off + 2 * 8 + 4)

        sec_off = opt + size_opt
        for i in range(num_sections):
            base = sec_off + i * 40
            if base + 40 > len(d):
                break
            name = d[base:base + 8].rstrip(b"\x00").decode("latin-1", "replace")
            vsize = self._u32(base + 8)
            va = self._u32(base + 12)
            rawsize = self._u32(base + 16)
            rawptr = self._u32(base + 20)
            self.sections.append({
                "name": name, "vsize": vsize, "va": va,
                "rawsize": rawsize, "rawptr": rawptr,
            })

    def rva_to_offset(self, rva):
        """Map an RVA to a file offset via the section table."""
        if rva == 0:
            return 0
        for s in self.sections:
            span = max(s["vsize"], s["rawsize"])
            if s["va"] <= rva < s["va"] + span:
                delta = rva - s["va"]
                if delta >= s["rawsize"]:
                    raise PEError("rva 0x%X in uninitialised part of section %s"
                                  % (rva, s["name"]))
                return s["rawptr"] + delta
        # header-resident RVA
        if rva < 0x1000:
            return rva
        raise PEError("rva 0x%X not mapped by any section" % rva)

    def read_rva(self, rva, size):
        off = self.rva_to_offset(rva)
        if off + size > len(self.data):
            raise PEError("read past EOF (rva=0x%X size=%d)" % (rva, size))
        return self.data[off:off + size]

    # -- resource tree ------------------------------------------------------
    def iter_resources(self, quiet=True):
        """Yield dicts: {type, type_id, name, name_id, lang, rva, size, data}."""
        if not self.resource_rva:
            return
        base_off = self.rva_to_offset(self.resource_rva)
        sec_end = base_off + max(self.resource_size, 1)
        # allow some slack -- SizeOfResource dirs are sometimes understated
        sec_end = min(len(self.data), sec_end + 4096)

        def read_dir(rel, depth, seen):
            if rel in seen or depth > 4:
                return
            seen = seen | {rel}
            off = base_off + rel
            if off + 16 > len(self.data):
                raise PEError("resource dir entry out of range at rel=0x%X" % rel)
            n_named = self._u16(off + 12)
            n_id = self._u16(off + 14)
            total = n_named + n_id
            for i in range(total):
                e = off + 16 + i * 8
                if e + 8 > len(self.data):
                    break
                name_field = self._u32(e)
                data_field = self._u32(e + 4)
                if name_field & 0x80000000:
                    # named entry: offset (relative to resource base) to
                    # IMAGE_RESOURCE_DIR_STRING_U
                    so = base_off + (name_field & 0x7FFFFFFF)
                    if so + 2 > sec_end:
                        ident = "<badname@0x%X>" % so
                        ident_id = None
                    else:
                        ln = self._u16(so)
                        raw = self.data[so + 2: so + 2 + ln * 2]
                        ident = raw.decode("utf-16-le", "replace")
                        ident_id = None
                else:
                    ident = name_field
                    ident_id = name_field
                if data_field & 0x80000000:
                    sub = data_field & 0x7FFFFFFF
                    for item in read_dir(sub, depth + 1, seen):
                        yield item
                else:
                    de = base_off + data_field
                    if de + 16 > len(self.data):
                        continue
                    drva = self._u32(de)
                    dsize = self._u32(de + 4)
                    codepage = self._u32(de + 8)
                    yield {
                        "ident": ident,
                        "ident_id": ident_id,
                        "lang": (ident if depth == 2 else None),
                        "rva": drva,
                        "size": dsize,
                        "codepage": codepage,
                    }

        # level 0 = type
        off0 = base_off
        n_named = self._u16(off0 + 12)
        n_id = self._u16(off0 + 14)
        for i in range(n_named + n_id):
            e = off0 + 16 + i * 8
            if e + 8 > len(self.data):
                break
            name_field = self._u32(e)
            data_field = self._u32(e + 4)
            if name_field & 0x80000000:
                so = base_off + (name_field & 0x7FFFFFFF)
                try:
                    ln = self._u16(so)
                    tname = self.data[so + 2: so + 2 + ln * 2].decode("utf-16-le", "replace")
                except Exception:
                    tname = "<badtype>"
                type_id = None
            else:
                type_id = name_field
                tname = RT_NAMES.get(type_id, "TYPE_%d" % type_id)
            if not (data_field & 0x80000000):
                continue
            sub = data_field & 0x7FFFFFFF
            # level 1 = name/ID
            off1 = base_off + sub
            try:
                n1 = self._u16(off1 + 12) + self._u16(off1 + 14)
            except Exception:
                continue
            for j in range(n1):
                e1 = off1 + 16 + j * 8
                if e1 + 8 > len(self.data):
                    break
                nf1 = self._u32(e1)
                df1 = self._u32(e1 + 4)
                if nf1 & 0x80000000:
                    so = base_off + (nf1 & 0x7FFFFFFF)
                    try:
                        ln = self._u16(so)
                        ident = self.data[so + 2: so + 2 + ln * 2].decode("utf-16-le", "replace")
                    except Exception:
                        ident = "<badname>"
                    ident_id = None
                else:
                    ident = nf1
                    ident_id = nf1
                if not (df1 & 0x80000000):
                    de = base_off + df1
                    if de + 16 > len(self.data):
                        continue
                    yield {
                        "type": tname, "type_id": type_id,
                        "ident": ident, "ident_id": ident_id,
                        "lang": 0, "rva": self._u32(de), "size": self._u32(de + 4),
                        "codepage": self._u32(de + 8),
                    }
                    continue
                off2 = base_off + (df1 & 0x7FFFFFFF)
                try:
                    n2 = self._u16(off2 + 12) + self._u16(off2 + 14)
                except Exception:
                    continue
                for k in range(n2):
                    e2 = off2 + 16 + k * 8
                    if e2 + 8 > len(self.data):
                        break
                    nf2 = self._u32(e2)
                    df2 = self._u32(e2 + 4)
                    if nf2 & 0x80000000:
                        lang = nf2 & 0x7FFFFFFF
                    else:
                        lang = nf2
                    if df2 & 0x80000000:
                        continue
                    de = base_off + df2
                    if de + 16 > len(self.data):
                        continue
                    yield {
                        "type": tname, "type_id": type_id,
                        "ident": ident, "ident_id": ident_id,
                        "lang": lang, "rva": self._u32(de), "size": self._u32(de + 4),
                        "codepage": self._u32(de + 8),
                    }

    def resources(self, quiet=True):
        out = []
        try:
            for r in self.iter_resources(quiet=quiet):
                try:
                    r["data"] = self.read_rva(r["rva"], r["size"])
                except Exception as exc:
                    log("  ! unreadable resource %s id=%s: %s"
                        % (r["type"], r["ident"], exc), quiet)
                    continue
                out.append(r)
        except Exception as exc:
            log("resource walk error: %s" % exc, quiet)
            if not quiet:
                traceback.print_exc()
        return out


# --------------------------------------------------------------------------
# DIB (device independent bitmap) decoding
# --------------------------------------------------------------------------

class DIBError(Exception):
    pass


def _mask_shift(mask):
    if mask == 0:
        return 0, 0
    shift = 0
    while not (mask >> shift) & 1:
        shift += 1
    width = 0
    m = mask >> shift
    while m & 1:
        width += 1
        m >>= 1
    return shift, width


def parse_dib_header(data, off=0, header_size=None):
    """Parse only the DIB header + colour table (no pixel decoding).

    RT_ICON payloads cannot be parsed with parse_dib() directly because their
    biHeight covers both the colour half and the 1bpp AND mask, which has a
    different row stride.  This split lets callers work out the geometry first.
    """
    if header_size is None:
        if off + 4 > len(data):
            raise DIBError("truncated DIB")
        header_size = struct.unpack_from("<I", data, off)[0]

    core = False
    size_image = 0
    if header_size == 12:
        core = True
        if off + 12 > len(data):
            raise DIBError("truncated BITMAPCOREHEADER")
        w, h, planes, bpp = struct.unpack_from("<HHHH", data, off + 4)
        compression = 0
        clr_used = 0
        clr_important = 0
        hdr_len = 12
        pal_entry = 3
    elif header_size >= 40:
        if off + 40 > len(data):
            raise DIBError("truncated BITMAPINFOHEADER")
        (w, h, planes, bpp, compression, size_image,
         xppm, yppm, clr_used, clr_important) = struct.unpack_from("<iiHHIIiiII", data, off + 4)
        hdr_len = header_size
        pal_entry = 4
    else:
        raise DIBError("bad DIB header size %d" % header_size)

    if w <= 0 or w > 20000:
        raise DIBError("implausible DIB width %d" % w)
    topdown = h < 0
    hh = -h if topdown else h
    if hh <= 0 or hh > 20000:
        raise DIBError("implausible DIB height %d" % h)
    if planes not in (0, 1):
        raise DIBError("implausible planes %d" % planes)
    if bpp not in (1, 4, 8, 16, 24, 32):
        raise DIBError("unsupported bpp %d" % bpp)
    # BI_RGB, BI_RLE8, BI_RLE4, BI_BITFIELDS, BI_ALPHABITFIELDS
    if compression not in (0, 1, 2, 3, 6):
        raise DIBError("unsupported compression %d" % compression)
    if compression == 1 and bpp != 8:
        raise DIBError("BI_RLE8 with bpp=%d" % bpp)
    if compression == 2 and bpp != 4:
        raise DIBError("BI_RLE4 with bpp=%d" % bpp)

    # ---- masks for BI_BITFIELDS ------------------------------------------
    rmask = gmask = bmask = amask = 0
    extra = 0
    if compression in (3, 6) and not core:
        nmask = 4 if compression == 6 else 3
        if header_size == 40:
            if off + 40 + nmask * 4 <= len(data):
                masks = struct.unpack_from("<" + "I" * nmask, data, off + 40)
                extra = nmask * 4
            else:
                masks = ()
        else:
            # masks live inside the (>=52 byte) header
            masks = struct.unpack_from("<" + "I" * nmask, data, off + 40)
        if len(masks) >= 3:
            rmask, gmask, bmask = masks[0], masks[1], masks[2]
            if len(masks) >= 4:
                amask = masks[3]
        if rmask == 0 and gmask == 0 and bmask == 0:
            raise DIBError("BI_BITFIELDS with zero masks")

    # ---- colour table -----------------------------------------------------
    palette = []
    entries = 0
    if bpp <= 8:
        entries = clr_used if clr_used else (1 << bpp)
        if entries > 256:
            entries = 256
    pal_off = off + hdr_len + extra
    if entries:
        need = pal_off + entries * pal_entry
        if need > len(data):
            raise DIBError("truncated DIB palette (need %d bytes, have %d)"
                           % (entries * pal_entry, max(0, len(data) - pal_off)))
        for i in range(entries):
            p = pal_off + i * pal_entry
            palette.append((data[p + 2], data[p + 1], data[p]))

    bits_off = pal_off + entries * pal_entry

    # 16bpp BI_RGB defaults to RGB555
    if bpp == 16 and compression == 0:
        rmask, gmask, bmask = 0x7C00, 0x03E0, 0x001F

    stride = ((w * bpp + 31) // 32) * 4
    return {
        "width": w,
        "height": hh,
        "bpp": bpp,
        "compression": compression,
        "core": core,
        "topdown": topdown,
        "header_size": header_size,
        "bits_offset": bits_off,
        "stride": stride,
        "size_image": size_image,
        "palette": palette,
        "palette_entries": entries,
        "rmask": rmask, "gmask": gmask, "bmask": bmask, "amask": amask,
    }


def decode_rle(data, off, w, h, bpp):
    """Decode BI_RLE8 / BI_RLE4 into a top-down palette-index array."""
    idx = bytearray(w * h)
    x, y = 0, h - 1          # RLE streams are bottom-up
    p = off
    n = len(data)

    def put(px, py, v):
        if 0 <= px < w and 0 <= py < h:
            idx[py * w + px] = v & 0xFF

    while p + 1 < n:
        cnt = data[p]
        val = data[p + 1]
        p += 2
        if cnt > 0:
            for i in range(cnt):
                if bpp == 8:
                    v = val
                else:
                    v = (val >> 4) if (i & 1) == 0 else (val & 0x0F)
                put(x, y, v)
                x += 1
            continue
        if val == 0:                      # end of line
            x = 0
            y -= 1
            if y < 0:
                break
        elif val == 1:                    # end of bitmap
            break
        elif val == 2:                    # delta
            if p + 1 >= n:
                break
            x += data[p]
            y -= data[p + 1]
            p += 2
            if y < 0:
                break
        else:                             # absolute run
            run = val
            nbytes = run if bpp == 8 else (run + 1) // 2
            if p + nbytes > n:
                break
            if bpp == 8:
                for i in range(run):
                    put(x, y, data[p + i])
                    x += 1
            else:
                for i in range(run):
                    byte = data[p + (i >> 1)]
                    put(x, y, (byte >> 4) if (i & 1) == 0 else (byte & 0x0F))
                    x += 1
            p += nbytes + (nbytes & 1)    # pad to word boundary
    return idx


def parse_dib(data, off=0, header_size=None, quiet=True):
    """Decode a Win32 DIB (BITMAPINFOHEADER/BITMAPCOREHEADER + colour table +
    pixels).  Returns (PIL.Image, info-dict)."""
    hdr = parse_dib_header(data, off, header_size)
    w = hdr["width"]
    hh = hdr["height"]
    bpp = hdr["bpp"]
    compression = hdr["compression"]
    bits_off = hdr["bits_offset"]
    stride = hdr["stride"]
    palette = hdr["palette"]

    if compression in (1, 2):
        indices = decode_rle(data, bits_off, w, hh, bpp)
        px = bytearray(w * hh * 4)
        for i, v in enumerate(indices):
            r, g, b = palette[v] if v < len(palette) else (0, 0, 0)
            o = i * 4
            px[o] = r
            px[o + 1] = g
            px[o + 2] = b
            px[o + 3] = 255
        img = Image.frombytes("RGBA", (w, hh), bytes(px))
        total = bits_off
    else:
        need = bits_off + stride * hh
        if need > len(data):
            raise DIBError("truncated pixel data (need %d bytes from %d, have %d)"
                           % (stride * hh, bits_off, len(data)))
        raw = data[bits_off:need]
        img = decode_pixels(raw, w, hh, bpp, stride, palette,
                            hdr["rmask"], hdr["gmask"], hdr["bmask"],
                            hdr["amask"], hdr["topdown"])
        total = need

    info = dict(hdr)
    info.pop("palette", None)
    info["mask_bytes"] = stride * hh
    info["total_size"] = total
    info["masks"] = {
        "r": list(_mask_shift(hdr["rmask"])),
        "g": list(_mask_shift(hdr["gmask"])),
        "b": list(_mask_shift(hdr["bmask"])),
        "a": list(_mask_shift(hdr["amask"])),
    }
    return img, info


def _scale_component(value, shift, bits):
    if bits <= 0:
        return 0
    v = (value >> shift) & ((1 << bits) - 1)
    if bits == 8:
        return v
    if bits == 5:
        return (v << 3) | (v >> 2)
    if bits == 6:
        return (v << 2) | (v >> 4)
    if bits == 4:
        return v * 17
    if bits == 1:
        return 255 if v else 0
    # generic
    return (v * 255) // ((1 << bits) - 1)


def decode_pixels(raw, w, h, bpp, stride, palette, rmask, gmask, bmask, amask,
                  topdown):
    """Return a PIL RGBA image.  Row 0 of the DIB is the *bottom* row unless
    topdown is set."""
    px = bytearray(w * h * 4)

    def put(row, col, r, g, b, a):
        y = row if topdown else (h - 1 - row)
        o = (y * w + col) * 4
        px[o] = r
        px[o + 1] = g
        px[o + 2] = b
        px[o + 3] = a

    if bpp == 1:
        for row in range(h):
            base = row * stride
            for col in range(w):
                byte = raw[base + (col >> 3)]
                idx = (byte >> (7 - (col & 7))) & 1
                r, g, b = palette[idx] if idx < len(palette) else (0, 0, 0)
                put(row, col, r, g, b, 255)
    elif bpp == 4:
        for row in range(h):
            base = row * stride
            for col in range(w):
                byte = raw[base + (col >> 1)]
                idx = (byte >> 4) & 0xF if (col & 1) == 0 else byte & 0xF
                r, g, b = palette[idx] if idx < len(palette) else (0, 0, 0)
                put(row, col, r, g, b, 255)
    elif bpp == 8:
        for row in range(h):
            base = row * stride
            for col in range(w):
                idx = raw[base + col]
                r, g, b = palette[idx] if idx < len(palette) else (0, 0, 0)
                put(row, col, r, g, b, 255)
    elif bpp == 24:
        for row in range(h):
            base = row * stride
            for col in range(w):
                o = base + col * 3
                put(row, col, raw[o + 2], raw[o + 1], raw[o], 255)
    elif bpp == 32:
        any_alpha = bool(amask)
        if not any_alpha:
            for row in range(h):
                base = row * stride
                for col in range(w):
                    o = base + col * 4
                    if raw[o + 3]:
                        any_alpha = True
                        break
                if any_alpha:
                    break
        for row in range(h):
            base = row * stride
            for col in range(w):
                o = base + col * 4
                b, g, r, a = raw[o], raw[o + 1], raw[o + 2], raw[o + 3]
                if amask:
                    v = struct.unpack_from("<I", raw, o)[0]
                    r = _scale_component(v, *_mask_shift(rmask)) if rmask else r
                    g = _scale_component(v, *_mask_shift(gmask)) if gmask else g
                    b = _scale_component(v, *_mask_shift(bmask)) if bmask else b
                    a = _scale_component(v, *_mask_shift(amask))
                elif not any_alpha:
                    a = 255
                put(row, col, r, g, b, a)
    elif bpp == 16:
        rs, rb = _mask_shift(rmask)
        gs, gb = _mask_shift(gmask)
        bs, bb = _mask_shift(bmask)
        for row in range(h):
            base = row * stride
            for col in range(w):
                v = struct.unpack_from("<H", raw, base + col * 2)[0]
                put(row, col,
                    _scale_component(v, rs, rb),
                    _scale_component(v, gs, gb),
                    _scale_component(v, bs, bb), 255)
    else:
        raise DIBError("decode: unsupported bpp %d" % bpp)

    return Image.frombytes("RGBA", (w, h), bytes(px))


def apply_and_mask(img, and_bits, stride, w, h):
    """AND mask: 1 = transparent, 0 = keep.  Returns a new RGBA image."""
    px = bytearray(img.tobytes())
    for row in range(h):
        base = row * stride
        for col in range(w):
            byte = and_bits[base + (col >> 3)]
            if (byte >> (7 - (col & 7))) & 1:
                # transparent; also zero RGB so downscales look right
                y = h - 1 - row
                o = (y * w + col) * 4
                px[o] = 0
                px[o + 1] = 0
                px[o + 2] = 0
                px[o + 3] = 0
    return Image.frombytes("RGBA", (w, h), bytes(px))


# --------------------------------------------------------------------------
# icon / cursor resource decoding
# --------------------------------------------------------------------------

PNG_SIG = b"\x89PNG\r\n\x1a\n"


def is_png_blob(data):
    return data[:8] == PNG_SIG


def decode_png_icon(data):
    """Vista+ RT_ICON payloads may be a bare PNG file instead of a DIB.
    Returns (PIL.Image, info-dict) in the same shape as decode_icon()."""
    im = Image.open(io.BytesIO(data))
    im.load()
    im = im.convert("RGBA")
    info = {
        "width": im.width,
        "height": im.height,
        "bpp": 32,
        "compression": None,
        "core": False,
        "topdown": False,
        "header_size": 0,
        "bits_offset": 0,
        "stride": 0,
        "palette_entries": 0,
        "masks": {"r": [0, 8], "g": [8, 8], "b": [16, 8], "a": [24, 8]},
        "is_png": True,
        "mask_present": False,
        "has_alpha_channel": True,
        "colour_bytes": len(data),
    }
    return im, info


def guess_icon_header_size(data, off=0):
    """RT_ICON DIBs in the wild occasionally carry a bogus biSize (0, 12 or a
    junk value).  Return a usable header size."""
    if off + 4 > len(data):
        raise DIBError("truncated icon payload")
    hs = struct.unpack_from("<I", data, off)[0]
    if hs in (12, 40, 52, 56, 64, 108, 124) and off + hs <= len(data):
        return hs
    # try 40
    if off + 40 <= len(data):
        w, h, planes, bpp = struct.unpack_from("<iiHH", data, off + 4)
        if 0 < w <= 1024 and 0 < abs(h) <= 2048 and bpp in (1, 4, 8, 16, 24, 32):
            return 40
    raise DIBError("cannot determine icon DIB header size (biSize=%d)" % hs)


def _dib_ok(data, off):
    """Cheap sanity test: does a plausible DIB header start at `off`?"""
    if off + 40 > len(data):
        return False
    try:
        hs, w, h, planes, bpp = struct.unpack_from("<IiiHH", data, off)
    except Exception:
        return False
    if hs not in (12, 40, 52, 56, 64, 108, 124):
        return False
    if not (0 < w <= 4096) or not (0 < abs(h) <= 4096):
        return False
    if planes not in (0, 1) or bpp not in (1, 4, 8, 16, 24, 32):
        return False
    return True


def decode_icon(data, res_id=None, lang=None, expected=None, quiet=True):
    """Decode an RT_ICON (or RT_CURSOR) payload.

    RT_ICON payloads are DIBs whose biHeight is normally 2x the logical height
    because an AND mask follows the colour data.  Some 32bpp icons store
    biHeight = the real height with no mask at all.

    `expected` is an optional (width, height) hint taken from the matching
    GRPICONDIRENTRY, which is authoritative when it is self-consistent.
    """
    if is_png_blob(data):
        return decode_png_icon(data)
    hs = guess_icon_header_size(data)
    hdr = parse_dib_header(data, 0, header_size=hs)
    w = hdr["width"]
    full_h = hdr["height"]
    bpp = hdr["bpp"]
    bits_off = hdr["bits_offset"]

    stride_c = ((w * bpp + 31) // 32) * 4
    stride_m = ((w + 31) // 32) * 4

    colour_h = None
    if expected:
        eh = expected[1]
        if eh and 0 < eh <= full_h:
            colour_h = eh
    if colour_h is None:
        need_doubled = bits_off + stride_c * (full_h // 2) + stride_m * (full_h // 2)
        if bpp == 32:
            # A flat (mask-less) 32bpp icon needs stride_c*full_h bytes.
            need_flat = bits_off + stride_c * full_h
            colour_h = full_h if len(data) >= need_flat else max(1, full_h // 2)
        elif full_h % 2 == 0 and len(data) >= need_doubled:
            colour_h = full_h // 2
        else:
            colour_h = full_h

    stride_c = ((w * bpp + 31) // 32) * 4
    colour_bytes = stride_c * colour_h
    if bits_off + colour_bytes > len(data):
        raise DIBError("icon colour data truncated")

    # Build a synthetic DIB containing only the colour half (height = colour_h)
    # and let parse_dib() do the pixel work; this reuses all bpp / palette /
    # bitfield handling and gives correct bottom-up row order.
    head = bytearray(data[:bits_off])
    if hs == 12:
        struct.pack_into("<HH", head, 4, w, colour_h)
    else:
        struct.pack_into("<ii", head, 4, w, colour_h)
    synthetic = bytes(head) + data[bits_off:bits_off + colour_bytes]
    colour, info_c = parse_dib(synthetic, 0, header_size=hs, quiet=quiet)

    mask_off = bits_off + colour_bytes
    stride_m = ((w + 31) // 32) * 4
    has_mask = mask_off + stride_m * colour_h <= len(data)
    real_alpha = bool(bpp == 32 and _has_real_alpha(data, bits_off, w, colour_h, stride_c))

    out = colour
    if has_mask:
        and_bits = data[mask_off:mask_off + stride_m * colour_h]
        # combine: colour where mask==0, transparent where mask==1
        cpx = colour.tobytes()
        opx = bytearray(w * colour_h * 4)
        for row in range(colour_h):
            base = row * stride_m
            y = colour_h - 1 - row
            for col in range(w):
                ci = (y * w + col) * 4
                if (and_bits[base + (col >> 3)] >> (7 - (col & 7))) & 1:
                    continue  # transparent
                oi = (y * w + col) * 4
                opx[oi] = cpx[ci]
                opx[oi + 1] = cpx[ci + 1]
                opx[oi + 2] = cpx[ci + 2]
                # a 32bpp icon with a real alpha channel keeps it; otherwise
                # the AND mask is the only transparency source
                opx[oi + 3] = cpx[ci + 3] if real_alpha else 255
        out = Image.frombytes("RGBA", (w, colour_h), bytes(opx))

    info2 = dict(hdr)
    info2.pop("palette", None)
    info2["height"] = colour_h
    info2["colour_bytes"] = colour_bytes
    info2["mask_present"] = has_mask
    info2["has_alpha_channel"] = real_alpha
    info2["masks"] = {k: list(_mask_shift(hdr[k + "mask"]))
                      for k in ("r", "g", "b", "a")}
    return out, info2


def _has_real_alpha(data, bits_off, w, h, stride):
    for row in range(h):
        base = bits_off + row * stride
        for col in range(w):
            o = base + col * 4
            if o + 4 > len(data):
                return False
            if data[o + 3]:
                return True
    return False


def parse_group_icon(data):
    """GRPICONDIR -> (list of entries, kind) where kind is 'icon' or 'cursor'."""
    if len(data) < 6:
        raise DIBError("group icon payload too small")
    reserved, rtype, count = struct.unpack_from("<HHH", data, 0)
    if count > 512:
        raise DIBError("implausible group count %d" % count)
    entries = []
    off = 6
    for _ in range(count):
        if off + 14 > len(data):
            break
        bw, bh, bcolor, bres, planes, bitcount, bytes_in_res, nid = \
            struct.unpack_from("<BBBBHHIH", data, off)
        off += 14
        entries.append({
            "width": bw or 256,
            "height": bh or 256,
            "color_count": bcolor,
            "planes": planes,
            "bitcount": bitcount,
            "bytes": bytes_in_res,
            "id": nid,
            "hotspot": (planes, bitcount),
        })
    return entries, ("cursor" if rtype == 2 else "icon")


def build_ico(images, kind="icon"):
    """images: list of dicts with keys dib(bytes), width, height, bpp,
    color_count, hotspot (for cursors).  Returns .ico/.cur bytes."""
    count = len(images)
    out = io.BytesIO()
    out.write(struct.pack("<HHH", 0, 2 if kind == "cursor" else 1, count))
    offset = 6 + 16 * count
    payloads = []
    for im in images:
        dib = im["dib"]
        payloads.append(dib)
        w = im["width"]
        h = im["height"]
        bw = 0 if w >= 256 else w
        bh = 0 if h >= 256 else h
        bcolor = im.get("color_count", 0)
        if im.get("bpp", 0) > 8:
            bcolor = 0
        if kind == "cursor":
            planes, bitcount = im.get("hotspot", (0, 0))
        else:
            planes = im.get("planes", 1) or 1
            bitcount = im.get("bpp", 0)
        out.write(struct.pack("<BBBBHHII", bw, bh, bcolor, 0,
                              planes & 0xFFFF, bitcount & 0xFFFF,
                              len(dib), offset))
        offset += len(dib)
    for p in payloads:
        out.write(p)
    return out.getvalue()


def repair_dib_for_ico(data, info, quiet=True):
    """Return a canonical icon DIB ready to be embedded in an .ico/.cur file.

    The .ico spec requires the image to be a DIB whose biHeight is 2x the
    logical height, with the colour (XOR) half followed by the 1bpp AND mask.
    Some RT_ICON payloads omit the AND mask entirely (typical for 32bpp icons
    that rely on the alpha channel).  In that case a zeroed -- i.e. fully
    opaque -- AND mask is appended so the resulting .ico is structurally valid.

    Returns (dib_bytes, mask_was_synthesized).
    """
    if info.get("is_png"):
        # Vista+ .ico files may embed a PNG verbatim; nothing to canonicalise.
        return data, False
    w = info["width"]
    h = info["height"]
    bpp = info["bpp"]
    hs = info["header_size"]
    bits_off = info["bits_offset"]
    stride_c = ((w * bpp + 31) // 32) * 4
    stride_m = ((w + 31) // 32) * 4
    colour_bytes = stride_c * h
    mask_bytes = stride_m * h

    if bits_off + colour_bytes > len(data):
        raise DIBError("DIB truncated while canonicalising")

    head = bytearray(data[:bits_off])
    if hs == 12:
        struct.pack_into("<HH", head, 4, w, 2 * h)
    else:
        struct.pack_into("<ii", head, 4, w, 2 * h)

    colour = data[bits_off:bits_off + colour_bytes]
    have_mask = bits_off + colour_bytes + mask_bytes <= len(data)
    if have_mask:
        mask = data[bits_off + colour_bytes:bits_off + colour_bytes + mask_bytes]
    else:
        mask = b"\x00" * mask_bytes
    return bytes(head) + colour + mask, (not have_mask)


# --------------------------------------------------------------------------
# extraction driver
# --------------------------------------------------------------------------

class Extractor(object):
    def __init__(self, pe, outdir, types, label=None, quiet=False):
        self.pe = pe
        self.outdir = outdir
        self.types = types
        self.quiet = quiet
        self.label = label or os.path.splitext(os.path.basename(pe.path))[0].lower()
        self.records = []
        self.errors = []
        self.dir_bitmaps = os.path.join(outdir, "bitmaps")
        self.dir_icons = os.path.join(outdir, "icons")
        self.dir_cursors = os.path.join(outdir, "cursors")

    def _ensure(self, d):
        os.makedirs(d, exist_ok=True)
        return d

    def _add(self, rec):
        rec["dll"] = self.label
        rec["source"] = self.pe.path
        self.records.append(rec)

    def run(self):
        res = self.pe.resources(quiet=self.quiet)
        by_type = {}
        for r in res:
            by_type.setdefault(r["type_id"], []).append(r)

        # ---- bitmaps ------------------------------------------------------
        if RT_BITMAP in self.types:
            self._ensure(self.dir_bitmaps)
            for r in sorted(by_type.get(RT_BITMAP, []),
                            key=lambda x: (str(type(x["ident"])), str(x["ident"]))):
                try:
                    self._do_bitmap(r)
                except Exception as exc:
                    self._fail(r, exc)

        # ---- icons --------------------------------------------------------
        if RT_ICON in self.types:
            self._ensure(self.dir_icons)
            groups = by_type.get(RT_GROUP_ICON, [])
            icons = {}
            for r in by_type.get(RT_ICON, []):
                icons.setdefault(r["ident_id"], r)
            used = set()
            for g in sorted(groups, key=lambda x: (str(type(x["ident"])), str(x["ident"]))):
                try:
                    used |= self._do_icon_group(g, icons)
                except Exception as exc:
                    self._fail(g, exc)
            for iid, r in sorted(icons.items(), key=lambda kv: (kv[0] is None, str(kv[0]))):
                if iid in used:
                    continue
                try:
                    self._do_orphan_icon(r)
                except Exception as exc:
                    self._fail(r, exc)

        # ---- cursors ------------------------------------------------------
        if RT_CURSOR in self.types:
            self._ensure(self.dir_cursors)
            groups = by_type.get(RT_GROUP_CURSOR, [])
            curs = {}
            for r in by_type.get(RT_CURSOR, []):
                curs.setdefault(r["ident_id"], r)
            used = set()
            for g in groups:
                try:
                    used |= self._do_cursor_group(g, curs)
                except Exception as exc:
                    self._fail(g, exc)

        self._write_manifest()
        return self.records

    def _fail(self, r, exc):
        msg = "%s id=%s lang=%s: %s: %s" % (
            r.get("type"), r.get("ident"), r.get("lang"), type(exc).__name__, exc)
        self.errors.append(msg)
        log("  ! skip %s" % msg, self.quiet)

    # -- bitmaps ------------------------------------------------------------
    def _do_bitmap(self, r):
        data = r["data"]
        img, info = parse_dib(data, 0, quiet=self.quiet)
        name = sanitize(r["ident"])
        base = "bitmap_%s" % (r["ident"] if isinstance(r["ident"], int) else name)
        path = os.path.join(self.dir_bitmaps, base + ".png")
        img.save(path, "PNG")
        self._add({
            "kind": "bitmap", "type": r["type"], "id": r["ident"],
            "lang": r["lang"], "width": info["width"], "height": info["height"],
            "bpp": info["bpp"], "file": os.path.relpath(path, self.outdir).replace("\\", "/"),
            "res_size": r["size"], "has_alpha": info["bpp"] == 32,
            "compression": info["compression"],
        })
        log("  bitmap %-10s %4dx%-4d %2dbpp -> %s"
            % (r["ident"], info["width"], info["height"], info["bpp"], base + ".png"),
            self.quiet)

    # -- icons --------------------------------------------------------------
    def _icon_png(self, img, w, h, stem, r):
        path = os.path.join(self.dir_icons, "%s_%dx%d.png" % (stem, w, h))
        i = 1
        while os.path.exists(path):
            i += 1
            path = os.path.join(self.dir_icons, "%s_%dx%d_%d.png" % (stem, w, h, i))
        img.save(path, "PNG")
        return path

    def _do_icon_group(self, g, icons):
        data = g["data"]
        entries, kind = parse_group_icon(data)
        built = []
        used = set()
        gid = g["ident"]
        for idx, e in enumerate(entries):
            r = icons.get(e["id"])
            if r is None:
                log("  ! group %s references missing RT_ICON %s" % (gid, e["id"]), self.quiet)
                continue
            used.add(e["id"])
            try:
                img, info = decode_icon(r["data"], res_id=e["id"], lang=r["lang"],
                                        expected=(e["width"], e["height"]),
                                        quiet=self.quiet)
            except Exception as exc:
                log("  ! icon %s in group %s: %s" % (e["id"], gid, exc), self.quiet)
                continue
            rep, synth = repair_dib_for_ico(r["data"], info, self.quiet)
            built.append({
                "dib": rep, "width": info["width"], "height": info["height"],
                "bpp": info["bpp"], "color_count": e["color_count"] or 0,
                "planes": e["planes"], "hotspot": (e["planes"], e["bitcount"]),
                "res_id": e["id"], "synthesized_mask": synth, "info": info,
                "img": img, "orig_dib": r["data"], "res_size": len(r["data"]),
            })
        if not built:
            raise DIBError("group icon contains no usable images")
        ico_path = os.path.join(self.dir_icons, "groupicon_%s.ico" % sanitize(str(gid)))
        with open(ico_path, "wb") as fh:
            fh.write(build_ico(built, "icon"))
        self._add({
            "kind": "group_icon", "type": g["type"], "id": gid, "lang": g["lang"],
            "count": len(built),
            "images": [{"id": b["res_id"], "w": b["width"], "h": b["height"],
                        "bpp": b["bpp"]} for b in built],
            "file": os.path.relpath(ico_path, self.outdir).replace("\\", "/"),
            "res_size": g["size"],
        })
        for idx, b in enumerate(built):
            p = self._icon_png(b["img"], b["width"], b["height"],
                               "icon_%s_%d" % (sanitize(str(gid)), idx), g)
            self._add({
                "kind": "icon_image", "type": "RT_ICON", "group": gid,
                "id": b["res_id"], "lang": g["lang"],
                "width": b["width"], "height": b["height"], "bpp": b["bpp"],
                "mask_present": b["info"].get("mask_present"),
                "alpha_channel": b["info"].get("has_alpha_channel"),
                "synthesized_mask": b["synthesized_mask"],
                "file": os.path.relpath(p, self.outdir).replace("\\", "/"),
                "res_size": b["res_size"],
            })
        log("  group icon %-8s %d image(s) -> %s" % (gid, len(built), os.path.basename(ico_path)),
            self.quiet)
        return used

    def _do_orphan_icon(self, r):
        img, info = decode_icon(r["data"], res_id=r["ident"], lang=r["lang"], quiet=self.quiet)
        rep, synth = repair_dib_for_ico(r["data"], info, self.quiet)
        res = sanitize(str(r["ident"]))
        p = self._icon_png(img, info["width"], info["height"],
                           "icon_res%s" % res, r)
        ico_path = os.path.join(self.dir_icons, "icon_res%s.ico" % res)
        with open(ico_path, "wb") as fh:
            fh.write(build_ico([{
                "dib": rep, "width": info["width"], "height": info["height"],
                "bpp": info["bpp"], "color_count": 0, "planes": 1,
            }], "icon"))
        self._add({
            "kind": "icon_image", "type": "RT_ICON", "group": None,
            "id": r["ident"], "lang": r["lang"],
            "width": info["width"], "height": info["height"], "bpp": info["bpp"],
            "mask_present": info.get("mask_present"),
            "alpha_channel": info.get("has_alpha_channel"),
            "synthesized_mask": synth,
            "file": os.path.relpath(p, self.outdir).replace("\\", "/"),
            "ico": os.path.relpath(ico_path, self.outdir).replace("\\", "/"),
            "res_size": r["size"],
        })
        log("  icon  %-10s %4dx%-4d %2dbpp (orphan)" % (r["ident"], info["width"],
                                                       info["height"], info["bpp"]),
            self.quiet)

    # -- cursors ------------------------------------------------------------
    def _do_cursor_group(self, g, cursors):
        entries, kind = parse_group_icon(g["data"])
        used = set()
        built = []
        gid = g["ident"]
        for e in entries:
            r = cursors.get(e["id"])
            if r is None:
                continue
            used.add(e["id"])
            try:
                payload = r["data"]
                # RT_CURSOR payloads carry a 4-byte hotspot prefix before the
                # DIB header in most resources built by rc.exe; probe for it.
                # The GRPCURSORDIRENTRY width/height are NOT usable as a
                # geometry hint (they store the doubled DIB height), so the DIB
                # itself decides the logical size.
                img = None
                order = (4, 0) if _dib_ok(payload, 4) else (0, 4)
                for shift in order:
                    cand = payload[shift:]
                    if not cand:
                        continue
                    try:
                        img, info = decode_icon(cand, res_id=e["id"],
                                                quiet=self.quiet)
                        payload = cand
                        break
                    except Exception:
                        continue
                if img is None:
                    raise DIBError("cursor %s: no decodable DIB" % e["id"])
            except Exception as exc:
                log("  ! cursor %s in group %s: %s" % (e["id"], gid, exc), self.quiet)
                continue
            rep, synth = repair_dib_for_ico(payload, info, self.quiet)
            p = self._icon_png(img, info["width"], info["height"],
                               "cursor_%s_%d" % (sanitize(str(gid)), e["id"]), g)
            built.append({"dib": rep, "width": info["width"], "height": info["height"],
                          "bpp": info["bpp"], "color_count": 0,
                          "hotspot": (e["planes"], e["bitcount"])})
            self._add({
                "kind": "cursor_image", "type": "RT_CURSOR", "group": gid,
                "id": e["id"], "lang": r["lang"],
                "width": info["width"], "height": info["height"], "bpp": info["bpp"],
                "hotspot": [e["planes"], e["bitcount"]],
                "file": os.path.relpath(p, self.outdir).replace("\\", "/"),
                "res_size": r["size"],
            })
        if not built:
            raise DIBError("group cursor contains no usable images")
        cur_path = os.path.join(self.dir_cursors, "groupcursor_%s.cur" % sanitize(str(gid)))
        with open(cur_path, "wb") as fh:
            fh.write(build_ico(built, "cursor"))
        self._add({"kind": "group_cursor", "type": g["type"], "id": gid,
                   "lang": g["lang"], "count": len(built),
                   "file": os.path.relpath(cur_path, self.outdir).replace("\\", "/"),
                   "res_size": g["size"]})
        return used

    # -- manifest -----------------------------------------------------------
    def _write_manifest(self):
        path = os.path.join(self.outdir, "_manifest.json")
        payload = {
            "source": self.pe.path,
            "label": self.label,
            "machine": "0x%04X" % self.pe.machine,
            "pe32_plus": self.pe.is_pe32_plus,
            "types_requested": sorted(self.types),
            "resource_count_total": None,
            "records": self.records,
            "errors": self.errors,
        }
        try:
            with open(path, "w", encoding="utf-8") as fh:
                json.dump(payload, fh, indent=1, ensure_ascii=False)
        except Exception as exc:
            log("manifest write failed: %s" % exc, self.quiet)


# --------------------------------------------------------------------------
# --list
# --------------------------------------------------------------------------

def cmd_list(pe, quiet=False, types_filter=None, as_json=False):
    res = pe.resources(quiet=quiet)
    if types_filter:
        res = [r for r in res if r["type_id"] in types_filter]
    rows = []
    for r in res:
        rows.append({
            "type": r["type"],
            "type_id": r["type_id"],
            "id": r["ident"],
            "numeric": isinstance(r["ident"], int),
            "lang": r["lang"],
            "size": r["size"],
            "rva": r["rva"],
        })
    if as_json:
        print(json.dumps(rows, indent=1, ensure_ascii=False))
        return rows
    print("%-16s %-10s %-12s %-8s %10s %10s" % ("TYPE", "TYPE_ID", "ID/NAME", "LANG", "SIZE", "RVA"))
    print("-" * 74)
    for r in rows:
        print("%-16s %-10s %-12s %-8s %10d 0x%08X"
              % (r["type"], r["type_id"], str(r["id"])[:12], r["lang"], r["size"], r["rva"]))
    print("-" * 74)
    counts = {}
    for r in rows:
        counts[r["type"]] = counts.get(r["type"], 0) + 1
    print("total resources: %d" % len(rows))
    for k in sorted(counts):
        print("  %-18s %d" % (k, counts[k]))
    return rows


# --------------------------------------------------------------------------
# contact sheets
# --------------------------------------------------------------------------

def _load_font(size):
    try:
        return ImageFont.load_default(size=size)
    except Exception:
        return ImageFont.load_default()


def build_contact_sheet(outdir, label=None, max_cell=96, max_cells=240,
                        quiet=False):
    """Build one or more contact sheets from the PNG files in <outdir>."""
    label = label or os.path.basename(os.path.normpath(outdir))
    manifest = {}
    mpath = os.path.join(outdir, "_manifest.json")
    if os.path.exists(mpath):
        try:
            with open(mpath, "r", encoding="utf-8") as fh:
                manifest = json.load(fh)
        except Exception:
            manifest = {}
    meta_by_file = {}
    for rec in manifest.get("records", []):
        if rec.get("file"):
            meta_by_file[rec["file"].replace("\\", "/")] = rec

    files = []
    for sub in ("bitmaps", "icons", "cursors"):
        d = os.path.join(outdir, sub)
        if not os.path.isdir(d):
            continue
        for fn in sorted(os.listdir(d)):
            if fn.lower().endswith(".png"):
                files.append(os.path.join(d, fn))
    if not files:
        log("contact sheet: no PNGs in %s" % outdir, quiet)
        return []

    cells = []
    for f in files:
        rel = os.path.relpath(f, outdir).replace("\\", "/")
        rec = meta_by_file.get(rel, {})
        try:
            im = Image.open(f)
            im.load()
        except Exception as exc:
            log("contact sheet: cannot open %s: %s" % (rel, exc), quiet)
            continue
        if im.mode != "RGBA":
            im = im.convert("RGBA")
        # composite over a light checker so transparency is visible
        bg = Image.new("RGBA", im.size, (0xE8, 0xE8, 0xE8, 255))
        comp = Image.alpha_composite(bg, im).convert("RGB")
        w, h = comp.size
        scale = max(1, min(max_cell // max(1, w), max_cell // max(1, h)))
        if scale > 1:
            disp = comp.resize((w * scale, h * scale), Image.NEAREST)
        else:
            disp = comp.resize((min(max(1, w), max_cell), min(max(1, h), max_cell)),
                               Image.NEAREST)
        cid = rec.get("id", "")
        if isinstance(cid, int) and rec.get("kind") in ("icon_image",):
            cid = "icon#%d" % cid
        line1 = "%s %s" % (
            {"bitmap": "BMP", "icon_image": "ICO", "cursor_image": "CUR",
             "group_icon": "GRP", "group_cursor": "GRP"}.get(rec.get("kind", ""), "?"),
            cid if cid != "" else os.path.splitext(os.path.basename(f))[0])
        line2 = "%dx%d %dbpp" % (w, h, rec.get("bpp", 0)) if rec.get("bpp") else "%dx%d" % (w, h)
        cells.append((disp, line1.strip(), line2, rel))

    if not cells:
        return []

    base, ext = os.path.splitext(files[0])  # unused, kept for clarity
    sheets = []
    for start in range(0, len(cells), max_cells):
        chunk = cells[start:start + max_cells]
        # keep sheets roughly landscape-ish: aim for <= max_rows rows
        max_rows = 16
        cols = max(1, min(24, -(-len(chunk) // max_rows)))
        if cols < 4:
            cols = min(4, max(1, len(chunk)))
        rows = (len(chunk) + cols - 1) // cols
        cell_w = max_cell + 16
        cell_h = max_cell + 34
        pad = 8
        W = cols * cell_w + pad
        H = rows * cell_h + pad + 26
        sheet = Image.new("RGB", (W, H), (0xD4, 0xD0, 0xC8))
        dr = ImageDraw.Draw(sheet)
        f_title = _load_font(15)
        f_lbl = _load_font(11)
        dr.text((pad, 6), "%s  -  %d asset(s)%s"
                % (label, len(cells),
                   "  [sheet %d/%d]" % (start // max_cells + 1,
                                        (len(cells) + max_cells - 1) // max_cells)
                   if len(cells) > max_cells else ""),
                fill=(0x20, 0x20, 0x20), font=f_title)
        for i, (disp, l1, l2, rel) in enumerate(chunk):
            cx = pad + (i % cols) * cell_w
            cy = pad + 26 + (i // cols) * cell_h
            dr.rectangle([cx, cy, cx + cell_w - 4, cy + cell_h - 4],
                         fill=(0xF2, 0xF2, 0xF2), outline=(0xA0, 0xA0, 0xA0))
            ox = cx + (cell_w - 4 - disp.size[0]) // 2
            oy = cy + 2 + (max_cell - disp.size[1]) // 2
            sheet.paste(disp, (max(cx + 1, ox), max(cy + 1, oy)))
            ty = cy + max_cell + 4
            dr.text((cx + 4, ty), l1[:26], fill=(0x18, 0x18, 0x18), font=f_lbl)
            dr.text((cx + 4, ty + 12), l2[:26], fill=(0x50, 0x50, 0x50), font=f_lbl)
        idx = start // max_cells + 1
        name = "_contact_sheet.png" if idx == 1 else "_contact_sheet_%d.png" % idx
        path = os.path.join(outdir, name)
        sheet.save(path, "PNG")
        sheets.append(path)
        log("contact sheet: %s (%d cells)" % (path, len(chunk)), quiet)
    return sheets


# --------------------------------------------------------------------------
# main
# --------------------------------------------------------------------------

def main(argv=None):
    ap = argparse.ArgumentParser(
        description="Pure-Python Win32 PE resource extractor (RT_BITMAP / RT_ICON / "
                    "RT_GROUP_ICON / RT_CURSOR).")
    ap.add_argument("pe", help="path to a PE file (.dll/.cpl/.exe)")
    ap.add_argument("--out", help="output directory")
    ap.add_argument("--types", default="bitmap,icon",
                    help="comma list of: bitmap,icon,cursor (default bitmap,icon)")
    ap.add_argument("--list", action="store_true", help="list resources and exit")
    ap.add_argument("--json", action="store_true", help="with --list: emit JSON")
    ap.add_argument("--contact-sheet", action="store_true",
                    help="build _contact_sheet.png after extraction")
    ap.add_argument("--sheet-only", action="store_true",
                    help="only (re)build the contact sheet for --out")
    ap.add_argument("--label", help="override label/folder name for this source")
    ap.add_argument("--max-cell", type=int, default=96,
                    help="contact sheet cell box in px (default 96)")
    ap.add_argument("--quiet", action="store_true")
    args = ap.parse_args(argv)

    if args.sheet_only:
        if not args.out:
            ap.error("--sheet-only requires --out")
        build_contact_sheet(args.out, args.label, args.max_cell, quiet=args.quiet)
        return 0

    types = set()
    for t in args.types.split(","):
        t = t.strip().lower()
        if not t:
            continue
        if t not in TYPE_KEYS:
            ap.error("unknown type %r (known: %s)" % (t, ", ".join(sorted(TYPE_KEYS))))
        types.add(TYPE_KEYS[t])
    # icon implies group icon
    if RT_ICON in types:
        types.add(RT_GROUP_ICON)
    if RT_CURSOR in types:
        types.add(RT_GROUP_CURSOR)

    try:
        pe = PEFile(args.pe)
    except Exception as exc:
        log("FATAL: cannot parse %s: %s" % (args.pe, exc), args.quiet)
        return 2

    if args.list:
        cmd_list(pe, quiet=args.quiet, types_filter=None, as_json=args.json)
        return 0

    if not args.out:
        ap.error("--out is required unless --list/--sheet-only is used")

    label = args.label or os.path.splitext(os.path.basename(args.pe))[0].lower()
    ex = Extractor(pe, args.out, types, label=label, quiet=args.quiet)
    recs = ex.run()
    log("%s: %d record(s), %d error(s)" % (label, len(recs), len(ex.errors)), args.quiet)

    if args.contact_sheet:
        build_contact_sheet(args.out, label, args.max_cell, quiet=args.quiet)
    return 0


if __name__ == "__main__":
    sys.exit(main())
