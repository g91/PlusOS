/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: TrueType Font 'cmap' Character Code to Glyph Index Mapping Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_TT_CMAP_H
#define LC3_TT_CMAP_H

#include <stdint.h>
#include <stdbool.h>

#define TT_CMAP_NO_ERR         0
#define TT_CMAP_ERR_FORMAT   -100
#define TT_CMAP_ERR_NOT_FOUND -101
#define TT_CMAP_ERR_PARAM     -102

/* Platform IDs */
#define TT_PLATFORM_UNICODE    0
#define TT_PLATFORM_MACINTOSH  1
#define TT_PLATFORM_MICROSOFT  3

/* Macintosh Encoding IDs */
#define TT_MAC_ROMAN           0

/* Microsoft Encoding IDs */
#define TT_MS_SYMBOL           0
#define TT_MS_UNICODE_BMP      1

typedef struct TTCMapSubtableInfo {
    uint16_t platformID;
    uint16_t encodingID;
    uint32_t offset;
    uint16_t format;
    uint16_t length;
    uint16_t language;
} TTCMapSubtableInfo;

/* Public API */
int16_t lc3_tt_cmap_find_subtable(const uint8_t *cmapData, uint32_t dataLen, uint16_t reqPlatform, uint16_t reqEncoding, TTCMapSubtableInfo *info);
uint16_t lc3_tt_cmap_map_format0(const uint8_t *subtableData, uint8_t charCode);
uint16_t lc3_tt_cmap_map_format4(const uint8_t *subtableData, uint16_t charCode);
uint16_t lc3_tt_cmap_map_format6(const uint8_t *subtableData, uint16_t charCode);
uint16_t lc3_tt_cmap_char_to_glyph(const uint8_t *subtableData, uint32_t charCode);
int16_t lc3_tt_cmap_get_supported_range(const uint8_t *subtableData, uint32_t *minChar, uint32_t *maxChar);

#endif /* LC3_TT_CMAP_H */
