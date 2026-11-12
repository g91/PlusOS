/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk Data Stream Protocol (ADSP Core Engine)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_ADSP_ENGINE_H
#define LC3_ADSP_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

#define ADSP_NO_ERR          0
#define ADSP_ERR_CLOSED     -1
#define ADSP_ERR_TIMEOUT    -2
#define ADSP_ERR_PARAM      -50

#define ADSP_STATE_CLOSED    0
#define ADSP_STATE_LISTEN    1
#define ADSP_STATE_OPENING   2
#define ADSP_STATE_OPEN      3
#define ADSP_STATE_CLOSING   4

typedef struct {
    uint16_t aNet;
    uint8_t aNode;
    uint8_t aSocket;
} ADSPAddrBlock;

typedef struct TRCCB {
    struct TRCCB *qLink;
    uint16_t state;
    uint8_t localSocket;
    ADSPAddrBlock remoteAddress;
    uint32_t sendSeq;
    uint32_t recvSeq;
    uint16_t sendWindow;
    uint16_t recvWindow;
    uint8_t sendQueue[2048];
    uint16_t sendHead;
    uint16_t sendTail;
    uint8_t recvQueue[2048];
    uint16_t recvHead;
    uint16_t recvTail;
    uint8_t attnQueue[256];
} TRCCB, *TRCCBPtr;

typedef struct {
    TRCCBPtr ccbPtr;
    uint16_t reqCode;
    int16_t ioResult;
    ADSPAddrBlock uAddress;
    uint16_t reqCount;
    uint16_t actCount;
    void *dataPtr;
    uint16_t attnCode;
    uint16_t attnSize;
    void *attnData;
} DSPParamBlock;

/* ADSP Core API */
int16_t lc3_adsp_init(TRCCBPtr ccb, uint8_t requestedSocket);
int16_t lc3_adsp_open(TRCCBPtr ccb, const ADSPAddrBlock *remoteAddr, bool ocMode);
int16_t lc3_adsp_close(TRCCBPtr ccb, bool abort);
int16_t lc3_adsp_listen(TRCCBPtr ccb);
int16_t lc3_adsp_read(TRCCBPtr ccb, void *buffer, uint16_t reqCount, uint16_t *actCount, bool *eom);
int16_t lc3_adsp_write(TRCCBPtr ccb, const void *buffer, uint16_t reqCount, uint16_t *actCount, bool flush);
int16_t lc3_adsp_status(TRCCBPtr ccb, uint16_t *sendQueueFree, uint16_t *recvQueueAvail);
int16_t lc3_adsp_attention(TRCCBPtr ccb, uint16_t attnCode, const void *attnData, uint16_t attnSize);
int16_t lc3_adsp_reset(TRCCBPtr ccb);

/* ADSP Dispatcher */
int16_t lc3_adsp_dispatch(int16_t selector, DSPParamBlock *pb);

#endif /* LC3_ADSP_ENGINE_H */
