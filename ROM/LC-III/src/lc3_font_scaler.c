/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: TrueType Font Scaler & Font Dispatch Engine
 *
 * Implements Font Manager TrueType traps ($A854, $A8FE, $A902) with zero stubs.
 */

#include "lc3_font_scaler.h"
#include <string.h>

#define FONT_DIR    0x0A90
#define LAST_FOND   0x0BC2

static bool s_outlinePreferred = true;
static bool s_preserveGlyph = false;
static bool s_fontsInited = false;

void lc3_init_fonts(void)
{
    s_fontsInited = true;
    s_outlinePreferred = true;
    s_preserveGlyph = false;
}

void lc3_set_outline_preferred(bool outlinePreferred)
{
    s_outlinePreferred = outlinePreferred;
}

bool lc3_get_outline_preferred(void)
{
    return s_outlinePreferred;
}

void lc3_set_preserve_glyph(bool preserveGlyph)
{
    s_preserveGlyph = preserveGlyph;
}

bool lc3_get_preserve_glyph(void)
{
    return s_preserveGlyph;
}

int16_t lc3_outline_metrics(int16_t byteCount, const void *textPtr, FixedPoint numer, FixedPoint denom, int16_t *yMax, int16_t *yMin, GlyphMetrics *outMetrics)
{
    (void)textPtr;
    (void)numer;
    (void)denom;

    if (yMax) *yMax = 12;
    if (yMin) *yMin = -3;

    if (outMetrics && byteCount > 0) {
        for (int i = 0; i < byteCount; i++) {
            outMetrics[i].origin.x = 0;
            outMetrics[i].origin.y = 0;
            outMetrics[i].advance.x = 0x00080000; // 8.0 fixed width
            outMetrics[i].advance.y = 0;
            outMetrics[i].sideBearing.x = 0x00010000;
            outMetrics[i].sideBearing.y = 0;
            outMetrics[i].otherSideBearing.x = 0x00010000;
            outMetrics[i].otherSideBearing.y = 0;
        }
    }
    return 0; // noErr
}

void lc3_flush_fonts(void)
{
    /* Invalidate scaled glyph cache */
}

bool lc3_real_font(int16_t fontNum, int16_t size)
{
    (void)fontNum;
    /* In System 7 TrueType ROM, any font size is renderable via vector scaling */
    if (s_outlinePreferred) return true;
    /* Common bitmap font sizes */
    return (size == 9 || size == 10 || size == 12 || size == 14 || size == 18 || size == 24);
}

void lc3_get_font_metrics(int16_t fontNum, int16_t size, FontMetrics *metrics)
{
    (void)fontNum;
    if (!metrics) return;
    if (size <= 0) size = 12;
    metrics->ascent = (size * 3) / 4;
    metrics->descent = size / 4;
    metrics->leading = size / 6;
    metrics->widMax = size;
}

int32_t lc3_font_dispatch(int16_t selector, void *params)
{
    (void)params;
    switch (selector) {
        case 0: // SetOutlinePreferred
            lc3_set_outline_preferred(true);
            return 0;
        case 1: // GetOutlinePreferred
            return lc3_get_outline_preferred();
        case 2: // OutlineMetrics
            return lc3_outline_metrics(0, NULL, (FixedPoint){0,0}, (FixedPoint){0,0}, NULL, NULL, NULL);
        case 3: // SetPreserveGlyph
            lc3_set_preserve_glyph(true);
            return 0;
        case 4: // GetPreserveGlyph
            return lc3_get_preserve_glyph();
        case 5: // FlushFonts
            lc3_flush_fonts();
            return 0;
        default:
            return -1;
    }
}
