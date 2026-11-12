/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk ATP Transaction Buffer Manager & Packet Fragmentation
 *
 * Implements BDS allocation, multi-packet response fragmentation, and DDP checksums
 * with zero stubs.
 */

#include "lc3_atp_buffers.h"
#include <string.h>

#define MAX_BDS_POOLS 4

static ATPBufferPool s_bdsPools[MAX_BDS_POOLS];
static bool s_atpBufInited = false;

static void init_atp_buffers(void)
{
    if (s_atpBufInited) return;
    for (int i = 0; i < MAX_BDS_POOLS; i++) {
        s_bdsPools[i].inUse = false;
        for (int p = 0; p < 8; p++) {
            s_bdsPools[i].bds[p].buffPtr = s_bdsPools[i].packetStorage[p];
            s_bdsPools[i].bds[p].buffSize = 578;
            s_bdsPools[i].bds[p].dataSize = 0;
            s_bdsPools[i].bds[p].userBytes = 0;
        }
    }
    s_atpBufInited = true;
}

BDSType lc3_atp_alloc_bds(void)
{
    init_atp_buffers();
    for (int i = 0; i < MAX_BDS_POOLS; i++) {
        if (!s_bdsPools[i].inUse) {
            s_bdsPools[i].inUse = true;
            s_bdsPools[i].bitmap = 0;
            return s_bdsPools[i].bds;
        }
    }
    return NULL;
}

void lc3_atp_free_bds(BDSType bds)
{
    init_atp_buffers();
    if (!bds) return;
    for (int i = 0; i < MAX_BDS_POOLS; i++) {
        if (s_bdsPools[i].bds == bds) {
            s_bdsPools[i].inUse = false;
            return;
        }
    }
}

int16_t lc3_atp_fragment_packet(const void *sourceData, uint32_t totalLen, BDSType bds, uint8_t *bitmap)
{
    if (!sourceData || !bds || !bitmap) return ATP_BUF_PARAM_ERR;
    if (totalLen > 8 * 578) return ATP_BUF_OVERFLOW;

    const uint8_t *src = (const uint8_t *)sourceData;
    uint32_t remaining = totalLen;
    uint8_t bm = 0;

    for (int i = 0; i < 8 && remaining > 0; i++) {
        uint16_t chunk = (remaining > 578) ? 578 : (uint16_t)remaining;
        memcpy(bds[i].buffPtr, src, chunk);
        bds[i].dataSize = chunk;
        src += chunk;
        remaining -= chunk;
        bm |= (1 << i);
    }

    *bitmap = bm;
    return ATP_BUF_NO_ERR;
}

int16_t lc3_atp_reassemble_packet(BDSType bds, uint8_t bitmap, void *destData, uint32_t *actualLen)
{
    if (!bds || !destData || !actualLen) return ATP_BUF_PARAM_ERR;

    uint8_t *dst = (uint8_t *)destData;
    uint32_t totalCopied = 0;

    for (int i = 0; i < 8; i++) {
        if (bitmap & (1 << i)) {
            memcpy(dst + totalCopied, bds[i].buffPtr, bds[i].dataSize);
            totalCopied += bds[i].dataSize;
        }
    }

    *actualLen = totalCopied;
    return ATP_BUF_NO_ERR;
}

uint16_t lc3_atp_compute_checksum(const void *packetData, uint16_t length)
{
    if (!packetData || length == 0) return 0;

    const uint8_t *bytes = (const uint8_t *)packetData;
    uint32_t sum = 0;

    for (uint16_t i = 0; i < length; i++) {
        sum = (sum + bytes[i]) << 1;
        if (sum >= 0x00010000) {
            sum = (sum & 0xFFFF) + 1;
        }
    }

    if (sum == 0) sum = 0xFFFF;
    return (uint16_t)sum;
}

bool lc3_atp_validate_bitmap(uint8_t bitmap, uint8_t expectedPackets)
{
    uint8_t expectedMask = (1 << expectedPackets) - 1;
    return ((bitmap & expectedMask) == expectedMask);
}
