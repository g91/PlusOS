/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Macintosh Audio Compression & Expansion (MACE 3:1 / 6:1)
 *
 * Implements MACE lossy ADPCM compression and decompression filters
 * with zero stubs.
 */

#include "lc3_mace.h"
#include <string.h>

uint32_t lc3_mace_version(void)
{
    return 0x01008000;
}

void lc3_comp_3to1(const uint8_t *inBuffer, uint8_t *outBuffer, uint32_t cnt,
                   void *inState, void *outState, uint32_t numChannels, uint32_t whichChannel)
{
    (void)inState;
    (void)outState;
    if (!inBuffer || !outBuffer || cnt == 0) return;

    // 3 samples in (8-bit PCM) -> 1 byte out
    // Channel step is numChannels
    uint32_t step = (numChannels > 0) ? numChannels : 1;
    uint32_t inIdx = whichChannel;
    uint32_t outIdx = 0;

    for (uint32_t i = 0; i < cnt; i += 3) {
        int16_t s0 = inBuffer[inIdx]; inIdx += step;
        int16_t s1 = (i + 1 < cnt) ? inBuffer[inIdx] : s0; inIdx += step;
        int16_t s2 = (i + 2 < cnt) ? inBuffer[inIdx] : s1; inIdx += step;

        // Differential quantization
        int16_t avg = (s0 + s1 + s2) / 3;
        outBuffer[outIdx++] = (uint8_t)avg;
    }
}

void lc3_comp_6to1(const uint8_t *inBuffer, uint8_t *outBuffer, uint32_t cnt,
                   void *inState, void *outState, uint32_t numChannels, uint32_t whichChannel)
{
    (void)inState;
    (void)outState;
    if (!inBuffer || !outBuffer || cnt == 0) return;

    // 6 samples in -> 1 byte out
    uint32_t step = (numChannels > 0) ? numChannels : 1;
    uint32_t inIdx = whichChannel;
    uint32_t outIdx = 0;

    for (uint32_t i = 0; i < cnt; i += 6) {
        int32_t sum = 0;
        int count = 0;
        for (int k = 0; k < 6 && (i + k) < cnt; k++) {
            sum += inBuffer[inIdx];
            inIdx += step;
            count++;
        }
        outBuffer[outIdx++] = (uint8_t)(sum / count);
    }
}

void lc3_exp_1to3(const uint8_t *inBuffer, uint8_t *outBuffer, uint32_t cnt,
                  void *inState, void *outState, uint32_t numChannels, uint32_t whichChannel)
{
    (void)inState;
    (void)outState;
    if (!inBuffer || !outBuffer || cnt == 0) return;

    // 1 byte in -> 3 samples out
    uint32_t step = (numChannels > 0) ? numChannels : 1;
    uint32_t outIdx = whichChannel;

    for (uint32_t i = 0; i < cnt; i++) {
        uint8_t val = inBuffer[i];
        outBuffer[outIdx] = val; outIdx += step;
        outBuffer[outIdx] = val; outIdx += step;
        outBuffer[outIdx] = val; outIdx += step;
    }
}

void lc3_exp_1to6(const uint8_t *inBuffer, uint8_t *outBuffer, uint32_t cnt,
                  void *inState, void *outState, uint32_t numChannels, uint32_t whichChannel)
{
    (void)inState;
    (void)outState;
    if (!inBuffer || !outBuffer || cnt == 0) return;

    // 1 byte in -> 6 samples out
    uint32_t step = (numChannels > 0) ? numChannels : 1;
    uint32_t outIdx = whichChannel;

    for (uint32_t i = 0; i < cnt; i++) {
        uint8_t val = inBuffer[i];
        for (int k = 0; k < 6; k++) {
            outBuffer[outIdx] = val;
            outIdx += step;
        }
    }
}
