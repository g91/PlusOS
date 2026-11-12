/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: SANE FP68K Package 4 / Package 5 Transcendental Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_FP_TRANSCENDENTAL_H
#define LC3_FP_TRANSCENDENTAL_H

#include <stdint.h>
#include <stdbool.h>

#define FP_NO_ERR            0
#define FP_ERR_DOMAIN      -300
#define FP_ERR_OVERFLOW    -301
#define FP_ERR_UNDERFLOW   -302

/* SANE 80-bit Extended Precision Struct */
typedef struct SANEExtended {
    uint16_t signAndExp;         /* Bit 15: sign, Bits 14-0: biased exponent */
    uint16_t mantissa[4];        /* 64-bit explicit leading bit mantissa */
} SANEExtended;

/* Public API */
int16_t lc3_fp_exp(double x, double *result);
int16_t lc3_fp_ln(double x, double *result);
int16_t lc3_fp_sin_cos(double radians, double *sinRes, double *cosRes);
int16_t lc3_fp_tan(double radians, double *result);
int16_t lc3_fp_atan(double y, double x, double *result);
int16_t lc3_fp_compound(double rate, double periods, double *result);

#endif /* LC3_FP_TRANSCENDENTAL_H */
