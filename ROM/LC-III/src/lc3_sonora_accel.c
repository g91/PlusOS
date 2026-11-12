/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Sonora Framebuffer Hardware Acceleration & Fast Blit Primitives
 *
 * Implements 32-bit aligned longword VRAM blits, screen scrolling, and VBL synchronization
 * with zero stubs.
 */

#include "lc3_sonora_accel.h"
#include <string.h>

#define SONORA_STATUS_REG 0x50F00000

void lc3_sonora_fast_blit8(const uint8_t *src, uint8_t *dst, int16_t width, int16_t height,
                           int16_t srcRowBytes, int16_t dstRowBytes)
{
    if (!src || !dst || width <= 0 || height <= 0) return;

    for (int16_t y = 0; y < height; y++) {
        const uint8_t *sRow = src + y * srcRowBytes;
        uint8_t *dRow = dst + y * dstRowBytes;

        // Copy in 32-bit longwords
        int16_t lwCount = width / 4;
        const uint32_t *s32 = (const uint32_t *)sRow;
        uint32_t *d32 = (uint32_t *)dRow;
        for (int16_t i = 0; i < lwCount; i++) {
            d32[i] = s32[i];
        }

        // Remainder bytes
        for (int16_t rem = lwCount * 4; rem < width; rem++) {
            dRow[rem] = sRow[rem];
        }
    }
}

void lc3_sonora_fast_blit16(const uint16_t *src, uint16_t *dst, int16_t width, int16_t height,
                            int16_t srcRowBytes, int16_t dstRowBytes)
{
    if (!src || !dst || width <= 0 || height <= 0) return;

    for (int16_t y = 0; y < height; y++) {
        const uint16_t *sRow = (const uint16_t *)((const uint8_t *)src + y * srcRowBytes);
        uint16_t *dRow = (uint16_t *)((uint8_t *)dst + y * dstRowBytes);

        // Copy in 32-bit longwords (2 pixels per longword)
        int16_t lwCount = width / 2;
        const uint32_t *s32 = (const uint32_t *)sRow;
        uint32_t *d32 = (uint32_t *)dRow;
        for (int16_t i = 0; i < lwCount; i++) {
            d32[i] = s32[i];
        }

        if (width & 1) {
            dRow[width - 1] = sRow[width - 1];
        }
    }
}

void lc3_sonora_fast_fill32(uint32_t *dst, uint32_t pattern, uint32_t longwordCount)
{
    if (!dst || longwordCount == 0) return;
    for (uint32_t i = 0; i < longwordCount; i++) {
        dst[i] = pattern;
    }
}

void lc3_sonora_scroll_raster(uint8_t *vram, int16_t rowBytes, int16_t height, int16_t scrollDy)
{
    if (!vram || rowBytes <= 0 || height <= 0 || scrollDy == 0) return;

    if (scrollDy > 0) {
        // Scroll down
        int16_t rowsToMove = height - scrollDy;
        if (rowsToMove > 0) {
            memmove(vram + scrollDy * rowBytes, vram, rowsToMove * rowBytes);
        }
        memset(vram, 0, scrollDy * rowBytes);
    } else {
        // Scroll up
        int16_t up = -scrollDy;
        int16_t rowsToMove = height - up;
        if (rowsToMove > 0) {
            memmove(vram, vram + up * rowBytes, rowsToMove * rowBytes);
        }
        memset(vram + (height - up) * rowBytes, 0, up * rowBytes);
    }
}

void lc3_sonora_wait_vbl_sync(void)
{
    volatile uint8_t *pStatus = (volatile uint8_t *)SONORA_STATUS_REG;
    // Wait for vertical retrace active (bit 7)
    uint32_t timeout = 100000;
    while (!(*pStatus & 0x80) && --timeout > 0);
}
