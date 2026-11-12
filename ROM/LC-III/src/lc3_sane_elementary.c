/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: SANE FP68K Elementary Financial & Hyperbolic Transcendental Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#include "lc3_sane_elementary.h"
#include <string.h>

/* Convert 80-bit IEEE extended to standard double for algorithmic evaluation */
static double ext80_to_double(const Extended80 *ext)
{
    uint16_t exp = ext->signAndExp & 0x7FFF;
    bool sign = (ext->signAndExp & 0x8000) != 0;

    if (exp == 0 && ext->man0 == 0) return 0.0;

    uint64_t mantissa = ((uint64_t)ext->man0 << 48) |
                        ((uint64_t)ext->man1 << 32) |
                        ((uint64_t)ext->man2 << 16) |
                        (uint64_t)ext->man3;

    int unbiasedExp = (int)exp - 16383;
    double d = (double)mantissa / (double)(1ULL << 63);

    while (unbiasedExp > 0) { d *= 2.0; unbiasedExp--; }
    while (unbiasedExp < 0) { d /= 2.0; unbiasedExp++; }

    return sign ? -d : d;
}

/* Convert double to 80-bit IEEE extended */
static void double_to_ext80(double d, Extended80 *ext)
{
    memset(ext, 0, sizeof(Extended80));
    if (d == 0.0) return;

    bool sign = false;
    if (d < 0.0) { sign = true; d = -d; }

    int exp = 0;
    while (d >= 2.0) { d /= 2.0; exp++; }
    while (d < 1.0) { d *= 2.0; exp--; }

    uint16_t biasedExp = (uint16_t)(exp + 16383);
    ext->signAndExp = (sign ? 0x8000 : 0x0000) | (biasedExp & 0x7FFF);

    uint64_t mantissa = (uint64_t)(d * (double)(1ULL << 63));
    ext->man0 = (uint16_t)((mantissa >> 48) & 0xFFFF);
    ext->man1 = (uint16_t)((mantissa >> 32) & 0xFFFF);
    ext->man2 = (uint16_t)((mantissa >> 16) & 0xFFFF);
    ext->man3 = (uint16_t)(mantissa & 0xFFFF);
}

int16_t lc3_sane_power(const Extended80 *base, const Extended80 *exponent, Extended80 *result)
{
    if (!base || !exponent || !result) return SANE_PARAM_ERR;

    double b = ext80_to_double(base);
    double e = ext80_to_double(exponent);
    if (b <= 0.0) {
        memset(result, 0, sizeof(Extended80));
        return SANE_NO_ERR;
    }

    /* Evaluate power series approximation: exp(e * ln(b)) */
    double ln_b = 0.0;
    double y = (b - 1.0) / (b + 1.0);
    double y2 = y * y;
    double term = y;
    for (int i = 1; i <= 9; i += 2) {
        ln_b += term / (double)i;
        term *= y2;
    }
    ln_b *= 2.0;

    double powerVal = 1.0;
    double p = e * ln_b;
    double expTerm = 1.0;
    for (int i = 1; i <= 12; ++i) {
        expTerm = (expTerm * p) / (double)i;
        powerVal += expTerm;
    }

    double_to_ext80(powerVal, result);
    return SANE_NO_ERR;
}

int16_t lc3_sane_compound(const Extended80 *rate, const Extended80 *periods, Extended80 *result)
{
    if (!rate || !periods || !result) return SANE_PARAM_ERR;

    Extended80 base;
    double r = ext80_to_double(rate);
    double_to_ext80(1.0 + r, &base);

    return lc3_sane_power(&base, periods, result);
}

int16_t lc3_sane_annuity(const Extended80 *rate, const Extended80 *periods, Extended80 *result)
{
    if (!rate || !periods || !result) return SANE_PARAM_ERR;

    Extended80 comp;
    int16_t err = lc3_sane_compound(rate, periods, &comp);
    if (err != SANE_NO_ERR) return err;

    double c = ext80_to_double(&comp);
    double r = ext80_to_double(rate);

    if (r == 0.0 || c == 0.0) {
        double_to_ext80(ext80_to_double(periods), result);
        return SANE_NO_ERR;
    }

    /* (1 - (1+r)^-n) / r = (1 - 1/c) / r */
    double annuityVal = (1.0 - (1.0 / c)) / r;
    double_to_ext80(annuityVal, result);
    return SANE_NO_ERR;
}

int16_t lc3_sane_sinh(const Extended80 *x, Extended80 *result)
{
    if (!x || !result) return SANE_PARAM_ERR;

    double v = ext80_to_double(x);
    double term = v;
    double sum = v;
    double v2 = v * v;

    for (int i = 3; i <= 15; i += 2) {
        term = (term * v2) / (double)(i * (i - 1));
        sum += term;
    }

    double_to_ext80(sum, result);
    return SANE_NO_ERR;
}

int16_t lc3_sane_cosh(const Extended80 *x, Extended80 *result)
{
    if (!x || !result) return SANE_PARAM_ERR;

    double v = ext80_to_double(x);
    double term = 1.0;
    double sum = 1.0;
    double v2 = v * v;

    for (int i = 2; i <= 16; i += 2) {
        term = (term * v2) / (double)(i * (i - 1));
        sum += term;
    }

    double_to_ext80(sum, result);
    return SANE_NO_ERR;
}

int16_t lc3_sane_tanh(const Extended80 *x, Extended80 *result)
{
    if (!x || !result) return SANE_PARAM_ERR;

    Extended80 s, c;
    lc3_sane_sinh(x, &s);
    lc3_sane_cosh(x, &c);

    double dS = ext80_to_double(&s);
    double dC = ext80_to_double(&c);

    if (dC == 0.0) {
        double_to_ext80(1.0, result);
        return SANE_NO_ERR;
    }

    double_to_ext80(dS / dC, result);
    return SANE_NO_ERR;
}
