/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Sound Manager 3.0 Rate Multiplier & Resampler Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#include "lc3_sound_resample.h"
#include <string.h>

int16_t lc3_resample_init(SoundResampleContext *ctx, uint32_t srcRate, uint32_t dstRate, uint8_t channels, uint8_t bitDepth)
{
    if (!ctx || srcRate == 0 || dstRate == 0 || (channels != 1 && channels != 2)) {
        return RESAMPLE_PARAM_ERR;
    }

    ctx->srcRate = srcRate;
    ctx->dstRate = dstRate;
    ctx->step16_16 = (uint32_t)(((uint64_t)srcRate << 16) / dstRate);
    ctx->accumulator = 0;
    ctx->channels = channels;
    ctx->bitDepth = (bitDepth == 16) ? 16 : 8;

    return RESAMPLE_NO_ERR;
}

int16_t lc3_resample_linear8(SoundResampleContext *ctx, const uint8_t *src, uint32_t srcFrames, uint8_t *dst, uint32_t *dstFrames)
{
    if (!ctx || !src || !dst || !dstFrames) return RESAMPLE_PARAM_ERR;

    uint32_t outCount = 0;
    uint32_t maxDst = *dstFrames;
    uint32_t step = ctx->step16_16;
    uint32_t pos = ctx->accumulator;

    if (srcFrames <= 1) {
        *dstFrames = 0;
        return RESAMPLE_NO_ERR;
    }

    while (outCount < maxDst) {
        uint32_t idx = pos >> 16;
        if (idx + 1 >= srcFrames) break;

        uint32_t frac = pos & 0xFFFF;
        int32_t s0 = (int32_t)src[idx];
        int32_t s1 = (int32_t)src[idx + 1];

        /* Linear interpolation: s0 + (frac * (s1 - s0)) >> 16 */
        int32_t val = s0 + (((s1 - s0) * (int32_t)frac) >> 16);
        if (val < 0) val = 0;
        if (val > 255) val = 255;

        dst[outCount++] = (uint8_t)val;
        pos += step;
    }

    ctx->accumulator = pos & 0xFFFF;
    *dstFrames = outCount;
    return RESAMPLE_NO_ERR;
}

int16_t lc3_resample_linear16(SoundResampleContext *ctx, const int16_t *src, uint32_t srcFrames, int16_t *dst, uint32_t *dstFrames)
{
    if (!ctx || !src || !dst || !dstFrames) return RESAMPLE_PARAM_ERR;

    uint32_t outCount = 0;
    uint32_t maxDst = *dstFrames;
    uint32_t step = ctx->step16_16;
    uint32_t pos = ctx->accumulator;

    if (srcFrames <= 1) {
        *dstFrames = 0;
        return RESAMPLE_NO_ERR;
    }

    while (outCount < maxDst) {
        uint32_t idx = pos >> 16;
        if (idx + 1 >= srcFrames) break;

        uint32_t frac = pos & 0xFFFF;
        int32_t s0 = (int32_t)src[idx];
        int32_t s1 = (int32_t)src[idx + 1];

        int32_t val = s0 + (((s1 - s0) * (int32_t)frac) >> 16);
        if (val < -32768) val = -32768;
        if (val > 32767) val = 32767;

        dst[outCount++] = (int16_t)val;
        pos += step;
    }

    ctx->accumulator = pos & 0xFFFF;
    *dstFrames = outCount;
    return RESAMPLE_NO_ERR;
}

int16_t lc3_resample_drop_sample(SoundResampleContext *ctx, const uint8_t *src, uint32_t srcFrames, uint8_t *dst, uint32_t *dstFrames)
{
    if (!ctx || !src || !dst || !dstFrames) return RESAMPLE_PARAM_ERR;

    uint32_t outCount = 0;
    uint32_t maxDst = *dstFrames;
    uint32_t step = ctx->step16_16;
    uint32_t pos = ctx->accumulator;

    while (outCount < maxDst) {
        uint32_t idx = pos >> 16;
        if (idx >= srcFrames) break;

        dst[outCount++] = src[idx];
        pos += step;
    }

    ctx->accumulator = pos & 0xFFFF;
    *dstFrames = outCount;
    return RESAMPLE_NO_ERR;
}

int16_t lc3_resample_polyphase(SoundResampleContext *ctx, const uint8_t *src, uint32_t srcFrames, uint8_t *dst, uint32_t *dstFrames)
{
    if (!ctx || !src || !dst || !dstFrames) return RESAMPLE_PARAM_ERR;

    uint32_t outCount = 0;
    uint32_t maxDst = *dstFrames;
    uint32_t step = ctx->step16_16;
    uint32_t pos = ctx->accumulator;

    if (srcFrames <= 2) {
        *dstFrames = 0;
        return RESAMPLE_NO_ERR;
    }

    while (outCount < maxDst) {
        uint32_t idx = pos >> 16;
        if (idx + 2 >= srcFrames) break;

        int32_t s0 = (int32_t)src[idx];
        int32_t s1 = (int32_t)src[idx + 1];
        int32_t s2 = (int32_t)src[idx + 2];

        /* 3-tap weighted average: (s0 + 2*s1 + s2) / 4 */
        int32_t val = (s0 + 2 * s1 + s2) >> 2;
        if (val < 0) val = 0;
        if (val > 255) val = 255;

        dst[outCount++] = (uint8_t)val;
        pos += step;
    }

    ctx->accumulator = pos & 0xFFFF;
    *dstFrames = outCount;
    return RESAMPLE_NO_ERR;
}

int16_t lc3_resample_process_buffer(SoundResampleContext *ctx, const void *srcBuf, uint32_t srcBytes, void *dstBuf, uint32_t *dstBytes)
{
    if (!ctx || !srcBuf || !dstBuf || !dstBytes) return RESAMPLE_PARAM_ERR;

    if (ctx->bitDepth == 8) {
        uint32_t srcFrames = srcBytes;
        uint32_t dstFrames = *dstBytes;
        int16_t err = lc3_resample_linear8(ctx, (const uint8_t *)srcBuf, srcFrames, (uint8_t *)dstBuf, &dstFrames);
        *dstBytes = dstFrames;
        return err;
    } else {
        uint32_t srcFrames = srcBytes / 2;
        uint32_t dstFrames = *dstBytes / 2;
        int16_t err = lc3_resample_linear16(ctx, (const int16_t *)srcBuf, srcFrames, (int16_t *)dstBuf, &dstFrames);
        *dstBytes = dstFrames * 2;
        return err;
    }
}
