/*
 * PlusOS Macintosh LC III ROM Compatibility Layer
 * Module 185: TrueType Rasterizer Curve Scaler & Drop-Out Control (GC)
 */

#include "lc3_tt_raster.h"
#include <string.h>

void lc3_tt_raster_init(lc3_tt_raster_ctx_t *ctx, uint16_t ppem, uint8_t dropout_mode)
{
    if (!ctx) return;
    memset(ctx, 0, sizeof(lc3_tt_raster_ctx_t));
    ctx->ppem = ppem ? ppem : 12;
    ctx->dropout_mode = dropout_mode;
    ctx->min_x = 32767;
    ctx->min_y = 32767;
    ctx->max_x = -32768;
    ctx->max_y = -32768;
}

void lc3_tt_raster_scale_point(const lc3_tt_raster_point_t *in_pt, uint32_t scale_fixed, lc3_tt_raster_point_t *out_pt)
{
    if (!in_pt || !out_pt) return;
    /* scale_fixed is 16.16 fixed point */
    int64_t sx = ((int64_t)in_pt->x * scale_fixed + 0x8000) >> 16;
    int64_t sy = ((int64_t)in_pt->y * scale_fixed + 0x8000) >> 16;
    out_pt->x = (lc3_f26dot6_t)sx;
    out_pt->y = (lc3_f26dot6_t)sy;
    out_pt->flags = in_pt->flags;
}

uint16_t lc3_tt_raster_subdivide_bezier(lc3_tt_raster_ctx_t *ctx,
                                        lc3_f26dot6_t x0, lc3_f26dot6_t y0,
                                        lc3_f26dot6_t cx, lc3_f26dot6_t cy,
                                        lc3_f26dot6_t x1, lc3_f26dot6_t y1)
{
    if (!ctx) return 0;
    /* De Casteljau subdivision for quadratic Bezier */
    /* Midpoint test: distance of control point from line chord */
    int32_t dx = x1 - x0;
    int32_t dy = y1 - y0;
    int32_t dev_x = (cx - x0) - (dx >> 1);
    int32_t dev_y = (cy - y0) - (dy >> 1);
    if (dev_x < 0) dev_x = -dev_x;
    if (dev_y < 0) dev_y = -dev_y;

    /* Threshold for flat enough (less than 1/4 pixel = 16 in 26.6) */
    if ((dev_x + dev_y < 16) || (ctx->num_edges >= LC3_TT_MAX_EDGES - 2)) {
        if (ctx->num_edges < LC3_TT_MAX_EDGES) {
            lc3_tt_raster_edge_t *e = &ctx->edges[ctx->num_edges++];
            e->x0 = x0; e->y0 = y0;
            e->x1 = x1; e->y1 = y1;
            e->direction = (y1 >= y0) ? 1 : -1;
            return 1;
        }
        return 0;
    }

    /* Subdivide at t = 0.5 */
    lc3_f26dot6_t q0x = (x0 + cx) >> 1;
    lc3_f26dot6_t q0y = (y0 + cy) >> 1;
    lc3_f26dot6_t q1x = (cx + x1) >> 1;
    lc3_f26dot6_t q1y = (cy + y1) >> 1;
    lc3_f26dot6_t mid_x = (q0x + q1x) >> 1;
    lc3_f26dot6_t mid_y = (q0y + q1y) >> 1;

    uint16_t c1 = lc3_tt_raster_subdivide_bezier(ctx, x0, y0, q0x, q0y, mid_x, mid_y);
    uint16_t c2 = lc3_tt_raster_subdivide_bezier(ctx, mid_x, mid_y, q1x, q1y, x1, y1);
    return c1 + c2;
}

static void sort_intersections(int32_t *arr, uint16_t n)
{
    for (uint16_t i = 0; i < n; i++) {
        for (uint16_t j = i + 1; j < n; j++) {
            if (arr[j] < arr[i]) {
                int32_t t = arr[i];
                arr[i] = arr[j];
                arr[j] = t;
            }
        }
    }
}

uint16_t lc3_tt_raster_render_scanline(lc3_tt_raster_ctx_t *ctx, int32_t y_scan, int32_t *x_intersections, uint16_t max_pts)
{
    if (!ctx || !x_intersections || max_pts == 0) return 0;
    uint16_t count = 0;

    for (uint16_t i = 0; i < ctx->num_edges && count < max_pts; i++) {
        lc3_tt_raster_edge_t *e = &ctx->edges[i];
        int32_t min_y = e->y0 < e->y1 ? e->y0 : e->y1;
        int32_t max_y = e->y0 > e->y1 ? e->y0 : e->y1;

        if (y_scan >= min_y && y_scan < max_y) {
            int32_t dy = e->y1 - e->y0;
            if (dy != 0) {
                int64_t x_int = e->x0 + ((int64_t)(y_scan - e->y0) * (e->x1 - e->x0)) / dy;
                x_intersections[count++] = (int32_t)x_int;
            }
        }
    }

    sort_intersections(x_intersections, count);
    return count;
}

