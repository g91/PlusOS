/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Color QuickDraw Dithering & Color Quantization Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_CQD_DITHER_H
#define LC3_CQD_DITHER_H

#include <stdint.h>
#include <stdbool.h>

#define DITHER_NO_ERR         0
#define DITHER_PARAM_ERR    -50

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} RGBColor8;

/* Dithering API */
int16_t lc3_dither_copy_bits(const void *srcBits, void *dstBits, int16_t width, int16_t height,
                             int16_t srcRowBytes, int16_t dstRowBytes, int16_t dstDepth);
uint8_t lc3_bayer_dither_8x8(int16_t x, int16_t y, uint8_t grayVal);
void lc3_floyd_steinberg_dither(const uint8_t *srcGray, uint8_t *dst1bpp, int16_t width, int16_t height);
uint8_t lc3_quantize_rgb24_to_clut8(uint8_t r, uint8_t g, uint8_t b, const RGBColor8 *clut, int16_t clutEntries);
int16_t lc3_build_inverse_table(const RGBColor8 *clut, int16_t numColors, uint8_t *iTab, int16_t resolution);

#endif /* LC3_CQD_DITHER_H */
