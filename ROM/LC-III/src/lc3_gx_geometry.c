/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: QuickDraw GX Vector Graphics & Geometric Path Engine
 *
 * Implements 16.16 fixed point matrices, cubic Bézier evaluation & subdivision,
 * Cohen-Sutherland line clipping, and vector path bounds calculation with zero stubs.
 */

#include "lc3_gx_geometry.h"

static inline Fixed16 fix_mul(Fixed16 a, Fixed16 b)
{
    int64_t prod = (int64_t)a * (int64_t)b;
    return (Fixed16)((prod + 0x8000) >> 16);
}

static inline Fixed16 fix_div(Fixed16 a, Fixed16 b)
{
    if (b == 0) return (a >= 0) ? 0x7FFFFFFF : (Fixed16)0x80000000;
    int64_t num = ((int64_t)a << 16);
    return (Fixed16)(num / b);
}

/* 1440: lc3_gx_init_transform */
int16_t lc3_gx_init_transform(GXMatrix *matrix, Fixed16 scaleX, Fixed16 scaleY, Fixed16 transX, Fixed16 transY)
{
    if (!matrix) return GX_PARAM_ERR;

    matrix->m[0][0] = scaleX;
    matrix->m[0][1] = 0;
    matrix->m[0][2] = transX;

    matrix->m[1][0] = 0;
    matrix->m[1][1] = scaleY;
    matrix->m[1][2] = transY;

    matrix->m[2][0] = 0;
    matrix->m[2][1] = 0;
    matrix->m[2][2] = FIXED_ONE;

    return GX_NO_ERR;
}

/* 1441: lc3_gx_apply_transform */
int16_t lc3_gx_apply_transform(const GXMatrix *matrix, const GXPoint *src, GXPoint *dst)
{
    if (!matrix || !src || !dst) return GX_PARAM_ERR;

    Fixed16 x = src->x;
    Fixed16 y = src->y;

    dst->x = fix_mul(matrix->m[0][0], x) + fix_mul(matrix->m[0][1], y) + matrix->m[0][2];
    dst->y = fix_mul(matrix->m[1][0], x) + fix_mul(matrix->m[1][1], y) + matrix->m[1][2];

    return GX_NO_ERR;
}

/* 1442: lc3_gx_eval_bezier_cubic */
int16_t lc3_gx_eval_bezier_cubic(const GXPoint p[4], Fixed16 t, GXPoint *result)
{
    if (!p || !result || t < 0 || t > FIXED_ONE) {
        return GX_PARAM_ERR;
    }

    Fixed16 u = FIXED_ONE - t;
    Fixed16 u2 = fix_mul(u, u);
    Fixed16 u3 = fix_mul(u2, u);

    Fixed16 t2 = fix_mul(t, t);
    Fixed16 t3 = fix_mul(t2, t);

    Fixed16 term0 = u3;
    Fixed16 term1 = 3 * fix_mul(u2, t);
    Fixed16 term2 = 3 * fix_mul(u, t2);
    Fixed16 term3 = t3;

    result->x = fix_mul(term0, p[0].x) + fix_mul(term1, p[1].x) + fix_mul(term2, p[2].x) + fix_mul(term3, p[3].x);
    result->y = fix_mul(term0, p[0].y) + fix_mul(term1, p[1].y) + fix_mul(term2, p[2].y) + fix_mul(term3, p[3].y);

    return GX_NO_ERR;
}

/* 1443: lc3_gx_subdivide_bezier */
int16_t lc3_gx_subdivide_bezier(const GXPoint p[4], GXPoint left[4], GXPoint right[4])
{
    if (!p || !left || !right) return GX_PARAM_ERR;

    /* de Casteljau midpoint subdivision */
    GXPoint p01, p12, p23;
    p01.x = (p[0].x + p[1].x) >> 1;
    p01.y = (p[0].y + p[1].y) >> 1;

    p12.x = (p[1].x + p[2].x) >> 1;
    p12.y = (p[1].y + p[2].y) >> 1;

    p23.x = (p[2].x + p[3].x) >> 1;
    p23.y = (p[2].y + p[3].y) >> 1;

    GXPoint p012, p123;
    p012.x = (p01.x + p12.x) >> 1;
    p012.y = (p01.y + p12.y) >> 1;

    p123.x = (p12.x + p23.x) >> 1;
    p123.y = (p12.y + p23.y) >> 1;

    GXPoint p0123;
    p0123.x = (p012.x + p123.x) >> 1;
    p0123.y = (p012.y + p123.y) >> 1;

    /* Left segment */
    left[0] = p[0];
    left[1] = p01;
    left[2] = p012;
    left[3] = p0123;

    /* Right segment */
    right[0] = p0123;
    right[1] = p123;
    right[2] = p23;
    right[3] = p[3];

    return GX_NO_ERR;
}

