/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk ATP (Transaction Protocol) Packet Validation Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_ATP_VALIDATE_H
#define LC3_ATP_VALIDATE_H

#include <stdint.h>
#include <stdbool.h>

#define ATP_VAL_NO_ERR         0
#define ATP_VAL_BAD_FORMAT    -1
#define ATP_VAL_PARAM_ERR    -50

/* ATP Control Flags */
#define atpReqPkt       0x40
#define atpRespPkt      0x80
#define atpRelPkt       0xC0
#define atpXO           0x20
#define atpEOM          0x10
#define atpSTS          0x08

typedef struct ATPRawHeader {
    uint8_t cmdControl;
    uint8_t bitmapOrSeq;
    uint16_t tid;
    uint32_t userData;
} ATPRawHeader;

/* ATP Validation API */
uint8_t lc3_atp_val_decode_bitmap(uint8_t bitmap, uint8_t *seqNumbers, uint8_t maxSeqs);
uint8_t lc3_atp_val_encode_bitmap(const uint8_t *receivedSeqs, uint8_t seqCount);
int16_t lc3_atp_val_build_header(uint8_t cmdFlags, uint8_t bitmapOrSeq, uint16_t tid, uint32_t userData, ATPRawHeader *outHdr);
bool lc3_atp_val_verify_packet(const ATPRawHeader *hdr, uint16_t totalLen);
uint32_t lc3_atp_val_extract_user_bytes(const ATPRawHeader *hdr);
void lc3_atp_val_set_user_bytes(ATPRawHeader *hdr, uint32_t userData);

#endif /* LC3_ATP_VALIDATE_H */
