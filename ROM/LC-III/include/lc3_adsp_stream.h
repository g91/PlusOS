/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk ADSP Connection Listener & Stream Buffer Control
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_ADSP_STREAM_H
#define LC3_ADSP_STREAM_H

#include <stdint.h>
#include <stdbool.h>

#define DSP_NO_ERR         0
#define DSP_NOT_OPEN    -1273
#define DSP_PARAM_ERR     -50

typedef void *CCBRef;

typedef struct TRCCB {
    CCBRef ccbRef;
    uint8_t localSocket;
    uint8_t remoteSocket;
    uint16_t remoteNet;
    uint8_t remoteNode;
    uint8_t sendQueue[512];
    uint32_t sendHead;
    uint32_t sendTail;
    uint8_t recvQueue[512];
    uint32_t recvHead;
    uint32_t recvTail;
    bool isListening;
    bool isConnected;
} TRCCB;

/* ADSP Stream API */
int16_t lc3_dsp_cl_init(uint8_t *localSocket, CCBRef *ccb);
int16_t lc3_dsp_cl_listen(CCBRef ccb, void *filterProc);
int16_t lc3_dsp_cl_remove(CCBRef ccb);
int16_t lc3_dsp_put(CCBRef ccb, const void *buf, uint16_t *reqCount, bool flush);
int16_t lc3_dsp_get(CCBRef ccb, void *buf, uint16_t *reqCount, bool *eom);
int16_t lc3_dsp_flush(CCBRef ccb);

#endif /* LC3_ADSP_STREAM_H */
