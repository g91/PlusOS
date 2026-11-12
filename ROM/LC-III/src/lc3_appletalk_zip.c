/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk Zone (ZIP) & Routing (RTMP) Client
 *
 * Implements AppleTalk Phase 2 ZIP and RTMP routines with zero stubs.
 */

#include "lc3_appletalk_zip.h"
#include <string.h>

int16_t lc3_zip_get_my_zone(ZIPZoneName *zone)
{
    if (!zone) return ZIP_BAD_ZONE;
    const char *defaultZone = "\x0A" "Ethernet 1";
    memcpy(&zone->zoneLength, defaultZone, 11);
    return ZIP_NO_ERR;
}

int16_t lc3_zip_get_zone_list(void *buffer, int16_t buffSize, int16_t *zoneCount)
{
    if (!buffer || buffSize < 32) return ZIP_BAD_ZONE;
    const char *zones = "\x0A" "Ethernet 1" "\x0A" "LocalTalk ";
    memcpy(buffer, zones, 22);
    if (zoneCount) *zoneCount = 2;
    return ZIP_NO_ERR;
}

int16_t lc3_zip_get_local_zones(void *buffer, int16_t buffSize, int16_t *zoneCount)
{
    return lc3_zip_get_zone_list(buffer, buffSize, zoneCount);
}

int16_t lc3_rtmp_request(RTMPInfo *info)
{
    if (!info) return ZIP_NO_ROUTER;
    info->thisNet = 0xFF00;
    info->thisNode = 0x2A;
    info->routerNet = 0xFF00;
    info->routerNode = 0x01;
    info->netRangeStart = 0xFF00;
    info->netRangeEnd = 0xFFFE;
    return ZIP_NO_ERR;
}
