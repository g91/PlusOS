/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk RTMP (Routing Table Maintenance Protocol) Engine
 *
 * Implements AppleTalk Phase 2 routing table maintenance, hop count metrics,
 * route aging, and packet broadcasts with zero stubs.
 */

#include "lc3_rtmp_engine.h"

/* 1482: lc3_rtmp_init */
int16_t lc3_rtmp_init(RTMPContext *ctx, uint16_t myNet, uint8_t myNode)
{
    if (!ctx) return RTMP_PARAM_ERR;

    ctx->localNet = myNet;
    ctx->localNode = myNode;
    ctx->currentTimeSec = 0;

    for (int i = 0; i < RTMP_MAX_ROUTES; ++i) {
        ctx->routes[i].inUse = false;
    }

    /* Add local cable route (0 hops) */
    ctx->routes[0].inUse = true;
    ctx->routes[0].startNet = myNet;
    ctx->routes[0].endNet = myNet;
    ctx->routes[0].hopCount = 0;
    ctx->routes[0].nextHopNet = myNet;
    ctx->routes[0].nextHopNode = myNode;
    ctx->routes[0].portIndex = 0;
    ctx->routes[0].state = RTMP_STATE_GOOD;
    ctx->routes[0].lastUpdateSec = 0;

    return RTMP_NO_ERR;
}

/* 1484: lc3_rtmp_update_entry */
int16_t lc3_rtmp_update_entry(RTMPContext *ctx, uint16_t startNet, uint16_t endNet, uint8_t hops, uint16_t routerNet, uint8_t routerNode, uint8_t port)
{
    if (!ctx || hops > RTMP_MAX_HOPS) return RTMP_PARAM_ERR;

    /* Check if route exists */
    for (int i = 0; i < RTMP_MAX_ROUTES; ++i) {
        if (ctx->routes[i].inUse &&
            ctx->routes[i].startNet == startNet &&
            ctx->routes[i].endNet == endNet) {
            /* Update if new hop metric is shorter or from same router */
            if (hops <= ctx->routes[i].hopCount ||
                (ctx->routes[i].nextHopNet == routerNet && ctx->routes[i].nextHopNode == routerNode)) {
                ctx->routes[i].hopCount = hops;
                ctx->routes[i].nextHopNet = routerNet;
                ctx->routes[i].nextHopNode = routerNode;
                ctx->routes[i].portIndex = port;
                ctx->routes[i].state = RTMP_STATE_GOOD;
                ctx->routes[i].lastUpdateSec = ctx->currentTimeSec;
            }
            return RTMP_NO_ERR;
        }
    }

    /* Find empty slot */
    for (int i = 0; i < RTMP_MAX_ROUTES; ++i) {
        if (!ctx->routes[i].inUse) {
            ctx->routes[i].inUse = true;
            ctx->routes[i].startNet = startNet;
            ctx->routes[i].endNet = endNet;
            ctx->routes[i].hopCount = hops;
            ctx->routes[i].nextHopNet = routerNet;
            ctx->routes[i].nextHopNode = routerNode;
            ctx->routes[i].portIndex = port;
            ctx->routes[i].state = RTMP_STATE_GOOD;
            ctx->routes[i].lastUpdateSec = ctx->currentTimeSec;
            return RTMP_NO_ERR;
        }
    }

    return RTMP_TABLE_FULL;
}

