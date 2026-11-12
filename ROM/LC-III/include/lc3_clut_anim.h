/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Color QuickDraw CLUT Hardware Animation & Palette Cycling
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_CLUT_ANIM_H
#define LC3_CLUT_ANIM_H

#include <stdint.h>
#include <stdbool.h>

#define CLUT_NO_ERR         0
#define CLUT_PARAM_ERR    -50

typedef struct RGBColor {
    uint16_t red;
    uint16_t green;
    uint16_t blue;
} RGBColor;

typedef struct ColorSpec {
    int16_t value;            /* Palette index */
    RGBColor rgb;
} ColorSpec;

typedef struct PaletteAnimContext {
    ColorSpec activePalette[256];
    bool isAnimated[256];
    uint16_t animCount;
    bool vblPending;
} PaletteAnimContext;

/* CLUT Animation API */
int16_t lc3_clut_anim_init(PaletteAnimContext *ctx);
int16_t lc3_clut_anim_set_entries(PaletteAnimContext *ctx, int16_t start, int16_t count, const ColorSpec *entries);
int16_t lc3_clut_anim_rotate(PaletteAnimContext *ctx, int16_t start, int16_t count, bool forward);
void lc3_clut_anim_vbl_sync(PaletteAnimContext *ctx);
int16_t lc3_clut_anim_entry(PaletteAnimContext *ctx, int16_t index, const RGBColor *newColor);
int16_t lc3_clut_anim_palette(PaletteAnimContext *ctx, const ColorSpec *newPalette, int16_t count);

#endif /* LC3_CLUT_ANIM_H */
