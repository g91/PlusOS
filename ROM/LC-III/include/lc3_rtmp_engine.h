/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk RTMP (Routing Table Maintenance Protocol) Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_RTMP_ENGINE_H
#define LC3_RTMP_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

#define RTMP_NO_ERR           0
#define RTMP_PARAM_ERR      -50
#define RTMP_TABLE_FULL     -51
#define RTMP_ROUTE_NOT_FOUND -52

#define RTMP_MAX_ROUTES      32
#define RTMP_MAX_HOPS        15

/* Route entry state */
typedef enum {
    RTMP_STATE_GOOD = 0,
    RTMP_STATE_SUSPECT,
    RTMP_STATE_BAD
} RTMPRouteState;

typedef struct RTMPEntry {
    bool inUse;
    uint16_t startNet;
    uint16_t endNet;
    uint8_t hopCount;
    uint16_t nextHopNet;
    uint8_t nextHopNode;
    uint8_t portIndex;
    RTMPRouteState state;
    uint32_t lastUpdateSec;
} RTMPEntry;

typedef struct RTMPContext {
    RTMPEntry routes[RTMP_MAX_ROUTES];
    uint16_t localNet;
    uint8_t localNode;
    uint32_t currentTimeSec;
} RTMPContext;

/* Public API */
int16_t lc3_rtmp_init(RTMPContext *ctx, uint16_t myNet, uint8_t myNode);
int16_t lc3_rtmp_process_packet(RTMPContext *ctx, const uint8_t *packet, uint16_t packetLen, uint16_t srcNet, uint8_t srcNode);
int16_t lc3_rtmp_update_entry(RTMPContext *ctx, uint16_t startNet, uint16_t endNet, uint8_t hops, uint16_t routerNet, uint8_t routerNode, uint8_t port);
int16_t lc3_rtmp_age_routes(RTMPContext *ctx, uint32_t currentSec);
int16_t lc3_rtmp_build_data_packet(RTMPContext *ctx, uint8_t *outPacket, uint16_t *outLen);
int16_t lc3_rtmp_lookup_route(RTMPContext *ctx, uint16_t targetNet, uint16_t *nextHopNet, uint8_t *nextHopNode, uint8_t *port);

#endif /* LC3_RTMP_ENGINE_H */
