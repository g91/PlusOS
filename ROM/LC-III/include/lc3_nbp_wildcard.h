/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk NBP (Name-Binding Protocol) Wildcard & Tuple Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_NBP_WILDCARD_H
#define LC3_NBP_WILDCARD_H

#include <stdint.h>
#include <stdbool.h>

#define NBP_NO_ERR         0
#define NBP_BUFFER_FULL   -1
#define NBP_PARAM_ERR    -50

typedef struct NBPEntity {
    char objStr[32];
    char typeStr[32];
    char zoneStr[32];
} NBPEntity;

typedef struct NBPTuple {
    uint16_t net;
    uint8_t node;
    uint8_t socket;
    uint8_t enumerator;
    NBPEntity entity;
} NBPTuple;

/* NBP Wildcard API */
int16_t lc3_nbp_parse_entity(const char *entityStr, NBPEntity *outEntity);
bool lc3_nbp_match_wildcard(const char *pattern, const char *target);
int16_t lc3_nbp_encode_tuple(const NBPTuple *tuple, uint8_t *buffer, uint16_t maxLen, uint16_t *actualLen);
int16_t lc3_nbp_decode_tuple(const uint8_t *buffer, uint16_t bufLen, NBPTuple *tuple, uint16_t *bytesRead);
int16_t lc3_nbp_pack_buffer(const NBPTuple tuples[], uint16_t tupleCount, uint8_t *outBuf, uint16_t maxLen, uint16_t *actualLen);
int16_t lc3_nbp_unpack_buffer(const uint8_t *inBuf, uint16_t inLen, NBPTuple tuples[], uint16_t maxTuples, uint16_t *actualTuples);

#endif /* LC3_NBP_WILDCARD_H */
