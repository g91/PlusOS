/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Color QuickDraw Palette Tolerance & Color Arbitration Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_PALETTE_TOLERANCE_H
#define LC3_PALETTE_TOLERANCE_H

#include <stdint.h>
#include <stdbool.h>

#define PAL_NO_ERR         0
#define PAL_NO_MATCH_ERR  -1
#define PAL_PARAM_ERR    -50

#define pmCourteous   0x0000
#define pmTolerant    0x0002
#define pmAnimated    0x0004
#define pmExplicit    0x0008

typedef struct RGBColor {
    uint16_t red;
    uint16_t green;
    uint16_t blue;
} RGBColor;

typedef struct PaletteEntry {
    RGBColor ciRGB;
    int16_t ciUsage;
    int16_t ciTolerance;
    int16_t ciDataFields[3];
} PaletteEntry;

/* Palette Tolerance API */
uint32_t lc3_pal_calc_color_distance(const RGBColor *c1, const RGBColor *c2);
int16_t lc3_pal_find_tolerant_match(const RGBColor *target, const RGBColor *clutEntries, uint16_t clutCount, int16_t tolerance, int16_t *matchedIndex);
int16_t lc3_pal_reserve_entry(int16_t clutIndex);
int16_t lc3_pal_release_entry(int16_t clutIndex);
int16_t lc3_pal_build_mapping(const PaletteEntry *pal, uint16_t palCount, const RGBColor *clutEntries, uint16_t clutCount, int16_t *outMap);
bool lc3_pal_check_inhibit(uint16_t inhibitFlags, const RGBColor *c);

#endif /* LC3_PALETTE_TOLERANCE_H */
