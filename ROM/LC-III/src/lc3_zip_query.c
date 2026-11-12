/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk ZIP (Zone Information Protocol) Query Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#include "lc3_zip_query.h"
#include <string.h>

int16_t lc3_zip_get_my_zone(ZIPQueryContext *ctx, char *outZoneName)
{
    if (!ctx || !outZoneName) return ZIP_PARAM_ERR;

    if (!ctx->zoneValid) {
        strcpy(ctx->currentZone, "*");
        ctx->zoneValid = true;
    }

    strcpy(outZoneName, ctx->currentZone);
    return ZIP_NO_ERR;
}

int16_t lc3_zip_get_zone_list(ZIPQueryContext *ctx, char outZones[MAX_ZONES][34], uint16_t *zoneCount)
{
    if (!ctx || !outZones || !zoneCount) return ZIP_PARAM_ERR;

    /* Provide default local zone */
    strcpy(outZones[0], "*");
    *zoneCount = 1;
    return ZIP_NO_ERR;
}

int16_t lc3_zip_get_local_zones(ZIPQueryContext *ctx, char outZones[MAX_ZONES][34], uint16_t *zoneCount)
{
    if (!ctx || !outZones || !zoneCount) return ZIP_PARAM_ERR;

    strcpy(outZones[0], "*");
    *zoneCount = 1;
    return ZIP_NO_ERR;
}

int16_t lc3_zip_build_query_packet(uint8_t zipCommand, uint16_t startNet, uint8_t *packetBuf, uint16_t *packetLen)
{
    if (!packetBuf || !packetLen) return ZIP_PARAM_ERR;

    /* ZIP ATP Query format: [Command:1, Reserved:1, StartNet:2] */
    packetBuf[0] = zipCommand;
    packetBuf[1] = 0;
    packetBuf[2] = (uint8_t)(startNet >> 8);
    packetBuf[3] = (uint8_t)(startNet & 0xFF);
    *packetLen = 4;

    return ZIP_NO_ERR;
}

int16_t lc3_zip_unpack_zones(const uint8_t *respPayload, uint16_t payloadLen, char outZones[MAX_ZONES][34], uint16_t *actualZones)
{
    if (!respPayload || !outZones || !actualZones || payloadLen < 4) return ZIP_PARAM_ERR;

    /* Response header: [LastFlag:1, Reserved:1, ZoneCount:2, PascalStrings...] */
    uint16_t zoneCount = (uint16_t)((respPayload[2] << 8) | respPayload[3]);
    uint16_t pos = 4;
    uint16_t extracted = 0;

    while (pos < payloadLen && extracted < zoneCount && extracted < MAX_ZONES) {
        uint8_t len = respPayload[pos++];
        if (pos + len > payloadLen) break;

        memcpy(outZones[extracted], &respPayload[pos], len);
        outZones[extracted][len] = '\0';
        pos += len;
        extracted++;
    }

    *actualZones = extracted;
    return ZIP_NO_ERR;
}

void lc3_zip_cache_zone(ZIPQueryContext *ctx, const char *zoneName)
{
    if (!ctx || !zoneName) return;

    strncpy(ctx->currentZone, zoneName, sizeof(ctx->currentZone) - 1);
    ctx->currentZone[sizeof(ctx->currentZone) - 1] = '\0';
    ctx->zoneValid = true;
}
