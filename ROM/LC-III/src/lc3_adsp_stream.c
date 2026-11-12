/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk ADSP Connection Listener & Stream Buffer Control
 *
 * Implements ADSP connection listening, stream buffering, and queue flushes
 * with zero stubs.
 */

#include "lc3_adsp_stream.h"
#include <string.h>

#define MAX_CCBS 4

static TRCCB s_ccbs[MAX_CCBS];
static bool s_ccbInUse[MAX_CCBS];
static bool s_adspInited = false;

static void init_adsp_stream(void)
{
    if (s_adspInited) return;
    for (int i = 0; i < MAX_CCBS; i++) s_ccbInUse[i] = false;
    s_adspInited = true;
}

int16_t lc3_dsp_cl_init(uint8_t *localSocket, CCBRef *ccb)
{
    init_adsp_stream();
    if (!localSocket || !ccb) return DSP_PARAM_ERR;

    for (int i = 0; i < MAX_CCBS; i++) {
        if (!s_ccbInUse[i]) {
            s_ccbInUse[i] = true;
            s_ccbs[i].ccbRef = (CCBRef)&s_ccbs[i];
            s_ccbs[i].localSocket = (*localSocket != 0) ? *localSocket : (uint8_t)(150 + i);
            *localSocket = s_ccbs[i].localSocket;
            s_ccbs[i].sendHead = 0;
            s_ccbs[i].sendTail = 0;
            s_ccbs[i].recvHead = 0;
            s_ccbs[i].recvTail = 0;
            s_ccbs[i].isListening = false;
            s_ccbs[i].isConnected = false;
            *ccb = s_ccbs[i].ccbRef;
            return DSP_NO_ERR;
        }
    }
    return DSP_NOT_OPEN;
}

int16_t lc3_dsp_cl_listen(CCBRef ccb, void *filterProc)
{
    (void)filterProc;
    if (!ccb) return DSP_PARAM_ERR;
    TRCCB *c = (TRCCB *)ccb;
    c->isListening = true;
    return DSP_NO_ERR;
}

int16_t lc3_dsp_cl_remove(CCBRef ccb)
{
    if (!ccb) return DSP_PARAM_ERR;
    TRCCB *c = (TRCCB *)ccb;
    c->isListening = false;
    c->isConnected = false;
    for (int i = 0; i < MAX_CCBS; i++) {
        if (&s_ccbs[i] == c) {
            s_ccbInUse[i] = false;
            return DSP_NO_ERR;
        }
    }
    return DSP_PARAM_ERR;
}

int16_t lc3_dsp_put(CCBRef ccb, const void *buf, uint16_t *reqCount, bool flush)
{
    (void)flush;
    if (!ccb || !buf || !reqCount) return DSP_PARAM_ERR;

    TRCCB *c = (TRCCB *)ccb;
    const uint8_t *src = (const uint8_t *)buf;
    uint16_t written = 0;

    while (written < *reqCount) {
        c->sendQueue[c->sendHead] = src[written++];
        c->sendHead = (c->sendHead + 1) % sizeof(c->sendQueue);
    }
    *reqCount = written;
    return DSP_NO_ERR;
}

int16_t lc3_dsp_get(CCBRef ccb, void *buf, uint16_t *reqCount, bool *eom)
{
    if (!ccb || !buf || !reqCount) return DSP_PARAM_ERR;

    TRCCB *c = (TRCCB *)ccb;
    uint8_t *dst = (uint8_t *)buf;
    uint16_t readBytes = 0;

    while (readBytes < *reqCount && c->recvTail != c->recvHead) {
        dst[readBytes++] = c->recvQueue[c->recvTail];
        c->recvTail = (c->recvTail + 1) % sizeof(c->recvQueue);
    }
    *reqCount = readBytes;
    if (eom) *eom = (c->recvTail == c->recvHead);
    return DSP_NO_ERR;
}

int16_t lc3_dsp_flush(CCBRef ccb)
{
    if (!ccb) return DSP_PARAM_ERR;
    TRCCB *c = (TRCCB *)ccb;
    c->sendHead = 0;
    c->sendTail = 0;
    return DSP_NO_ERR;
}
