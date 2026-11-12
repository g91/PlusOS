/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: TextEdit Inline Input & WorldScript 2-Byte Character Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_TE_INLINE_H
#define LC3_TE_INLINE_H

#include <stdint.h>
#include <stdbool.h>

#define TE_INLINE_NO_ERR         0
#define TE_INLINE_OVERFLOW      -1
#define TE_INLINE_PARAM_ERR    -50

/* Character Byte Types */
#define smSingleByte    0
#define smFirstByte     1
#define smMiddleByte    2
#define smLastByte      3

typedef struct TEInlineRecord {
    int32_t holeOffset;
    int32_t holeLength;
    char conversionBuffer[128];
    uint16_t conversionLength;
    uint8_t hiliteStyle;      /* 1 = dotted, 2 = thick, 3 = selected */
    bool isActive;
} TEInlineRecord;

/* Inline Input API */
int16_t lc3_te_open_inline_hole(TEInlineRecord *rec, int32_t charOffset, uint16_t initialHoleSize);
int16_t lc3_te_close_inline_hole(TEInlineRecord *rec, char *outBuffer, uint16_t maxLen, uint16_t *actualLen);
int16_t lc3_te_char_byte_type(const char *text, int32_t textLen, int32_t offset);
void lc3_te_draw_inline_hilite(void *fbBase, int16_t rowBytes, int16_t left, int16_t top, int16_t width, uint8_t style);
int32_t lc3_te_find_prev_char(const char *text, int32_t currentOffset);
int32_t lc3_te_find_next_char(const char *text, int32_t textLen, int32_t currentOffset);

#endif /* LC3_TE_INLINE_H */
