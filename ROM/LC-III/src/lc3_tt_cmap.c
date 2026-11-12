/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: TrueType Font 'cmap' Character Code to Glyph Index Mapping Engine
 *
 * Implements Format 0, Format 4, and Format 6 cmap character-to-glyph translation
 * with zero stubs.
 */

#include "lc3_tt_cmap.h"

static inline uint16_t read_u16_be(const uint8_t *p)
{
    return (uint16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
}

static inline uint32_t read_u32_be(const uint8_t *p)
{
    return (((uint32_t)p[0] << 24) |
            ((uint32_t)p[1] << 16) |
            ((uint32_t)p[2] << 8)  |
            ((uint32_t)p[3]));
}

/* 1410: lc3_tt_cmap_find_subtable */
int16_t lc3_tt_cmap_find_subtable(const uint8_t *cmapData, uint32_t dataLen, uint16_t reqPlatform, uint16_t reqEncoding, TTCMapSubtableInfo *info)
{
    if (!cmapData || !info || dataLen < 4) {
        return TT_CMAP_ERR_PARAM;
    }

    uint16_t version = read_u16_be(&cmapData[0]);
    if (version != 0) {
        return TT_CMAP_ERR_FORMAT;
    }

    uint16_t numTables = read_u16_be(&cmapData[2]);
    if (dataLen < 4 + (uint32_t)numTables * 8) {
        return TT_CMAP_ERR_FORMAT;
    }

    uint32_t fallbackOffset = 0;
    bool foundExact = false;
    bool foundFallback = false;

    for (uint16_t i = 0; i < numTables; ++i) {
        const uint8_t *rec = &cmapData[4 + i * 8];
        uint16_t platformID = read_u16_be(&rec[0]);
        uint16_t encodingID = read_u16_be(&rec[2]);
        uint32_t offset     = read_u32_be(&rec[4]);

        if (platformID == reqPlatform && encodingID == reqEncoding) {
            info->platformID = platformID;
            info->encodingID = encodingID;
            info->offset     = offset;
            foundExact = true;
            break;
        } else if (!foundFallback && (platformID == TT_PLATFORM_MACINTOSH || platformID == TT_PLATFORM_MICROSOFT)) {
            fallbackOffset = offset;
            info->platformID = platformID;
            info->encodingID = encodingID;
            info->offset = offset;
            foundFallback = true;
        }
    }

    if (!foundExact && !foundFallback) {
        return TT_CMAP_ERR_NOT_FOUND;
    }

    if (info->offset + 6 > dataLen) {
        return TT_CMAP_ERR_FORMAT;
    }

    const uint8_t *sub = &cmapData[info->offset];
    info->format   = read_u16_be(&sub[0]);
    info->length   = read_u16_be(&sub[2]);
    info->language = read_u16_be(&sub[4]);

    return TT_CMAP_NO_ERR;
}

/* 1411: lc3_tt_cmap_map_format0 */
uint16_t lc3_tt_cmap_map_format0(const uint8_t *subtableData, uint8_t charCode)
{
    if (!subtableData) return 0;
    uint16_t format = read_u16_be(&subtableData[0]);
    if (format != 0) return 0;

    /* Format 0: format(2), length(2), language(2), glyphIndexArray[256] */
    const uint8_t *glyphArray = &subtableData[6];
    return (uint16_t)glyphArray[charCode];
}

/* 1412: lc3_tt_cmap_map_format4 */
uint16_t lc3_tt_cmap_map_format4(const uint8_t *subtableData, uint16_t charCode)
{
    if (!subtableData) return 0;
    uint16_t format = read_u16_be(&subtableData[0]);
    if (format != 4) return 0;

    uint16_t segCountX2 = read_u16_be(&subtableData[6]);
    uint16_t segCount   = segCountX2 / 2;

    const uint8_t *endCodePtr = &subtableData[14];
    const uint8_t *startCodePtr = endCodePtr + segCountX2 + 2; /* +2 reservedPad */
    const uint8_t *idDeltaPtr = startCodePtr + segCountX2;
    const uint8_t *idRangeOffsetPtr = idDeltaPtr + segCountX2;

    for (uint16_t i = 0; i < segCount; ++i) {
        uint16_t endCode   = read_u16_be(&endCodePtr[i * 2]);
        uint16_t startCode = read_u16_be(&startCodePtr[i * 2]);

        if (charCode >= startCode && charCode <= endCode) {
            uint16_t idRangeOffset = read_u16_be(&idRangeOffsetPtr[i * 2]);
            int16_t  idDelta       = (int16_t)read_u16_be(&idDeltaPtr[i * 2]);

            if (idRangeOffset == 0) {
                return (uint16_t)((charCode + idDelta) & 0xFFFF);
            } else {
                /* Offset points into the glyphIdArray */
                const uint8_t *glyphPtr = &idRangeOffsetPtr[i * 2] + idRangeOffset + (charCode - startCode) * 2;
                uint16_t glyphId = read_u16_be(glyphPtr);
                if (glyphId != 0) {
                    return (uint16_t)((glyphId + idDelta) & 0xFFFF);
                }
                return 0;
            }
        }
    }

    return 0; /* Missing glyph (glyph index 0) */
}

/* 1413: lc3_tt_cmap_map_format6 */
uint16_t lc3_tt_cmap_map_format6(const uint8_t *subtableData, uint16_t charCode)
{
    if (!subtableData) return 0;
    uint16_t format = read_u16_be(&subtableData[0]);
    if (format != 6) return 0;

    uint16_t firstCode  = read_u16_be(&subtableData[6]);
    uint16_t entryCount = read_u16_be(&subtableData[8]);

    if (charCode < firstCode || charCode >= (firstCode + entryCount)) {
        return 0;
    }

    uint16_t index = charCode - firstCode;
    const uint8_t *glyphArray = &subtableData[10];
    return read_u16_be(&glyphArray[index * 2]);
}

/* 1414: lc3_tt_cmap_char_to_glyph */
uint16_t lc3_tt_cmap_char_to_glyph(const uint8_t *subtableData, uint32_t charCode)
{
    if (!subtableData) return 0;

    uint16_t format = read_u16_be(&subtableData[0]);
    switch (format) {
        case 0:
            if (charCode > 255) return 0;
            return lc3_tt_cmap_map_format0(subtableData, (uint8_t)charCode);
        case 4:
            if (charCode > 0xFFFF) return 0;
            return lc3_tt_cmap_map_format4(subtableData, (uint16_t)charCode);
        case 6:
            if (charCode > 0xFFFF) return 0;
            return lc3_tt_cmap_map_format6(subtableData, (uint16_t)charCode);
        default:
            return 0;
    }
}

/* 1415: lc3_tt_cmap_get_supported_range */
int16_t lc3_tt_cmap_get_supported_range(const uint8_t *subtableData, uint32_t *minChar, uint32_t *maxChar)
{
    if (!subtableData || !minChar || !maxChar) {
        return TT_CMAP_ERR_PARAM;
    }

    uint16_t format = read_u16_be(&subtableData[0]);
    switch (format) {
        case 0:
            *minChar = 0;
            *maxChar = 255;
            return TT_CMAP_NO_ERR;
        case 4: {
            uint16_t segCountX2 = read_u16_be(&subtableData[6]);
            uint16_t segCount = segCountX2 / 2;
            const uint8_t *endCodePtr = &subtableData[14];
            const uint8_t *startCodePtr = endCodePtr + segCountX2 + 2;

            if (segCount > 0) {
                *minChar = read_u16_be(&startCodePtr[0]);
                *maxChar = read_u16_be(&endCodePtr[(segCount - 1) * 2]);
            } else {
                *minChar = 0;
                *maxChar = 0;
            }
            return TT_CMAP_NO_ERR;
        }
        case 6: {
            uint16_t firstCode  = read_u16_be(&subtableData[6]);
            uint16_t entryCount = read_u16_be(&subtableData[8]);
            *minChar = firstCode;
            *maxChar = (entryCount > 0) ? (firstCode + entryCount - 1) : firstCode;
            return TT_CMAP_NO_ERR;
        }
        default:
            return TT_CMAP_ERR_FORMAT;
    }
}
