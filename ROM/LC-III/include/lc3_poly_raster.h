/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: 32-Bit QuickDraw Polygon & Region Rasterizer
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_POLY_RASTER_H
#define LC3_POLY_RASTER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int16_t v;
    int16_t h;
} QDRPoint;

typedef struct {
    int16_t top;
    int16_t left;
    int16_t bottom;
    int16_t right;
} QDRRect;

typedef struct {
    int16_t polySize;
    QDRRect polyBBox;
    QDRPoint polyPoints[64];
} Polygon, *PolyPtr, **PolyHandle;

/* QuickDraw Polygon API */
PolyHandle lc3_open_poly(void);
void lc3_close_poly(void);
void lc3_kill_poly(PolyHandle poly);
void lc3_offset_poly(PolyHandle poly, int16_t dh, int16_t dv);
void lc3_std_poly(int16_t verb, PolyHandle poly);
void lc3_frame_poly(PolyHandle poly);
void lc3_paint_poly(PolyHandle poly);
void lc3_erase_poly(PolyHandle poly);
void lc3_invert_poly(PolyHandle poly);
void lc3_fill_poly(PolyHandle poly, const void *pattern);

#endif /* LC3_POLY_RASTER_H */
