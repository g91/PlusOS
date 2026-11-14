/* libm.c - double-precision math library for the PlusOS Python port.
 * The 68000 has no FPU: every operation here runs on the p68 softfloat runtime, so these
 * favour small, straightforward algorithms (range reduction + series) over speed. */
#include <math.h>
#include <errno.h>

typedef unsigned long u32;
typedef union { double d; struct { u32 hi, lo; } w; } dbits;   /* big-endian */
static u32 HI(double x) { dbits b; b.d = x; return b.w.hi; }
static u32 LO(double x) { dbits b; b.d = x; return b.w.lo; }
static double mk(u32 hi, u32 lo) { dbits b; b.w.hi = hi; b.w.lo = lo; return b.d; }

#define PI      3.14159265358979323846
#define PIO2    1.57079632679489661923
#define LN2     0.69314718055994530942
#define LN2HI   6.93147180369123816490e-01
#define LN2LO   1.90821492927058770002e-10
#define NANV    ((HUGE_VAL) - (HUGE_VAL))

int __isnand(double x) { u32 h = HI(x) & 0x7fffffffUL; return h > 0x7ff00000UL || (h == 0x7ff00000UL && LO(x) != 0); }
int __isinfd(double x) { return (HI(x) & 0x7fffffffUL) == 0x7ff00000UL && LO(x) == 0; }
int __signbitd(double x) { return (HI(x) >> 31) & 1; }

double fabs(double x) { return mk(HI(x) & 0x7fffffffUL, LO(x)); }
double copysign(double x, double y) { return mk((HI(x) & 0x7fffffffUL) | (HI(y) & 0x80000000UL), LO(x)); }
double ldexp(double x, int n);
double fmod(double x, double y);

double frexp(double x, int *e)
{
    u32 h = HI(x), l = LO(x);
    int ex = (int)((h >> 20) & 0x7ff);
    *e = 0;
    if (ex == 0x7ff || x == 0.0) return x;
    if (ex == 0) {
        x *= 18014398509481984.0;
        h = HI(x); l = LO(x);
        ex = (int)((h >> 20) & 0x7ff) - 54;
    }
    *e = ex - 1022;
    return mk((h & 0x800fffffUL) | 0x3fe00000UL, l);
}

double ldexp(double x, int n)
{
    if (x == 0.0 || __isnand(x) || __isinfd(x)) return x;
    if (n > 3000) n = 3000;
    if (n < -3000) n = -3000;
    while (n > 1000) { x *= 8.98846567431158e307; n -= 1023; }
    while (n < -1000) { x *= 2.2250738585072014e-308; n += 1022; }
    return x * mk((u32)(n + 1023) << 20, 0);
}
double scalbn(double x, int n) { return ldexp(x, n); }

double trunc(double x)
{
    int e = (int)((HI(x) >> 20) & 0x7ff) - 1023;
    if (e >= 52 || x == 0.0 || __isnand(x)) return x;
    if (e < 0) return mk(HI(x) & 0x80000000UL, 0);
    if (e < 20) return mk(HI(x) & ~(0x000fffffUL >> e), 0);
    return mk(HI(x), LO(x) & ~(0xffffffffUL >> (e - 20)));
}
double floor(double x) { double t = trunc(x); return (x < 0 && t != x) ? t - 1.0 : t; }
double ceil(double x)
{
    double t = trunc(x);
    if (x > 0 && t != x) return t + 1.0;
    if (t == 0.0 && x < 0) return -0.0;
    return t;
}
double round(double x)
{
    double t = trunc(x), d = x - t;
    if (d >= 0.5) t += 1.0; else if (d <= -0.5) t -= 1.0;
    return t == 0.0 ? copysign(0.0, x) : t;
}
double rint(double x)
{
    double t = floor(x), d = x - t;
    if (d > 0.5 || (d == 0.5 && fmod(t, 2.0) != 0.0)) t += 1.0;
    return t == 0.0 ? copysign(0.0, x) : t;
}
double nearbyint(double x) { return rint(x); }
long lround(double x) { return (long)round(x); }
double modf(double x, double *ip)
{
    double t = trunc(x);
    *ip = t;
    if (__isinfd(x)) return copysign(0.0, x);
    return copysign(x - t, x);
}
double fmax(double a, double b) { if (__isnand(a)) return b; if (__isnand(b)) return a; return a > b ? a : b; }
double fmin(double a, double b) { if (__isnand(a)) return b; if (__isnand(b)) return a; return a < b ? a : b; }
double nextafter(double x, double y)
{
    if (__isnand(x) || __isnand(y)) return x + y;
    if (x == y) return y;
    if (x == 0.0) return copysign(mk(0, 1), y);
    u32 h = HI(x), l = LO(x);
    int up = (y > x) == (x > 0);
    if (up) { l++; if (l == 0) h++; } else { if (l == 0) h--; l--; }
    return mk(h, l);
}

