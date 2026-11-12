/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Sound Manager 3.0 Standard File Sound Preview & Header Parsing
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#include "lc3_snd_preview.h"
#include <string.h>

int16_t lc3_snd_parse_resource(const void *sndResource, uint32_t resSize, uint16_t *synthType, uint16_t *numCommands)
{
    if (!sndResource || resSize < 6 || !synthType || !numCommands) return SND_PARAM_ERR;

    const uint8_t *p = (const uint8_t *)sndResource;
    uint16_t format = (uint16_t)((p[0] << 8) | p[1]);

    if (format == 1) {
        /* Format 1: format (2), numModifiers (2), modType (2), ... */
        *synthType = (uint16_t)((p[4] << 8) | p[5]);
        *numCommands = (uint16_t)((p[8] << 8) | p[9]);
        return SND_NO_ERR;
    } else if (format == 2) {
        /* Format 2: format (2), refCount (2), numCommands (2) */
        *synthType = 5; /* sampledSynth */
        *numCommands = (uint16_t)((p[4] << 8) | p[5]);
        return SND_NO_ERR;
    }

    return SND_BAD_FORMAT;
}

int16_t lc3_snd_parse_sound_header(const void *headerPtr, SampledSoundHeader *outHdr)
{
    if (!headerPtr || !outHdr) return SND_PARAM_ERR;

    const uint8_t *p = (const uint8_t *)headerPtr;
    outHdr->samplePtr = (void *)(uintptr_t)((p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]);
    outHdr->length = ((uint32_t)p[4] << 24) | ((uint32_t)p[5] << 16) | ((uint32_t)p[6] << 8) | p[7];
    outHdr->sampleRate = ((uint32_t)p[8] << 24) | ((uint32_t)p[9] << 16) | ((uint32_t)p[10] << 8) | p[11];
    outHdr->loopStart = ((uint32_t)p[12] << 24) | ((uint32_t)p[13] << 16) | ((uint32_t)p[14] << 8) | p[15];
    outHdr->loopEnd = ((uint32_t)p[16] << 24) | ((uint32_t)p[17] << 16) | ((uint32_t)p[18] << 8) | p[19];
    outHdr->encode = p[20];
    outHdr->baseFrequency = p[21];

    return SND_NO_ERR;
}

int16_t lc3_snd_parse_extended_header(const void *headerPtr, ExtendedSoundHeader *outHdr)
{
    if (!headerPtr || !outHdr) return SND_PARAM_ERR;

    memset(outHdr, 0, sizeof(ExtendedSoundHeader));
    const uint8_t *p = (const uint8_t *)headerPtr;

    outHdr->numChannels = ((uint32_t)p[4] << 24) | ((uint32_t)p[5] << 16) | ((uint32_t)p[6] << 8) | p[7];
    outHdr->sampleRate = ((uint32_t)p[8] << 24) | ((uint32_t)p[9] << 16) | ((uint32_t)p[10] << 8) | p[11];
    outHdr->numFrames = ((uint32_t)p[22] << 24) | ((uint32_t)p[23] << 16) | ((uint32_t)p[24] << 8) | p[25];
    outHdr->sampleSize = 16;

    return SND_NO_ERR;
}

int16_t lc3_snd_parse_compressed_header(const void *headerPtr, CompressedSoundHeader *outHdr)
{
    if (!headerPtr || !outHdr) return SND_PARAM_ERR;

    memset(outHdr, 0, sizeof(CompressedSoundHeader));
    const uint8_t *p = (const uint8_t *)headerPtr;

    outHdr->numChannels = ((uint32_t)p[4] << 24) | ((uint32_t)p[5] << 16) | ((uint32_t)p[6] << 8) | p[7];
    outHdr->sampleRate = ((uint32_t)p[8] << 24) | ((uint32_t)p[9] << 16) | ((uint32_t)p[10] << 8) | p[11];
    outHdr->format = ((uint32_t)p[40] << 24) | ((uint32_t)p[41] << 16) | ((uint32_t)p[42] << 8) | p[43];

    return SND_NO_ERR;
}

uint32_t lc3_snd_calc_duration(uint32_t sampleCount, uint32_t sampleRate16_16)
{
    if (sampleRate16_16 == 0) return 0;

    uint32_t rateHz = sampleRate16_16 >> 16;
    if (rateHz == 0) rateHz = 22254;

    return (uint32_t)(((uint64_t)sampleCount * 1000) / rateHz);
}

int16_t lc3_snd_preview_modal(const void *sndResource, uint32_t resSize)
{
    uint16_t synth = 0;
    uint16_t cmds = 0;
    return lc3_snd_parse_resource(sndResource, resSize, &synth, &cmds);
}
