/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Color QuickDraw Geometric Regions Core Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_REGION_ENGINE_H
#define LC3_REGION_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int16_t v;
    int16_t h;
} Point;

typedef struct {
    int16_t top;
    int16_t left;
    int16_t bottom;
    int16_t right;
} Rect;

typedef struct Region {
    int16_t rgnSize;
    Rect rgnBBox;
    // In complex regions, scanline inversions follow
} Region, *RegionPtr, **RgnHandle;

/* QuickDraw Region API */
RgnHandle lc3_new_rgn(void);
void lc3_dispose_rgn(RgnHandle rgn);
void lc3_open_rgn(void);
void lc3_close_rgn(RgnHandle dstRgn);
void lc3_copy_rgn(RgnHandle srcRgn, RgnHandle dstRgn);
void lc3_set_empty_rgn(RgnHandle rgn);
void lc3_set_rect_rgn(RgnHandle rgn, int16_t left, int16_t top, int16_t right, int16_t bottom);
void lc3_rect_rgn(RgnHandle rgn, const Rect *r);
void lc3_offset_rgn(RgnHandle rgn, int16_t dh, int16_t dv);
void lc3_inset_rgn(RgnHandle rgn, int16_t dh, int16_t dv);
void lc3_sect_rgn(RgnHandle srcRgnA, RgnHandle srcRgnB, RgnHandle dstRgn);
void lc3_union_rgn(RgnHandle srcRgnA, RgnHandle srcRgnB, RgnHandle dstRgn);
void lc3_diff_rgn(RgnHandle srcRgnA, RgnHandle srcRgnB, RgnHandle dstRgn);
void lc3_xor_rgn(RgnHandle srcRgnA, RgnHandle srcRgnB, RgnHandle dstRgn);
bool lc3_pt_in_rgn(Point pt, RgnHandle rgn);
bool lc3_rect_in_rgn(const Rect *r, RgnHandle rgn);
bool lc3_equal_rgn(RgnHandle rgnA, RgnHandle rgnB);
bool lc3_empty_rgn(RgnHandle rgn);

#endif /* LC3_REGION_ENGINE_H */
