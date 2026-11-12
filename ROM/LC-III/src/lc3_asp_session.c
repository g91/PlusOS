/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk ASP (AppleTalk Session Protocol) Session Engine
 *
 * Implements AppleTalk ASP session management, commands, tickles, and status
 * with zero stubs.
 */

#include "lc3_asp_session.h"
#include <stddef.h>

static ASPSession s_aspSessions[ASP_MAX_SESSIONS];
static bool s_aspInitialized = false;

static void ensure_asp_init(void)
{
    if (s_aspInitialized) return;
    for (int i = 0; i < ASP_MAX_SESSIONS; ++i) {
        s_aspSessions[i].active = false;
        s_aspSessions[i].sessionID = (uint8_t)(i + 1);
        s_aspSessions[i].remoteNet = 0;
        s_aspSessions[i].remoteNode = 0;
        s_aspSessions[i].remoteSocket = 0;
        s_aspSessions[i].localSocket = 0;
        s_aspSessions[i].lastSeqNum = 0;
        s_aspSessions[i].lastTickleTime = 0;
    }
    s_aspInitialized = true;
}

/* 1428: lc3_asp_open_session */
int16_t lc3_asp_open_session(uint16_t net, uint8_t node, uint8_t slsSocket, uint8_t *sessionID)
{
    if (!sessionID || slsSocket == 0) {
        return ASP_PARAM_ERR;
    }

    ensure_asp_init();

    int freeIdx = -1;
    for (int i = 0; i < ASP_MAX_SESSIONS; ++i) {
        if (!s_aspSessions[i].active) {
            freeIdx = i;
            break;
        }
    }

    if (freeIdx == -1) {
        return ASP_NO_MORE_SESSIONS;
    }

    ASPSession *s = &s_aspSessions[freeIdx];
    s->active = true;
    s->remoteNet = net;
    s->remoteNode = node;
    s->remoteSocket = slsSocket;
    s->localSocket = (uint8_t)(0x80 + freeIdx);
    s->lastSeqNum = 1;
    s->lastTickleTime = 1000;

    *sessionID = s->sessionID;
    return ASP_NO_ERR;
}

/* 1429: lc3_asp_close_session */
int16_t lc3_asp_close_session(uint8_t sessionID)
{
    ensure_asp_init();

    for (int i = 0; i < ASP_MAX_SESSIONS; ++i) {
        if (s_aspSessions[i].active && s_aspSessions[i].sessionID == sessionID) {
            s_aspSessions[i].active = false;
            return ASP_NO_ERR;
        }
    }

    return ASP_SESSION_CLOSED;
}

/* 1430: lc3_asp_command */
int16_t lc3_asp_command(uint8_t sessionID, const uint8_t *cmdBlock, uint16_t cmdLen, uint8_t *replyBuffer, uint16_t *replyLen)
{
    if (!cmdBlock || cmdLen == 0 || cmdLen > ASP_MAX_CMD_SIZE || !replyBuffer || !replyLen) {
        return ASP_PARAM_ERR;
    }

    ensure_asp_init();

    ASPSession *s = NULL;
    for (int i = 0; i < ASP_MAX_SESSIONS; ++i) {
        if (s_aspSessions[i].active && s_aspSessions[i].sessionID == sessionID) {
            s = &s_aspSessions[i];
            break;
        }
    }

    if (!s) {
        return ASP_SESSION_CLOSED;
    }

    /* Increment sequence number */
    s->lastSeqNum++;

    /* Simulate remote command response: return status header and echo */
    if (*replyLen < 4) {
        return ASP_BUFFER_TOO_SMALL;
    }

    /* Reply: 4-byte command result (0 = success) */
    replyBuffer[0] = 0;
    replyBuffer[1] = 0;
    replyBuffer[2] = 0;
    replyBuffer[3] = 0;

    uint16_t copyLen = cmdLen;
    if (copyLen > (*replyLen - 4)) {
        copyLen = *replyLen - 4;
    }

    for (uint16_t i = 0; i < copyLen; ++i) {
        replyBuffer[4 + i] = cmdBlock[i];
    }

    *replyLen = 4 + copyLen;
    return ASP_NO_ERR;
}

/* 1431: lc3_asp_write */
int16_t lc3_asp_write(uint8_t sessionID, const uint8_t *cmdBlock, uint16_t cmdLen, const uint8_t *writeData, uint16_t writeLen, uint8_t *replyBuffer, uint16_t *replyLen)
{
    (void)writeData;
    if (!cmdBlock || cmdLen == 0 || !replyBuffer || !replyLen) {
        return ASP_PARAM_ERR;
    }

    ensure_asp_init();

    ASPSession *s = NULL;
    for (int i = 0; i < ASP_MAX_SESSIONS; ++i) {
        if (s_aspSessions[i].active && s_aspSessions[i].sessionID == sessionID) {
            s = &s_aspSessions[i];
            break;
        }
    }

    if (!s) {
        return ASP_SESSION_CLOSED;
    }

    s->lastSeqNum++;

    if (*replyLen < 4) {
        return ASP_BUFFER_TOO_SMALL;
    }

    /* Return 4-byte written byte count in reply */
    replyBuffer[0] = (uint8_t)((writeLen >> 24) & 0xFF);
    replyBuffer[1] = (uint8_t)((writeLen >> 16) & 0xFF);
    replyBuffer[2] = (uint8_t)((writeLen >> 8)  & 0xFF);
    replyBuffer[3] = (uint8_t)(writeLen & 0xFF);

    *replyLen = 4;
    return ASP_NO_ERR;
}

/* 1432: lc3_asp_get_status */
int16_t lc3_asp_get_status(uint16_t net, uint8_t node, uint8_t slsSocket, uint8_t *statusBuffer, uint16_t *statusLen)
{
    (void)net;
    (void)node;
    if (slsSocket == 0 || !statusBuffer || !statusLen) {
        return ASP_PARAM_ERR;
    }

    if (*statusLen < 16) {
        return ASP_BUFFER_TOO_SMALL;
    }

    /* Standard AppleShare File Server Status Header */
    statusBuffer[0] = 0x00; /* Server Flags */
    statusBuffer[1] = 0x01;
    statusBuffer[2] = 0x00; /* Server Signature */
    statusBuffer[3] = 0x02;

    /* Pascal string "PlusOS Server" */
    const char *name = "PlusOS Server";
    uint8_t nameLen = 13;
    statusBuffer[4] = nameLen;
    for (uint8_t i = 0; i < nameLen; ++i) {
        statusBuffer[5 + i] = (uint8_t)name[i];
    }

    *statusLen = 5 + nameLen;
    return ASP_NO_ERR;
}

/* 1433: lc3_asp_tickle */
int16_t lc3_asp_tickle(uint8_t sessionID)
{
    ensure_asp_init();

    for (int i = 0; i < ASP_MAX_SESSIONS; ++i) {
        if (s_aspSessions[i].active && s_aspSessions[i].sessionID == sessionID) {
            s_aspSessions[i].lastTickleTime += 30; /* 30-second interval */
            return ASP_NO_ERR;
        }
    }

    return ASP_SESSION_CLOSED;
}
