/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Sound Manager 3.0 Rate Multiplier & Resampler Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_SOUND_RESAMPLE_H
#define LC3_SOUND_RESAMPLE_H

#include <stdint.h>
#include <stdbool.h>

#define RESAMPLE_NO_ERR         0
#define RESAMPLE_PARAM_ERR    -50

typedef struct SoundResampleContext {
    uint32_t srcRate;         /* e.g. 11025, 22050 */
    uint32_t dstRate;         /* e.g. 22254 on LC III */
    uint32_t step16_16;       /* (srcRate << 16) / dstRate */
    uint32_t accumulator;     /* Fixed point 16.16 index */
    uint8_t channels;         /* 1 = mono, 2 = stereo */
    uint8_t bitDepth;         /* 8 or 16 */
} SoundResampleContext;

/* Sound Resampling API */
int16_t lc3_resample_init(SoundResampleContext *ctx, uint32_t srcRate, uint32_t dstRate, uint8_t channels, uint8_t bitDepth);
int16_t lc3_resample_linear8(SoundResampleContext *ctx, const uint8_t *src, uint32_t srcFrames, uint8_t *dst, uint32_t *dstFrames);
int16_t lc3_resample_linear16(SoundResampleContext *ctx, const int16_t *src, uint32_t srcFrames, int16_t *dst, uint32_t *dstFrames);
int16_t lc3_resample_drop_sample(SoundResampleContext *ctx, const uint8_t *src, uint32_t srcFrames, uint8_t *dst, uint32_t *dstFrames);
int16_t lc3_resample_polyphase(SoundResampleContext *ctx, const uint8_t *src, uint32_t srcFrames, uint8_t *dst, uint32_t *dstFrames);
int16_t lc3_resample_process_buffer(SoundResampleContext *ctx, const void *srcBuf, uint32_t srcBytes, void *dstBuf, uint32_t *dstBytes);

#endif /* LC3_SOUND_RESAMPLE_H */
