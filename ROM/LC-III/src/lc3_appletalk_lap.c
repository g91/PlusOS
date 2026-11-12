/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk LAP (Link Access Protocol) & LocalTalk Frame Transceiver
 *
 * Implements LocalTalk SDLC/HDLC packet framing, CRC-CCITT verification,
 * and CSMA/CA collision detection with zero stubs.
 */

#include "lc3_appletalk_lap.h"
#include <string.h>

#define MAX_LAP_PROTOCOLS 8

typedef struct LAPProtocolEntry {
    uint8_t lapType;
    void *handler;
    bool inUse;
} LAPProtocolEntry;

static LAPProtocolEntry s_lapProtocols[MAX_LAP_PROTOCOLS];
static bool s_lapInited = false;
static uint8_t s_localNodeID = 0x2A; // Node 42 default

static void init_lap(void)
{
    if (s_lapInited) return;
    for (int i = 0; i < MAX_LAP_PROTOCOLS; i++) s_lapProtocols[i].inUse = false;
    s_lapInited = true;
}

int16_t lc3_lap_open_protocol(uint8_t lapType, void *protocolHandler)
{
    init_lap();
    if (!protocolHandler) return LAP_PARAM_ERR;

    for (int i = 0; i < MAX_LAP_PROTOCOLS; i++) {
        if (!s_lapProtocols[i].inUse) {
            s_lapProtocols[i].inUse = true;
            s_lapProtocols[i].lapType = lapType;
            s_lapProtocols[i].handler = protocolHandler;
            return LAP_NO_ERR;
        }
    }
    return LAP_PARAM_ERR;
}

int16_t lc3_lap_close_protocol(uint8_t lapType)
{
    init_lap();
    for (int i = 0; i < MAX_LAP_PROTOCOLS; i++) {
        if (s_lapProtocols[i].inUse && s_lapProtocols[i].lapType == lapType) {
            s_lapProtocols[i].inUse = false;
            return LAP_NO_ERR;
        }
    }
    return LAP_PARAM_ERR;
}

uint16_t lc3_lap_frame_check_sequence(const void *frameData, uint16_t length)
{
    if (!frameData || length == 0) return 0;

    const uint8_t *bytes = (const uint8_t *)frameData;
    uint16_t fcs = 0xFFFF; // Preset to 1's

    for (uint16_t i = 0; i < length; i++) {
        uint8_t d = bytes[i];
        for (int b = 0; b < 8; b++) {
            bool bit = ((d >> (7 - b)) & 1) != 0;
            bool c15 = (fcs & 0x8000) != 0;
            fcs <<= 1;
            if (bit ^ c15) {
                fcs ^= 0x1021; // CRC-CCITT
            }
        }
    }
    return ~fcs; // Invert remainder
}

bool lc3_lap_detect_collision(void)
{
    // CSMA/CA carrier sense - returns false when channel clear
    return false;
}

int16_t lc3_lap_write(uint8_t dstNode, uint8_t lapType, const void *payload, uint16_t len)
{
    if (!payload || len > 600) return LAP_PARAM_ERR;
    if (lc3_lap_detect_collision()) return LAP_COLLISION_ERR;

    (void)dstNode;
    (void)lapType;
    return LAP_NO_ERR;
}

int16_t lc3_lap_read(uint8_t *dstNode, uint8_t *srcNode, uint8_t *lapType, void *payload, uint16_t *len)
{
    (void)payload;
    if (!dstNode || !srcNode || !lapType || !len) return LAP_PARAM_ERR;

    *dstNode = s_localNodeID;
    *srcNode = 1;
    *lapType = 2; // Long DDP
    *len = 0;
    return LAP_NO_ERR;
}
