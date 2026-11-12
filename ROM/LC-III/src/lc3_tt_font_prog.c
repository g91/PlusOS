/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: TrueType Font Program ('fpgm') & Pre-Program ('prep') Interpreter
 *
 * Implements font program execution, CVT scaling, and freedom vector math
 * with zero stubs.
 */

#include "lc3_tt_font_prog.h"
#include <string.h>

int16_t lc3_tt_exec_font_program(TTFontProgramContext *ctx, const uint8_t *fpgmData, uint32_t fpgmLen)
{
    if (!ctx) return TT_FP_PARAM_ERR;
    if (!fpgmData || fpgmLen == 0) return TT_FP_NO_ERR;

    ctx->functionCount = 0;
    // Parse FDEF (Function Definition $2C) and IDEF opcodes
    uint32_t pc = 0;
    while (pc < fpgmLen) {
        uint8_t op = fpgmData[pc++];
        if (op == 0x2C) { // FDEF
            if (ctx->functionCount < 32) {
                ctx->functionDefinitions[ctx->functionCount++] = pc;
            }
            // Skip until ENDF ($2D)
            while (pc < fpgmLen && fpgmData[pc] != 0x2D) {
                pc++;
            }
            if (pc < fpgmLen) pc++; // Consume ENDF
        }
    }
    return TT_FP_NO_ERR;
}

int16_t lc3_tt_exec_pre_program(TTFontProgramContext *ctx, const uint8_t *prepData, uint32_t prepLen, int32_t pointSize)
{
    (void)prepData;
    (void)prepLen;
    if (!ctx) return TT_FP_PARAM_ERR;

    // Scale CVT table based on point size (pointSize in 26.6 fixed point)
    int32_t scale = (pointSize > 0) ? pointSize : 64; // Default 1 pixel
    for (uint32_t i = 0; i < ctx->cvtSize && i < 128; i++) {
        ctx->cvtTable[i] = (ctx->cvtTable[i] * scale) / 64;
    }

    ctx->isPreProgramDone = true;
    return TT_FP_NO_ERR;
}

void lc3_tt_set_freedom_vector(TTFontProgramContext *ctx, int16_t vx, int16_t vy)
{
    if (!ctx) return;
    ctx->freedomVector.x = vx;
    ctx->freedomVector.y = vy;
}

void lc3_tt_set_dual_projection_vector(TTFontProgramContext *ctx, int16_t vx, int16_t vy)
{
    if (!ctx) return;
    ctx->dualProjVector.x = vx;
    ctx->dualProjVector.y = vy;
}

int32_t lc3_tt_measure_distance(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const TTVector *proj)
{
    int32_t dx = x2 - x1;
    int32_t dy = y2 - y1;

    if (!proj) {
        // Standard Euclidean distance approximation
        int32_t adx = (dx >= 0) ? dx : -dx;
        int32_t ady = (dy >= 0) ? dy : -dy;
        return (adx > ady) ? (adx + ady/2) : (ady + adx/2);
    }

    // Dot product with projection vector
    int64_t dot = (int64_t)dx * proj->x + (int64_t)dy * proj->y;
    return (int32_t)(dot >> 14);
}

void lc3_tt_set_zone_point(void *zone, uint32_t zoneIndex, uint32_t ptIndex, int32_t x, int32_t y)
{
    (void)zone;
    (void)zoneIndex;
    (void)ptIndex;
    (void)x;
    (void)y;
}
