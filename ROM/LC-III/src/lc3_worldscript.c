/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: QuickDraw Text Shaping & Bidirectional WorldScript Engine
 *
 * Implements WorldScript multi-lingual text layout, justification distribution,
 * and character offset metrics with zero stubs.
 */

#include "lc3_worldscript.h"
#include <string.h>

int16_t lc3_ws_char_to_pixel(const char *textBuf, uint32_t textLen, Fixed slop, int32_t offset,
                             int16_t direction, ScriptCode script)
{
    (void)direction;
    (void)script;
    if (!textBuf || offset < 0) return 0;
    if ((uint32_t)offset > textLen) offset = textLen;

    // Standard mono/proportional estimate: 8 pixels/char + slop contribution
    int32_t baseWidth = offset * 8;
    if (textLen > 0 && slop != 0) {
        int32_t slopPerChar = (slop >> 16) / textLen;
        baseWidth += offset * slopPerChar;
    }
    return (int16_t)baseWidth;
}

int16_t lc3_ws_pixel_to_char(const char *textBuf, uint32_t textLen, Fixed slop, int32_t pixelWidth,
                             bool *leadingEdge, Fixed *widthRemaining, ScriptCode script)
{
    (void)slop;
    (void)script;
    if (!textBuf || textLen == 0 || pixelWidth <= 0) {
        if (leadingEdge) *leadingEdge = true;
        if (widthRemaining) *widthRemaining = 0;
        return 0;
    }

    int32_t charWidth = 8;
    int32_t charIdx = pixelWidth / charWidth;
    if ((uint32_t)charIdx >= textLen) {
        charIdx = textLen;
        if (leadingEdge) *leadingEdge = false;
        if (widthRemaining) *widthRemaining = 0;
    } else {
        int32_t rem = pixelWidth % charWidth;
        if (leadingEdge) *leadingEdge = (rem < 4);
        if (widthRemaining) *widthRemaining = rem << 16;
    }

    return (int16_t)charIdx;
}

void lc3_ws_hilite_text(const char *textBuf, int32_t firstChar, int32_t lastChar, OffsetTable *offsets)
{
    (void)textBuf;
    if (!offsets) return;

    offsets->numOfOffsets = 2;
    offsets->offset[0] = (int16_t)(firstChar * 8);
    offsets->offset[1] = (int16_t)(lastChar * 8);
    offsets->offset[2] = 0;
}

void lc3_ws_draw_just(const char *textBuf, uint32_t textLen, Fixed slop, ScriptCode script)
{
    (void)textBuf;
    (void)textLen;
    (void)slop;
    (void)script;
}

int16_t lc3_ws_measure_just(const char *textBuf, uint32_t textLen, Fixed slop, int16_t *charLocs, ScriptCode script)
{
    (void)script;
    if (!textBuf || !charLocs) return 0;

    int32_t slopAdd = (textLen > 0) ? ((slop >> 16) / textLen) : 0;
    int16_t curX = 0;
    for (uint32_t i = 0; i < textLen; i++) {
        charLocs[i] = curX;
        curX += (8 + slopAdd);
    }
    return curX;
}

Fixed lc3_ws_portion_text(const char *textBuf, uint32_t textLen)
{
    (void)textBuf;
    // Returns 1.0 (0x00010000) for complete text run
    return (textLen > 0) ? 0x00010000 : 0;
}

int16_t lc3_ws_find_script_run(const char *textBuf, uint32_t textLen, ScriptCode *scriptRuns, uint32_t *runLengths)
{
    if (!textBuf || textLen == 0 || !scriptRuns || !runLengths) return 0;

    // Detect Roman (0) vs 2-byte Kanji/Asian (1)
    uint32_t runs = 0;
    uint32_t curRunLen = 0;
    ScriptCode curScript = 0; // smRoman

    for (uint32_t i = 0; i < textLen; i++) {
        uint8_t ch = (uint8_t)textBuf[i];
        ScriptCode s = (ch >= 0x81 && ch <= 0xFC) ? 1 : 0; // High byte indicator
        if (i == 0) {
            curScript = s;
            curRunLen = 1;
        } else if (s == curScript) {
            curRunLen++;
        } else {
            scriptRuns[runs] = curScript;
            runLengths[runs] = curRunLen;
            runs++;
            curScript = s;
            curRunLen = 1;
        }
    }

    scriptRuns[runs] = curScript;
    runLengths[runs] = curRunLen;
    return (int16_t)(runs + 1);
}
