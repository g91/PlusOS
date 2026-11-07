/* math.h - freestanding single/double precision math for PlusOS 68000 */
#ifndef _PLUSOS_MATH_H
#define _PLUSOS_MATH_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static inline float fabsf(float x) { return x < 0.0f ? -x : x; }
static inline double fabs(double x) { return x < 0.0 ? -x : x; }

static inline float floorf(float x) {
    int i = (int)x;
    return (x < (float)i) ? (float)(i - 1) : (float)i;
}

static inline float ceilf(float x) {
    int i = (int)x;
    return (x > (float)i) ? (float)(i + 1) : (float)i;
}

static inline float fmodf(float x, float y) {
    if (y == 0.0f) return 0.0f;
    int q = (int)(x / y);
    return x - (float)q * y;
}

static inline float sqrtf(float x) {
    if (x <= 0.0f) return 0.0f;
    float r = x > 1.0f ? x * 0.5f : 1.0f;
    for (int i = 0; i < 6; i++)
        r = 0.5f * (r + x / r);
    return r;
}

static inline double sqrt(double x) { return (double)sqrtf((float)x); }

/* Range-reduced minimax polynomial for sinf(x) over [-pi, pi] */
static inline float sinf(float x) {
    const float pi = 3.14159265f;
    const float two_pi = 6.28318531f;
    const float inv_two_pi = 0.15915494f;
    int k = (int)(x * inv_two_pi);
    x -= (float)k * two_pi;
    if (x > pi) x -= two_pi;
    else if (x < -pi) x += two_pi;
    float sign = 1.0f;
    if (x < 0.0f) { x = -x; sign = -1.0f; }
    if (x > 1.57079633f) x = pi - x;
    float x2 = x * x;
    return sign * x * (1.0f - x2 * (0.16666667f - x2 * (0.00833333f - x2 * 0.00019841f)));
}

static inline float cosf(float x) {
    return sinf(x + 1.57079633f);
}

static inline float tanf(float x) {
    float c = cosf(x);
    if (fabsf(c) < 0.0001f) c = (c < 0.0f) ? -0.0001f : 0.0001f;
    return sinf(x) / c;
}

static inline double sin(double x) { return (double)sinf((float)x); }
static inline double cos(double x) { return (double)cosf((float)x); }
static inline double tan(double x) { return (double)tanf((float)x); }

#ifdef __cplusplus
}
#endif

#endif /* _PLUSOS_MATH_H */
