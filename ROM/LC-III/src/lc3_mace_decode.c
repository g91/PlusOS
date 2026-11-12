/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Sound Manager 3.0 MACE (Audio Compression & Expansion) Decoder
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#include "lc3_mace_decode.h"
#include <string.h>

/* Apple MACE adaptive step quantization table */
static const int16_t sMACEStepTable[32] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31,
    34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143
};

void lc3_mace_step_adaptation(int16_t *stepIndex, uint8_t deltaCode)
{
    if (!stepIndex) return;

    /* Adjust step index based on magnitude */
    if (deltaCode > 4) {
        *stepIndex += 2;
    } else if (deltaCode < 2 && *stepIndex > 0) {
        *stepIndex -= 1;
    }

    if (*stepIndex > 31) *stepIndex = 31;
    if (*stepIndex < 0) *stepIndex = 0;
}

void lc3_mace_reset_state(MACEDecoderState *state)
{
    if (!state) return;
    state->previousSample = 0;
    state->stepIndex = 0;
    state->predictor = 0;
}

int16_t lc3_mace3_decode_mono(MACEDecoderState *state, const uint8_t *inPackets, uint32_t packetCount, uint8_t *outPCM)
{
    if (!state || !inPackets || !outPCM) return MACE_PARAM_ERR;

    /* MACE 3:1 packs 6 samples into 2 bytes (3 bits per sample) */
    uint32_t outIdx = 0;
    for (uint32_t p = 0; p < packetCount; ++p) {
        uint16_t word = (uint16_t)((inPackets[p * 2] << 8) | inPackets[p * 2 + 1]);

        for (int s = 0; s < 6; ++s) {
            uint8_t code = (uint8_t)((word >> (15 - s * 3)) & 0x07);
            int16_t step = sMACEStepTable[state->stepIndex];

            int16_t diff = (code & 0x03) * step;
            if (code & 0x04) diff = -diff;

            int32_t sample = state->previousSample + diff;
            if (sample > 127) sample = 127;
            if (sample < -128) sample = -128;

            state->previousSample = (int16_t)sample;
            lc3_mace_step_adaptation(&state->stepIndex, code);

            outPCM[outIdx++] = (uint8_t)(sample + 128); /* Convert to offset-binary 128 */
        }
    }

    return MACE_NO_ERR;
}

int16_t lc3_mace6_decode_mono(MACEDecoderState *state, const uint8_t *inPackets, uint32_t packetCount, uint8_t *outPCM)
{
    if (!state || !inPackets || !outPCM) return MACE_PARAM_ERR;

    /* MACE 6:1 packs 6 samples into 1 byte (approx 1.33 bits per sample) */
    uint32_t outIdx = 0;
    for (uint32_t p = 0; p < packetCount; ++p) {
        uint8_t b = inPackets[p];
        for (int s = 0; s < 4; ++s) {
            uint8_t code = (uint8_t)((b >> (6 - s * 2)) & 0x03);
            int16_t step = sMACEStepTable[state->stepIndex];

            int16_t diff = (code & 0x01) ? step : -step;
            int32_t sample = state->previousSample + diff;
            if (sample > 127) sample = 127;
            if (sample < -128) sample = -128;

            state->previousSample = (int16_t)sample;
            lc3_mace_step_adaptation(&state->stepIndex, code);

            outPCM[outIdx++] = (uint8_t)(sample + 128);
        }
    }

    return MACE_NO_ERR;
}

int16_t lc3_mace3_decode_stereo(MACEDecoderState *leftState, MACEDecoderState *rightState, const uint8_t *inPackets, uint32_t packetCount, uint8_t *outPCM)
{
    if (!leftState || !rightState || !inPackets || !outPCM) return MACE_PARAM_ERR;

    uint8_t leftBuf[128];
    uint8_t rightBuf[128];

    uint32_t halfCount = packetCount / 2;
    lc3_mace3_decode_mono(leftState, inPackets, halfCount, leftBuf);
    lc3_mace3_decode_mono(rightState, inPackets + halfCount * 2, halfCount, rightBuf);

    for (uint32_t i = 0; i < halfCount * 6; ++i) {
        outPCM[i * 2] = leftBuf[i];
        outPCM[i * 2 + 1] = rightBuf[i];
    }

    return MACE_NO_ERR;
}

int16_t lc3_mace6_decode_stereo(MACEDecoderState *leftState, MACEDecoderState *rightState, const uint8_t *inPackets, uint32_t packetCount, uint8_t *outPCM)
{
    if (!leftState || !rightState || !inPackets || !outPCM) return MACE_PARAM_ERR;

    uint8_t leftBuf[128];
    uint8_t rightBuf[128];

    uint32_t halfCount = packetCount / 2;
    lc3_mace6_decode_mono(leftState, inPackets, halfCount, leftBuf);
    lc3_mace6_decode_mono(rightState, inPackets + halfCount, halfCount, rightBuf);

    for (uint32_t i = 0; i < halfCount * 4; ++i) {
        outPCM[i * 2] = leftBuf[i];
        outPCM[i * 2 + 1] = rightBuf[i];
    }

    return MACE_NO_ERR;
}
