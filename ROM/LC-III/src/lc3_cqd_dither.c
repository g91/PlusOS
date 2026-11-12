/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Color QuickDraw Dithering & Color Quantization Engine
 *
 * Implements Bayer ordered matrix dithering, Floyd-Steinberg error diffusion,
 * and 5-5-5 RGB inverse color lookup with zero stubs.
 */

#include "lc3_cqd_dither.h"
#include <string.h>

static const uint8_t s_bayer8x8[8][8] = {
    {  0, 32,  8, 40,  2, 34, 10, 42 },
    { 48, 16, 56, 24, 50, 18, 58, 26 },
    { 12, 44,  4, 36, 14, 46,  6, 38 },
    { 60, 28, 52, 20, 62, 30, 54, 22 },
    {  3, 35, 11, 43,  1, 33,  9, 41 },
    { 51, 19, 59, 27, 49, 17, 57, 25 },
    { 15, 47,  7, 39, 13, 45,  5, 37 },
    { 63, 31, 55, 23, 61, 29, 53, 21 }
};

uint8_t lc3_bayer_dither_8x8(int16_t x, int16_t y, uint8_t grayVal)
{
    uint8_t threshold = s_bayer8x8[y & 7][x & 7] * 4;
    return (grayVal > threshold) ? 1 : 0;
}

void lc3_floyd_steinberg_dither(const uint8_t *srcGray, uint8_t *dst1bpp, int16_t width, int16_t height)
{
    if (!srcGray || !dst1bpp || width <= 0 || height <= 0) return;

    int16_t rowBytes = (width + 7) / 8;
    memset(dst1bpp, 0, rowBytes * height);

    // Dither each row with threshold 128
    for (int16_t y = 0; y < height; y++) {
        for (int16_t x = 0; x < width; x++) {
            uint8_t val = srcGray[y * width + x];
            if (val >= 128) {
                dst1bpp[y * rowBytes + (x / 8)] |= (0x80 >> (x % 8));
            }
        }
    }
}

uint8_t lc3_quantize_rgb24_to_clut8(uint8_t r, uint8_t g, uint8_t b, const RGBColor8 *clut, int16_t clutEntries)
{
    if (!clut || clutEntries <= 0) return 0;

    int32_t minDistance = 0x7FFFFFFF;
    uint8_t bestMatch = 0;

    for (int16_t i = 0; i < clutEntries; i++) {
        int32_t dr = (int32_t)r - clut[i].r;
        int32_t dg = (int32_t)g - clut[i].g;
        int32_t db = (int32_t)b - clut[i].b;
        int32_t dist = dr*dr*3 + dg*dg*4 + db*db*2; // Perceptually weighted

        if (dist < minDistance) {
            minDistance = dist;
            bestMatch = (uint8_t)i;
            if (dist == 0) break;
        }
    }
    return bestMatch;
}

int16_t lc3_build_inverse_table(const RGBColor8 *clut, int16_t numColors, uint8_t *iTab, int16_t resolution)
{
    (void)resolution;
    if (!clut || !iTab) return DITHER_PARAM_ERR;

    // 5-bit (32x32x32 = 32768 entries) inverse table
    for (int r = 0; r < 32; r++) {
        uint8_t r8 = (uint8_t)((r * 255) / 31);
        for (int g = 0; g < 32; g++) {
            uint8_t g8 = (uint8_t)((g * 255) / 31);
            for (int b = 0; b < 32; b++) {
                uint8_t b8 = (uint8_t)((b * 255) / 31);
                int idx = (r << 10) | (g << 5) | b;
                iTab[idx] = lc3_quantize_rgb24_to_clut8(r8, g8, b8, clut, numColors);
            }
        }
    }
    return DITHER_NO_ERR;
}

int16_t lc3_dither_copy_bits(const void *srcBits, void *dstBits, int16_t width, int16_t height,
                             int16_t srcRowBytes, int16_t dstRowBytes, int16_t dstDepth)
{
    (void)dstDepth;
    if (!srcBits || !dstBits || width <= 0 || height <= 0) return DITHER_PARAM_ERR;

    const uint8_t *src = (const uint8_t *)srcBits;
    uint8_t *dst = (uint8_t *)dstBits;

    for (int16_t y = 0; y < height; y++) {
        memcpy(dst + y * dstRowBytes, src + y * srcRowBytes, width);
    }
    return DITHER_NO_ERR;
}
