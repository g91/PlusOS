/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk ATP (Transaction Protocol) Packet Validation Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#include "lc3_atp_validate.h"
#include <string.h>

uint8_t lc3_atp_val_decode_bitmap(uint8_t bitmap, uint8_t *seqNumbers, uint8_t maxSeqs)
{
    if (!seqNumbers || maxSeqs == 0) return 0;

    uint8_t count = 0;
    for (uint8_t i = 0; i < 8 && count < maxSeqs; ++i) {
        if (bitmap & (1 << i)) {
            seqNumbers[count++] = i;
        }
    }

    return count;
}

uint8_t lc3_atp_val_encode_bitmap(const uint8_t *receivedSeqs, uint8_t seqCount)
{
    if (!receivedSeqs || seqCount == 0) return 0;

    uint8_t mask = 0;
    for (uint8_t i = 0; i < seqCount; ++i) {
        if (receivedSeqs[i] < 8) {
            mask |= (1 << receivedSeqs[i]);
        }
    }

    return mask;
}

int16_t lc3_atp_val_build_header(uint8_t cmdFlags, uint8_t bitmapOrSeq, uint16_t tid, uint32_t userData, ATPRawHeader *outHdr)
{
    if (!outHdr) return ATP_VAL_PARAM_ERR;

    outHdr->cmdControl = cmdFlags;
    outHdr->bitmapOrSeq = bitmapOrSeq;
    outHdr->tid = tid;
    outHdr->userData = userData;

    return ATP_VAL_NO_ERR;
}

bool lc3_atp_val_verify_packet(const ATPRawHeader *hdr, uint16_t totalLen)
{
    if (!hdr || totalLen < sizeof(ATPRawHeader)) return false;

    uint8_t func = hdr->cmdControl & 0xC0;
    if (func != atpReqPkt && func != atpRespPkt && func != atpRelPkt) {
        return false;
    }

    return true;
}

uint32_t lc3_atp_val_extract_user_bytes(const ATPRawHeader *hdr)
{
    if (!hdr) return 0;
    return hdr->userData;
}

void lc3_atp_val_set_user_bytes(ATPRawHeader *hdr, uint32_t userData)
{
    if (!hdr) return;
    hdr->userData = userData;
}
