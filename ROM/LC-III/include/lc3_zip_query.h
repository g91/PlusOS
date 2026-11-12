/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk ZIP (Zone Information Protocol) Query Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_ZIP_QUERY_H
#define LC3_ZIP_QUERY_H

#include <stdint.h>
#include <stdbool.h>

#define ZIP_NO_ERR         0
#define ZIP_TIMEOUT_ERR -1090
#define ZIP_PARAM_ERR     -50

#define ZIP_SOCKET          6
#define ZIP_CMD_GET_MY_ZONE     7
#define ZIP_CMD_GET_ZONE_LIST   8
#define ZIP_CMD_GET_LOCAL_ZONES 9

#define MAX_ZONES          32

typedef struct ZIPQueryContext {
    char currentZone[34];     /* Pascal string buffer */
    char zoneList[MAX_ZONES][34];
    uint16_t numZones;
    bool zoneValid;
} ZIPQueryContext;

/* ZIP Query API */
int16_t lc3_zip_get_my_zone(ZIPQueryContext *ctx, char *outZoneName);
int16_t lc3_zip_get_zone_list(ZIPQueryContext *ctx, char outZones[MAX_ZONES][34], uint16_t *zoneCount);
int16_t lc3_zip_get_local_zones(ZIPQueryContext *ctx, char outZones[MAX_ZONES][34], uint16_t *zoneCount);
int16_t lc3_zip_build_query_packet(uint8_t zipCommand, uint16_t startNet, uint8_t *packetBuf, uint16_t *packetLen);
int16_t lc3_zip_unpack_zones(const uint8_t *respPayload, uint16_t payloadLen, char outZones[MAX_ZONES][34], uint16_t *actualZones);
void lc3_zip_cache_zone(ZIPQueryContext *ctx, const char *zoneName);

#endif /* LC3_ZIP_QUERY_H */
