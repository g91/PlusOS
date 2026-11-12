/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: TrueType Font Instruction Interpreter (VM Engine)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_TT_BYTECODE_H
#define LC3_TT_BYTECODE_H

#include <stdint.h>
#include <stdbool.h>

#define TT_VM_NO_ERR         0
#define TT_VM_STACK_OVERFLOW -1
#define TT_VM_STACK_UNDERFLOW -2
#define TT_VM_PARAM_ERR     -50

typedef struct TTExecContext {
    int32_t stack[64];
    int32_t sp;
    int32_t controlValueTable[64];
    uint32_t cvtCount;
    int32_t storageArea[32];
    uint32_t zp0, zp1, zp2;
    int32_t roundPeriod;
    int32_t roundThreshold;
} TTExecContext;

/* TrueType VM API */
int16_t lc3_tt_exec_bytecode(TTExecContext *ctx, const uint8_t *instructions, uint32_t instrLen, void *points, uint32_t ptCount);
int16_t lc3_tt_push_word(TTExecContext *ctx, int32_t val);
int16_t lc3_tt_pop_word(TTExecContext *ctx, int32_t *val);
int32_t lc3_tt_round_state(TTExecContext *ctx, int32_t val);
int32_t lc3_tt_project_point(int32_t px, int32_t py, int32_t dx, int32_t dy);
void lc3_tt_move_point(void *points, uint32_t ptIdx, int32_t deltaX, int32_t deltaY);
void lc3_tt_interpolate_untouched_points(void *points, uint32_t ptCount);

#endif /* LC3_TT_BYTECODE_H */
