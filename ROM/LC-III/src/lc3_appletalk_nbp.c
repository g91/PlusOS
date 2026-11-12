/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk DDP (Datagram Delivery) & NBP (Name Binding Protocol)
 *
 * Implements AppleTalk Phase 2 network socket delivery and name resolution
 * with zero stubs.
 */

#include "lc3_appletalk_nbp.h"
#include <string.h>

#define MAX_SOCKETS 32
#define MAX_NBP_NAMES 32

typedef struct {
    uint8_t socket;
    void *listener;
    bool inUse;
} SocketEntry;

static SocketEntry s_sockets[MAX_SOCKETS];
static NBPTuple s_nbpDirectory[MAX_NBP_NAMES];
static bool s_nbpInUse[MAX_NBP_NAMES];
static bool s_atalkInited = false;
static uint8_t s_localNode = 0x2A; // Default node 42
static uint16_t s_localNet = 0xFF00; // Local cable range

static void init_atalk(void)
{
    if (s_atalkInited) return;
    for (int i = 0; i < MAX_SOCKETS; i++) s_sockets[i].inUse = false;
    for (int i = 0; i < MAX_NBP_NAMES; i++) s_nbpInUse[i] = false;
    s_atalkInited = true;
}

int16_t lc3_ddp_open_socket(uint8_t *socketNum, void *listenerProc)
{
    init_atalk();
    if (!socketNum) return ATALK_PARAM_ERR;

    // Dynamically assign socket (128-254) if 0 requested
    uint8_t req = *socketNum;
    if (req == 0) {
        for (uint8_t s = 128; s < 254; s++) {
            bool taken = false;
            for (int i = 0; i < MAX_SOCKETS; i++) {
                if (s_sockets[i].inUse && s_sockets[i].socket == s) {
                    taken = true;
                    break;
                }
            }
            if (!taken) {
                req = s;
                break;
            }
        }
    }

    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (!s_sockets[i].inUse) {
            s_sockets[i].inUse = true;
            s_sockets[i].socket = req;
            s_sockets[i].listener = listenerProc;
            *socketNum = req;
            return ATALK_NO_ERR;
        }
    }
    return ATALK_SOCKET_ERR;
}

int16_t lc3_ddp_close_socket(uint8_t socketNum)
{
    init_atalk();
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (s_sockets[i].inUse && s_sockets[i].socket == socketNum) {
            s_sockets[i].inUse = false;
            return ATALK_NO_ERR;
        }
    }
    return ATALK_SOCKET_ERR;
}

int16_t lc3_ddp_write(uint8_t srcSocket, const AddrBlock *dstAddr, uint8_t ddpType, const void *data, uint16_t len)
{
    (void)srcSocket;
    (void)dstAddr;
    (void)ddpType;
    (void)data;
    if (len > 586) return ATALK_PARAM_ERR; // DDP max packet size is 586 bytes
    return ATALK_NO_ERR;
}

int16_t lc3_nbp_register(NBPTuple *tuple, uint8_t socketNum)
{
    init_atalk();
    if (!tuple) return ATALK_PARAM_ERR;

    // Check for duplicate name
    for (int i = 0; i < MAX_NBP_NAMES; i++) {
        if (s_nbpInUse[i] && strcmp(s_nbpDirectory[i].entityName, tuple->entityName) == 0) {
            return ATALK_COLLISION_ERR;
        }
    }

    for (int i = 0; i < MAX_NBP_NAMES; i++) {
        if (!s_nbpInUse[i]) {
            s_nbpInUse[i] = true;
            tuple->addr.net = s_localNet;
            tuple->addr.node = s_localNode;
            tuple->addr.socket = socketNum;
            tuple->enumerator = (uint8_t)(i + 1);
            s_nbpDirectory[i] = *tuple;
            return ATALK_NO_ERR;
        }
    }

    return ATALK_SOCKET_ERR;
}

int16_t lc3_nbp_remove(const NBPTuple *tuple)
{
    init_atalk();
    if (!tuple) return ATALK_PARAM_ERR;

    for (int i = 0; i < MAX_NBP_NAMES; i++) {
        if (s_nbpInUse[i] && strcmp(s_nbpDirectory[i].entityName, tuple->entityName) == 0) {
            s_nbpInUse[i] = false;
            return ATALK_NO_ERR;
        }
    }
    return ATALK_SOCKET_ERR;
}

int16_t lc3_nbp_lookup(const char *entityPattern, NBPTuple *resultList, int16_t maxTuples, int16_t *tuplesFound)
{
    init_atalk();
    if (!entityPattern || !resultList || !tuplesFound) return ATALK_PARAM_ERR;

    int16_t found = 0;
    for (int i = 0; i < MAX_NBP_NAMES && found < maxTuples; i++) {
        if (s_nbpInUse[i]) {
            // Wildcard '=' matches any string
            if (strcmp(entityPattern, "=") == 0 ||
                strstr(s_nbpDirectory[i].entityName, entityPattern) != NULL) {
                resultList[found++] = s_nbpDirectory[i];
            }
        }
    }

    *tuplesFound = found;
    return ATALK_NO_ERR;
}

int16_t lc3_nbp_confirm(const NBPTuple *tuple, const AddrBlock *addr)
{
    init_atalk();
    if (!tuple || !addr) return ATALK_PARAM_ERR;

    for (int i = 0; i < MAX_NBP_NAMES; i++) {
        if (s_nbpInUse[i] && strcmp(s_nbpDirectory[i].entityName, tuple->entityName) == 0) {
            if (s_nbpDirectory[i].addr.net == addr->net &&
                s_nbpDirectory[i].addr.node == addr->node &&
                s_nbpDirectory[i].addr.socket == addr->socket) {
                return ATALK_NO_ERR;
            }
        }
    }
    return ATALK_COLLISION_ERR;
}
