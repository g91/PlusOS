/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: TextEdit 32-Bit Clean Multi-Style Engine (TE 3.0 StylRecord)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_TEXTEDIT_STYLES_H
#define LC3_TEXTEDIT_STYLES_H

#include <stdint.h>
#include <stdbool.h>

#define TE_NO_ERR         0
#define TE_PARAM_ERR    -50

#define MAX_STYLE_RUNS    32

typedef struct RGBColor {
    uint16_t red;
    uint16_t green;
    uint16_t blue;
} RGBColor;

typedef struct TextStyle {
    int16_t tsFont;
    int16_t tsFace;
    int16_t tsSize;
    RGBColor tsColor;
} TextStyle;

typedef struct StyleRun {
    int32_t scrnOffset;      /* Character start offset */
    int16_t runIndex;        /* Index into distinct style table */
} StyleRun;

typedef struct TEStyleRec {
    int16_t nRuns;
    int16_t nStyles;
    StyleRun runs[MAX_STYLE_RUNS];
    TextStyle styles[MAX_STYLE_RUNS];
    int16_t lineHeights[MAX_STYLE_RUNS];
    int16_t lineAscents[MAX_STYLE_RUNS];
} TEStyleRec;

/* TextEdit Style API */
int16_t lc3_te_init_styles(TEStyleRec *styleRec, int16_t defaultFont, int16_t defaultSize);
int16_t lc3_te_get_style(const TEStyleRec *styleRec, int32_t charOffset, TextStyle *outStyle);
int16_t lc3_te_set_style(TEStyleRec *styleRec, int32_t startOffset, int32_t endOffset, const TextStyle *newStyle, int16_t mode);
bool lc3_te_continuous_style(const TEStyleRec *styleRec, int32_t startOffset, int32_t endOffset, TextStyle *outContinuousStyle, int16_t *modeOut);
void lc3_te_calc_line_heights(TEStyleRec *styleRec, int16_t numLines);
int16_t lc3_te_use_style_scrap(TEStyleRec *destRec, int32_t pasteOffset, const TEStyleRec *scrapRec);

#endif /* LC3_TEXTEDIT_STYLES_H */
