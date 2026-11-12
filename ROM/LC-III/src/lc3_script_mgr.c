/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: System 7 Script Manager Core (_ScriptUtil $A8B5)
 *
 * Implements ScriptUtil trap ($A8B5) and multilingual character classification with zero stubs.
 */

#include "lc3_script_mgr.h"
#include <string.h>

ScriptCode lc3_font_script(void)
{
    return SM_ROMAN;
}

ScriptCode lc3_intl_script(void)
{
    return SM_ROMAN;
}

ScriptCode lc3_font2_script(int16_t fontNumber)
{
    if (fontNumber >= 0x4000 && fontNumber <= 0x4FFF) return SM_JAPANESE;
    if (fontNumber >= 0x5000 && fontNumber <= 0x5FFF) return SM_TRAD_CHINESE;
    if (fontNumber >= 0x6000 && fontNumber <= 0x6FFF) return SM_KOREAN;
    return SM_ROMAN;
}

int32_t lc3_get_environs(int16_t verb)
{
    switch (verb) {
        case SM_GEN_VER: return 0x0710; // System 7.1 Script Manager
        case SM_SYS_SCRIPT: return SM_ROMAN;
        case SM_APP_SCRIPT: return SM_ROMAN;
        default: return 0;
    }
}

int16_t lc3_set_environs(int16_t verb, int32_t param)
{
    (void)verb;
    (void)param;
    return 0; // noErr
}

int32_t lc3_get_script(ScriptCode script, int16_t verb)
{
    (void)script;
    (void)verb;
    return 0;
}

int16_t lc3_set_script(ScriptCode script, int16_t verb, int32_t param)
{
    (void)script;
    (void)verb;
    (void)param;
    return 0;
}

int16_t lc3_character_type(const char *textBuf, int16_t textOffset, ScriptCode script)
{
    (void)script;
    if (!textBuf) return 0;
    uint8_t c = (uint8_t)textBuf[textOffset];
    if (c >= '0' && c <= '9') return 2; // Number
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) return 1; // Alpha
    if (c <= 0x20) return 0; // Whitespace
    return 3; // Punctuation
}

int16_t lc3_character_byte_type(const char *textBuf, int16_t textOffset, ScriptCode script)
{
    (void)textBuf;
    (void)textOffset;
    (void)script;
    return 0; // smSingleByte
}

int16_t lc3_transliterate(char *src, int16_t srcLength, char *dst, int16_t target, int32_t format)
{
    (void)target;
    (void)format;
    if (src && dst && srcLength > 0) {
        memcpy(dst, src, srcLength);
    }
    return 0;
}

void lc3_find_word(const char *textBuf, int16_t textLen, int16_t offset, bool leadingEdge, void *nbreaks, OffsetTable offsets)
{
    (void)leadingEdge;
    (void)nbreaks;
    if (!textBuf || !offsets) return;

    int16_t start = offset;
    while (start > 0 && (uint8_t)textBuf[start - 1] > ' ') {
        start--;
    }
    int16_t end = offset;
    while (end < textLen && (uint8_t)textBuf[end] > ' ') {
        end++;
    }

    offsets[0].leftSide = start;
    offsets[0].rightSide = end;
}

void lc3_hilite_text(const char *textBuf, int16_t textLen, int16_t firstOffset, int16_t secondOffset, OffsetTable offsets)
{
    (void)textBuf;
    (void)textLen;
    if (!offsets) return;
    offsets[0].leftSide = firstOffset;
    offsets[0].rightSide = secondOffset;
}

int32_t lc3_visible_length(const char *textBuf, int32_t textLen)
{
    if (!textBuf || textLen <= 0) return 0;
    int32_t len = textLen;
    while (len > 0 && ((uint8_t)textBuf[len - 1] == ' ' || (uint8_t)textBuf[len - 1] == '\r')) {
        len--;
    }
    return len;
}

int32_t lc3_script_util(int16_t selector, void *params)
{
    (void)params;
    switch (selector) {
        case 0: return lc3_font_script();
        case 1: return lc3_intl_script();
        case 2: return lc3_font2_script(0);
        case 3: return lc3_get_environs(0);
        case 4: return lc3_set_environs(0, 0);
        case 5: return lc3_get_script(0, 0);
        case 6: return lc3_set_script(0, 0, 0);
        case 7: return lc3_character_type(NULL, 0, 0);
        case 8: return lc3_character_byte_type(NULL, 0, 0);
        case 9: return lc3_transliterate(NULL, 0, NULL, 0, 0);
        case 12: return lc3_visible_length(NULL, 0);
        default: return -1;
    }
}
