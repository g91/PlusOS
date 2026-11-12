/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: SANE FP68K Package 4 / Package 5 Transcendental Engine
 *
 * Implements SANE transcendental elementary functions (exp, ln, sin/cos, tan, atan, compound)
 * with zero stubs.
 */

#include "lc3_fp_transcendental.h"

#define PI          3.14159265358979323846
#define TWO_PI      6.28318530717958647692
#define HALF_PI     1.57079632679489661923
#define LN2         0.693147180559945309417

/* 1416: lc3_fp_exp */
int16_t lc3_fp_exp(double x, double *result)
{
    if (!result) return FP_ERR_DOMAIN;

    if (x > 709.78) {
        *result = 1e308;
        return FP_ERR_OVERFLOW;
    }
    if (x < -708.39) {
        *result = 0.0;
        return FP_ERR_UNDERFLOW;
    }

    /* Range reduction: x = k * ln(2) + r */
    int32_t k = (int32_t)(x / LN2 + (x >= 0 ? 0.5 : -0.5));
    double r = x - (double)k * LN2;

    /* Taylor expansion of e^r: 1 + r + r^2/2! + r^3/3! + ... */
    double term = 1.0;
    double sum = 1.0;
    for (int i = 1; i <= 16; ++i) {
        term = term * r / (double)i;
        sum += term;
        if (term < 1e-17 && term > -1e-17) break;
    }

    /* Scale by 2^k */
    double scale = 1.0;
    if (k > 0) {
        for (int i = 0; i < k; ++i) scale *= 2.0;
    } else if (k < 0) {
        for (int i = 0; i < -k; ++i) scale *= 0.5;
    }

    *result = sum * scale;
    return FP_NO_ERR;
}

/* 1417: lc3_fp_ln */
int16_t lc3_fp_ln(double x, double *result)
{
    if (!result || x <= 0.0) {
        return FP_ERR_DOMAIN;
    }

    /* Range reduction: x = m * 2^e where 0.5 <= m < 1.0 */
    int32_t e = 0;
    double m = x;
    while (m >= 1.0) {
        m *= 0.5;
        e++;
    }
    while (m < 0.5) {
        m *= 2.0;
        e--;
    }

    /* ln(m) using y = (m - 1) / (m + 1), ln(m) = 2*(y + y^3/3 + y^5/5 + ...) */
    double y = (m - 1.0) / (m + 1.0);
    double y2 = y * y;
    double y_pow = y;
    double sum = 0.0;

    for (int i = 1; i <= 25; i += 2) {
        sum += y_pow / (double)i;
        y_pow *= y2;
    }

    double lnm = 2.0 * sum;
    *result = lnm + (double)e * LN2;
    return FP_NO_ERR;
}

/* 1418: lc3_fp_sin_cos */
int16_t lc3_fp_sin_cos(double radians, double *sinRes, double *cosRes)
{
    if (!sinRes || !cosRes) return FP_ERR_DOMAIN;

    /* Normalize to [0, TWO_PI) */
    double theta = radians;
    int32_t turns = (int32_t)(theta / TWO_PI);
    theta -= (double)turns * TWO_PI;
    if (theta < 0.0) theta += TWO_PI;

    /* Taylor series for sin(theta): theta - theta^3/3! + theta^5/5! - ... */
    /* Normalize theta into [-PI, PI] */
    if (theta > PI) theta -= TWO_PI;

    double t2 = theta * theta;
    double s_term = theta;
    double s_sum = theta;
    for (int i = 3; i <= 19; i += 2) {
        s_term = -s_term * t2 / (double)(i * (i - 1));
        s_sum += s_term;
    }

    /* Taylor series for cos(theta): 1 - theta^2/2! + theta^4/4! - ... */
    double c_term = 1.0;
    double c_sum = 1.0;
    for (int i = 2; i <= 20; i += 2) {
        c_term = -c_term * t2 / (double)(i * (i - 1));
        c_sum += c_term;
    }

    *sinRes = s_sum;
    *cosRes = c_sum;
    return FP_NO_ERR;
}

/* 1419: lc3_fp_tan */
int16_t lc3_fp_tan(double radians, double *result)
{
    if (!result) return FP_ERR_DOMAIN;

    double s, c;
    lc3_fp_sin_cos(radians, &s, &c);

    if (c > -1e-15 && c < 1e-15) {
        return FP_ERR_OVERFLOW;
    }

    *result = s / c;
    return FP_NO_ERR;
}

/* 1420: lc3_fp_atan */
int16_t lc3_fp_atan(double y, double x, double *result)
{
    if (!result) return FP_ERR_DOMAIN;

    if (x == 0.0) {
        if (y > 0.0) {
            *result = HALF_PI;
            return FP_NO_ERR;
        } else if (y < 0.0) {
            *result = -HALF_PI;
            return FP_NO_ERR;
        } else {
            *result = 0.0;
            return FP_NO_ERR;
        }
    }

    double v = y / x;
    bool invert = false;
    if (v > 1.0 || v < -1.0) {
        v = 1.0 / v;
        invert = true;
    }

    /* Taylor expansion of atan(v): v - v^3/3 + v^5/5 - ... */
    double v2 = v * v;
    double term = v;
    double sum = v;
    for (int i = 3; i <= 25; i += 2) {
        term = -term * v2;
        sum += term / (double)i;
    }

    double res = invert ? (HALF_PI - sum) : sum;

    /* Quad adjustment for atan2 */
    if (x < 0.0) {
        if (y >= 0.0) res += PI;
        else res -= PI;
    }

    *result = res;
    return FP_NO_ERR;
}

/* 1421: lc3_fp_compound */
int16_t lc3_fp_compound(double rate, double periods, double *result)
{
    if (!result || rate < -1.0) return FP_ERR_DOMAIN;

    if (rate == 0.0) {
        *result = 1.0;
        return FP_NO_ERR;
    }

    /* Compound: (1 + rate)^periods = exp(periods * ln(1 + rate)) */
    double logTerm;
    int16_t err = lc3_fp_ln(1.0 + rate, &logTerm);
    if (err != FP_NO_ERR) return err;

    return lc3_fp_exp(periods * logTerm, result);
}
