/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk ATP (Transaction Protocol) Client Response Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_ATP_CLIENT_H
#define LC3_ATP_CLIENT_H

#include <stdint.h>
#include <stdbool.h>

#define ATP_NO_ERR         0
#define ATP_TIMEOUT_ERR -3101
#define ATP_PARAM_ERR     -50

#define MAX_ATP_PENDING   16

typedef struct ATPTransaction {
    uint16_t tid;
    uint8_t destNode;
    uint8_t destSocket;
    uint8_t bitmap;
    bool exactOnce;
    uint8_t retriesRemaining;
    bool inUse;
} ATPTransaction;

/* ATP Client API */
int16_t lc3_atp_send_request(uint8_t destNode, uint8_t destSocket, const void *reqData, uint16_t reqLen, uint8_t expectPackets, bool exactOnce, uint16_t *outTID);
int16_t lc3_atp_get_response(uint16_t tid, void *respBuffers[], uint16_t bufSizes[], uint8_t numBuffers, uint8_t *actualPackets);
int16_t lc3_atp_send_response(uint16_t tid, uint8_t destNode, uint8_t destSocket, const void *respPackets[], uint16_t packetLens[], uint8_t packetCount);
int16_t lc3_atp_cancel_request(uint16_t tid);
int16_t lc3_atp_cancel_response(uint16_t tid);
bool lc3_atp_check_xo_cache(uint16_t tid, uint8_t srcNode);

#endif /* LC3_ATP_CLIENT_H */