void lc3_tt_raster_apply_dropout(lc3_tt_raster_ctx_t *ctx, uint8_t *bitmap, int16_t width, int16_t height, int16_t rowbytes)
{
    if (!ctx || !bitmap || (ctx->dropout_mode == LC3_TT_DROPOUT_NONE)) return;

    /* TrueType Dropout Control rule: connect disconnected pixels in narrow stems */
    for (int16_t y = 1; y < height - 1; y++) {
        uint8_t *row_prev = bitmap + (y - 1) * rowbytes;
        uint8_t *row_curr = bitmap + y * rowbytes;
        uint8_t *row_next = bitmap + (y + 1) * rowbytes;

        for (int16_t x = 1; x < width - 1; x++) {
            int byte_idx = x >> 3;
            int bit_mask = 0x80 >> (x & 7);

            bool curr = (row_curr[byte_idx] & bit_mask) != 0;
            if (!curr) {
                bool above = (row_prev[byte_idx] & bit_mask) != 0;
                bool below = (row_next[byte_idx] & bit_mask) != 0;

                /* Vertical stem gap */
                if (above && below) {
                    row_curr[byte_idx] |= bit_mask;
                    continue;
                }

                /* Horizontal stem gap */
                int left_byte = (x - 1) >> 3;
                int left_bit = 0x80 >> ((x - 1) & 7);
                int right_byte = (x + 1) >> 3;
                int right_bit = 0x80 >> ((x + 1) & 7);

                bool left = (row_curr[left_byte] & left_bit) != 0;
                bool right = (row_curr[right_byte] & right_bit) != 0;

                if (left && right) {
                    row_curr[byte_idx] |= bit_mask;
                }
            }
        }
    }
}

int16_t lc3_tt_raster_emit_bitmap(lc3_tt_raster_ctx_t *ctx, uint8_t *out_bitmap, int16_t max_w, int16_t max_h, int16_t *actual_w, int16_t *actual_h, int16_t *actual_rowbytes)
{
    if (!ctx || !out_bitmap) return -1;

    /* Compute bounds from edges */
    int32_t min_x = 0x7FFFFFFF, min_y = 0x7FFFFFFF;
    int32_t max_x = -0x7FFFFFFF, max_y = -0x7FFFFFFF;

    for (uint16_t i = 0; i < ctx->num_edges; i++) {
        if (ctx->edges[i].x0 < min_x) min_x = ctx->edges[i].x0;
        if (ctx->edges[i].x1 < min_x) min_x = ctx->edges[i].x1;
        if (ctx->edges[i].x0 > max_x) max_x = ctx->edges[i].x0;
        if (ctx->edges[i].x1 > max_x) max_x = ctx->edges[i].x1;

        if (ctx->edges[i].y0 < min_y) min_y = ctx->edges[i].y0;
        if (ctx->edges[i].y1 < min_y) min_y = ctx->edges[i].y1;
        if (ctx->edges[i].y0 > max_y) max_y = ctx->edges[i].y0;
        if (ctx->edges[i].y1 > max_y) max_y = ctx->edges[i].y1;
    }

    int16_t px_min_x = (int16_t)(min_x >> 6);
    int16_t px_max_x = (int16_t)((max_x + 63) >> 6);
    int16_t px_min_y = (int16_t)(min_y >> 6);
    int16_t px_max_y = (int16_t)((max_y + 63) >> 6);

    int16_t w = px_max_x - px_min_x;
    int16_t h = px_max_y - px_min_y;
    if (w <= 0 || h <= 0) {
        w = 1; h = 1;
    }
    if (w > max_w) w = max_w;
    if (h > max_h) h = max_h;

    int16_t rb = (w + 15) / 16 * 2; /* 16-bit word aligned */
    memset(out_bitmap, 0, (size_t)rb * h);

    /* Fill scanlines */
    int32_t x_pts[LC3_TT_MAX_SCANLINE_PTS];
    for (int16_t y = 0; y < h; y++) {
        int32_t y_coord = ((px_min_y + y) << 6) + 32; /* sample at pixel center */
        uint16_t num_pts = lc3_tt_raster_render_scanline(ctx, y_coord, x_pts, LC3_TT_MAX_SCANLINE_PTS);

        uint8_t *row = out_bitmap + y * rb;
        for (uint16_t p = 0; p + 1 < num_pts; p += 2) {
            int16_t x_start = (int16_t)((x_pts[p] + 32) >> 6) - px_min_x;
            int16_t x_end = (int16_t)((x_pts[p + 1] + 32) >> 6) - px_min_x;
            if (x_start < 0) x_start = 0;
            if (x_end > w) x_end = w;

            for (int16_t x = x_start; x < x_end; x++) {
                row[x >> 3] |= (0x80 >> (x & 7));
            }
        }
    }

    if (ctx->dropout_mode != LC3_TT_DROPOUT_NONE) {
        lc3_tt_raster_apply_dropout(ctx, out_bitmap, w, h, rb);
    }

    if (actual_w) *actual_w = w;
    if (actual_h) *actual_h = h;
    if (actual_rowbytes) *actual_rowbytes = rb;
    return 0;
}
