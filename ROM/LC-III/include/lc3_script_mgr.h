/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: System 7 Script Manager Core (_ScriptUtil $A8B5)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_SCRIPT_MGR_H
#define LC3_SCRIPT_MGR_H

#include <stdint.h>
#include <stdbool.h>

#define SM_ROMAN          0
#define SM_JAPANESE       1
#define SM_TRAD_CHINESE   2
#define SM_KOREAN         3
#define SM_ARABIC         4
#define SM_HEBREW         5

#define SM_GEN_VER        0
#define SM_SCRIPT         2
#define SM_FONT           4
#define SM_KEYBOARD       6
#define SM_SYS_SCRIPT     8
#define SM_APP_SCRIPT    10

typedef int16_t ScriptCode;

typedef struct {
    int16_t leftSide;
    int16_t rightSide;
} OffsetTable[3];

/* Script Manager API */
ScriptCode lc3_font_script(void);
ScriptCode lc3_intl_script(void);
ScriptCode lc3_font2_script(int16_t fontNumber);
int32_t lc3_get_environs(int16_t verb);
int16_t lc3_set_environs(int16_t verb, int32_t param);
int32_t lc3_get_script(ScriptCode script, int16_t verb);
int16_t lc3_set_script(ScriptCode script, int16_t verb, int32_t param);
int16_t lc3_character_type(const char *textBuf, int16_t textOffset, ScriptCode script);
int16_t lc3_character_byte_type(const char *textBuf, int16_t textOffset, ScriptCode script);
int16_t lc3_transliterate(char *src, int16_t srcLength, char *dst, int16_t target, int32_t format);
void lc3_find_word(const char *textBuf, int16_t textLen, int16_t offset, bool leadingEdge, void *nbreaks, OffsetTable offsets);
void lc3_hilite_text(const char *textBuf, int16_t textLen, int16_t firstOffset, int16_t secondOffset, OffsetTable offsets);
int32_t lc3_visible_length(const char *textBuf, int32_t textLen);

/* ScriptUtil Dispatcher ($A8B5) */
int32_t lc3_script_util(int16_t selector, void *params);

#endif /* LC3_SCRIPT_MGR_H */
