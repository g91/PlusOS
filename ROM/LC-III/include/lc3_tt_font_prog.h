/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: TrueType Font Program ('fpgm') & Pre-Program ('prep') Interpreter
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_TT_FONT_PROG_H
#define LC3_TT_FONT_PROG_H

#include <stdint.h>
#include <stdbool.h>

#define TT_FP_NO_ERR         0
#define TT_FP_PARAM_ERR    -50

typedef struct TTVector {
    int16_t x;
    int16_t y;
} TTVector;

typedef struct TTFontProgramContext {
    int32_t cvtTable[128];
    uint32_t cvtSize;
    TTVector projVector;
    TTVector freedomVector;
    TTVector dualProjVector;
    uint32_t functionDefinitions[32];
    uint32_t functionCount;
    bool isPreProgramDone;
} TTFontProgramContext;

/* Font Program API */
int16_t lc3_tt_exec_font_program(TTFontProgramContext *ctx, const uint8_t *fpgmData, uint32_t fpgmLen);
int16_t lc3_tt_exec_pre_program(TTFontProgramContext *ctx, const uint8_t *prepData, uint32_t prepLen, int32_t pointSize);
void lc3_tt_set_freedom_vector(TTFontProgramContext *ctx, int16_t vx, int16_t vy);
void lc3_tt_set_dual_projection_vector(TTFontProgramContext *ctx, int16_t vx, int16_t vy);
int32_t lc3_tt_measure_distance(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const TTVector *proj);
void lc3_tt_set_zone_point(void *zone, uint32_t zoneIndex, uint32_t ptIndex, int32_t x, int32_t y);

#endif /* LC3_TT_FONT_PROG_H */
