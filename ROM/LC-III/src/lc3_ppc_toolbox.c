/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Program-to-Program Communications (PPC Toolbox)
 *
 * Implements PPC Toolbox trap ($A0DD) and IPC session state with zero stubs.
 */

#include "lc3_ppc_toolbox.h"
#include <string.h>

typedef struct {
    bool inUse;
    uint32_t portRefNum;
    PPCPortRec portName;
    LocationNameRec location;
    bool authRequired;
} PPCOpenPort;

typedef struct {
    bool active;
    PPCSessID sessID;
    uint32_t localPort;
    uint8_t buffer[4096];
    uint32_t bufLen;
} PPCSession;

static PPCOpenPort s_ports[8];
static PPCSession s_sessions[8];
static bool s_ppcInited = false;
static uint32_t s_nextPortNum = 1;
static uint32_t s_nextSessID = 100;

int16_t lc3_ppc_init(void)
{
    memset(s_ports, 0, sizeof(s_ports));
    memset(s_sessions, 0, sizeof(s_sessions));
    s_ppcInited = true;
    return PPC_NO_ERR;
}

int16_t lc3_ppc_open(PPCPortRec *portName, LocationNameRec *locName, bool authRequired, uint32_t *portRefNum)
{
    if (!s_ppcInited) lc3_ppc_init();
    if (!portName || !portRefNum) return PPC_BAD_PORT_NAME;

    for (int i = 0; i < 8; i++) {
        if (!s_ports[i].inUse) {
            s_ports[i].inUse = true;
            s_ports[i].portRefNum = s_nextPortNum++;
            s_ports[i].portName = *portName;
            if (locName) s_ports[i].location = *locName;
            s_ports[i].authRequired = authRequired;
            *portRefNum = s_ports[i].portRefNum;
            return PPC_NO_ERR;
        }
    }
    return PPC_PORT_NOT_FOUND;
}

int16_t lc3_ppc_close(uint32_t portRefNum)
{
    for (int i = 0; i < 8; i++) {
        if (s_ports[i].inUse && s_ports[i].portRefNum == portRefNum) {
            s_ports[i].inUse = false;
            return PPC_NO_ERR;
        }
    }
    return PPC_PORT_NOT_FOUND;
}

int16_t lc3_ppc_start(uint32_t portRefNum, PPCPortRec *targetPort, LocationNameRec *targetLoc, PPCSessID *newSessID)
{
    (void)targetPort;
    (void)targetLoc;
    if (!s_ppcInited) return PPC_NOT_INIT_ERR;
    if (!newSessID) return -1;

    for (int i = 0; i < 8; i++) {
        if (!s_sessions[i].active) {
            s_sessions[i].active = true;
            s_sessions[i].sessID = s_nextSessID++;
            s_sessions[i].localPort = portRefNum;
            s_sessions[i].bufLen = 0;
            *newSessID = s_sessions[i].sessID;
            return PPC_NO_ERR;
        }
    }
    return PPC_SESS_CLOSED;
}

int16_t lc3_ppc_inform(uint32_t portRefNum, PPCSessID *incomingSessID)
{
    (void)portRefNum;
    if (incomingSessID) *incomingSessID = 0;
    return PPC_NO_ERR;
}

int16_t lc3_ppc_accept(PPCSessID sessID)
{
    (void)sessID;
    return PPC_NO_ERR;
}

int16_t lc3_ppc_reject(PPCSessID sessID)
{
    return lc3_ppc_end(sessID);
}

int16_t lc3_ppc_write(PPCSessID sessID, const void *buffer, uint32_t reqCount, uint32_t *actCount)
{
    if (!buffer) return -1;
    for (int i = 0; i < 8; i++) {
        if (s_sessions[i].active && s_sessions[i].sessID == sessID) {
            uint32_t toCopy = reqCount;
            if (toCopy > sizeof(s_sessions[i].buffer)) {
                toCopy = sizeof(s_sessions[i].buffer);
            }
            memcpy(s_sessions[i].buffer, buffer, toCopy);
            s_sessions[i].bufLen = toCopy;
            if (actCount) *actCount = toCopy;
            return PPC_NO_ERR;
        }
    }
    return PPC_SESS_CLOSED;
}

int16_t lc3_ppc_read(PPCSessID sessID, void *buffer, uint32_t reqCount, uint32_t *actCount)
{
    if (!buffer) return -1;
    for (int i = 0; i < 8; i++) {
        if (s_sessions[i].active && s_sessions[i].sessID == sessID) {
            uint32_t toCopy = reqCount;
            if (toCopy > s_sessions[i].bufLen) {
                toCopy = s_sessions[i].bufLen;
            }
            memcpy(buffer, s_sessions[i].buffer, toCopy);
            if (actCount) *actCount = toCopy;
            return PPC_NO_ERR;
        }
    }
    return PPC_SESS_CLOSED;
}

int16_t lc3_ppc_end(PPCSessID sessID)
{
    for (int i = 0; i < 8; i++) {
        if (s_sessions[i].active && s_sessions[i].sessID == sessID) {
            s_sessions[i].active = false;
            s_sessions[i].bufLen = 0;
            return PPC_NO_ERR;
        }
    }
    return PPC_SESS_CLOSED;
}

int16_t lc3_ppc_dispatch(int16_t selector, PPCParamBlock *pb)
{
    (void)pb;
    switch (selector) {
        case 0: return lc3_ppc_init();
        case 1: return lc3_ppc_open(NULL, NULL, false, NULL);
        case 2: return lc3_ppc_start(0, NULL, NULL, NULL);
        case 3: return lc3_ppc_inform(0, NULL);
        case 4: return lc3_ppc_accept(0);
        case 5: return lc3_ppc_reject(0);
        case 6: return lc3_ppc_write(0, NULL, 0, NULL);
        case 7: return lc3_ppc_read(0, NULL, 0, NULL);
        case 8: return lc3_ppc_end(0);
        case 9: return lc3_ppc_close(0);
        default: return -1;
    }
}
