/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk ADSP (Data Stream Protocol) Sliding Window Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_ADSP_WINDOW_H
#define LC3_ADSP_WINDOW_H

#include <stdint.h>
#include <stdbool.h>

#define ADSP_NO_ERR         0
#define ADSP_SEQ_ERR       -1
#define ADSP_WINDOW_FULL   -2
#define ADSP_PARAM_ERR    -50

typedef struct ADSPWindowContext {
    uint32_t sendSeq;         /* Next outgoing byte sequence number */
    uint32_t recvSeq;         /* Next expected incoming byte sequence number */
    uint16_t sendWindow;      /* Remote receiver available credit */
    uint16_t recvWindow;      /* Local receiver buffer credit */
    uint16_t rttSmoothedMs;   /* Smoothed round-trip time */
    uint16_t rttVarianceMs;   /* Round-trip time variation */
    uint16_t rtoMs;           /* Retransmission timeout */
    uint32_t attnSendSeq;     /* Expedited attention sequence number */
    uint32_t attnRecvSeq;
} ADSPWindowContext;

typedef struct ADSPHeader {
    uint16_t srcConnID;
    uint16_t dstConnID;
    uint32_t pktSeq;
    uint32_t pktAck;
    uint16_t pktWindow;
    uint8_t flags;            /* Control, AckRequest, EOM, Attention */
    uint8_t version;
} ADSPHeader;

/* ADSP Window API */
int16_t lc3_adsp_init_window(ADSPWindowContext *ctx, uint16_t localBufferCredit);
void lc3_adsp_update_rtt(ADSPWindowContext *ctx, uint16_t sampleRttMs);
int16_t lc3_adsp_send_data_seq(ADSPWindowContext *ctx, uint16_t dataLen, ADSPHeader *outHdr);
int16_t lc3_adsp_recv_data_seq(ADSPWindowContext *ctx, const ADSPHeader *inHdr, uint16_t dataLen);
int16_t lc3_adsp_send_attention(ADSPWindowContext *ctx, uint16_t attnCode, ADSPHeader *outHdr);
int16_t lc3_adsp_send_probe(ADSPWindowContext *ctx, ADSPHeader *outHdr);

#endif /* LC3_ADSP_WINDOW_H */
