/* math.h - full double-precision libm declarations for the PlusOS Python port (see libm.c) */
#ifndef PLUSOS_PY_MATH_H
#define PLUSOS_PY_MATH_H
#define M_E 2.7182818284590452354
#define M_PI 3.14159265358979323846
#define M_PI_2 1.57079632679489661923
#define M_PI_4 0.78539816339744830962
#define M_LN2 0.69314718055994530942
#define M_LN10 2.30258509299404568402
#define M_SQRT2 1.41421356237309504880
#define M_LOG2E 1.4426950408889634074
#define HUGE_VAL (__builtin_huge_val())
#define INFINITY (__builtin_huge_val())
#define NAN (__builtin_nan(""))
#define FP_NAN 0
#define FP_INFINITE 1
#define FP_ZERO 2
#define FP_SUBNORMAL 3
#define FP_NORMAL 4
double __plusos_bits_dummy(void);
int __isnand(double x); int __isinfd(double x); int __signbitd(double x);
#define isnan(x) __isnand((double)(x))
#define isinf(x) __isinfd((double)(x))
#define isfinite(x) (!__isnand((double)(x)) && !__isinfd((double)(x)))
#define signbit(x) __signbitd((double)(x))
double fabs(double); double floor(double); double ceil(double); double trunc(double);
double round(double); double fmod(double, double); double sqrt(double); double cbrt(double);
double exp(double); double exp2(double); double expm1(double);
double log(double); double log2(double); double log10(double); double log1p(double);
double pow(double, double); double hypot(double, double);
double sin(double); double cos(double); double tan(double);
double asin(double); double acos(double); double atan(double); double atan2(double, double);
double sinh(double); double cosh(double); double tanh(double);
double asinh(double); double acosh(double); double atanh(double);
double frexp(double, int *); double ldexp(double, int); double modf(double, double *);
double copysign(double, double); double nextafter(double, double);
double erf(double); double erfc(double); double tgamma(double); double lgamma(double);
double fmax(double, double); double fmin(double, double); double rint(double);
double nearbyint(double); double scalbn(double, int);
long lround(double);
float fabsf(float); float floorf(float); float ceilf(float); float sqrtf(float);
float sinf(float); float cosf(float); float tanf(float); float fmodf(float, float);
#endif
