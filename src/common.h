/* ===========================================================================
 * common.h -- shared identity and small helpers for KeyODO
 *
 * KeyODO 0.1 -- keyboard odometer, Windows 98 SE visual target.
 * Native Win32 + GDI. No MFC, no .NET, no Direct2D, no theme API.
 * ======================================================================== */

#ifndef KEYODO_COMMON_H
#define KEYODO_COMMON_H

#define KEYODO_NAME          "KeyODO"
/* Version numbers live in res/resource.h, which both the resource compiler and
 * the C sources can include. */

/* Every module is plain C. It must compile unchanged with MSVC 6.0, which is
 * the realistic Windows 98 SE toolchain, so:
 *   - declarations at the top of a block, no C99 mixed declarations
 *   - no stdint.h, no variable length arrays, no compound literals
 *   - 64-bit integers only through KeyU64 below, which maps to the Microsoft
 *     __int64 extension that MSVC 6 already understood.
 */

#ifndef ARRAY_LEN
#define ARRAY_LEN(a) ((int)(sizeof(a) / sizeof((a)[0])))
#endif

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

#ifdef _MSC_VER
typedef unsigned __int64 KeyU64;
#else
typedef unsigned long long KeyU64;
#endif

/* Plain decimal formatting; avoids the %I64u / %llu portability trap. */
void KeyU64_Format(KeyU64 v, char *buf, int cch);
/* Right-aligned, zero padded to `digits` places (extra digits are kept). */
void KeyU64_FormatPad(KeyU64 v, int digits, char *buf, int cch);

#endif /* KEYODO_COMMON_H */
