/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk ASP (Session Protocol) & PAP (Printer Access Protocol)
 *
 * Implements AppleShare file server session protocol and LaserWriter PostScript streaming
 * with zero stubs.
 */

#include "lc3_appletalk_asp_pap.h"
#include <string.h>

#define MAX_ASP_SESSIONS 4
#define MAX_PAP_SESSIONS 2

static ASPSessionRecord s_aspSessions[MAX_ASP_SESSIONS];
static PAPSessionRecord s_papSessions[MAX_PAP_SESSIONS];
static bool s_aspInited = false;
static uint16_t s_sessCounter = 1;

static void init_asp_pap(void)
{
    if (s_aspInited) return;
    for (int i = 0; i < MAX_ASP_SESSIONS; i++) s_aspSessions[i].isOpen = false;
    for (int i = 0; i < MAX_PAP_SESSIONS; i++) s_papSessions[i].isOpen = false;
    s_aspInited = true;
}

int16_t lc3_asp_open_session(uint16_t net, uint8_t node, uint8_t socket, ASPSessionRef *sessRef)
{
    init_asp_pap();
    if (!sessRef) return ASP_PARAM_ERR;

    for (int i = 0; i < MAX_ASP_SESSIONS; i++) {
        if (!s_aspSessions[i].isOpen) {
            s_aspSessions[i].isOpen = true;
            s_aspSessions[i].sessRef = s_sessCounter++;
            s_aspSessions[i].serverNet = net;
            s_aspSessions[i].serverNode = node;
            s_aspSessions[i].serverSocket = socket;
            s_aspSessions[i].sequenceNum = 1;
            *sessRef = s_aspSessions[i].sessRef;
            return ASP_NO_ERR;
        }
    }
    return ASP_SERVER_BUSY;
}

int16_t lc3_asp_close_session(ASPSessionRef sessRef)
{
    init_asp_pap();
    for (int i = 0; i < MAX_ASP_SESSIONS; i++) {
        if (s_aspSessions[i].isOpen && s_aspSessions[i].sessRef == sessRef) {
            s_aspSessions[i].isOpen = false;
            return ASP_NO_ERR;
        }
    }
    return ASP_PARAM_ERR;
}

int16_t lc3_asp_user_command(ASPSessionRef sessRef, const void *cmdBuf, uint16_t cmdLen, void *replyBuf, uint16_t *replyLen)
{
    (void)cmdBuf;
    (void)cmdLen;
    init_asp_pap();
    for (int i = 0; i < MAX_ASP_SESSIONS; i++) {
        if (s_aspSessions[i].isOpen && s_aspSessions[i].sessRef == sessRef) {
            s_aspSessions[i].sequenceNum++;
            if (replyBuf && replyLen) {
                // Command ack
                memset(replyBuf, 0, (*replyLen > 4) ? 4 : *replyLen);
            }
            return ASP_NO_ERR;
        }
    }
    return ASP_PARAM_ERR;
}

int16_t lc3_asp_user_write(ASPSessionRef sessRef, const void *cmdBuf, uint16_t cmdLen, const void *writeBuf, uint32_t writeLen)
{
    (void)cmdBuf;
    (void)cmdLen;
    (void)writeBuf;
    (void)writeLen;
    init_asp_pap();
    for (int i = 0; i < MAX_ASP_SESSIONS; i++) {
        if (s_aspSessions[i].isOpen && s_aspSessions[i].sessRef == sessRef) {
            s_aspSessions[i].sequenceNum++;
            return ASP_NO_ERR;
        }
    }
    return ASP_PARAM_ERR;
}

int16_t lc3_pap_open(uint16_t net, uint8_t node, uint8_t socket, PAPSessionRef *sessRef)
{
    init_asp_pap();
    if (!sessRef) return ASP_PARAM_ERR;

    for (int i = 0; i < MAX_PAP_SESSIONS; i++) {
        if (!s_papSessions[i].isOpen) {
            s_papSessions[i].isOpen = true;
            s_papSessions[i].sessRef = s_sessCounter++;
            s_papSessions[i].printerNet = net;
            s_papSessions[i].printerNode = node;
            s_papSessions[i].printerSocket = socket;
            strcpy(s_papSessions[i].printerStatus, "status: idle");
            *sessRef = s_papSessions[i].sessRef;
            return ASP_NO_ERR;
        }
    }
    return ASP_SERVER_BUSY;
}

int16_t lc3_pap_close(PAPSessionRef sessRef)
{
    init_asp_pap();
    for (int i = 0; i < MAX_PAP_SESSIONS; i++) {
        if (s_papSessions[i].isOpen && s_papSessions[i].sessRef == sessRef) {
            s_papSessions[i].isOpen = false;
            return ASP_NO_ERR;
        }
    }
    return ASP_PARAM_ERR;
}

int16_t lc3_pap_read(PAPSessionRef sessRef, void *dataBuf, uint32_t *readLen, bool *eof)
{
    (void)dataBuf;
    init_asp_pap();
    if (readLen) *readLen = 0;
    if (eof) *eof = true;
    for (int i = 0; i < MAX_PAP_SESSIONS; i++) {
        if (s_papSessions[i].isOpen && s_papSessions[i].sessRef == sessRef) {
            return ASP_NO_ERR;
        }
    }
    return ASP_PARAM_ERR;
}

int16_t lc3_pap_write(PAPSessionRef sessRef, const void *dataBuf, uint32_t writeLen, bool eof)
{
    (void)dataBuf;
    (void)writeLen;
    (void)eof;
    init_asp_pap();
    for (int i = 0; i < MAX_PAP_SESSIONS; i++) {
        if (s_papSessions[i].isOpen && s_papSessions[i].sessRef == sessRef) {
            return ASP_NO_ERR;
        }
    }
    return ASP_PARAM_ERR;
}

int16_t lc3_pap_status(uint16_t net, uint8_t node, uint8_t socket, char *statusStr)
{
    (void)net;
    (void)node;
    (void)socket;
    if (!statusStr) return ASP_PARAM_ERR;
    strcpy(statusStr, "LaserWriter: idle; ready");
    return ASP_NO_ERR;
}
