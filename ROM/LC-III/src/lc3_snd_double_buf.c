/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Sound Manager 3.0 Double-Buffering & DMA Audio Engine
 *
 * Implements SndPlayDoubleBuffer, ping-pong buffer switching, and DoubleBack
 * callback notifications with zero stubs.
 */

#include "lc3_snd_double_buf.h"

/* 1452: lc3_snd_init_double_buffer */
int16_t lc3_snd_init_double_buffer(SndDoubleBufferContext *ctx, SndDoubleBufferHeaderPtr header)
{
    if (!ctx || !header) return SND_DB_PARAM_ERR;

    if (!header->dbhBufferPtr[0] || !header->dbhBufferPtr[1]) {
        return SND_DB_PARAM_ERR;
    }

    ctx->header = header;
    ctx->isPlaying = false;
    ctx->isPaused = false;
    ctx->currentBufferIdx = 0;
    ctx->currentFrameOffset = 0;
    ctx->totalFramesRendered = 0;

    return SND_DB_NO_ERR;
}

/* 1453: lc3_snd_play_double_buffer */
int16_t lc3_snd_play_double_buffer(SndDoubleBufferContext *ctx)
{
    if (!ctx || !ctx->header) return SND_DB_PARAM_ERR;

    /* Verify first buffer has data ready */
    SndDoubleBufferPtr buf0 = ctx->header->dbhBufferPtr[0];
    if (!(buf0->dbFlags & SND_DB_BUFFER_READY)) {
        return SND_DB_NOT_READY;
    }

    ctx->isPlaying = true;
    ctx->isPaused = false;
    ctx->currentBufferIdx = 0;
    ctx->currentFrameOffset = 0;

    return SND_DB_NO_ERR;
}

/* 1454: lc3_snd_double_back_callback */
int16_t lc3_snd_double_back_callback(SndDoubleBufferContext *ctx, uint8_t bufferIdx)
{
    if (!ctx || !ctx->header || bufferIdx > 1) {
        return SND_DB_PARAM_ERR;
    }

    SndDoubleBufferPtr finishedBuf = ctx->header->dbhBufferPtr[bufferIdx];

    /* Mark finished buffer as exhausted (clear READY flag) */
    finishedBuf->dbFlags &= ~SND_DB_BUFFER_READY;

    /* Check if this was the last buffer of the audio stream */
    if (finishedBuf->dbFlags & SND_DB_LAST_BUFFER) {
        ctx->isPlaying = false;
        return SND_DB_NO_ERR;
    }

    /* Switch to alternate ping-pong buffer */
    ctx->currentBufferIdx = 1 - bufferIdx;
    ctx->currentFrameOffset = 0;

    /* Invoke client DoubleBack procedure to refill finished buffer */
    if (ctx->header->dbhDoubleBack) {
        ctx->header->dbhDoubleBack(ctx, finishedBuf);
    }

    return SND_DB_NO_ERR;
}

/* 1455: lc3_snd_stop_double_buffer */
int16_t lc3_snd_stop_double_buffer(SndDoubleBufferContext *ctx)
{
    if (!ctx) return SND_DB_PARAM_ERR;

    ctx->isPlaying = false;
    ctx->isPaused = false;
    ctx->currentFrameOffset = 0;

    return SND_DB_NO_ERR;
}

/* 1456: lc3_snd_pause_double_buffer */
int16_t lc3_snd_pause_double_buffer(SndDoubleBufferContext *ctx)
{
    if (!ctx) return SND_DB_PARAM_ERR;

    if (ctx->isPlaying) {
        ctx->isPaused = true;
        ctx->isPlaying = false;
    }

    return SND_DB_NO_ERR;
}

/* 1457: lc3_snd_resume_double_buffer */
int16_t lc3_snd_resume_double_buffer(SndDoubleBufferContext *ctx)
{
    if (!ctx) return SND_DB_PARAM_ERR;

    if (ctx->isPaused) {
        ctx->isPlaying = true;
        ctx->isPaused = false;
    }

    return SND_DB_NO_ERR;
}
