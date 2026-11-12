/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: SANE FP68K Elementary Financial & Hyperbolic Transcendental Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_SANE_ELEMENTARY_H
#define LC3_SANE_ELEMENTARY_H

#include <stdint.h>
#include <stdbool.h>

#define SANE_NO_ERR         0
#define SANE_OVERFLOW_ERR  -1
#define SANE_UNDERFLOW_ERR -2
#define SANE_PARAM_ERR    -50

typedef struct Extended80 {
    uint16_t signAndExp;      /* 1 sign bit, 15 exponent bits (bias 16383) */
    uint16_t man0;            /* High 16 bits of significand (includes explicit integer bit) */
    uint16_t man1;
    uint16_t man2;
    uint16_t man3;            /* Low 16 bits */
} Extended80;

/* SANE Elementary & Financial API */
int16_t lc3_sane_power(const Extended80 *base, const Extended80 *exponent, Extended80 *result);
int16_t lc3_sane_compound(const Extended80 *rate, const Extended80 *periods, Extended80 *result);
int16_t lc3_sane_annuity(const Extended80 *rate, const Extended80 *periods, Extended80 *result);
int16_t lc3_sane_sinh(const Extended80 *x, Extended80 *result);
int16_t lc3_sane_cosh(const Extended80 *x, Extended80 *result);
int16_t lc3_sane_tanh(const Extended80 *x, Extended80 *result);

#endif /* LC3_SANE_ELEMENTARY_H */
