/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk NBP (Name-Binding Protocol) Wildcard & Tuple Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#include "lc3_nbp_wildcard.h"
#include <string.h>
#include <ctype.h>

int16_t lc3_nbp_parse_entity(const char *entityStr, NBPEntity *outEntity)
{
    if (!entityStr || !outEntity) return NBP_PARAM_ERR;

    memset(outEntity, 0, sizeof(NBPEntity));
    const char *colon = strchr(entityStr, ':');
    const char *at = strchr(entityStr, '@');

    if (!colon) {
        strncpy(outEntity->objStr, entityStr, 31);
        strcpy(outEntity->typeStr, "=");
        strcpy(outEntity->zoneStr, "*");
        return NBP_NO_ERR;
    }

    size_t objLen = (size_t)(colon - entityStr);
    if (objLen > 31) objLen = 31;
    strncpy(outEntity->objStr, entityStr, objLen);

    if (at && at > colon) {
        size_t typeLen = (size_t)(at - colon - 1);
        if (typeLen > 31) typeLen = 31;
        strncpy(outEntity->typeStr, colon + 1, typeLen);
        strncpy(outEntity->zoneStr, at + 1, 31);
    } else {
        strncpy(outEntity->typeStr, colon + 1, 31);
        strcpy(outEntity->zoneStr, "*");
    }

    return NBP_NO_ERR;
}

bool lc3_nbp_match_wildcard(const char *pattern, const char *target)
{
    if (!pattern || !target) return false;

    /* '=' matches any string */
    if (strcmp(pattern, "=") == 0 || strcmp(pattern, "*") == 0) {
        return true;
    }

    /* Case-insensitive string match */
    while (*pattern && *target) {
        if (*pattern == (char)0xC5) { /* AppleTalk prefix wildcard \approx */
            return true;
        }
        if (tolower((unsigned char)*pattern) != tolower((unsigned char)*target)) {
            return false;
        }
        pattern++;
        target++;
    }

    if (*pattern == (char)0xC5) return true;
    return (*pattern == '\0' && *target == '\0');
}

int16_t lc3_nbp_encode_tuple(const NBPTuple *tuple, uint8_t *buffer, uint16_t maxLen, uint16_t *actualLen)
{
    if (!tuple || !buffer || !actualLen) return NBP_PARAM_ERR;

    uint8_t oLen = (uint8_t)strlen(tuple->entity.objStr);
    uint8_t tLen = (uint8_t)strlen(tuple->entity.typeStr);
    uint8_t zLen = (uint8_t)strlen(tuple->entity.zoneStr);

    uint16_t total = (uint16_t)(5 + 1 + oLen + 1 + tLen + 1 + zLen);
    if (total > maxLen) return NBP_BUFFER_FULL;

    buffer[0] = (uint8_t)(tuple->net >> 8);
    buffer[1] = (uint8_t)(tuple->net & 0xFF);
    buffer[2] = tuple->node;
    buffer[3] = tuple->socket;
    buffer[4] = tuple->enumerator;

    uint16_t pos = 5;
    buffer[pos++] = oLen;
    memcpy(&buffer[pos], tuple->entity.objStr, oLen);
    pos += oLen;

    buffer[pos++] = tLen;
    memcpy(&buffer[pos], tuple->entity.typeStr, tLen);
    pos += tLen;

    buffer[pos++] = zLen;
    memcpy(&buffer[pos], tuple->entity.zoneStr, zLen);
    pos += zLen;

    *actualLen = pos;
    return NBP_NO_ERR;
}

int16_t lc3_nbp_decode_tuple(const uint8_t *buffer, uint16_t bufLen, NBPTuple *tuple, uint16_t *bytesRead)
{
    if (!buffer || !tuple || !bytesRead || bufLen < 8) return NBP_PARAM_ERR;

    tuple->net = (uint16_t)((buffer[0] << 8) | buffer[1]);
    tuple->node = buffer[2];
    tuple->socket = buffer[3];
    tuple->enumerator = buffer[4];

    uint16_t pos = 5;
    uint8_t oLen = buffer[pos++];
    if (pos + oLen > bufLen) return NBP_PARAM_ERR;
    memcpy(tuple->entity.objStr, &buffer[pos], oLen);
    tuple->entity.objStr[oLen] = '\0';
    pos += oLen;

    if (pos >= bufLen) return NBP_PARAM_ERR;
    uint8_t tLen = buffer[pos++];
    if (pos + tLen > bufLen) return NBP_PARAM_ERR;
    memcpy(tuple->entity.typeStr, &buffer[pos], tLen);
    tuple->entity.typeStr[tLen] = '\0';
    pos += tLen;

    if (pos >= bufLen) return NBP_PARAM_ERR;
    uint8_t zLen = buffer[pos++];
    if (pos + zLen > bufLen) return NBP_PARAM_ERR;
    memcpy(tuple->entity.zoneStr, &buffer[pos], zLen);
    tuple->entity.zoneStr[zLen] = '\0';
    pos += zLen;

    *bytesRead = pos;
    return NBP_NO_ERR;
}

int16_t lc3_nbp_pack_buffer(const NBPTuple tuples[], uint16_t tupleCount, uint8_t *outBuf, uint16_t maxLen, uint16_t *actualLen)
{
    if (!tuples || !outBuf || !actualLen) return NBP_PARAM_ERR;

    uint16_t total = 0;
    for (uint16_t i = 0; i < tupleCount; ++i) {
        uint16_t len = 0;
        int16_t err = lc3_nbp_encode_tuple(&tuples[i], &outBuf[total], maxLen - total, &len);
        if (err != NBP_NO_ERR) return err;
        total += len;
    }

    *actualLen = total;
    return NBP_NO_ERR;
}

int16_t lc3_nbp_unpack_buffer(const uint8_t *inBuf, uint16_t inLen, NBPTuple tuples[], uint16_t maxTuples, uint16_t *actualTuples)
{
    if (!inBuf || !tuples || !actualTuples) return NBP_PARAM_ERR;

    uint16_t count = 0;
    uint16_t pos = 0;

    while (pos < inLen && count < maxTuples) {
        uint16_t readBytes = 0;
        int16_t err = lc3_nbp_decode_tuple(&inBuf[pos], inLen - pos, &tuples[count], &readBytes);
        if (err != NBP_NO_ERR) break;
        pos += readBytes;
        count++;
    }

    *actualTuples = count;
    return NBP_NO_ERR;
}
