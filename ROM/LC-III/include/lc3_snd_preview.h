/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Sound Manager 3.0 Standard File Sound Preview & Header Parsing
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_SND_PREVIEW_H
#define LC3_SND_PREVIEW_H

#include <stdint.h>
#include <stdbool.h>

#define SND_NO_ERR         0
#define SND_BAD_FORMAT    -1
#define SND_PARAM_ERR    -50

typedef struct SampledSoundHeader {
    void *samplePtr;
    uint32_t length;
    uint32_t sampleRate;      /* 16.16 fixed-point */
    uint32_t loopStart;
    uint32_t loopEnd;
    uint8_t encode;           /* 0 = standard PCM */
    uint8_t baseFrequency;
} SampledSoundHeader;

typedef struct ExtendedSoundHeader {
    void *samplePtr;
    uint32_t numChannels;
    uint32_t sampleRate;
    uint32_t loopStart;
    uint32_t loopEnd;
    uint8_t encode;
    uint8_t baseFrequency;
    uint32_t numFrames;
    uint8_t aiffSampleRate[10];
    void *markerChunk;
    void *instrumentChunks;
    void *aesRecording;
    uint16_t sampleSize;      /* 8 or 16 bits */
} ExtendedSoundHeader;

typedef struct CompressedSoundHeader {
    void *samplePtr;
    uint32_t numChannels;
    uint32_t sampleRate;
    uint32_t loopStart;
    uint32_t loopEnd;
    uint8_t encode;           /* -1 = compressed */
    uint8_t baseFrequency;
    uint32_t numFrames;
    uint8_t aiffSampleRate[10];
    void *markerChunk;
    uint32_t format;          /* 'MAC3', 'MAC6' */
} CompressedSoundHeader;

/* Sound Preview API */
int16_t lc3_snd_parse_resource(const void *sndResource, uint32_t resSize, uint16_t *synthType, uint16_t *numCommands);
int16_t lc3_snd_parse_sound_header(const void *headerPtr, SampledSoundHeader *outHdr);
int16_t lc3_snd_parse_extended_header(const void *headerPtr, ExtendedSoundHeader *outHdr);
int16_t lc3_snd_parse_compressed_header(const void *headerPtr, CompressedSoundHeader *outHdr);
uint32_t lc3_snd_calc_duration(uint32_t sampleCount, uint32_t sampleRate16_16);
int16_t lc3_snd_preview_modal(const void *sndResource, uint32_t resSize);

#endif /* LC3_SND_PREVIEW_H */
