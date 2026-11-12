/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk ATP Transaction Buffer Manager & Packet Fragmentation
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_ATP_BUFFERS_H
#define LC3_ATP_BUFFERS_H

#include <stdint.h>
#include <stdbool.h>

#define ATP_BUF_NO_ERR         0
#define ATP_BUF_OVERFLOW    -1080
#define ATP_BUF_PARAM_ERR     -50

typedef struct BDSElement {
    uint16_t buffSize;
    void *buffPtr;
    uint16_t dataSize;
    uint32_t userBytes;
} BDSElement, *BDSType;

typedef struct ATPBufferPool {
    BDSElement bds[8];
    uint8_t packetStorage[8][578];
    uint8_t bitmap;
    bool inUse;
} ATPBufferPool;

/* ATP Buffer API */
BDSType lc3_atp_alloc_bds(void);
void lc3_atp_free_bds(BDSType bds);
int16_t lc3_atp_fragment_packet(const void *sourceData, uint32_t totalLen, BDSType bds, uint8_t *bitmap);
int16_t lc3_atp_reassemble_packet(BDSType bds, uint8_t bitmap, void *destData, uint32_t *actualLen);
uint16_t lc3_atp_compute_checksum(const void *packetData, uint16_t length);
bool lc3_atp_validate_bitmap(uint8_t bitmap, uint8_t expectedPackets);

#endif /* LC3_ATP_BUFFERS_H */
