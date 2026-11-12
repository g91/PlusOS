/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Sound Manager 3.0 Double-Buffering & DMA Audio Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_SND_DOUBLE_BUF_H
#define LC3_SND_DOUBLE_BUF_H

#include <stdint.h>
#include <stdbool.h>

#define SND_DB_NO_ERR         0
#define SND_DB_PARAM_ERR    -50
#define SND_DB_NOT_READY    -51
#define SND_DB_BUSY         -52

#define SND_DB_BUFFER_READY  0x0001
#define SND_DB_LAST_BUFFER   0x0002

typedef struct SndDoubleBuffer {
    uint32_t dbNumFrames;
    uint32_t dbFlags;
    uint32_t dbReserved[2];
    uint8_t  *dbSoundData;
} SndDoubleBuffer, *SndDoubleBufferPtr;

typedef void (*SndDoubleBackProc)(void *chan, SndDoubleBufferPtr doubleBuffer);

typedef struct SndDoubleBufferHeader {
    int16_t  dbhNumChannels;
    int16_t  dbhSampleSize;
    int32_t  dbhCompressionID;
    int32_t  dbhPacketSize;
    uint32_t dbhSampleRate;
    SndDoubleBufferPtr dbhBufferPtr[2];
    SndDoubleBackProc  dbhDoubleBack;
} SndDoubleBufferHeader, *SndDoubleBufferHeaderPtr;

typedef struct SndDoubleBufferContext {
    SndDoubleBufferHeaderPtr header;
    bool isPlaying;
    bool isPaused;
    uint8_t currentBufferIdx;
    uint32_t currentFrameOffset;
    uint32_t totalFramesRendered;
} SndDoubleBufferContext;

/* Public API */
int16_t lc3_snd_init_double_buffer(SndDoubleBufferContext *ctx, SndDoubleBufferHeaderPtr header);
int16_t lc3_snd_play_double_buffer(SndDoubleBufferContext *ctx);
int16_t lc3_snd_double_back_callback(SndDoubleBufferContext *ctx, uint8_t bufferIdx);
int16_t lc3_snd_stop_double_buffer(SndDoubleBufferContext *ctx);
int16_t lc3_snd_pause_double_buffer(SndDoubleBufferContext *ctx);
int16_t lc3_snd_resume_double_buffer(SndDoubleBufferContext *ctx);

#endif /* LC3_SND_DOUBLE_BUF_H */
