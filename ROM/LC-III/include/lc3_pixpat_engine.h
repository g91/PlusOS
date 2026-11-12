/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Color QuickDraw Pixel Pattern & Dither Pattern Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_PIXPAT_ENGINE_H
#define LC3_PIXPAT_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

#define PIXPAT_NO_ERR         0
#define PIXPAT_PARAM_ERR    -50
#define PIXPAT_MEM_FULL    -108

#define PAT_TYPE_BW           0
#define PAT_TYPE_COLOR        1
#define PAT_TYPE_RGB_DITHER   2

typedef struct RGBColor {
    uint16_t red;
    uint16_t green;
    uint16_t blue;
} RGBColor;

typedef struct MacRect {
    int16_t top;
    int16_t left;
    int16_t bottom;
    int16_t right;
} MacRect;

typedef struct PixPatRecord {
    int16_t patType;             /* 0 = 1-bpp pattern, 1 = PixMap pattern, 2 = RGB dither */
    void *patMap;                /* PixMap handle */
    void *patData;               /* Pixel data handle */
    void *patXData;              /* Expanded pattern data */
    int16_t patXValid;
    int32_t patXMap;
    uint8_t pat1Data[8];         /* 8x8 1-bpp fallback pattern */
    uint8_t patternData8x8[64];  /* 8x8 8-bpp expanded pattern */
} PixPatRecord, *PixPatPtr, **PixPatHandle;

/* Public API */
PixPatHandle lc3_pixpat_new(void);
int16_t lc3_pixpat_copy(PixPatHandle src, PixPatHandle dst);
int16_t lc3_pixpat_make_rgb_pat(PixPatHandle pp, const RGBColor *myColor);
int16_t lc3_pixpat_fill_rect(const MacRect *r, PixPatHandle pp, uint8_t *fb, int16_t rowBytes, int16_t depth);
int16_t lc3_pixpat_fill_oval(const MacRect *r, PixPatHandle pp, uint8_t *fb, int16_t rowBytes, int16_t depth);
int16_t lc3_pixpat_dispose(PixPatHandle pp);

#endif /* LC3_PIXPAT_ENGINE_H */
