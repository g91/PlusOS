/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk RTMP (Routing Table Maintenance Protocol) Router
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_APPLETALK_RTMP_H
#define LC3_APPLETALK_RTMP_H

#include <stdint.h>
#include <stdbool.h>

#define RTMP_NO_ERR         0
#define RTMP_ROUTE_NOT_FND -1085
#define RTMP_PARAM_ERR     -50

typedef struct RTMPEntry {
    uint16_t targetNetStart;
    uint16_t targetNetEnd;
    uint8_t nextRouterNode;
    uint16_t nextRouterNet;
    uint8_t hopCount;
    uint8_t routeState; // 0 = Good, 1 = Suspect, 2 = Bad
    bool inUse;
} RTMPEntry;

/* RTMP API */
int16_t lc3_rtmp_init(uint16_t localNetStart, uint16_t localNetEnd);
int16_t lc3_rtmp_update_route(uint16_t netStart, uint16_t netEnd, uint16_t routerNet, uint8_t routerNode, uint8_t hops);
int16_t lc3_rtmp_lookup_route(uint16_t targetNet, RTMPEntry *outEntry);
int16_t lc3_rtmp_delete_route(uint16_t netStart);
int16_t lc3_rtmp_get_cable_range(uint16_t *netStart, uint16_t *netEnd);
void lc3_rtmp_send_split_horizon(uint8_t outSocket);

#endif /* LC3_APPLETALK_RTMP_H */
