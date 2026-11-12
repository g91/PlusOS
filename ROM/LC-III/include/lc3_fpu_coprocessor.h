/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Motorola 68881/68882 FPU Coprocessor Interface
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_FPU_COPROCESSOR_H
#define LC3_FPU_COPROCESSOR_H

#include <stdint.h>
#include <stdbool.h>

#define FPU_TYPE_NONE      0
#define FPU_TYPE_68881     1
#define FPU_TYPE_68882     2

typedef struct {
    uint32_t fpcr; // Control Register
    uint32_t fpsr; // Status Register
    uint32_t fpiar; // Instruction Address Register
    uint8_t fpRegs[8][12]; // FP0..FP7 (96-bit extended)
    uint8_t internalState[216]; // 68882 FSAVE frame
} FPUContext;

/* FPU Hardware Interface API */
int16_t lc3_fpu_detect(void);
void lc3_fpu_save_state(FPUContext *ctx);
void lc3_fpu_restore_state(const FPUContext *ctx);
uint32_t lc3_fpu_get_status(void);
void lc3_fpu_set_control(uint32_t controlBits);

#endif /* LC3_FPU_COPROCESSOR_H */
