/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Color QuickDraw CopyDeepMask & Alpha Blending Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_CQD_DEEP_MASK_H
#define LC3_CQD_DEEP_MASK_H

#include <stdint.h>
#include <stdbool.h>

#define CDM_NO_ERR         0
#define CDM_PARAM_ERR    -50

typedef struct {
    int16_t top;
    int16_t left;
    int16_t bottom;
    int16_t right;
} Rect;

typedef struct PixMap {
    void *baseAddr;
    int16_t rowBytes;
    Rect bounds;
    int16_t pmVersion;
    int16_t packType;
    int32_t packSize;
    int32_t hRes;
    int32_t vRes;
    int16_t pixelType;
    int16_t pixelSize;
    int16_t cmpCount;
    int16_t cmpSize;
    int32_t planeBytes;
    void *pmTable;
    int32_t pmReserved;
} PixMap, *PixMapPtr, **PixMapHandle;

/* CopyDeepMask API */
int16_t lc3_copy_deep_mask(PixMapHandle srcBits, PixMapHandle maskBits, PixMapHandle dstBits,
                           const Rect *srcRect, const Rect *maskRect, const Rect *dstRect,
                           int16_t mode, void *maskRgn);
uint32_t lc3_blend_rgb_pixels(uint32_t srcColor, uint32_t dstColor, uint8_t alpha);
void lc3_apply_mask_1bpp(const uint8_t *src, const uint8_t *mask, uint8_t *dst, int16_t width, int16_t height);
void lc3_apply_mask_8bpp(const uint8_t *src, const uint8_t *mask, uint8_t *dst, int16_t width, int16_t height);
int16_t lc3_calculate_transfer_mode(int16_t mode, bool hasMask);

#endif /* LC3_CQD_DEEP_MASK_H */