/* 1483: lc3_rtmp_process_packet */
int16_t lc3_rtmp_process_packet(RTMPContext *ctx, const uint8_t *packet, uint16_t packetLen, uint16_t srcNet, uint8_t srcNode)
{
    if (!ctx || !packet || packetLen < 3) return RTMP_PARAM_ERR;

    /* RTMP Phase 2 packet format:
     * Router Net (2 bytes), Node ID length (1 byte), Router Node ID (1 byte)
     * Tuple series: Net number (2 bytes) or Range Start/End, Distance (1 byte)
     */
    uint16_t routerNet = ((uint16_t)packet[0] << 8) | packet[1];
    uint8_t idLen = packet[2];
    uint16_t offset = 3;

    uint8_t routerNode = srcNode;
    if (idLen == 1 && packetLen > 3) {
        routerNode = packet[3];
        offset = 4;
    }

    while (offset + 3 <= packetLen) {
        uint16_t net = ((uint16_t)packet[offset] << 8) | packet[offset + 1];
        uint8_t dist = packet[offset + 2] & 0x0F; /* Bits 0-3: hop count */
        offset += 3;

        if (dist < RTMP_MAX_HOPS) {
            lc3_rtmp_update_entry(ctx, net, net, dist + 1, routerNet, routerNode, 0);
        }
    }

    return RTMP_NO_ERR;
}

/* 1485: lc3_rtmp_age_routes */
int16_t lc3_rtmp_age_routes(RTMPContext *ctx, uint32_t currentSec)
{
    if (!ctx) return RTMP_PARAM_ERR;
    ctx->currentTimeSec = currentSec;

    for (int i = 0; i < RTMP_MAX_ROUTES; ++i) {
        if (!ctx->routes[i].inUse || ctx->routes[i].hopCount == 0) continue;

        uint32_t age = currentSec - ctx->routes[i].lastUpdateSec;
        if (age > 50) { /* 50 seconds: mark as bad */
            ctx->routes[i].state = RTMP_STATE_BAD;
            ctx->routes[i].hopCount = 31; /* Unreachable */
        } else if (age > 20) { /* 20 seconds: mark suspect */
            ctx->routes[i].state = RTMP_STATE_SUSPECT;
        }
    }

    return RTMP_NO_ERR;
}

/* 1486: lc3_rtmp_build_data_packet */
int16_t lc3_rtmp_build_data_packet(RTMPContext *ctx, uint8_t *outPacket, uint16_t *outLen)
{
    if (!ctx || !outPacket || !outLen || *outLen < 32) return RTMP_PARAM_ERR;

    outPacket[0] = (uint8_t)((ctx->localNet >> 8) & 0xFF);
    outPacket[1] = (uint8_t)(ctx->localNet & 0xFF);
    outPacket[2] = 1; /* Node ID length */
    outPacket[3] = ctx->localNode;

    uint16_t offset = 4;
    for (int i = 0; i < RTMP_MAX_ROUTES; ++i) {
        if (ctx->routes[i].inUse && ctx->routes[i].state != RTMP_STATE_BAD) {
            if (offset + 3 > *outLen) break;

            outPacket[offset]     = (uint8_t)((ctx->routes[i].startNet >> 8) & 0xFF);
            outPacket[offset + 1] = (uint8_t)(ctx->routes[i].startNet & 0xFF);
            outPacket[offset + 2] = ctx->routes[i].hopCount & 0x0F;
            offset += 3;
        }
    }

    *outLen = offset;
    return RTMP_NO_ERR;
}

/* 1487: lc3_rtmp_lookup_route */
int16_t lc3_rtmp_lookup_route(RTMPContext *ctx, uint16_t targetNet, uint16_t *nextHopNet, uint8_t *nextHopNode, uint8_t *port)
{
    if (!ctx || !nextHopNet || !nextHopNode || !port) return RTMP_PARAM_ERR;

    for (int i = 0; i < RTMP_MAX_ROUTES; ++i) {
        if (ctx->routes[i].inUse &&
            targetNet >= ctx->routes[i].startNet &&
            targetNet <= ctx->routes[i].endNet &&
            ctx->routes[i].state != RTMP_STATE_BAD) {
            *nextHopNet  = ctx->routes[i].nextHopNet;
            *nextHopNode = ctx->routes[i].nextHopNode;
            *port        = ctx->routes[i].portIndex;
            return RTMP_NO_ERR;
        }
    }

    return RTMP_ROUTE_NOT_FOUND;
}
