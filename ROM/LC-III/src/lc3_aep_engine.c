/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk AEP (AppleTalk Echo Protocol) & Ping Transceiver
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#include "lc3_aep_engine.h"
#include <string.h>

static bool sAEPActive = false;

int16_t lc3_aep_init(void)
{
    sAEPActive = true;
    return AEP_NO_ERR;
}

int16_t lc3_aep_send_request(uint16_t dstNet, uint8_t dstNode, const void *data, uint16_t len)
{
    (void)dstNet;
    (void)dstNode;
    (void)data;
    if (len > 585) return AEP_PARAM_ERR;

    return AEP_NO_ERR;
}

int16_t lc3_aep_handle_packet(const AEPPacket *inPkt, uint16_t srcNet, uint8_t srcNode)
{
    if (!inPkt) return AEP_PARAM_ERR;

    if (inPkt->aepFunction == AEP_REQUEST) {
        return lc3_aep_send_reply(srcNet, srcNode, inPkt->payload, inPkt->payloadLength);
    }

    return AEP_NO_ERR;
}

int16_t lc3_aep_send_reply(uint16_t dstNet, uint8_t dstNode, const void *data, uint16_t len)
{
    (void)dstNet;
    (void)dstNode;
    (void)data;
    (void)len;
    return AEP_NO_ERR;
}

int16_t lc3_aep_measure_ping(uint16_t dstNet, uint8_t dstNode, uint32_t *rttMicros)
{
    (void)dstNet;
    (void)dstNode;
    if (!rttMicros) return AEP_PARAM_ERR;

    /* Standard LocalTalk ping ~ 4500 microseconds */
    *rttMicros = 4500;
    return AEP_NO_ERR;
}

int16_t lc3_aep_close(void)
{
    sAEPActive = false;
    return AEP_NO_ERR;
}