double fmod(double x, double y)
{
    if (y == 0.0 || __isinfd(x) || __isnand(x) || __isnand(y)) return NANV;
    if (__isinfd(y)) return x;
    double ax = fabs(x), ay = fabs(y);
    if (ax < ay) return x;
    int ex, ey;
    frexp(ax, &ex);
    frexp(ay, &ey);
    double r = ax;
    int d;
    for (d = ex - ey; d >= 0; d--) {
        double t = ldexp(ay, d);
        if (r >= t) r -= t;
    }
    return x < 0 ? -r : r;
}

double sqrt(double x)
{
    if (x != x || x == 0.0) return x;
    if (x < 0) return NANV;
    if (__isinfd(x)) return x;
    int e;
    double m = frexp(x, &e);
    if (e & 1) { m *= 2.0; e--; }
    double r = 0.5 + 0.5 * m;
    int i;
    for (i = 0; i < 6; i++) r = 0.5 * (r + m / r);
    double r2 = r * r;
    if (r2 > m) { double rn = nextafter(r, 0.0); if (fabs(rn * rn - m) < fabs(r2 - m)) r = rn; }
    else if (r2 < m) { double rn = nextafter(r, 4.0); if (fabs(rn * rn - m) < fabs(r2 - m)) r = rn; }
    return ldexp(r, e / 2);
}
double cbrt(double x)
{
    if (x == 0.0 || __isnand(x) || __isinfd(x)) return x;
    double a = fabs(x);
    int e;
    double m = frexp(a, &e);
    int r3 = e % 3;
    if (r3 < 0) r3 += 3;
    m = ldexp(m, r3);
    e -= r3;
    double g = 0.5 + 0.5 * m;
    int i;
    for (i = 0; i < 12; i++) g = g - (g * g * g - m) / (3.0 * g * g);
    g = ldexp(g, e / 3);
    return x < 0 ? -g : g;
}
double hypot(double x, double y)
{
    x = fabs(x); y = fabs(y);
    if (__isinfd(x) || __isinfd(y)) return HUGE_VAL;
    if (__isnand(x) || __isnand(y)) return x + y;
    if (x < y) { double t = x; x = y; y = t; }
    if (x == 0.0) return 0.0;
    double r = y / x;
    return x * sqrt(1.0 + r * r);
}

double exp(double x)
{
    if (x != x) return x;
    if (x > 709.782712893384) return HUGE_VAL;
    if (x < -745.2) return 0.0;
    double kd = floor(x / LN2 + 0.5);
    int k = (int)kd;
    double r = (x - kd * LN2HI) - kd * LN2LO;
    double term = 1.0, sum = 1.0;
    int i;
    for (i = 1; i < 20; i++) {
        term *= r / i;
        sum += term;
        if (fabs(term) < 1e-18 * fabs(sum)) break;
    }
    return ldexp(sum, k);
}
double expm1(double x)
{
    if (fabs(x) < 0.25) {
        double term = x, sum = x;
        int i;
        for (i = 2; i < 24; i++) { term *= x / i; sum += term; }
        return sum;
    }
    return exp(x) - 1.0;
}
double exp2(double x)
{
    if (x == floor(x) && fabs(x) < 1000) return ldexp(1.0, (int)x);
    return exp(x * LN2);
}
double log(double x)
{
    if (x != x) return x;
    if (x < 0) return NANV;
    if (x == 0.0) return -HUGE_VAL;
    if (__isinfd(x)) return x;
    int e;
    double m = frexp(x, &e);
    if (m < 0.70710678118654752) { m *= 2.0; e--; }
    double s = (m - 1.0) / (m + 1.0), s2 = s * s, term = s, sum = 0.0;
    int i;
    for (i = 1; i < 60; i += 2) {
        sum += term / i;
        term *= s2;
        if (fabs(term) < 1e-19) break;
    }
    return 2.0 * sum + (double)e * LN2LO + (double)e * LN2HI;
}
double log1p(double x)
{
    if (x < -1.0) return NANV;
    double u = 1.0 + x;
    if (u == 1.0) return x;
    return log(u) * x / (u - 1.0);
}
double log2(double x)
{
    int e;
    double m = frexp(x, &e);
    if (m == 0.5 && x > 0) return (double)(e - 1);
    return log(x) * 1.44269504088896340736;
}
double log10(double x)
{
    double r = log(x) * 0.43429448190325182765;
    if (x >= 1.0 && x < 1e22 && x == floor(x)) {
        double rr = rint(r), p = 1.0;
        int i, n = (int)rr;
        for (i = 0; i < n; i++) p *= 10.0;
        if (p == x) return rr;
    }
    return r;
}

