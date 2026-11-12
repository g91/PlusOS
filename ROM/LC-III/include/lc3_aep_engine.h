/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk AEP (AppleTalk Echo Protocol) & Ping Transceiver
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_AEP_ENGINE_H
#define LC3_AEP_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

#define AEP_NO_ERR         0
#define AEP_TIMEOUT_ERR -1090
#define AEP_PARAM_ERR     -50

#define AEP_SOCKET          4
#define AEP_REQUEST         1
#define AEP_REPLY           2

typedef struct AEPPacket {
    uint8_t aepFunction;      /* 1 = Request, 2 = Reply */
    uint8_t payload[585];
    uint16_t payloadLength;
} AEPPacket;

/* AEP API */
int16_t lc3_aep_init(void);
int16_t lc3_aep_send_request(uint16_t dstNet, uint8_t dstNode, const void *data, uint16_t len);
int16_t lc3_aep_handle_packet(const AEPPacket *inPkt, uint16_t srcNet, uint8_t srcNode);
int16_t lc3_aep_send_reply(uint16_t dstNet, uint8_t dstNode, const void *data, uint16_t len);
int16_t lc3_aep_measure_ping(uint16_t dstNet, uint8_t dstNode, uint32_t *rttMicros);
int16_t lc3_aep_close(void);

#endif /* LC3_AEP_ENGINE_H */
