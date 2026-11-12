/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk DDP (Datagram Delivery Protocol) Phase 2 Routing
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_DDP_ENGINE_H
#define LC3_DDP_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

#define DDP_NO_ERR         0
#define DDP_BAD_CHECKSUM -94
#define DDP_HOP_EXCEEDED -95
#define DDP_PARAM_ERR    -50

typedef struct LongDDPHeader {
    uint16_t lengthAndHops;   /* 6 bits hops (max 15), 10 bits length */
    uint16_t checksum;        /* 16-bit one's complement */
    uint16_t dstNet;
    uint16_t srcNet;
    uint8_t dstNode;
    uint8_t srcNode;
    uint8_t dstSocket;
    uint8_t srcSocket;
    uint8_t ddpType;
} LongDDPHeader;

typedef struct ShortDDPHeader {
    uint16_t length;          /* 10 bits length */
    uint8_t dstSocket;
    uint8_t srcSocket;
    uint8_t ddpType;
} ShortDDPHeader;

/* DDP Engine API */
uint16_t lc3_ddp_calc_checksum(const void *packetData, uint16_t length);
bool lc3_ddp_verify_checksum(const LongDDPHeader *hdr, const void *payload, uint16_t payloadLen);
int16_t lc3_ddp_build_long_header(LongDDPHeader *hdr, uint16_t dstNet, uint8_t dstNode, uint8_t dstSocket,
                                  uint16_t srcNet, uint8_t srcNode, uint8_t srcSocket, uint8_t ddpType, uint16_t dataLen);
int16_t lc3_ddp_build_short_header(ShortDDPHeader *hdr, uint8_t dstSocket, uint8_t srcSocket, uint8_t ddpType, uint16_t dataLen);
int16_t lc3_ddp_route_packet(LongDDPHeader *hdr, uint16_t thisNet, uint8_t *nextHopNode);
int16_t lc3_ddp_dispatch_socket(uint8_t socketNum, const void *payload, uint16_t length);

#endif /* LC3_DDP_ENGINE_H */
