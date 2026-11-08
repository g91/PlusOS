/* plus3d_math.c - Vector, Matrix, and Fixed-Point Math Engine */
#include "plus3d_internal.h"

/* ============================================================================
 * Fixed-point helpers: sqrt and trig table
 * ============================================================================ */

/* 16.16 square root. For a = A*2^16, computes sqrt(A)*2^16 via a 64-bit
 * bitwise integer sqrt of (a << 16). ~32 iterations, no floating point. */
fix_t fix_sqrt(fix_t a) {
    if (a <= 0) return 0;
    uint64_t v = ((uint64_t)(uint32_t)a) << 16;
    uint64_t rem = 0, root = 0;
    for (int i = 0; i < 32; i++) {
        root <<= 1;
        rem = (rem << 2) | (v >> 62);
        v <<= 2;
        root++;
        if (root <= rem) { rem -= root; root++; }
        else root--;
    }
    return (fix_t)(root >> 1);
}

/* Sine table: 1024 steps over 2*pi, in 16.16. Built once from sinf() (setup
 * only - this runs a single time, not per frame). */
#define TRIG_STEPS 1024
static fix_t g_sin[TRIG_STEPS];
static int   g_trig_ready = 0;
#define TRIG_SCALE ((float)TRIG_STEPS / (2.0f * (float)M_PI))

void ensure_trig(void) {
    if (g_trig_ready) return;
    for (int i = 0; i < TRIG_STEPS; i++)
        g_sin[i] = FIXF(sinf((float)i * (2.0f * (float)M_PI / (float)TRIG_STEPS)));
    g_trig_ready = 1;
}

/* fixed sin/cos of a float radian angle (one float mul at the boundary, then
 * a table lookup - the expensive libgcc sinf/cosf never runs per frame). */
fix_t fsin(float rad) {
    ensure_trig();
    int idx = (int)(rad * TRIG_SCALE) & (TRIG_STEPS - 1);
    return g_sin[idx];
}
fix_t fcos(float rad) {
    ensure_trig();
    int idx = ((int)(rad * TRIG_SCALE) + TRIG_STEPS / 4) & (TRIG_STEPS - 1);
    return g_sin[idx];
}

/* ============================================================================
 * Vector & Matrix Implementation (fixed-point)
 * ============================================================================ */

Vec3 vec3_add(Vec3 a, Vec3 b) { return vec3f(a.x + b.x, a.y + b.y, a.z + b.z); }
Vec3 vec3_sub(Vec3 a, Vec3 b) { return vec3f(a.x - b.x, a.y - b.y, a.z - b.z); }

Vec3 vec3_scale(Vec3 v, float s) {
    fix_t fs = FIXF(s);
    return vec3f(fix_mul(v.x, fs), fix_mul(v.y, fs), fix_mul(v.z, fs));
}

fix_t vec3_dot(Vec3 a, Vec3 b) {
    return fix_mul(a.x, b.x) + fix_mul(a.y, b.y) + fix_mul(a.z, b.z);
}

Vec3 vec3_cross(Vec3 a, Vec3 b) {
    return vec3f(
        fix_mul(a.y, b.z) - fix_mul(a.z, b.y),
        fix_mul(a.z, b.x) - fix_mul(a.x, b.z),
        fix_mul(a.x, b.y) - fix_mul(a.y, b.x)
    );
}

fix_t vec3_length(Vec3 v) {
    return fix_sqrt(vec3_dot(v, v));
}

Vec3 vec3_normalize(Vec3 v) {
    fix_t len = vec3_length(v);
    if (len > 64) {   /* ~0.001 */
        return vec3f(fix_div(v.x, len), fix_div(v.y, len), fix_div(v.z, len));
    }
    return vec3f(0, 0, 0);
}

Vec3 vec3_lerp(Vec3 a, Vec3 b, float t) {
    fix_t ft = FIXF(t);
    return vec3f(
        a.x + fix_mul(b.x - a.x, ft),
        a.y + fix_mul(b.y - a.y, ft),
        a.z + fix_mul(b.z - a.z, ft)
    );
}

Mat4 mat4_identity(void) {
    Mat4 m;
    memset(&m, 0, sizeof(m));
    m.m[0][0] = FIX_ONE;
    m.m[1][1] = FIX_ONE;
    m.m[2][2] = FIX_ONE;
    m.m[3][3] = FIX_ONE;
    return m;
}

Mat4 mat4_translate(float tx, float ty, float tz) {
    Mat4 m = mat4_identity();
    m.m[3][0] = FIXF(tx);
    m.m[3][1] = FIXF(ty);
    m.m[3][2] = FIXF(tz);
    return m;
}

/* internal: translate/scale straight from fixed components (for the ECS) */
Mat4 mat4_translate_fix(fix_t x, fix_t y, fix_t z) {
    Mat4 m = mat4_identity();
    m.m[3][0] = x; m.m[3][1] = y; m.m[3][2] = z;
    return m;
}
Mat4 mat4_scale_fix(fix_t x, fix_t y, fix_t z) {
    Mat4 m = mat4_identity();
    m.m[0][0] = x; m.m[1][1] = y; m.m[2][2] = z;
    return m;
}

