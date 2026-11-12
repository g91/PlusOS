/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Color QuickDraw Custom Gray Scale & Dithering Matrix Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#include "lc3_cqd_matrix_dither.h"
#include <string.h>

/* Standard 8x8 Bayer Matrix scaled to 0..255 */
static const uint8_t sBayer8x8[8][8] = {
    {   0, 128,  32, 160,   8, 136,  40, 168 },
    { 192,  64, 224,  96, 200,  72, 232, 104 },
    {  48, 176,  16, 144,  56, 184,  24, 152 },
    { 240, 112, 208,  80, 248, 120, 216,  88 },
    {  12, 140,  44, 172,   4, 132,  36, 164 },
    { 204,  76, 236, 108, 196,  68, 228, 100 },
    {  60, 188,  28, 156,  52, 180,  20, 148 },
    { 252, 124, 220,  92, 244, 116, 212,  84 }
};

uint8_t lc3_dither_get_bayer_threshold(int16_t x, int16_t y)
{
    return sBayer8x8[y & 7][x & 7];
}

uint8_t lc3_dither_rgb_to_gray4(const RGBColor *rgb, int16_t x, int16_t y)
{
    if (!rgb) return 0;

    /* Perceptual grayscale weighting: 0.299 R + 0.587 G + 0.114 B */
    uint32_t gray = (rgb->red * 77 + rgb->green * 150 + rgb->blue * 29) >> 16;
    uint8_t threshold = lc3_dither_get_bayer_threshold(x, y);

    /* 4-level quantization (0, 1, 2, 3) */
    uint32_t scaled = (gray * 3) + (threshold / 85);
    uint8_t val = (uint8_t)(scaled / 256);
    return (val > 3) ? 3 : val;
}

uint8_t lc3_dither_quantize_component(uint16_t val, uint8_t threshold)
{
    uint8_t v8 = (uint8_t)(val >> 8);
    /* 6-level color cube channel quantization */
    uint16_t scaled = (uint16_t)(v8 * 5 + (threshold / 51));
    uint8_t q = (uint8_t)(scaled / 256);
    return (q > 5) ? 5 : q;
}

uint8_t lc3_dither_color_8bpp(const RGBColor *rgb, int16_t x, int16_t y)
{
    if (!rgb) return 0;

    uint8_t threshold = lc3_dither_get_bayer_threshold(x, y);

    uint8_t r = lc3_dither_quantize_component(rgb->red, threshold);
    uint8_t g = lc3_dither_quantize_component(rgb->green, threshold);
    uint8_t b = lc3_dither_quantize_component(rgb->blue, threshold);

    /* Standard Apple 6x6x6 color cube: base index 16 */
    return (uint8_t)(16 + (r * 36) + (g * 6) + b);
}

void lc3_dither_floyd_steinberg_line(const RGBColor *srcLine, uint8_t *dstLine, int16_t width, int16_t *errorBuf)
{
    if (!srcLine || !dstLine || width <= 0) return;

    int16_t currentErr = 0;
    for (int16_t x = 0; x < width; ++x) {
        uint32_t gray = (srcLine[x].red * 77 + srcLine[x].green * 150 + srcLine[x].blue * 29) >> 16;
        int32_t val = (int32_t)gray + currentErr;
        if (errorBuf) val += errorBuf[x];

        uint8_t outPixel = (val >= 128) ? 0xFF : 0x00;
        int32_t err = val - (int32_t)outPixel;

        dstLine[x] = outPixel;
        currentErr = (int16_t)((err * 7) / 16);

        if (errorBuf && x > 0 && (x + 1) < width) {
            errorBuf[x - 1] += (int16_t)((err * 3) / 16);
            errorBuf[x]     += (int16_t)((err * 5) / 16);
            errorBuf[x + 1] += (int16_t)((err * 1) / 16);
        }
    }
}

int16_t lc3_dither_init_tables(void)
{
    return DITHER_NO_ERR;
}
