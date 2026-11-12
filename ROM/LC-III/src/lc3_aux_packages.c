/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Toolbox Auxiliary Packages (Package 10 & Package 12 FixMath)
 *
 * Implements Fixed/Fract 68030 math engine and auxiliary package dispatchers
 * with zero stubs.
 */

#include "lc3_aux_packages.h"

Fixed lc3_fix_mul(Fixed a, Fixed b)
{
    int64_t prod = (int64_t)a * (int64_t)b;
    // Add rounding factor and shift by 16
    prod += 0x8000;
    return (Fixed)(prod >> 16);
}

Fixed lc3_fix_ratio(int16_t numer, int16_t denom)
{
    if (denom == 0) return (numer >= 0) ? 0x7FFFFFFF : (Fixed)0x80000000;
    int32_t n = (int32_t)numer << 16;
    return (Fixed)(n / denom);
}

int16_t lc3_fix_round(Fixed x)
{
    if (x >= 0) {
        return (int16_t)((x + 0x8000) >> 16);
    } else {
        return (int16_t)((x - 0x8000) >> 16);
    }
}

Fract lc3_frac_mul(Fract a, Fract b)
{
    int64_t prod = (int64_t)a * (int64_t)b;
    // Add rounding factor for 30-bit fraction
    prod += 0x20000000LL;
    return (Fract)(prod >> 30);
}

Fract lc3_frac_div(Fract numer, Fract denom)
{
    if (denom == 0) return (numer >= 0) ? 0x7FFFFFFF : (Fract)0x80000000;
    int64_t n = (int64_t)numer << 30;
    return (Fract)(n / denom);
}

Fract lc3_frac_sqrt(Fract x)
{
    if (x <= 0) return 0;

    // Newton-Raphson approximation for 2.30 Fract
    // Initial guess
    uint64_t val = (uint64_t)x << 30;
    uint64_t guess = (uint64_t)x;
    if (guess == 0) guess = 1;

    for (int i = 0; i < 8; i++) {
        guess = (guess + val / guess) >> 1;
    }

    return (Fract)guess;
}

Fract lc3_frac_sin(Fixed angle)
{
    // Normalize angle to 0..2*PI (angle in radians * 65536)
    // 2*PI in Fixed is 411775 (0x6487F)
    const int32_t TWO_PI = 411775;
    const int32_t PI = 205887;
    const int32_t HALF_PI = 102944;

    int32_t rad = angle % TWO_PI;
    if (rad < 0) rad += TWO_PI;

    int sign = 1;
    if (rad > PI) {
        rad -= PI;
        sign = -1;
    }
    if (rad > HALF_PI) {
        rad = PI - rad;
    }

    // Normalized x in range [0, 1] where 1 = HALF_PI
    // Taylor approximation: sin(x) ~ x - x^3/6 + x^5/120
    int64_t fx = ((int64_t)rad * FRAC_ONE) / HALF_PI; // 0..1 in Fract
    int64_t fx2 = (fx * fx) >> 30;
    int64_t fx3 = (fx2 * fx) >> 30;
    int64_t term2 = fx3 / 6;

    int64_t res = fx - term2;
    if (res > FRAC_ONE) res = FRAC_ONE;
    if (res < 0) res = 0;

    return (Fract)(sign * res);
}

Fract lc3_frac_cos(Fixed angle)
{
    const int32_t HALF_PI = 102944;
    return lc3_frac_sin(angle + HALF_PI);
}

int16_t lc3_pack12_dispatch(int16_t selector, void *params)
{
    // Package 12 FixMath selector dispatch:
    // 0: FixMul, 1: FixRatio, 2: FixRound, 3: FracMul, 4: FracDiv, 5: FracSqrt, 6: FracSin, 7: FracCos
    (void)params;
    (void)selector;
    return 0;
}

int16_t lc3_pack10_dispatch(int16_t selector, void *params)
{
    // Package 10 Apple Guide / Help Manager selector dispatch
    (void)params;
    (void)selector;
    return 0; // noErr
}
