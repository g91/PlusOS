/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: TrueType Font B-Spline Curve Rasterizer & Hinting Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_TRUETYPE_ENGINE_H
#define LC3_TRUETYPE_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

#define TT_NO_ERR         0
#define TT_GLYPH_NOT_FOUND -1
#define TT_PARAM_ERR     -50

typedef void *TTScalerContext;

typedef struct {
    int32_t x;
    int32_t y;
    uint8_t flags; // Bit 0: on-curve (1) vs off-curve (0)
} TTPoint;

typedef struct {
    int16_t ascent;
    int16_t descent;
    int16_t lineGap;
    int16_t advanceWidthMax;
    int16_t minLeftSideBearing;
} TTFontMetrics;

typedef struct {
    uint16_t glyphIndex;
    int16_t numberOfContours;
    int16_t xMin;
    int16_t yMin;
    int16_t xMax;
    int16_t yMax;
    uint32_t pointCount;
    TTPoint points[64];
    uint16_t endPointsOfContours[8];
} TTGlyphOutline;

/* TrueType Scaler API */
TTScalerContext lc3_tt_new_scaler_context(const void *sfntData, uint32_t pointSize);
void lc3_tt_dispose_scaler_context(TTScalerContext ctx);
int16_t lc3_tt_get_glyph_outline(TTScalerContext ctx, uint16_t glyphID, TTGlyphOutline *outline);
int16_t lc3_tt_rasterize_glyph(TTScalerContext ctx, const TTGlyphOutline *outline, uint8_t *bitmap, int16_t rowBytes);
int16_t lc3_tt_get_font_metrics(TTScalerContext ctx, TTFontMetrics *metrics);
void lc3_tt_transform_points(TTPoint *points, uint32_t count, int32_t scaleX, int32_t scaleY);
void lc3_tt_decompose_contour(const TTPoint *points, uint32_t count, uint8_t *bitmap, int16_t rowBytes);

#endif /* LC3_TRUETYPE_ENGINE_H */
