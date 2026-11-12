/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk Zone (ZIP) & Routing (RTMP) Client
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_APPLETALK_ZIP_H
#define LC3_APPLETALK_ZIP_H

#include <stdint.h>
#include <stdbool.h>

#define ZIP_NO_ERR          0
#define ZIP_BAD_ZONE       -1
#define ZIP_NO_ROUTER      -2

typedef struct {
    uint8_t zoneLength;
    char zoneName[32];
} ZIPZoneName;

typedef struct {
    uint16_t thisNet;
    uint8_t thisNode;
    uint16_t routerNet;
    uint8_t routerNode;
    uint16_t netRangeStart;
    uint16_t netRangeEnd;
} RTMPInfo;

/* ZIP and RTMP API */
int16_t lc3_zip_get_my_zone(ZIPZoneName *zone);
int16_t lc3_zip_get_zone_list(void *buffer, int16_t buffSize, int16_t *zoneCount);
int16_t lc3_zip_get_local_zones(void *buffer, int16_t buffSize, int16_t *zoneCount);
int16_t lc3_rtmp_request(RTMPInfo *info);

#endif /* LC3_APPLETALK_ZIP_H */
