/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Serial (RS-422) LocalTalk Link Controller & CSMA/CA Pacing
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_LOCALTALK_LINK_H
#define LC3_LOCALTALK_LINK_H

#include <stdint.h>
#include <stdbool.h>

#define LT_NO_ERR         0
#define LT_COLLISION_ERR -94
#define LT_LINE_BUSY     -95
#define LT_TIMEOUT_ERR   -96
#define LT_PARAM_ERR     -50

typedef enum LTLinkState {
    LT_STATE_IDLE = 0,
    LT_STATE_SENSING,
    LT_STATE_DEFERRING,
    LT_STATE_TRANSMITTING,
    LT_STATE_WAIT_CTS
} LTLinkState;

typedef struct LTLinkContext {
    LTLinkState state;
    uint32_t randomSeed;
    uint16_t slotTimeUs;      /* 400 microseconds standard LocalTalk slot */
    uint16_t interPacketGapUs;/* 200 microseconds IFG */
    uint8_t localNodeID;
    bool carrierSenseActive;
    uint32_t totalPacketsSent;
    uint32_t totalCollisions;
} LTLinkContext;

/* LocalTalk Link API */
int16_t lc3_lt_link_init(LTLinkContext *ctx, uint8_t localNodeID);
bool lc3_lt_sense_carrier(LTLinkContext *ctx);
uint16_t lc3_lt_generate_backoff(LTLinkContext *ctx);
int16_t lc3_lt_send_rts(LTLinkContext *ctx, uint8_t destNodeID);
int16_t lc3_lt_wait_cts(LTLinkContext *ctx, uint8_t destNodeID);
int16_t lc3_lt_transmit_frame(LTLinkContext *ctx, uint8_t destNodeID, const void *frameData, uint16_t length);

#endif /* LC3_LOCALTALK_LINK_H */
