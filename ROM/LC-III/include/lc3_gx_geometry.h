/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: QuickDraw GX Vector Graphics & Geometric Path Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_GX_GEOMETRY_H
#define LC3_GX_GEOMETRY_H

#include <stdint.h>
#include <stdbool.h>

#define GX_NO_ERR          0
#define GX_PARAM_ERR     -50

/* 16.16 Fixed point representations */
typedef int32_t Fixed16;

#define FIXED_ONE        0x00010000
#define FIXED_HALF       0x00008000

typedef struct GXPoint {
    Fixed16 x;
    Fixed16 y;
} GXPoint;

typedef struct GXRect {
    Fixed16 left;
    Fixed16 top;
    Fixed16 right;
    Fixed16 bottom;
} GXRect;

typedef struct GXMatrix {
    Fixed16 m[3][3]; /* [row][col] */
} GXMatrix;

/* Public API */
int16_t lc3_gx_init_transform(GXMatrix *matrix, Fixed16 scaleX, Fixed16 scaleY, Fixed16 transX, Fixed16 transY);
int16_t lc3_gx_apply_transform(const GXMatrix *matrix, const GXPoint *src, GXPoint *dst);
int16_t lc3_gx_eval_bezier_cubic(const GXPoint p[4], Fixed16 t, GXPoint *result);
int16_t lc3_gx_subdivide_bezier(const GXPoint p[4], GXPoint left[4], GXPoint right[4]);
int16_t lc3_gx_clip_polygon_line(const GXPoint *p1, const GXPoint *p2, const GXRect *clipRect, GXPoint *outP1, GXPoint *outP2, bool *visible);
int16_t lc3_gx_calc_path_bounds(const GXPoint *points, uint32_t pointCount, GXRect *bounds);

#endif /* LC3_GX_GEOMETRY_H */
