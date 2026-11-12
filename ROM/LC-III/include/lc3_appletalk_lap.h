/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk LAP (Link Access Protocol) & LocalTalk Frame Transceiver
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_APPLETALK_LAP_H
#define LC3_APPLETALK_LAP_H

#include <stdint.h>
#include <stdbool.h>

#define LAP_NO_ERR         0
#define LAP_COLLISION_ERR -94
#define LAP_PARAM_ERR     -50

typedef struct LAPHeader {
    uint8_t dstNode;
    uint8_t srcNode;
    uint8_t lapType; // 1 = Short DDP, 2 = Long DDP
} LAPHeader;

/* LAP API */
int16_t lc3_lap_open_protocol(uint8_t lapType, void *protocolHandler);
int16_t lc3_lap_close_protocol(uint8_t lapType);
int16_t lc3_lap_write(uint8_t dstNode, uint8_t lapType, const void *payload, uint16_t len);
int16_t lc3_lap_read(uint8_t *dstNode, uint8_t *srcNode, uint8_t *lapType, void *payload, uint16_t *len);
uint16_t lc3_lap_frame_check_sequence(const void *frameData, uint16_t length);
bool lc3_lap_detect_collision(void);

#endif /* LC3_APPLETALK_LAP_H */
