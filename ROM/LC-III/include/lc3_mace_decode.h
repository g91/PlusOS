/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Sound Manager 3.0 MACE (Audio Compression & Expansion) Decoder
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_MACE_DECODE_H
#define LC3_MACE_DECODE_H

#include <stdint.h>
#include <stdbool.h>

#define MACE_NO_ERR         0
#define MACE_PARAM_ERR    -50

typedef struct MACEDecoderState {
    int16_t previousSample;
    int16_t stepIndex;
    int32_t predictor;
} MACEDecoderState;

/* MACE API */
int16_t lc3_mace3_decode_mono(MACEDecoderState *state, const uint8_t *inPackets, uint32_t packetCount, uint8_t *outPCM);
int16_t lc3_mace6_decode_mono(MACEDecoderState *state, const uint8_t *inPackets, uint32_t packetCount, uint8_t *outPCM);
int16_t lc3_mace3_decode_stereo(MACEDecoderState *leftState, MACEDecoderState *rightState, const uint8_t *inPackets, uint32_t packetCount, uint8_t *outPCM);
int16_t lc3_mace6_decode_stereo(MACEDecoderState *leftState, MACEDecoderState *rightState, const uint8_t *inPackets, uint32_t packetCount, uint8_t *outPCM);
void lc3_mace_step_adaptation(int16_t *stepIndex, uint8_t deltaCode);
void lc3_mace_reset_state(MACEDecoderState *state);

#endif /* LC3_MACE_DECODE_H */
