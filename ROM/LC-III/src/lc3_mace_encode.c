/*
 * PlusOS Macintosh LC III ROM Compatibility Layer
 * Module 188: Sound Manager 3.0 MACE Compression Engine (GF)
 */

#include "lc3_mace_encode.h"
#include <string.h>

static const int16_t mace_step_table[16] = {
    7, 8, 9, 10, 11, 12, 13, 14,
    16, 17, 19, 21, 23, 25, 28, 31
};

void lc3_mace_init(lc3_mace_state_t *state)
{
    if (!state) return;
    state->prev_sample = 0;
    state->step_index = 0;
    state->accumulator = 0;
}

void lc3_mace_reset(lc3_mace_state_t *state)
{
    lc3_mace_init(state);
}

uint32_t lc3_mace_compress_3to1(lc3_mace_state_t *state, const int8_t *in_pcm, uint32_t in_samples, uint8_t *out_mace)
{
    if (!state || !in_pcm || !out_mace) return 0;
    /* 3:1 packs 3 samples into 1 byte (approx 2.67 bits each -> two 3-bit and one 2-bit code) */
    uint32_t out_bytes = 0;
    for (uint32_t i = 0; i + 2 < in_samples; i += 3) {
        int16_t step = mace_step_table[state->step_index & 15];

        /* Sample 1 */
        int16_t diff1 = (int16_t)in_pcm[i] - state->prev_sample;
        int code1 = (diff1 / (step ? step : 1)) + 4;
        if (code1 < 0) code1 = 0;
        if (code1 > 7) code1 = 7;
        state->prev_sample += (code1 - 4) * step;

        /* Sample 2 */
        int16_t diff2 = (int16_t)in_pcm[i + 1] - state->prev_sample;
        int code2 = (diff2 / (step ? step : 1)) + 4;
        if (code2 < 0) code2 = 0;
        if (code2 > 7) code2 = 7;
        state->prev_sample += (code2 - 4) * step;

        /* Sample 3 */
        int16_t diff3 = (int16_t)in_pcm[i + 2] - state->prev_sample;
        int code3 = (diff3 / (step ? step : 1)) + 2;
        if (code3 < 0) code3 = 0;
        if (code3 > 3) code3 = 3;
        state->prev_sample += (code3 - 2) * step;

        /* Pack 3 + 3 + 2 = 8 bits */
        out_mace[out_bytes++] = (uint8_t)((code1 << 5) | (code2 << 2) | code3);

        /* Adapt step index */
        if (code1 > 5 || code2 > 5) {
            if (state->step_index < 15) state->step_index++;
        } else if (code1 < 2 && code2 < 2) {
            if (state->step_index > 0) state->step_index--;
        }
    }

    return out_bytes;
}

uint32_t lc3_mace_expand_3to1(lc3_mace_state_t *state, const uint8_t *in_mace, uint32_t in_bytes, int8_t *out_pcm)
{
    if (!state || !in_mace || !out_pcm) return 0;
    uint32_t out_samples = 0;

    for (uint32_t i = 0; i < in_bytes; i++) {
        uint8_t byte = in_mace[i];
        int code1 = (byte >> 5) & 7;
        int code2 = (byte >> 2) & 7;
        int code3 = byte & 3;

        int16_t step = mace_step_table[state->step_index & 15];

        state->prev_sample += (code1 - 4) * step;
        if (state->prev_sample > 127) state->prev_sample = 127;
        if (state->prev_sample < -128) state->prev_sample = -128;
        out_pcm[out_samples++] = (int8_t)state->prev_sample;

        state->prev_sample += (code2 - 4) * step;
        if (state->prev_sample > 127) state->prev_sample = 127;
        if (state->prev_sample < -128) state->prev_sample = -128;
        out_pcm[out_samples++] = (int8_t)state->prev_sample;

        state->prev_sample += (code3 - 2) * step;
        if (state->prev_sample > 127) state->prev_sample = 127;
        if (state->prev_sample < -128) state->prev_sample = -128;
        out_pcm[out_samples++] = (int8_t)state->prev_sample;

        /* Adapt step */
        if (code1 > 5 || code2 > 5) {
            if (state->step_index < 15) state->step_index++;
        } else if (code1 < 2 && code2 < 2) {
            if (state->step_index > 0) state->step_index--;
        }
    }

    return out_samples;
}

uint32_t lc3_mace_compress_6to1(lc3_mace_state_t *state, const int8_t *in_pcm, uint32_t in_samples, uint8_t *out_mace)
{
    if (!state || !in_pcm || !out_mace) return 0;
    /* 6:1 packs 6 samples into 1 byte (approx 1.33 bits each -> four 1-bit, two 2-bit) */
    uint32_t out_bytes = 0;
    for (uint32_t i = 0; i + 5 < in_samples; i += 6) {
        int16_t step = mace_step_table[state->step_index & 15] * 2;

        uint8_t byte = 0;
        for (int s = 0; s < 6; s++) {
            int16_t diff = (int16_t)in_pcm[i + s] - state->prev_sample;
            int bit = (diff >= 0) ? 1 : 0;
            state->prev_sample += (bit ? step : -step);
            byte = (byte << 1) | bit;
        }

        if (state->prev_sample > 127) state->prev_sample = 127;
        if (state->prev_sample < -128) state->prev_sample = -128;

        out_mace[out_bytes++] = byte;
    }

    return out_bytes;
}

uint32_t lc3_mace_expand_6to1(lc3_mace_state_t *state, const uint8_t *in_mace, uint32_t in_bytes, int8_t *out_pcm)
{
    if (!state || !in_mace || !out_pcm) return 0;
    uint32_t out_samples = 0;

    for (uint32_t i = 0; i < in_bytes; i++) {
        uint8_t byte = in_mace[i];
        int16_t step = mace_step_table[state->step_index & 15] * 2;

        for (int s = 5; s >= 0; s--) {
            int bit = (byte >> s) & 1;
            state->prev_sample += (bit ? step : -step);
            if (state->prev_sample > 127) state->prev_sample = 127;
            if (state->prev_sample < -128) state->prev_sample = -128;
            out_pcm[out_samples++] = (int8_t)state->prev_sample;
        }
    }

    return out_samples;
}
