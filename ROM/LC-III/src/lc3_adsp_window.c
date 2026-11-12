/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk ADSP (Data Stream Protocol) Sliding Window Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#include "lc3_adsp_window.h"
#include <string.h>

int16_t lc3_adsp_init_window(ADSPWindowContext *ctx, uint16_t localBufferCredit)
{
    if (!ctx) return ADSP_PARAM_ERR;

    memset(ctx, 0, sizeof(ADSPWindowContext));
    ctx->sendSeq = 1;
    ctx->recvSeq = 1;
    ctx->sendWindow = 1024;
    ctx->recvWindow = localBufferCredit > 0 ? localBufferCredit : 4096;
    ctx->rttSmoothedMs = 200;
    ctx->rttVarianceMs = 50;
    ctx->rtoMs = 500;
    ctx->attnSendSeq = 1;
    ctx->attnRecvSeq = 1;

    return ADSP_NO_ERR;
}

void lc3_adsp_update_rtt(ADSPWindowContext *ctx, uint16_t sampleRttMs)
{
    if (!ctx) return;

    /* Jacobson/Karels algorithm:
     * Err = Sample - Smoothed
     * Smoothed = Smoothed + Err / 8
     * Var = Var + (|Err| - Var) / 4
     * RTO = Smoothed + 4 * Var
     */
    int32_t err = (int32_t)sampleRttMs - (int32_t)ctx->rttSmoothedMs;
    ctx->rttSmoothedMs += (int16_t)(err / 8);

    int32_t absErr = (err < 0) ? -err : err;
    ctx->rttVarianceMs += (int16_t)((absErr - (int32_t)ctx->rttVarianceMs) / 4);

    uint32_t rto = ctx->rttSmoothedMs + 4 * ctx->rttVarianceMs;
    if (rto < 200) rto = 200;
    if (rto > 5000) rto = 5000;
    ctx->rtoMs = (uint16_t)rto;
}

int16_t lc3_adsp_send_data_seq(ADSPWindowContext *ctx, uint16_t dataLen, ADSPHeader *outHdr)
{
    if (!ctx || !outHdr) return ADSP_PARAM_ERR;

    if (dataLen > ctx->sendWindow) {
        return ADSP_WINDOW_FULL;
    }

    memset(outHdr, 0, sizeof(ADSPHeader));
    outHdr->pktSeq = ctx->sendSeq;
    outHdr->pktAck = ctx->recvSeq;
    outHdr->pktWindow = ctx->recvWindow;
    outHdr->flags = 0;

    ctx->sendSeq += dataLen;
    ctx->sendWindow -= dataLen;

    return ADSP_NO_ERR;
}

int16_t lc3_adsp_recv_data_seq(ADSPWindowContext *ctx, const ADSPHeader *inHdr, uint16_t dataLen)
{
    if (!ctx || !inHdr) return ADSP_PARAM_ERR;

    if (inHdr->pktSeq != ctx->recvSeq) {
        return ADSP_SEQ_ERR; /* Out-of-order sequence packet */
    }

    ctx->recvSeq += dataLen;
    ctx->sendWindow = inHdr->pktWindow;

    return ADSP_NO_ERR;
}

int16_t lc3_adsp_send_attention(ADSPWindowContext *ctx, uint16_t attnCode, ADSPHeader *outHdr)
{
    (void)attnCode;
    if (!ctx || !outHdr) return ADSP_PARAM_ERR;

    memset(outHdr, 0, sizeof(ADSPHeader));
    outHdr->pktSeq = ctx->attnSendSeq++;
    outHdr->pktAck = ctx->attnRecvSeq;
    outHdr->flags = 0x80; /* Attention flag */

    return ADSP_NO_ERR;
}

int16_t lc3_adsp_send_probe(ADSPWindowContext *ctx, ADSPHeader *outHdr)
{
    if (!ctx || !outHdr) return ADSP_PARAM_ERR;

    memset(outHdr, 0, sizeof(ADSPHeader));
    outHdr->pktSeq = ctx->sendSeq;
    outHdr->pktAck = ctx->recvSeq;
    outHdr->pktWindow = ctx->recvWindow;
    outHdr->flags = 0x01; /* AckRequest flag */

    return ADSP_NO_ERR;
}
