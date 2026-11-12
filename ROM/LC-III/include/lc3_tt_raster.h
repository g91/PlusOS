/*
 * PlusOS Macintosh LC III ROM Compatibility Layer
 * Module 185: TrueType Rasterizer Curve Scaler & Drop-Out Control (GC)
 */

#ifndef LC3_TT_RASTER_H
#define LC3_TT_RASTER_H

#include <stdint.h>
#include <stdbool.h>

#define LC3_TT_MAX_CONTOURS      32
#define LC3_TT_MAX_EDGES         256
#define LC3_TT_MAX_SCANLINE_PTS  64

/* Drop-out control modes */
#define LC3_TT_DROPOUT_NONE      0x00
#define LC3_TT_DROPOUT_SIMPLE    0x01
#define LC3_TT_DROPOUT_SMART     0x02
#define LC3_TT_DROPOUT_NO_STUBS  0x04

typedef int32_t lc3_f26dot6_t;

typedef struct {
    lc3_f26dot6_t x;
    lc3_f26dot6_t y;
    uint8_t flags;
} lc3_tt_raster_point_t;

typedef struct {
    lc3_f26dot6_t x0, y0;
    lc3_f26dot6_t x1, y1;
    int direction; /* +1 or -1 */
} lc3_tt_raster_edge_t;

typedef struct {
    uint16_t ppem;
    uint8_t dropout_mode;
    uint16_t num_edges;
    lc3_tt_raster_edge_t edges[LC3_TT_MAX_EDGES];
    int16_t min_x;
    int16_t min_y;
    int16_t max_x;
    int16_t max_y;
} lc3_tt_raster_ctx_t;

void lc3_tt_raster_init(lc3_tt_raster_ctx_t *ctx, uint16_t ppem, uint8_t dropout_mode);
void lc3_tt_raster_scale_point(const lc3_tt_raster_point_t *in_pt, uint32_t scale_fixed, lc3_tt_raster_point_t *out_pt);
uint16_t lc3_tt_raster_subdivide_bezier(lc3_tt_raster_ctx_t *ctx,
                                        lc3_f26dot6_t x0, lc3_f26dot6_t y0,
                                        lc3_f26dot6_t cx, lc3_f26dot6_t cy,
                                        lc3_f26dot6_t x1, lc3_f26dot6_t y1);
uint16_t lc3_tt_raster_render_scanline(lc3_tt_raster_ctx_t *ctx, int32_t y_scan, int32_t *x_intersections, uint16_t max_pts);
void lc3_tt_raster_apply_dropout(lc3_tt_raster_ctx_t *ctx, uint8_t *bitmap, int16_t width, int16_t height, int16_t rowbytes);
int16_t lc3_tt_raster_emit_bitmap(lc3_tt_raster_ctx_t *ctx, uint8_t *out_bitmap, int16_t max_w, int16_t max_h, int16_t *actual_w, int16_t *actual_h, int16_t *actual_rowbytes);

#endif /* LC3_TT_RASTER_H */