double pow(double x, double y)
{
    if (y == 0.0) return 1.0;
    if (x == 1.0) return 1.0;
    if (x != x || y != y) return x + y;
    int yint = (y == floor(y));
    int yodd = yint && fabs(y) < 4503599627370496.0 && fmod(y, 2.0) != 0.0;
    if (__isinfd(y)) {
        double ax = fabs(x);
        if (ax == 1.0) return 1.0;
        return ((ax > 1.0) == (y > 0)) ? HUGE_VAL : 0.0;
    }
    if (x == 0.0) {
        if (y < 0) return yodd ? copysign(HUGE_VAL, x) : HUGE_VAL;
        return yodd ? x : 0.0;
    }
    if (__isinfd(x)) {
        if (x > 0) return y > 0 ? HUGE_VAL : 0.0;
        if (y > 0) return yodd ? -HUGE_VAL : HUGE_VAL;
        return yodd ? -0.0 : 0.0;
    }
    if (x < 0 && !yint) return NANV;
    if (yint && fabs(y) <= 1024.0) {
        long n = (long)fabs(y);
        double r = 1.0, b = x;
        while (n) { if (n & 1) r *= b; b *= b; n >>= 1; }
        return y < 0 ? 1.0 / r : r;
    }
    double r = exp(y * log(fabs(x)));
    return (x < 0 && yodd) ? -r : r;
}

static double sin_k(double r)
{
    double r2 = r * r, t = r, s = r;
    int i;
    for (i = 1; i < 14; i++) { t *= -r2 / ((2 * i) * (2 * i + 1)); s += t; }
    return s;
}
static double cos_k(double r)
{
    double r2 = r * r, t = 1.0, s = 1.0;
    int i;
    for (i = 1; i < 14; i++) { t *= -r2 / ((2 * i - 1) * (2 * i)); s += t; }
    return s;
}
static int reduce(double x, double *r)
{
    double nd = floor(x / PIO2 + 0.5);
    *r = ((x - nd * 1.57079632673412561417e+00) - nd * 6.07710050630396597660e-11) - nd * 2.02226624879595063154e-21;
    double m = fmod(nd, 4.0);
    if (m < 0) m += 4.0;
    return (int)m;
}
double sin(double x)
{
    if (x != x || __isinfd(x)) return NANV;
    double r;
    int n = reduce(x, &r);
    switch (n) { case 0: return sin_k(r); case 1: return cos_k(r); case 2: return -sin_k(r); default: return -cos_k(r); }
}
double cos(double x)
{
    if (x != x || __isinfd(x)) return NANV;
    double r;
    int n = reduce(x, &r);
    switch (n) { case 0: return cos_k(r); case 1: return -sin_k(r); case 2: return -cos_k(r); default: return sin_k(r); }
}
double tan(double x)
{
    if (x != x || __isinfd(x)) return NANV;
    double r;
    int n = reduce(x, &r);
    double s = sin_k(r), c = cos_k(r);
    return (n & 1) ? -c / s : s / c;
}
static double atan_k(double x)
{
    double base = 0.0;
    if (x > 0.41421356237309503) { x = (x - 1.0) / (x + 1.0); base = PI / 4.0; }
    double x2 = x * x, t = x, s = x;
    int i;
    for (i = 1; i < 40; i++) {
        t *= -x2;
        s += t / (2 * i + 1);
        if (fabs(t) < 1e-19) break;
    }
    return base + s;
}
double atan(double x)
{
    if (x != x) return x;
    double a = fabs(x), r;
    if (__isinfd(a)) r = PIO2;
    else if (a > 1.0) r = PIO2 - atan_k(1.0 / a);
    else r = atan_k(a);
    return x < 0 ? -r : r;
}
double atan2(double y, double x)
{
    if (x != x || y != y) return x + y;
    if (y == 0.0) {
        if (x > 0 || (x == 0.0 && !__signbitd(x))) return y;
        return copysign(PI, y);
    }
    if (x == 0.0) return copysign(PIO2, y);
    if (__isinfd(x)) {
        if (__isinfd(y)) return copysign(x > 0 ? PI / 4 : 3 * PI / 4, y);
        return x > 0 ? copysign(0.0, y) : copysign(PI, y);
    }
    if (__isinfd(y)) return copysign(PIO2, y);
    double r = atan(y / x);
    if (x < 0) r += (y < 0) ? -PI : PI;
    return r;
}
double asin(double x) { if (x != x || fabs(x) > 1.0) return NANV; return atan2(x, sqrt((1.0 - x) * (1.0 + x))); }
double acos(double x) { if (x != x || fabs(x) > 1.0) return NANV; return atan2(sqrt((1.0 - x) * (1.0 + x)), x); }
double sinh(double x)
{
    double a = fabs(x), r;
    if (a < 0.25) { double e = expm1(a); r = 0.5 * (e + e / (e + 1.0)); }
    else if (a > 710) r = exp(a - LN2);
    else { double e = exp(a); r = 0.5 * (e - 1.0 / e); }
    return x < 0 ? -r : r;
}
double cosh(double x) { double a = fabs(x); if (a > 710) return exp(a - LN2); double e = exp(a); return 0.5 * (e + 1.0 / e); }
double tanh(double x)
{
    double a = fabs(x), r;
    if (a > 22) r = 1.0; else { double e = expm1(2.0 * a); r = e / (e + 2.0); }
    return x < 0 ? -r : r;
}
double asinh(double x)
{
    double a = fabs(x), r;
    if (a > 1e8) r = log(a) + LN2; else r = log1p(a + a * a / (1.0 + sqrt(1.0 + a * a)));
    return x < 0 ? -r : r;
}
double acosh(double x) { if (x < 1.0) return NANV; if (x > 1e8) return log(x) + LN2; return log(x + sqrt(x * x - 1.0)); }
double atanh(double x)
{
    double a = fabs(x);
    if (a > 1.0) return NANV;
    if (a == 1.0) return copysign(HUGE_VAL, x);
    double r = 0.5 * log1p(2.0 * a / (1.0 - a));
    return x < 0 ? -r : r;
}

