/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: TrueType Font Scaler & Font Dispatch Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_FONT_SCALER_H
#define LC3_FONT_SCALER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int32_t x;
    int32_t y;
} FixedPoint;

typedef struct {
    int32_t top;
    int32_t left;
    int32_t bottom;
    int32_t right;
} FixedRect;

typedef struct {
    FixedPoint origin;
    FixedPoint advance;
    FixedPoint sideBearing;
    FixedPoint otherSideBearing;
} GlyphMetrics;

typedef struct {
    int16_t ascent;
    int16_t descent;
    int16_t widMax;
    int16_t leading;
} FontMetrics;

/* TrueType Font Scaler API */
void lc3_init_fonts(void);
void lc3_set_outline_preferred(bool outlinePreferred);
bool lc3_get_outline_preferred(void);
void lc3_set_preserve_glyph(bool preserveGlyph);
bool lc3_get_preserve_glyph(void);
int16_t lc3_outline_metrics(int16_t byteCount, const void *textPtr, FixedPoint numer, FixedPoint denom, int16_t *yMax, int16_t *yMin, GlyphMetrics *outMetrics);
void lc3_flush_fonts(void);
bool lc3_real_font(int16_t fontNum, int16_t size);
void lc3_get_font_metrics(int16_t fontNum, int16_t size, FontMetrics *metrics);

/* Font Dispatch ($A854) */
int32_t lc3_font_dispatch(int16_t selector, void *params);

#endif /* LC3_FONT_SCALER_H */
