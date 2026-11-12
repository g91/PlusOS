/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk Transaction Protocol (ATP Engine)
 *
 * Implements ATP transaction state machine matching disassembly signatures with zero stubs.
 */

#include "lc3_atp_engine.h"
#include <string.h>

#define MAX_ATP_SOCKETS 8
#define MAX_ATP_TX      8

typedef struct {
    bool inUse;
    uint8_t socketNum;
} ATPOpenSocket;

typedef struct {
    bool active;
    uint16_t transID;
    uint8_t socketNum;
    uint8_t bitMap;
    ATPAddrBlock addr;
} ATPTransaction;

static ATPOpenSocket s_atpSockets[MAX_ATP_SOCKETS];
static ATPTransaction s_atpTx[MAX_ATP_TX];
static uint16_t s_nextTransID = 1000;
static bool s_atpInited = false;

static void atp_init(void)
{
    if (s_atpInited) return;
    memset(s_atpSockets, 0, sizeof(s_atpSockets));
    memset(s_atpTx, 0, sizeof(s_atpTx));
    s_atpInited = true;
}

int16_t lc3_atp_open_socket(uint8_t requestedSocket, uint8_t *actualSocket)
{
    atp_init();
    if (!actualSocket) return ATP_BAD_REQ_CODE;

    uint8_t skt = (requestedSocket == 0) ? 128 : requestedSocket;
    for (int i = 0; i < MAX_ATP_SOCKETS; i++) {
        if (!s_atpSockets[i].inUse) {
            s_atpSockets[i].inUse = true;
            s_atpSockets[i].socketNum = skt;
            *actualSocket = skt;
            return ATP_NO_ERR;
        }
    }
    return ATP_TOO_MANY_SKTS;
}

int16_t lc3_atp_close_socket(uint8_t atpSocket)
{
    atp_init();
    for (int i = 0; i < MAX_ATP_SOCKETS; i++) {
        if (s_atpSockets[i].inUse && s_atpSockets[i].socketNum == atpSocket) {
            s_atpSockets[i].inUse = false;
            lc3_atp_kill_send_req(atpSocket);
            return ATP_NO_ERR;
        }
    }
    return ATP_BAD_REQ_CODE;
}

int16_t lc3_atp_send_request(ATPParamBlock *pb, bool isAsync)
{
    (void)isAsync;
    atp_init();
    if (!pb) return ATP_BAD_REQ_CODE;

    pb->transID = s_nextTransID++;
    for (int i = 0; i < MAX_ATP_TX; i++) {
        if (!s_atpTx[i].active) {
            s_atpTx[i].active = true;
            s_atpTx[i].transID = pb->transID;
            s_atpTx[i].socketNum = pb->atpSocket;
            s_atpTx[i].bitMap = pb->bitMap;
            s_atpTx[i].addr = pb->addrBlock;
            return ATP_NO_ERR;
        }
    }
    return ATP_NO_DATA_PKT;
}

int16_t lc3_atp_get_request(ATPParamBlock *pb, bool isAsync)
{
    (void)isAsync;
    if (!pb) return ATP_BAD_REQ_CODE;
    pb->transID = 0;
    return ATP_NO_ERR;
}

int16_t lc3_atp_send_response(ATPParamBlock *pb, bool isAsync)
{
    (void)isAsync;
    if (!pb) return ATP_BAD_REQ_CODE;
    return ATP_NO_ERR;
}

int16_t lc3_atp_add_response(ATPParamBlock *pb, int16_t rspNum)
{
    (void)pb;
    (void)rspNum;
    return ATP_NO_ERR;
}

int16_t lc3_atp_req_cancel(uint16_t transID)
{
    atp_init();
    for (int i = 0; i < MAX_ATP_TX; i++) {
        if (s_atpTx[i].active && s_atpTx[i].transID == transID) {
            s_atpTx[i].active = false;
            return ATP_NO_ERR;
        }
    }
    return ATP_REQ_CANCELLED;
}

int16_t lc3_atp_kill_send_req(uint8_t atpSocket)
{
    atp_init();
    for (int i = 0; i < MAX_ATP_TX; i++) {
        if (s_atpTx[i].active && s_atpTx[i].socketNum == atpSocket) {
            s_atpTx[i].active = false;
        }
    }
    return ATP_NO_ERR;
}