double erf(double x)
{
    if (x != x) return x;
    double a = fabs(x), r;
    if (a < 3.0) {
        double t = a, s = a, a2 = a * a;
        int n;
        for (n = 1; n < 80; n++) { t *= -a2 / n; s += t / (2 * n + 1); if (fabs(t) < 1e-18) break; }
        r = 1.1283791670955126 * s;
    } else if (a > 6.0) r = 1.0;
    else {
        double f = 0.0;
        int n;
        for (n = 60; n >= 1; n--) f = n * 0.5 / (a + f);
        r = 1.0 - exp(-a * a) / 1.7724538509055160273 / (a + f);
    }
    return x < 0 ? -r : r;
}
double erfc(double x)
{
    if (x != x) return x;
    if (x < 3.0) return 1.0 - erf(x);
    if (x > 27.0) return 0.0;
    double f = 0.0;
    int n;
    for (n = 80; n >= 1; n--) f = n * 0.5 / (x + f);
    return exp(-x * x) / 1.7724538509055160273 / (x + f);
}
static double lgam_pos(double x)
{
    static const double c[9] = { 0.99999999999980993, 676.5203681218851, -1259.1392167224028, 771.32342877765313,
        -176.61502916214059, 12.507343278686905, -0.13857109526572012, 9.9843695780195716e-6, 1.5056327351493116e-7 };
    x -= 1.0;
    double a = c[0], t = x + 7.5;
    int i;
    for (i = 1; i < 9; i++) a += c[i] / (x + i);
    return 0.91893853320467274178 + (x + 0.5) * log(t) - t + log(a);
}
double lgamma(double x)
{
    if (x != x) return x;
    if (__isinfd(x)) return HUGE_VAL;
    if (x <= 0 && x == floor(x)) return HUGE_VAL;
    if (x < 0.5) return log(PI / fabs(sin(PI * x))) - lgam_pos(1.0 - x);
    return lgam_pos(x);
}
double tgamma(double x)
{
    if (x != x) return x;
    if (x == floor(x)) {
        if (x <= 0) return NANV;
        if (x < 24) { double r = 1.0; int i, n = (int)x; for (i = 2; i < n; i++) r *= i; return r; }
    }
    if (x > 171.7) return HUGE_VAL;
    if (x < 0.5) return PI / (sin(PI * x) * tgamma(1.0 - x));
    return exp(lgam_pos(x));
}

float fabsf(float x) { return (float)fabs(x); }
float floorf(float x) { return (float)floor(x); }
float ceilf(float x) { return (float)ceil(x); }
float sqrtf(float x) { return (float)sqrt(x); }
float sinf(float x) { return (float)sin(x); }
float cosf(float x) { return (float)cos(x); }
float tanf(float x) { return (float)tan(x); }
float fmodf(float x, float y) { return (float)fmod(x, y); }
double fma(double a, double b, double c) { return a * b + c; }
int isnormal(double x) { double a = fabs(x); return a >= 2.2250738585072014e-308 && a < HUGE_VAL; }
