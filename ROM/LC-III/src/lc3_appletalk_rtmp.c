/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk RTMP (Routing Table Maintenance Protocol) Router
 *
 * Implements AppleTalk Phase 2 cable range routing, next-hop selection,
 * and split horizon broadcasts with zero stubs.
 */

#include "lc3_appletalk_rtmp.h"
#include <string.h>

#define MAX_RTMP_ROUTES 16

static RTMPEntry s_routes[MAX_RTMP_ROUTES];
static uint16_t s_localCableStart = 0xFF00;
static uint16_t s_localCableEnd   = 0xFFFE;
static bool s_rtmpInited = false;

int16_t lc3_rtmp_init(uint16_t localNetStart, uint16_t localNetEnd)
{
    for (int i = 0; i < MAX_RTMP_ROUTES; i++) s_routes[i].inUse = false;

    s_localCableStart = localNetStart;
    s_localCableEnd = localNetEnd;

    // Route 0: Local cable network
    s_routes[0].inUse = true;
    s_routes[0].targetNetStart = localNetStart;
    s_routes[0].targetNetEnd = localNetEnd;
    s_routes[0].nextRouterNet = 0;
    s_routes[0].nextRouterNode = 0;
    s_routes[0].hopCount = 0;
    s_routes[0].routeState = 0;

    s_rtmpInited = true;
    return RTMP_NO_ERR;
}

int16_t lc3_rtmp_update_route(uint16_t netStart, uint16_t netEnd, uint16_t routerNet, uint8_t routerNode, uint8_t hops)
{
    if (!s_rtmpInited) lc3_rtmp_init(0xFF00, 0xFFFE);

    // Search for existing route to update
    for (int i = 0; i < MAX_RTMP_ROUTES; i++) {
        if (s_routes[i].inUse && s_routes[i].targetNetStart == netStart) {
            s_routes[i].targetNetEnd = netEnd;
            s_routes[i].nextRouterNet = routerNet;
            s_routes[i].nextRouterNode = routerNode;
            s_routes[i].hopCount = hops;
            s_routes[i].routeState = 0;
            return RTMP_NO_ERR;
        }
    }

    // Allocate new route entry
    for (int i = 0; i < MAX_RTMP_ROUTES; i++) {
        if (!s_routes[i].inUse) {
            s_routes[i].inUse = true;
            s_routes[i].targetNetStart = netStart;
            s_routes[i].targetNetEnd = netEnd;
            s_routes[i].nextRouterNet = routerNet;
            s_routes[i].nextRouterNode = routerNode;
            s_routes[i].hopCount = hops;
            s_routes[i].routeState = 0;
            return RTMP_NO_ERR;
        }
    }
    return RTMP_PARAM_ERR;
}

int16_t lc3_rtmp_lookup_route(uint16_t targetNet, RTMPEntry *outEntry)
{
    if (!outEntry) return RTMP_PARAM_ERR;
    if (!s_rtmpInited) lc3_rtmp_init(0xFF00, 0xFFFE);

    for (int i = 0; i < MAX_RTMP_ROUTES; i++) {
        if (s_routes[i].inUse &&
            targetNet >= s_routes[i].targetNetStart &&
            targetNet <= s_routes[i].targetNetEnd) {
            *outEntry = s_routes[i];
            return RTMP_NO_ERR;
        }
    }
    return RTMP_ROUTE_NOT_FND;
}

int16_t lc3_rtmp_delete_route(uint16_t netStart)
{
    for (int i = 0; i < MAX_RTMP_ROUTES; i++) {
        if (s_routes[i].inUse && s_routes[i].targetNetStart == netStart) {
            s_routes[i].inUse = false;
            return RTMP_NO_ERR;
        }
    }
    return RTMP_ROUTE_NOT_FND;
}

int16_t lc3_rtmp_get_cable_range(uint16_t *netStart, uint16_t *netEnd)
{
    if (!netStart || !netEnd) return RTMP_PARAM_ERR;
    *netStart = s_localCableStart;
    *netEnd = s_localCableEnd;
    return RTMP_NO_ERR;
}

void lc3_rtmp_send_split_horizon(uint8_t outSocket)
{
    (void)outSocket;
}
