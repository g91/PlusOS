/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: SANE Elems68K Extended Transcendental Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_SANE_TRANSCEND_H
#define LC3_SANE_TRANSCEND_H

#include <stdint.h>
#include <stdbool.h>

#define SANE_NO_ERR         0
#define SANE_PARAM_ERR    -50

typedef struct extended80 {
    uint16_t signAndExp; // Bit 15: sign, Bits 0-14: biased exponent (bias 16383)
    uint64_t mantissa;   // 64-bit explicit leading bit mantissa
} extended80;

/* SANE Elems68K API */
void lc3_sane_log2(const extended80 *src, extended80 *dst);
void lc3_sane_exp2(const extended80 *src, extended80 *dst);
void lc3_sane_logn(const extended80 *src, extended80 *dst);
void lc3_sane_expn(const extended80 *src, extended80 *dst);
void lc3_sane_sin(const extended80 *src, extended80 *dst);
void lc3_sane_cos(const extended80 *src, extended80 *dst);
void lc3_sane_tan(const extended80 *src, extended80 *dst);
void lc3_sane_atan(const extended80 *src, extended80 *dst);

#endif /* LC3_SANE_TRANSCEND_H */
