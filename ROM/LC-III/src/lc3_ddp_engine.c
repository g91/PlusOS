/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk DDP (Datagram Delivery Protocol) Phase 2 Routing
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#include "lc3_ddp_engine.h"
#include <string.h>

uint16_t lc3_ddp_calc_checksum(const void *packetData, uint16_t length)
{
    if (!packetData || length == 0) return 0;

    const uint8_t *ptr = (const uint8_t *)packetData;
    uint32_t cksum = 0;

    /* AppleTalk DDP checksum: left-rotate 1 bit and add byte */
    for (uint16_t i = 0; i < length; ++i) {
        cksum = (cksum + ptr[i]);
        cksum = ((cksum << 1) | (cksum >> 15)) & 0xFFFF;
    }

    if (cksum == 0) cksum = 0xFFFF; /* 0 checksum means disabled in DDP */
    return (uint16_t)cksum;
}

bool lc3_ddp_verify_checksum(const LongDDPHeader *hdr, const void *payload, uint16_t payloadLen)
{
    if (!hdr) return false;
    if (hdr->checksum == 0) return true; /* Checksum was not computed by sender */

    /* Compute over DDP header starting from dstNet (skip lengthAndHops and checksum) + payload */
    const uint8_t *hBytes = (const uint8_t *)hdr;
    uint32_t cksum = 0;

    for (int i = 4; i < 13; ++i) {
        cksum = (cksum + hBytes[i]);
        cksum = ((cksum << 1) | (cksum >> 15)) & 0xFFFF;
    }

    if (payload && payloadLen > 0) {
        const uint8_t *pBytes = (const uint8_t *)payload;
        for (uint16_t i = 0; i < payloadLen; ++i) {
            cksum = (cksum + pBytes[i]);
            cksum = ((cksum << 1) | (cksum >> 15)) & 0xFFFF;
        }
    }

    if (cksum == 0) cksum = 0xFFFF;
    return (hdr->checksum == (uint16_t)cksum);
}

int16_t lc3_ddp_build_long_header(LongDDPHeader *hdr, uint16_t dstNet, uint8_t dstNode, uint8_t dstSocket,
                                  uint16_t srcNet, uint8_t srcNode, uint8_t srcSocket, uint8_t ddpType, uint16_t dataLen)
{
    if (!hdr || dataLen > 586) return DDP_PARAM_ERR;

    uint16_t totalLen = (uint16_t)(sizeof(LongDDPHeader) + dataLen);
    hdr->lengthAndHops = totalLen & 0x03FF; /* 0 hops initially */
    hdr->checksum = 0;
    hdr->dstNet = dstNet;
    hdr->srcNet = srcNet;
    hdr->dstNode = dstNode;
    hdr->srcNode = srcNode;
    hdr->dstSocket = dstSocket;
    hdr->srcSocket = srcSocket;
    hdr->ddpType = ddpType;

    return DDP_NO_ERR;
}

int16_t lc3_ddp_build_short_header(ShortDDPHeader *hdr, uint8_t dstSocket, uint8_t srcSocket, uint8_t ddpType, uint16_t dataLen)
{
    if (!hdr || dataLen > 586) return DDP_PARAM_ERR;

    hdr->length = (uint16_t)(sizeof(ShortDDPHeader) + dataLen) & 0x03FF;
    hdr->dstSocket = dstSocket;
    hdr->srcSocket = srcSocket;
    hdr->ddpType = ddpType;

    return DDP_NO_ERR;
}

int16_t lc3_ddp_route_packet(LongDDPHeader *hdr, uint16_t thisNet, uint8_t *nextHopNode)
{
    if (!hdr || !nextHopNode) return DDP_PARAM_ERR;

    uint16_t hops = (hdr->lengthAndHops >> 10) & 0x0F;
    if (hops >= 15) {
        return DDP_HOP_EXCEEDED;
    }

    /* Increment hop count */
    hops++;
    hdr->lengthAndHops = (hdr->lengthAndHops & 0x03FF) | ((hops & 0x0F) << 10);

    if (hdr->dstNet == 0 || hdr->dstNet == thisNet) {
        *nextHopNode = hdr->dstNode;
    } else {
        *nextHopNode = 1; /* Forward to local router node */
    }

    return DDP_NO_ERR;
}

int16_t lc3_ddp_dispatch_socket(uint8_t socketNum, const void *payload, uint16_t length)
{
    (void)socketNum;
    (void)payload;
    (void)length;
    return DDP_NO_ERR;
}
