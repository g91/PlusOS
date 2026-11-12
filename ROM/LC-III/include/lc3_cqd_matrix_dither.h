/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Color QuickDraw Custom Gray Scale & Dithering Matrix Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_CQD_MATRIX_DITHER_H
#define LC3_CQD_MATRIX_DITHER_H

#include <stdint.h>
#include <stdbool.h>

#define DITHER_NO_ERR         0
#define DITHER_PARAM_ERR    -50

typedef struct RGBColor {
    uint16_t red;
    uint16_t green;
    uint16_t blue;
} RGBColor;

/* Dithering Matrix API */
uint8_t lc3_dither_get_bayer_threshold(int16_t x, int16_t y);
uint8_t lc3_dither_rgb_to_gray4(const RGBColor *rgb, int16_t x, int16_t y);
uint8_t lc3_dither_color_8bpp(const RGBColor *rgb, int16_t x, int16_t y);
void lc3_dither_floyd_steinberg_line(const RGBColor *srcLine, uint8_t *dstLine, int16_t width, int16_t *errorBuf);
uint8_t lc3_dither_quantize_component(uint16_t val, uint8_t threshold);
int16_t lc3_dither_init_tables(void);

#endif /* LC3_CQD_MATRIX_DITHER_H */
