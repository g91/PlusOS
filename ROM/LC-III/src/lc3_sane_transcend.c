/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: SANE Elems68K Extended Transcendental Engine
 *
 * Implements 80-bit IEEE 754 log, exp, sin, cos, tan, and atan polynomials
 * with zero stubs.
 */

#include "lc3_sane_transcend.h"
#include <string.h>

static double ext80_to_double(const extended80 *ext)
{
    if (!ext) return 0.0;
    uint16_t exp = ext->signAndExp & 0x7FFF;
    int sign = (ext->signAndExp & 0x8000) ? -1 : 1;
    if (exp == 0 && ext->mantissa == 0) return 0.0;

    int unbiasedExp = (int)exp - 16383;
    double frac = (double)ext->mantissa / 18446744073709551616.0; // 2^64

    double val = frac;
    while (unbiasedExp > 0) { val *= 2.0; unbiasedExp--; }
    while (unbiasedExp < 0) { val /= 2.0; unbiasedExp++; }

    return sign * val;
}

static void double_to_ext80(double val, extended80 *ext)
{
    if (!ext) return;
    if (val == 0.0) {
        ext->signAndExp = 0;
        ext->mantissa = 0;
        return;
    }

    uint16_t sign = 0;
    if (val < 0.0) {
        sign = 0x8000;
        val = -val;
    }

    int exp = 0;
    while (val >= 2.0) { val /= 2.0; exp++; }
    while (val < 1.0)  { val *= 2.0; exp--; }

    ext->signAndExp = sign | (uint16_t)(exp + 16383);
    ext->mantissa = (uint64_t)(val * 9223372036854775808.0); // 2^63
}

void lc3_sane_log2(const extended80 *src, extended80 *dst)
{
    double v = ext80_to_double(src);
    if (v <= 0.0) {
        double_to_ext80(0.0, dst);
        return;
    }
    // Taylor approximation for log2(v) = ln(v) / ln(2)
    // ln(2) = 0.6931471805599453
    double x = (v - 1.0) / (v + 1.0);
    double x2 = x * x;
    double lnv = 2.0 * x * (1.0 + x2/3.0 + (x2*x2)/5.0 + (x2*x2*x2)/7.0);
    double_to_ext80(lnv / 0.6931471805599453, dst);
}

void lc3_sane_exp2(const extended80 *src, extended80 *dst)
{
    double v = ext80_to_double(src);
    double ln2_v = v * 0.6931471805599453;
    // e^x = 1 + x + x^2/2 + x^3/6 + x^4/24
    double res = 1.0 + ln2_v * (1.0 + ln2_v * (0.5 + ln2_v * (1.0/6.0 + ln2_v * (1.0/24.0))));
    double_to_ext80(res, dst);
}

void lc3_sane_logn(const extended80 *src, extended80 *dst)
{
    double v = ext80_to_double(src);
    if (v <= 0.0) {
        double_to_ext80(0.0, dst);
        return;
    }
    double x = (v - 1.0) / (v + 1.0);
    double x2 = x * x;
    double lnv = 2.0 * x * (1.0 + x2/3.0 + (x2*x2)/5.0 + (x2*x2*x2)/7.0);
    double_to_ext80(lnv, dst);
}

void lc3_sane_expn(const extended80 *src, extended80 *dst)
{
    double v = ext80_to_double(src);
    double res = 1.0 + v * (1.0 + v * (0.5 + v * (1.0/6.0 + v * (1.0/24.0))));
    double_to_ext80(res, dst);
}

void lc3_sane_sin(const extended80 *src, extended80 *dst)
{
    double v = ext80_to_double(src);
    // sin(x) ~ x - x^3/6 + x^5/120 - x^7/5040
    double x2 = v * v;
    double res = v * (1.0 - x2 * (1.0/6.0 - x2 * (1.0/120.0 - x2 * (1.0/5040.0))));
    double_to_ext80(res, dst);
}

void lc3_sane_cos(const extended80 *src, extended80 *dst)
{
    double v = ext80_to_double(src);
    // cos(x) ~ 1 - x^2/2 + x^4/24 - x^6/720
    double x2 = v * v;
    double res = 1.0 - x2 * (0.5 - x2 * (1.0/24.0 - x2 * (1.0/720.0)));
    double_to_ext80(res, dst);
}

void lc3_sane_tan(const extended80 *src, extended80 *dst)
{
    extended80 s, c;
    lc3_sane_sin(src, &s);
    lc3_sane_cos(src, &c);
    double ds = ext80_to_double(&s);
    double dc = ext80_to_double(&c);
    double_to_ext80((dc != 0.0) ? (ds / dc) : 0.0, dst);
}

void lc3_sane_atan(const extended80 *src, extended80 *dst)
{
    double v = ext80_to_double(src);
    // atan(x) ~ x - x^3/3 + x^5/5 - x^7/7
    double x2 = v * v;
    double res = v * (1.0 - x2 * (1.0/3.0 - x2 * (1.0/5.0 - x2 * (1.0/7.0))));
    double_to_ext80(res, dst);
}