Mat4 mat4_rotate_x(float rad) {
    Mat4 m = mat4_identity();
    fix_t c = fcos(rad), s = fsin(rad);
    m.m[1][1] = c;
    m.m[1][2] = s;
    m.m[2][1] = -s;
    m.m[2][2] = c;
    return m;
}

Mat4 mat4_rotate_y(float rad) {
    Mat4 m = mat4_identity();
    fix_t c = fcos(rad), s = fsin(rad);
    m.m[0][0] = c;
    m.m[0][2] = -s;
    m.m[2][0] = s;
    m.m[2][2] = c;
    return m;
}

Mat4 mat4_rotate_z(float rad) {
    Mat4 m = mat4_identity();
    fix_t c = fcos(rad), s = fsin(rad);
    m.m[0][0] = c;
    m.m[0][1] = s;
    m.m[1][0] = -s;
    m.m[1][1] = c;
    return m;
}

Mat4 mat4_scale(float sx, float sy, float sz) {
    return mat4_scale_fix(FIXF(sx), FIXF(sy), FIXF(sz));
}

/* Row-by-row: r[i] = sum_k a[i][k] * b[k]. Rotation/translation/projection
 * matrices are mostly 0s and 1s, so those terms skip the multiply entirely. */
Mat4 mat4_mul(Mat4 a, Mat4 b) {
    Mat4 r;
    memset(&r, 0, sizeof(r));
    for (int i = 0; i < 4; i++) {
        for (int k = 0; k < 4; k++) {
            fix_t s = a.m[i][k];
            if (s == 0) continue;
            if (s == FIX_ONE) {
                r.m[i][0] += b.m[k][0]; r.m[i][1] += b.m[k][1];
                r.m[i][2] += b.m[k][2]; r.m[i][3] += b.m[k][3];
            } else {
                r.m[i][0] += fix_mul(s, b.m[k][0]); r.m[i][1] += fix_mul(s, b.m[k][1]);
                r.m[i][2] += fix_mul(s, b.m[k][2]); r.m[i][3] += fix_mul(s, b.m[k][3]);
            }
        }
    }
    return r;
}

Vec3 mat4_mul_vec3(Mat4 m, Vec3 v) {
    Vec3 r;
    r.x = fix_mul(v.x, m.m[0][0]) + fix_mul(v.y, m.m[1][0]) + fix_mul(v.z, m.m[2][0]) + m.m[3][0];
    r.y = fix_mul(v.x, m.m[0][1]) + fix_mul(v.y, m.m[1][1]) + fix_mul(v.z, m.m[2][1]) + m.m[3][1];
    r.z = fix_mul(v.x, m.m[0][2]) + fix_mul(v.y, m.m[1][2]) + fix_mul(v.z, m.m[2][2]) + m.m[3][2];
    fix_t w = fix_mul(v.x, m.m[0][3]) + fix_mul(v.y, m.m[1][3]) + fix_mul(v.z, m.m[2][3]) + m.m[3][3];
    if (w != FIX_ONE && (w > 64 || w < -64)) {
        r.x = fix_div(r.x, w);
        r.y = fix_div(r.y, w);
        r.z = fix_div(r.z, w);
    }
    return r;
}

Mat4 mat4_perspective(float fov_rad, float aspect, float near_z, float far_z) {
    /* one-time setup: float math here, stored as fixed */
    Mat4 m;
    memset(&m, 0, sizeof(m));
    float tan_half = tanf(fov_rad * 0.5f);
    m.m[0][0] = FIXF(1.0f / (aspect * tan_half));
    m.m[1][1] = FIXF(1.0f / tan_half);
    m.m[2][2] = FIXF(-(far_z + near_z) / (far_z - near_z));
    m.m[2][3] = FIXF(-1.0f);
    m.m[3][2] = FIXF(-(2.0f * far_z * near_z) / (far_z - near_z));
    return m;
}

Mat4 mat4_lookat(Vec3 eye, Vec3 target, Vec3 up) {
    Vec3 f = vec3_normalize(vec3_sub(target, eye));
    Vec3 s = vec3_normalize(vec3_cross(f, up));
    Vec3 u = vec3_cross(s, f);

    Mat4 m = mat4_identity();
    m.m[0][0] = s.x;  m.m[1][0] = s.y;  m.m[2][0] = s.z;
    m.m[0][1] = u.x;  m.m[1][1] = u.y;  m.m[2][1] = u.z;
    m.m[0][2] = -f.x; m.m[1][2] = -f.y; m.m[2][2] = -f.z;
    m.m[3][0] = -vec3_dot(s, eye);
    m.m[3][1] = -vec3_dot(u, eye);
    m.m[3][2] =  vec3_dot(f, eye);
    return m;
}

