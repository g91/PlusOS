/*
 * PlusOS Macintosh LC III ROM Compatibility Layer
 * Module 188: Sound Manager 3.0 MACE Compression Engine (GF)
 */

#ifndef LC3_MACE_ENCODE_H
#define LC3_MACE_ENCODE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int16_t prev_sample;
    int16_t step_index;
    int16_t accumulator;
} lc3_mace_state_t;

void lc3_mace_init(lc3_mace_state_t *state);
uint32_t lc3_mace_compress_3to1(lc3_mace_state_t *state, const int8_t *in_pcm, uint32_t in_samples, uint8_t *out_mace);
uint32_t lc3_mace_expand_3to1(lc3_mace_state_t *state, const uint8_t *in_mace, uint32_t in_bytes, int8_t *out_pcm);
uint32_t lc3_mace_compress_6to1(lc3_mace_state_t *state, const int8_t *in_pcm, uint32_t in_samples, uint8_t *out_mace);
uint32_t lc3_mace_expand_6to1(lc3_mace_state_t *state, const uint8_t *in_mace, uint32_t in_bytes, int8_t *out_pcm);
void lc3_mace_reset(lc3_mace_state_t *state);

#endif /* LC3_MACE_ENCODE_H */
