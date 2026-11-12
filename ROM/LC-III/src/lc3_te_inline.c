/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: TextEdit Inline Input & WorldScript 2-Byte Character Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#include "lc3_te_inline.h"
#include <string.h>

int16_t lc3_te_open_inline_hole(TEInlineRecord *rec, int32_t charOffset, uint16_t initialHoleSize)
{
    if (!rec) return TE_INLINE_PARAM_ERR;

    memset(rec, 0, sizeof(TEInlineRecord));
    rec->holeOffset = charOffset;
    rec->holeLength = initialHoleSize;
    rec->conversionLength = 0;
    rec->hiliteStyle = 1; /* Dotted line default */
    rec->isActive = true;

    return TE_INLINE_NO_ERR;
}

int16_t lc3_te_close_inline_hole(TEInlineRecord *rec, char *outBuffer, uint16_t maxLen, uint16_t *actualLen)
{
    if (!rec || !outBuffer || !actualLen) return TE_INLINE_PARAM_ERR;
    if (!rec->isActive) return TE_INLINE_PARAM_ERR;

    uint16_t copyLen = (rec->conversionLength < maxLen) ? rec->conversionLength : (maxLen - 1);
    memcpy(outBuffer, rec->conversionBuffer, copyLen);
    outBuffer[copyLen] = '\0';
    *actualLen = copyLen;

    rec->isActive = false;
    rec->holeLength = 0;
    rec->conversionLength = 0;

    return TE_INLINE_NO_ERR;
}

int16_t lc3_te_char_byte_type(const char *text, int32_t textLen, int32_t offset)
{
    if (!text || offset < 0 || offset >= textLen) return smSingleByte;

    /* MacJapanese / Shift-JIS high-byte detection: 0x81-0x9F and 0xE0-0xFC */
    uint8_t c = (uint8_t)text[offset];
    bool isLead = ((c >= 0x81 && c <= 0x9F) || (c >= 0xE0 && c <= 0xFC));

    if (isLead && (offset + 1 < textLen)) {
        return smFirstByte;
    }

    if (offset > 0) {
        uint8_t prev = (uint8_t)text[offset - 1];
        bool prevLead = ((prev >= 0x81 && prev <= 0x9F) || (prev >= 0xE0 && prev <= 0xFC));
        if (prevLead) return smLastByte;
    }

    return smSingleByte;
}

void lc3_te_draw_inline_hilite(void *fbBase, int16_t rowBytes, int16_t left, int16_t top, int16_t width, uint8_t style)
{
    if (!fbBase || width <= 0) return;

    uint8_t *fb = (uint8_t *)fbBase;
    int16_t y = top + 13; /* Baseline underline */

    for (int16_t x = left; x < left + width; ++x) {
        if (style == 1) {
            /* Dotted line: alternate pixels */
            if (x % 2 == 0) fb[y * rowBytes + x] = 0xFF;
        } else if (style == 2) {
            /* Thick underline: 2 pixels tall */
            fb[y * rowBytes + x] = 0xFF;
            fb[(y + 1) * rowBytes + x] = 0xFF;
        } else {
            /* Solid single line */
            fb[y * rowBytes + x] = 0xFF;
        }
    }
}

int32_t lc3_te_find_prev_char(const char *text, int32_t currentOffset)
{
    if (!text || currentOffset <= 0) return 0;

    if (currentOffset >= 2) {
        int16_t type = lc3_te_char_byte_type(text, currentOffset, currentOffset - 1);
        if (type == smLastByte) {
            return currentOffset - 2;
        }
    }

    return currentOffset - 1;
}

int32_t lc3_te_find_next_char(const char *text, int32_t textLen, int32_t currentOffset)
{
    if (!text || currentOffset >= textLen) return textLen;

    int16_t type = lc3_te_char_byte_type(text, textLen, currentOffset);
    if (type == smFirstByte && currentOffset + 2 <= textLen) {
        return currentOffset + 2;
    }

    return currentOffset + 1;
}
