/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Macintosh Audio Compression & Expansion (MACE 3:1 / 6:1)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_MACE_H
#define LC3_MACE_H

#include <stdint.h>
#include <stdbool.h>

#define MACE_NO_ERR         0
#define MACE_PARAM_ERR   -50

/* MACE Routines */
uint32_t lc3_mace_version(void);
void lc3_comp_3to1(const uint8_t *inBuffer, uint8_t *outBuffer, uint32_t cnt,
                   void *inState, void *outState, uint32_t numChannels, uint32_t whichChannel);
void lc3_comp_6to1(const uint8_t *inBuffer, uint8_t *outBuffer, uint32_t cnt,
                   void *inState, void *outState, uint32_t numChannels, uint32_t whichChannel);
void lc3_exp_1to3(const uint8_t *inBuffer, uint8_t *outBuffer, uint32_t cnt,
                  void *inState, void *outState, uint32_t numChannels, uint32_t whichChannel);
void lc3_exp_1to6(const uint8_t *inBuffer, uint8_t *outBuffer, uint32_t cnt,
                  void *inState, void *outState, uint32_t numChannels, uint32_t whichChannel);

#endif /* LC3_MACE_H */
