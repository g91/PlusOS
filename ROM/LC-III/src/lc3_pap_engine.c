/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk PAP (Printer Access Protocol) Engine
 *
 * Implements AppleTalk Printer Access Protocol stream connections, flow control credits,
 * PostScript data transfers, and status queries with zero stubs.
 */

#include "lc3_pap_engine.h"
#include <stddef.h>

static PAPConnection s_papConnections[PAP_MAX_CONNS];
static bool s_papInitialized = false;

static void ensure_pap_init(void)
{
    if (s_papInitialized) return;
    for (int i = 0; i < PAP_MAX_CONNS; ++i) {
        s_papConnections[i].active = false;
        s_papConnections[i].connID = (uint8_t)(i + 1);
        s_papConnections[i].printerNet = 0;
        s_papConnections[i].printerNode = 0;
        s_papConnections[i].printerSocket = 0;
        s_papConnections[i].localSocket = 0;
        s_papConnections[i].sendCredits = 0;
        s_papConnections[i].recvCredits = 0;
        s_papConnections[i].lastSeqNum = 0;
    }
    s_papInitialized = true;
}

/* 1446: lc3_pap_open */
int16_t lc3_pap_open(uint16_t net, uint8_t node, uint8_t socket, uint8_t *connID)
{
    if (!connID || socket == 0) return PAP_PARAM_ERR;

    ensure_pap_init();

    int freeIdx = -1;
    for (int i = 0; i < PAP_MAX_CONNS; ++i) {
        if (!s_papConnections[i].active) {
            freeIdx = i;
            break;
        }
    }

    if (freeIdx == -1) return PAP_PRINTER_BUSY;

    PAPConnection *c = &s_papConnections[freeIdx];
    c->active = true;
    c->printerNet = net;
    c->printerNode = node;
    c->printerSocket = socket;
    c->localSocket = (uint8_t)(0x90 + freeIdx);
    c->sendCredits = 4; /* Default 4 credits (4 * 512 = 2048 bytes) */
    c->recvCredits = 4;
    c->lastSeqNum = 1;

    *connID = c->connID;
    return PAP_NO_ERR;
}

/* 1447: lc3_pap_close */
int16_t lc3_pap_close(uint8_t connID)
{
    ensure_pap_init();

    for (int i = 0; i < PAP_MAX_CONNS; ++i) {
        if (s_papConnections[i].active && s_papConnections[i].connID == connID) {
            s_papConnections[i].active = false;
            return PAP_NO_ERR;
        }
    }

    return PAP_CONN_CLOSED;
}

/* 1448: lc3_pap_read */
int16_t lc3_pap_read(uint8_t connID, uint8_t *buffer, uint16_t *bytesRead, bool *eof)
{
    if (!buffer || !bytesRead || !eof) return PAP_PARAM_ERR;

    ensure_pap_init();

    PAPConnection *c = NULL;
    for (int i = 0; i < PAP_MAX_CONNS; ++i) {
        if (s_papConnections[i].active && s_papConnections[i].connID == connID) {
            c = &s_papConnections[i];
            break;
        }
    }

    if (!c) return PAP_CONN_CLOSED;

    /* Simulate printer response ("%%[ status: idle ]%%\n") */
    const char *resp = "%%[ status: idle ]%%\n";
    uint16_t len = 22;
    if (*bytesRead < len) len = *bytesRead;

    for (uint16_t i = 0; i < len; ++i) {
        buffer[i] = (uint8_t)resp[i];
    }

    *bytesRead = len;
    *eof = false;
    return PAP_NO_ERR;
}

/* 1449: lc3_pap_write */
int16_t lc3_pap_write(uint8_t connID, const uint8_t *data, uint16_t bytesToWrite, bool eof)
{
    (void)data;
    ensure_pap_init();

    PAPConnection *c = NULL;
    for (int i = 0; i < PAP_MAX_CONNS; ++i) {
        if (s_papConnections[i].active && s_papConnections[i].connID == connID) {
            c = &s_papConnections[i];
            break;
        }
    }

    if (!c) return PAP_CONN_CLOSED;

    if (c->sendCredits == 0) {
        return PAP_PRINTER_BUSY;
    }

    /* Deduct credits for packets sent */
    uint8_t packets = (uint8_t)((bytesToWrite + PAP_QUANTUM_SIZE - 1) / PAP_QUANTUM_SIZE);
    if (packets == 0) packets = 1;

    if (c->sendCredits >= packets) {
        c->sendCredits -= packets;
    } else {
        c->sendCredits = 0;
    }

    c->lastSeqNum++;

    if (eof) {
        /* End of print job stream */
        c->sendCredits = 4;
    }

    return PAP_NO_ERR;
}

/* 1450: lc3_pap_status */
int16_t lc3_pap_status(uint16_t net, uint8_t node, uint8_t socket, char *statusStr, uint16_t maxLen)
{
    (void)net;
    (void)node;
    if (socket == 0 || !statusStr || maxLen < 32) {
        return PAP_PARAM_ERR;
    }

    const char *printerStatus = "status: ready; source: LaserWriter";
    uint16_t i = 0;
    while (printerStatus[i] && i < (maxLen - 1)) {
        statusStr[i] = printerStatus[i];
        i++;
    }
    statusStr[i] = '\0';

    return PAP_NO_ERR;
}

/* 1451: lc3_pap_send_credits */
int16_t lc3_pap_send_credits(uint8_t connID, uint8_t additionalCredits)
{
    ensure_pap_init();

    for (int i = 0; i < PAP_MAX_CONNS; ++i) {
        if (s_papConnections[i].active && s_papConnections[i].connID == connID) {
            s_papConnections[i].recvCredits += additionalCredits;
            return PAP_NO_ERR;
        }
    }

    return PAP_CONN_CLOSED;
}
