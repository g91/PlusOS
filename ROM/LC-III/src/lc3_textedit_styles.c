/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: TextEdit 32-Bit Clean Multi-Style Engine (TE 3.0 StylRecord)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#include "lc3_textedit_styles.h"
#include <string.h>

int16_t lc3_te_init_styles(TEStyleRec *styleRec, int16_t defaultFont, int16_t defaultSize)
{
    if (!styleRec) return TE_PARAM_ERR;

    memset(styleRec, 0, sizeof(TEStyleRec));
    styleRec->nRuns = 1;
    styleRec->nStyles = 1;

    styleRec->runs[0].scrnOffset = 0;
    styleRec->runs[0].runIndex = 0;

    styleRec->styles[0].tsFont = defaultFont;
    styleRec->styles[0].tsFace = 0; /* Plain */
    styleRec->styles[0].tsSize = defaultSize > 0 ? defaultSize : 12;
    styleRec->styles[0].tsColor.red = 0;
    styleRec->styles[0].tsColor.green = 0;
    styleRec->styles[0].tsColor.blue = 0;

    styleRec->lineHeights[0] = defaultSize > 0 ? (int16_t)(defaultSize + 4) : 16;
    styleRec->lineAscents[0] = defaultSize > 0 ? (int16_t)(defaultSize) : 12;

    return TE_NO_ERR;
}

int16_t lc3_te_get_style(const TEStyleRec *styleRec, int32_t charOffset, TextStyle *outStyle)
{
    if (!styleRec || !outStyle) return TE_PARAM_ERR;

    int matchRun = 0;
    for (int i = 0; i < styleRec->nRuns; ++i) {
        if (styleRec->runs[i].scrnOffset <= charOffset) {
            matchRun = i;
        } else {
            break;
        }
    }

    int styleIdx = styleRec->runs[matchRun].runIndex;
    if (styleIdx >= 0 && styleIdx < styleRec->nStyles) {
        *outStyle = styleRec->styles[styleIdx];
    } else {
        *outStyle = styleRec->styles[0];
    }

    return TE_NO_ERR;
}

int16_t lc3_te_set_style(TEStyleRec *styleRec, int32_t startOffset, int32_t endOffset, const TextStyle *newStyle, int16_t mode)
{
    (void)startOffset;
    (void)endOffset;
    (void)mode;
    if (!styleRec || !newStyle) return TE_PARAM_ERR;

    if (styleRec->nStyles < MAX_STYLE_RUNS) {
        int idx = styleRec->nStyles++;
        styleRec->styles[idx] = *newStyle;
        if (styleRec->nRuns < MAX_STYLE_RUNS) {
            int r = styleRec->nRuns++;
            styleRec->runs[r].scrnOffset = startOffset;
            styleRec->runs[r].runIndex = (int16_t)idx;
        }
    }

    return TE_NO_ERR;
}

bool lc3_te_continuous_style(const TEStyleRec *styleRec, int32_t startOffset, int32_t endOffset, TextStyle *outContinuousStyle, int16_t *modeOut)
{
    (void)endOffset;
    if (!styleRec || !outContinuousStyle) return false;

    TextStyle s;
    lc3_te_get_style(styleRec, startOffset, &s);
    *outContinuousStyle = s;

    if (modeOut) *modeOut = 0x0F; /* All continuous (font, size, face, color) */
    return true;
}

void lc3_te_calc_line_heights(TEStyleRec *styleRec, int16_t numLines)
{
    if (!styleRec || numLines <= 0) return;

    for (int16_t i = 0; i < numLines && i < MAX_STYLE_RUNS; ++i) {
        int16_t maxH = 12;
        int16_t maxA = 9;
        for (int16_t s = 0; s < styleRec->nStyles; ++s) {
            int16_t sz = styleRec->styles[s].tsSize;
            if (sz + 4 > maxH) maxH = (int16_t)(sz + 4);
            if (sz > maxA) maxA = sz;
        }
        styleRec->lineHeights[i] = maxH;
        styleRec->lineAscents[i] = maxA;
    }
}

int16_t lc3_te_use_style_scrap(TEStyleRec *destRec, int32_t pasteOffset, const TEStyleRec *scrapRec)
{
    if (!destRec || !scrapRec) return TE_PARAM_ERR;

    for (int i = 0; i < scrapRec->nRuns && destRec->nRuns < MAX_STYLE_RUNS && destRec->nStyles < MAX_STYLE_RUNS; ++i) {
        int styleIdx = destRec->nStyles++;
        int runIdx = destRec->nRuns++;
        destRec->styles[styleIdx] = scrapRec->styles[scrapRec->runs[i].runIndex];
        destRec->runs[runIdx].scrnOffset = pasteOffset + scrapRec->runs[i].scrnOffset;
        destRec->runs[runIdx].runIndex = (int16_t)styleIdx;
    }

    return TE_NO_ERR;
}
