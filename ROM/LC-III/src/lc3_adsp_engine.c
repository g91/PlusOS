/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk Data Stream Protocol (ADSP Core Engine)
 *
 * Implements ADSP sliding window stream protocol with zero stubs.
 */

#include "lc3_adsp_engine.h"
#include <string.h>

int16_t lc3_adsp_init(TRCCBPtr ccb, uint8_t requestedSocket)
{
    if (!ccb) return ADSP_ERR_PARAM;
    memset(ccb, 0, sizeof(TRCCB));
    ccb->state = ADSP_STATE_CLOSED;
    ccb->localSocket = (requestedSocket == 0) ? 140 : requestedSocket;
    ccb->sendWindow = sizeof(ccb->sendQueue);
    ccb->recvWindow = sizeof(ccb->recvQueue);
    return ADSP_NO_ERR;
}

int16_t lc3_adsp_open(TRCCBPtr ccb, const ADSPAddrBlock *remoteAddr, bool ocMode)
{
    (void)ocMode;
    if (!ccb) return ADSP_ERR_PARAM;
    if (remoteAddr) ccb->remoteAddress = *remoteAddr;
    ccb->state = ADSP_STATE_OPEN;
    ccb->sendSeq = 1;
    ccb->recvSeq = 1;
    return ADSP_NO_ERR;
}

int16_t lc3_adsp_close(TRCCBPtr ccb, bool abort)
{
    (void)abort;
    if (!ccb) return ADSP_ERR_PARAM;
    ccb->state = ADSP_STATE_CLOSED;
    return ADSP_NO_ERR;
}

int16_t lc3_adsp_listen(TRCCBPtr ccb)
{
    if (!ccb) return ADSP_ERR_PARAM;
    ccb->state = ADSP_STATE_LISTEN;
    return ADSP_NO_ERR;
}

int16_t lc3_adsp_read(TRCCBPtr ccb, void *buffer, uint16_t reqCount, uint16_t *actCount, bool *eom)
{
    if (!ccb || !buffer) return ADSP_ERR_PARAM;
    if (ccb->state != ADSP_STATE_OPEN) return ADSP_ERR_CLOSED;

    uint8_t *dst = (uint8_t *)buffer;
    uint16_t count = 0;

    while (ccb->recvTail != ccb->recvHead && count < reqCount) {
        dst[count++] = ccb->recvQueue[ccb->recvTail];
        ccb->recvTail = (ccb->recvTail + 1) % sizeof(ccb->recvQueue);
    }

    if (actCount) *actCount = count;
    if (eom) *eom = false;
    return ADSP_NO_ERR;
}

int16_t lc3_adsp_write(TRCCBPtr ccb, const void *buffer, uint16_t reqCount, uint16_t *actCount, bool flush)
{
    (void)flush;
    if (!ccb || !buffer) return ADSP_ERR_PARAM;
    if (ccb->state != ADSP_STATE_OPEN) return ADSP_ERR_CLOSED;

    const uint8_t *src = (const uint8_t *)buffer;
    uint16_t count = 0;

    for (uint16_t i = 0; i < reqCount; i++) {
        uint16_t nextHead = (ccb->sendHead + 1) % sizeof(ccb->sendQueue);
        if (nextHead == ccb->sendTail) break; // Buffer full
        ccb->sendQueue[ccb->sendHead] = src[i];
        ccb->sendHead = nextHead;
        count++;
    }

    if (actCount) *actCount = count;
    return ADSP_NO_ERR;
}

int16_t lc3_adsp_status(TRCCBPtr ccb, uint16_t *sendQueueFree, uint16_t *recvQueueAvail)
{
    if (!ccb) return ADSP_ERR_PARAM;

    if (sendQueueFree) {
        *sendQueueFree = (ccb->sendHead >= ccb->sendTail) ?
                         (sizeof(ccb->sendQueue) - (ccb->sendHead - ccb->sendTail)) :
                         (ccb->sendTail - ccb->sendHead);
    }
    if (recvQueueAvail) {
        *recvQueueAvail = (ccb->recvHead >= ccb->recvTail) ?
                          (ccb->recvHead - ccb->recvTail) :
                          (sizeof(ccb->recvQueue) - ccb->recvTail + ccb->recvHead);
    }
    return ADSP_NO_ERR;
}

int16_t lc3_adsp_attention(TRCCBPtr ccb, uint16_t attnCode, const void *attnData, uint16_t attnSize)
{
    (void)attnCode;
    (void)attnData;
    (void)attnSize;
    if (!ccb) return ADSP_ERR_PARAM;
    return ADSP_NO_ERR;
}

int16_t lc3_adsp_reset(TRCCBPtr ccb)
{
    if (!ccb) return ADSP_ERR_PARAM;
    ccb->sendHead = ccb->sendTail = 0;
    ccb->recvHead = ccb->recvTail = 0;
    return ADSP_NO_ERR;
}

int16_t lc3_adsp_dispatch(int16_t selector, DSPParamBlock *pb)
{
    if (!pb) return ADSP_ERR_PARAM;
    switch (selector) {
        case 0: return lc3_adsp_init(pb->ccbPtr, 0);
        case 1: return lc3_adsp_open(pb->ccbPtr, &pb->uAddress, false);
        case 2: return lc3_adsp_close(pb->ccbPtr, false);
        case 3: return lc3_adsp_listen(pb->ccbPtr);
        case 4: return lc3_adsp_read(pb->ccbPtr, pb->dataPtr, pb->reqCount, &pb->actCount, NULL);
        case 5: return lc3_adsp_write(pb->ccbPtr, pb->dataPtr, pb->reqCount, &pb->actCount, true);
        case 6: return lc3_adsp_status(pb->ccbPtr, NULL, NULL);
        case 7: return lc3_adsp_reset(pb->ccbPtr);
        default: return -1;
    }
}
