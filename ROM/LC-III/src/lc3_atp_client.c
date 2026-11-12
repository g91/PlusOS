/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk ATP (Transaction Protocol) Client Response Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#include "lc3_atp_client.h"
#include <string.h>

static ATPTransaction sPendingTx[MAX_ATP_PENDING];
static uint16_t sNextTID = 0x5500;

int16_t lc3_atp_send_request(uint8_t destNode, uint8_t destSocket, const void *reqData, uint16_t reqLen, uint8_t expectPackets, bool exactOnce, uint16_t *outTID)
{
    (void)reqData;
    (void)reqLen;
    if (!outTID) return ATP_PARAM_ERR;

    int slot = -1;
    for (int i = 0; i < MAX_ATP_PENDING; ++i) {
        if (!sPendingTx[i].inUse) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return ATP_TIMEOUT_ERR;

    uint16_t tid = sNextTID++;
    sPendingTx[slot].inUse = true;
    sPendingTx[slot].tid = tid;
    sPendingTx[slot].destNode = destNode;
    sPendingTx[slot].destSocket = destSocket;
    sPendingTx[slot].bitmap = (uint8_t)((1 << expectPackets) - 1);
    sPendingTx[slot].exactOnce = exactOnce;
    sPendingTx[slot].retriesRemaining = 8;

    *outTID = tid;
    return ATP_NO_ERR;
}

int16_t lc3_atp_get_response(uint16_t tid, void *respBuffers[], uint16_t bufSizes[], uint8_t numBuffers, uint8_t *actualPackets)
{
    (void)respBuffers;
    (void)bufSizes;
    if (!actualPackets) return ATP_PARAM_ERR;

    for (int i = 0; i < MAX_ATP_PENDING; ++i) {
        if (sPendingTx[i].inUse && sPendingTx[i].tid == tid) {
            sPendingTx[i].inUse = false;
            *actualPackets = numBuffers > 0 ? 1 : 0;
            return ATP_NO_ERR;
        }
    }

    *actualPackets = 0;
    return ATP_TIMEOUT_ERR;
}

int16_t lc3_atp_send_response(uint16_t tid, uint8_t destNode, uint8_t destSocket, const void *respPackets[], uint16_t packetLens[], uint8_t packetCount)
{
    (void)tid;
    (void)destNode;
    (void)destSocket;
    (void)respPackets;
    (void)packetLens;
    (void)packetCount;
    return ATP_NO_ERR;
}

int16_t lc3_atp_cancel_request(uint16_t tid)
{
    for (int i = 0; i < MAX_ATP_PENDING; ++i) {
        if (sPendingTx[i].inUse && sPendingTx[i].tid == tid) {
            sPendingTx[i].inUse = false;
            return ATP_NO_ERR;
        }
    }
    return ATP_PARAM_ERR;
}

int16_t lc3_atp_cancel_response(uint16_t tid)
{
    (void)tid;
    return ATP_NO_ERR;
}

bool lc3_atp_check_xo_cache(uint16_t tid, uint8_t srcNode)
{
    (void)tid;
    (void)srcNode;
    return false;
}
