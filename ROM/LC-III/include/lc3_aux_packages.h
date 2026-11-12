/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Toolbox Auxiliary Packages (Package 10 & Package 12 FixMath)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_AUX_PACKAGES_H
#define LC3_AUX_PACKAGES_H

#include <stdint.h>
#include <stdbool.h>

typedef int32_t Fixed; // 16.16 fixed-point
typedef int32_t Fract; // 2.30 fixed-point

#define FIX_ONE   0x00010000
#define FRAC_ONE  0x40000000

/* Package 12: FixMath Routines */
Fixed lc3_fix_mul(Fixed a, Fixed b);
Fixed lc3_fix_ratio(int16_t numer, int16_t denom);
int16_t lc3_fix_round(Fixed x);
Fract lc3_frac_mul(Fract a, Fract b);
Fract lc3_frac_div(Fract numer, Fract denom);
Fract lc3_frac_sqrt(Fract x);
Fract lc3_frac_sin(Fixed angle);
Fract lc3_frac_cos(Fixed angle);
int16_t lc3_pack12_dispatch(int16_t selector, void *params);

/* Package 10: Apple Guide / Online Help Package */
int16_t lc3_pack10_dispatch(int16_t selector, void *params);

#endif /* LC3_AUX_PACKAGES_H */
