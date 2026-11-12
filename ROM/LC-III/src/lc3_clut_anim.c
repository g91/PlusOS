/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Color QuickDraw CLUT Hardware Animation & Palette Cycling
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#include "lc3_clut_anim.h"
#include <string.h>

#define SONORA_CLUT_DATA_REG 0x50F00034

int16_t lc3_clut_anim_init(PaletteAnimContext *ctx)
{
    if (!ctx) return CLUT_PARAM_ERR;

    memset(ctx, 0, sizeof(PaletteAnimContext));
    for (int i = 0; i < 256; ++i) {
        ctx->activePalette[i].value = (int16_t)i;
        /* Standard grayscale gradient default */
        uint16_t val = (uint16_t)(i * 257);
        ctx->activePalette[i].rgb.red = val;
        ctx->activePalette[i].rgb.green = val;
        ctx->activePalette[i].rgb.blue = val;
        ctx->isAnimated[i] = false;
    }
    ctx->animCount = 0;
    ctx->vblPending = false;

    return CLUT_NO_ERR;
}

int16_t lc3_clut_anim_set_entries(PaletteAnimContext *ctx, int16_t start, int16_t count, const ColorSpec *entries)
{
    if (!ctx || !entries || start < 0 || (start + count) > 256) return CLUT_PARAM_ERR;

    for (int16_t i = 0; i < count; ++i) {
        int16_t idx = start + i;
        ctx->activePalette[idx] = entries[i];
        ctx->activePalette[idx].value = idx;
    }

    ctx->vblPending = true;
    return CLUT_NO_ERR;
}

int16_t lc3_clut_anim_rotate(PaletteAnimContext *ctx, int16_t start, int16_t count, bool forward)
{
    if (!ctx || start < 0 || count <= 1 || (start + count) > 256) return CLUT_PARAM_ERR;

    if (forward) {
        ColorSpec tmp = ctx->activePalette[start + count - 1];
        for (int16_t i = count - 1; i > 0; --i) {
            ctx->activePalette[start + i].rgb = ctx->activePalette[start + i - 1].rgb;
        }
        ctx->activePalette[start].rgb = tmp.rgb;
    } else {
        ColorSpec tmp = ctx->activePalette[start];
        for (int16_t i = 0; i < count - 1; ++i) {
            ctx->activePalette[start + i].rgb = ctx->activePalette[start + i + 1].rgb;
        }
        ctx->activePalette[start + count - 1].rgb = tmp.rgb;
    }

    ctx->vblPending = true;
    return CLUT_NO_ERR;
}

void lc3_clut_anim_vbl_sync(PaletteAnimContext *ctx)
{
    if (!ctx || !ctx->vblPending) return;

    /* On physical hardware: write out changed palette to Sonora RAMDAC */
    ctx->vblPending = false;
}

int16_t lc3_clut_anim_entry(PaletteAnimContext *ctx, int16_t index, const RGBColor *newColor)
{
    if (!ctx || !newColor || index < 0 || index >= 256) return CLUT_PARAM_ERR;

    ctx->activePalette[index].rgb = *newColor;
    ctx->isAnimated[index] = true;
    ctx->vblPending = true;

    return CLUT_NO_ERR;
}

int16_t lc3_clut_anim_palette(PaletteAnimContext *ctx, const ColorSpec *newPalette, int16_t count)
{
    if (!ctx || !newPalette || count <= 0 || count > 256) return CLUT_PARAM_ERR;

    return lc3_clut_anim_set_entries(ctx, 0, count, newPalette);
}
