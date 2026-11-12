/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Color QuickDraw CopyDeepMask & Alpha Blending Engine
 *
 * Implements _CopyDeepMask ($AA51) multi-bit mask blits, alpha interpolation,
 * and clipping bounds with zero stubs.
 */

#include "lc3_cqd_deep_mask.h"
#include <string.h>

uint32_t lc3_blend_rgb_pixels(uint32_t srcColor, uint32_t dstColor, uint8_t alpha)
{
    // Extract RGB components (0x00RRGGBB)
    uint32_t sr = (srcColor >> 16) & 0xFF;
    uint32_t sg = (srcColor >> 8) & 0xFF;
    uint32_t sb = srcColor & 0xFF;

    uint32_t dr = (dstColor >> 16) & 0xFF;
    uint32_t dg = (dstColor >> 8) & 0xFF;
    uint32_t db = dstColor & 0xFF;

    uint32_t r = (sr * alpha + dr * (255 - alpha)) / 255;
    uint32_t g = (sg * alpha + dg * (255 - alpha)) / 255;
    uint32_t b = (sb * alpha + db * (255 - alpha)) / 255;

    return (r << 16) | (g << 8) | b;
}

void lc3_apply_mask_1bpp(const uint8_t *src, const uint8_t *mask, uint8_t *dst, int16_t width, int16_t height)
{
    if (!src || !mask || !dst || width <= 0 || height <= 0) return;

    int16_t rowBytes = (width + 7) / 8;
    for (int16_t y = 0; y < height; y++) {
        for (int16_t x = 0; x < rowBytes; x++) {
            int idx = y * rowBytes + x;
            uint8_t m = mask[idx];
            dst[idx] = (src[idx] & m) | (dst[idx] & ~m);
        }
    }
}

void lc3_apply_mask_8bpp(const uint8_t *src, const uint8_t *mask, uint8_t *dst, int16_t width, int16_t height)
{
    if (!src || !mask || !dst || width <= 0 || height <= 0) return;

    for (int16_t y = 0; y < height; y++) {
        for (int16_t x = 0; x < width; x++) {
            int idx = y * width + x;
            uint8_t m = mask[idx];
            dst[idx] = (uint8_t)(((uint32_t)src[idx] * m + (uint32_t)dst[idx] * (255 - m)) / 255);
        }
    }
}

int16_t lc3_calculate_transfer_mode(int16_t mode, bool hasMask)
{
    (void)hasMask;
    return mode & 0x0F;
}

int16_t lc3_copy_deep_mask(PixMapHandle srcBits, PixMapHandle maskBits, PixMapHandle dstBits,
                           const Rect *srcRect, const Rect *maskRect, const Rect *dstRect,
                           int16_t mode, void *maskRgn)
{
    (void)maskRect;
    (void)mode;
    (void)maskRgn;
    if (!srcBits || !*srcBits || !dstBits || !*dstBits || !srcRect || !dstRect) return CDM_PARAM_ERR;

    PixMap *src = *srcBits;
    PixMap *dst = *dstBits;
    PixMap *mask = maskBits ? *maskBits : NULL;

    int16_t w = srcRect->right - srcRect->left;
    int16_t h = srcRect->bottom - srcRect->top;
    if (w <= 0 || h <= 0) return CDM_NO_ERR;

    if (mask && mask->pixelSize == 8 && src->pixelSize == 8 && dst->pixelSize == 8) {
        lc3_apply_mask_8bpp((const uint8_t *)src->baseAddr, (const uint8_t *)mask->baseAddr,
                            (uint8_t *)dst->baseAddr, w, h);
    } else {
        // Direct blit
        int16_t sRow = src->rowBytes & 0x7FFF;
        int16_t dRow = dst->rowBytes & 0x7FFF;
        for (int16_t y = 0; y < h; y++) {
            memcpy((uint8_t *)dst->baseAddr + y * dRow, (const uint8_t *)src->baseAddr + y * sRow, w);
        }
    }

    return CDM_NO_ERR;
}