/* 1444: lc3_gx_clip_polygon_line */
#define CLIP_INSIDE 0
#define CLIP_LEFT   1
#define CLIP_RIGHT  2
#define CLIP_BOTTOM 4
#define CLIP_TOP    8

static uint8_t compute_outcode(Fixed16 x, Fixed16 y, const GXRect *r)
{
    uint8_t code = CLIP_INSIDE;
    if (x < r->left)   code |= CLIP_LEFT;
    else if (x > r->right) code |= CLIP_RIGHT;
    if (y < r->top)    code |= CLIP_TOP;
    else if (y > r->bottom) code |= CLIP_BOTTOM;
    return code;
}

int16_t lc3_gx_clip_polygon_line(const GXPoint *p1, const GXPoint *p2, const GXRect *clipRect, GXPoint *outP1, GXPoint *outP2, bool *visible)
{
    if (!p1 || !p2 || !clipRect || !outP1 || !outP2 || !visible) {
        return GX_PARAM_ERR;
    }

    Fixed16 x0 = p1->x, y0 = p1->y;
    Fixed16 x1 = p2->x, y1 = p2->y;

    uint8_t outcode0 = compute_outcode(x0, y0, clipRect);
    uint8_t outcode1 = compute_outcode(x1, y1, clipRect);

    bool accept = false;

    while (true) {
        if ((outcode0 | outcode1) == 0) {
            accept = true;
            break;
        } else if (outcode0 & outcode1) {
            break;
        } else {
            Fixed16 x = 0, y = 0;
            uint8_t outcodeOut = outcode0 ? outcode0 : outcode1;

            if (outcodeOut & CLIP_BOTTOM) {
                x = x0 + fix_mul(fix_div(x1 - x0, y1 - y0), clipRect->bottom - y0);
                y = clipRect->bottom;
            } else if (outcodeOut & CLIP_TOP) {
                x = x0 + fix_mul(fix_div(x1 - x0, y1 - y0), clipRect->top - y0);
                y = clipRect->top;
            } else if (outcodeOut & CLIP_RIGHT) {
                y = y0 + fix_mul(fix_div(y1 - y0, x1 - x0), clipRect->right - x0);
                x = clipRect->right;
            } else if (outcodeOut & CLIP_LEFT) {
                y = y0 + fix_mul(fix_div(y1 - y0, x1 - x0), clipRect->left - x0);
                x = clipRect->left;
            }

            if (outcodeOut == outcode0) {
                x0 = x;
                y0 = y;
                outcode0 = compute_outcode(x0, y0, clipRect);
            } else {
                x1 = x;
                y1 = y;
                outcode1 = compute_outcode(x1, y1, clipRect);
            }
        }
    }

    *visible = accept;
    if (accept) {
        outP1->x = x0;
        outP1->y = y0;
        outP2->x = x1;
        outP2->y = y1;
    }

    return GX_NO_ERR;
}

/* 1445: lc3_gx_calc_path_bounds */
int16_t lc3_gx_calc_path_bounds(const GXPoint *points, uint32_t pointCount, GXRect *bounds)
{
    if (!points || pointCount == 0 || !bounds) {
        return GX_PARAM_ERR;
    }

    Fixed16 minX = points[0].x, maxX = points[0].x;
    Fixed16 minY = points[0].y, maxY = points[0].y;

    for (uint32_t i = 1; i < pointCount; ++i) {
        if (points[i].x < minX) minX = points[i].x;
        if (points[i].x > maxX) maxX = points[i].x;
        if (points[i].y < minY) minY = points[i].y;
        if (points[i].y > maxY) maxY = points[i].y;
    }

    bounds->left   = minX;
    bounds->top    = minY;
    bounds->right  = maxX;
    bounds->bottom = maxY;

    return GX_NO_ERR;
}
