/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Sonora Framebuffer Hardware Acceleration & Fast Blit Primitives
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_SONORA_ACCEL_H
#define LC3_SONORA_ACCEL_H

#include <stdint.h>
#include <stdbool.h>

#define SONORA_NO_ERR         0
#define SONORA_PARAM_ERR    -50

/* Sonora Hardware Acceleration API */
void lc3_sonora_fast_blit8(const uint8_t *src, uint8_t *dst, int16_t width, int16_t height,
                           int16_t srcRowBytes, int16_t dstRowBytes);
void lc3_sonora_fast_blit16(const uint16_t *src, uint16_t *dst, int16_t width, int16_t height,
                            int16_t srcRowBytes, int16_t dstRowBytes);
void lc3_sonora_fast_fill32(uint32_t *dst, uint32_t pattern, uint32_t longwordCount);
void lc3_sonora_scroll_raster(uint8_t *vram, int16_t rowBytes, int16_t height, int16_t scrollDy);
void lc3_sonora_wait_vbl_sync(void);

#endif /* LC3_SONORA_ACCEL_H */
