/* plusos/fastmath.h - arithmetic the 68000 can actually do fast.
 *
 * The 68000 (Mac Plus / SE) has exactly two fast arithmetic instructions:
 *     MULS.W / MULU.W   16 x 16 -> 32 bit multiply   (~70 cycles)
 *     DIVS.W / DIVU.W   32 / 16 -> 16 bit divide     (~140 cycles)
 * Everything wider is a libgcc SOFTWARE routine:
 *     int * int (32x32)      -> __mulsi3   (a function call + many MULs)
 *     int / int (32/32)      -> __divsi3   (a bit loop, hundreds of cycles)
 *     int64_t  * and /       -> __muldi3 / __divdi3  (thousands of cycles)
 *     float anything         -> soft-float (no FPU)
 * A plain `y * rowbytes` in C is therefore a library call. Hot loops must
 * keep operands in 16 bits so GCC emits MULS.W, and use divs16() for division.
 *
 * Rules of thumb:
 *   - multiply two values that fit in int16_t:  muls16(a, b)
 *   - divide an int32 by an int16 when the quotient fits in int16: divs16(n, d)
 *   - row addressing: ROWOFF(y, rowbytes) instead of y * rowbytes
 *   - 16.16 fixed multiply: fix_mul_fast(a, b) (4 hardware multiplies)
 */
#ifndef PLUSOS_FASTMATH_H
#define PLUSOS_FASTMATH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FM_INLINE static inline __attribute__((always_inline))

/* 16 x 16 -> 32 signed multiply: one MULS.W.
 * NOTE: inline asm is REQUIRED. Retro68's GCC emits MULU.W for the unsigned
 * case but calls the __mulsi3 library routine for signed 16x16 products, even
 * when both operands are sign-extended shorts (verified by disassembly). */
FM_INLINE int32_t muls16(int16_t a, int16_t b)
{
#if defined(__m68k__) || defined(__mc68000__)
    int32_t r = a;
    __asm__("muls.w %1,%0" : "+d"(r) : "dmi"(b) : "cc");
    return r;
#else
    return (int32_t)a * (int32_t)b;
#endif
}

/* 16 x 16 -> 32 unsigned multiply: one MULU.W. */
FM_INLINE uint32_t mulu16(uint16_t a, uint16_t b)
{
#if defined(__m68k__) || defined(__mc68000__)
    uint32_t r = a;
    __asm__("mulu.w %1,%0" : "+d"(r) : "dmi"(b) : "cc");
    return r;
#else
    return (uint32_t)a * (uint32_t)b;
#endif
}

/* 32 / 16 -> 16 signed divide: one DIVS.W. The caller guarantees d != 0 and
 * that the quotient fits in int16_t (otherwise the CPU flags overflow and the
 * result is meaningless). */
FM_INLINE int16_t divs16(int32_t n, int16_t d)
{
#if defined(__m68k__) || defined(__mc68000__)
    __asm__("divs.w %1,%0" : "+d"(n) : "dmi"(d) : "cc");
    return (int16_t)n;                       /* low word = quotient */
#else
    return (int16_t)(n / d);
#endif
}

/* Byte offset of row y in a bitmap: one MULS.W instead of __mulsi3. */
#define ROWOFF(y, rowbytes) muls16((int16_t)(y), (int16_t)(rowbytes))

/* 16.16 fixed-point multiply from four hardware 16x16 multiplies (no int64).
 *   a = ah*2^16 + al  (ah signed, al unsigned)   likewise b
 *   a*b >> 16 = (ah*bh << 16) + ah*bl + al*bh + (al*bl >> 16)
 * Valid while |a|,|b| < 32768.0. */
FM_INLINE int32_t fix_mul_fast(int32_t a, int32_t b)
{
    int16_t  ah = (int16_t)(a >> 16), bh = (int16_t)(b >> 16);
    uint16_t al = (uint16_t)a,        bl = (uint16_t)b;
    int32_t r = muls16(ah, bh) << 16;
    r += muls16(ah, (int16_t)bl);
    if ((int16_t)bl < 0) r += (int32_t)ah << 16;     /* bl >= 32768 correction */
    r += muls16(bh, (int16_t)al);
    if ((int16_t)al < 0) r += (int32_t)bh << 16;
    r += (int32_t)(mulu16(al, bl) >> 16);
    return r;
}

#ifdef __cplusplus
}
#endif

#endif /* PLUSOS_FASTMATH_H */
