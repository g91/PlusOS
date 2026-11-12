/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: QuickDraw Text Shaping & Bidirectional WorldScript Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_WORLDSCRIPT_H
#define LC3_WORLDSCRIPT_H

#include <stdint.h>
#include <stdbool.h>

#define WS_NO_ERR         0
#define WS_PARAM_ERR    -50

typedef int16_t ScriptCode;
typedef int32_t Fixed;

typedef struct OffsetTable {
    int16_t numOfOffsets;
    int16_t offset[3];
} OffsetTable;

/* WorldScript API */
int16_t lc3_ws_char_to_pixel(const char *textBuf, uint32_t textLen, Fixed slop, int32_t offset,
                             int16_t direction, ScriptCode script);
int16_t lc3_ws_pixel_to_char(const char *textBuf, uint32_t textLen, Fixed slop, int32_t pixelWidth,
                             bool *leadingEdge, Fixed *widthRemaining, ScriptCode script);
void lc3_ws_hilite_text(const char *textBuf, int32_t firstChar, int32_t lastChar, OffsetTable *offsets);
void lc3_ws_draw_just(const char *textBuf, uint32_t textLen, Fixed slop, ScriptCode script);
int16_t lc3_ws_measure_just(const char *textBuf, uint32_t textLen, Fixed slop, int16_t *charLocs, ScriptCode script);
Fixed lc3_ws_portion_text(const char *textBuf, uint32_t textLen);
int16_t lc3_ws_find_script_run(const char *textBuf, uint32_t textLen, ScriptCode *scriptRuns, uint32_t *runLengths);

#endif /* LC3_WORLDSCRIPT_H */
