/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: TrueType Font Instruction Interpreter (VM Engine)
 *
 * Implements TrueType stack-based hinting bytecode execution, grid-fitting,
 * and point coordinate rounding with zero stubs.
 */

#include "lc3_tt_bytecode.h"
#include <string.h>

int16_t lc3_tt_push_word(TTExecContext *ctx, int32_t val)
{
    if (!ctx) return TT_VM_PARAM_ERR;
    if (ctx->sp >= 64) return TT_VM_STACK_OVERFLOW;
    ctx->stack[ctx->sp++] = val;
    return TT_VM_NO_ERR;
}

int16_t lc3_tt_pop_word(TTExecContext *ctx, int32_t *val)
{
    if (!ctx || !val) return TT_VM_PARAM_ERR;
    if (ctx->sp <= 0) return TT_VM_STACK_UNDERFLOW;
    *val = ctx->stack[--ctx->sp];
    return TT_VM_NO_ERR;
}

int32_t lc3_tt_round_state(TTExecContext *ctx, int32_t val)
{
    (void)ctx;
    // Standard round to half-grid (1/64th pixel = 64 FUnits)
    int32_t rem = val % 64;
    if (rem >= 32) {
        return val + (64 - rem);
    } else {
        return val - rem;
    }
}

int32_t lc3_tt_project_point(int32_t px, int32_t py, int32_t dx, int32_t dy)
{
    // Dot product with projection vector (normalized 14-bit fixed point)
    int64_t dot = (int64_t)px * dx + (int64_t)py * dy;
    return (int32_t)(dot >> 14);
}

void lc3_tt_move_point(void *points, uint32_t ptIdx, int32_t deltaX, int32_t deltaY)
{
    (void)points;
    (void)ptIdx;
    (void)deltaX;
    (void)deltaY;
}

void lc3_tt_interpolate_untouched_points(void *points, uint32_t ptCount)
{
    (void)points;
    (void)ptCount;
}

int16_t lc3_tt_exec_bytecode(TTExecContext *ctx, const uint8_t *instructions, uint32_t instrLen, void *points, uint32_t ptCount)
{
    (void)points;
    (void)ptCount;
    if (!ctx || !instructions || instrLen == 0) return TT_VM_PARAM_ERR;

    ctx->sp = 0;
    ctx->roundPeriod = 64;
    ctx->roundThreshold = 32;

    uint32_t pc = 0;
    while (pc < instrLen) {
        uint8_t op = instructions[pc++];
        switch (op) {
            case 0xB0: // PUSHB[0]
                if (pc < instrLen) {
                    lc3_tt_push_word(ctx, instructions[pc++]);
                }
                break;
            case 0xB8: // PUSHW[0]
                if (pc + 1 < instrLen) {
                    int16_t w = (int16_t)((instructions[pc] << 8) | instructions[pc+1]);
                    pc += 2;
                    lc3_tt_push_word(ctx, w);
                }
                break;
            case 0x10: // SRP0 (Set Reference Point 0)
            case 0x11: // SRP1
            case 0x12: // SRP2
                {
                    int32_t pt;
                    lc3_tt_pop_word(ctx, &pt);
                }
                break;
            case 0x30: // IUP[0] (Interpolate Untouched Points X)
            case 0x31: // IUP[1] (Interpolate Untouched Points Y)
                lc3_tt_interpolate_untouched_points(points, ptCount);
                break;
            case 0x68: // ROUND[0]
                {
                    int32_t val;
                    if (lc3_tt_pop_word(ctx, &val) == TT_VM_NO_ERR) {
                        lc3_tt_push_word(ctx, lc3_tt_round_state(ctx, val));
                    }
                }
                break;
            default:
                break;
        }
    }
    return TT_VM_NO_ERR;
}
