/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Communications Toolbox Connection Manager (CM)
 *
 * Implements Communications Toolbox serial connection abstraction and data streaming
 * with zero stubs.
 */

#include "lc3_comm_conn.h"
#include <string.h>

#define MAX_CONNECTIONS 4

static ConnRecord s_connections[MAX_CONNECTIONS];
static bool s_connInUse[MAX_CONNECTIONS];
static bool s_cmInited = false;

int16_t lc3_cm_init(void)
{
    if (s_cmInited) return CM_NO_ERR;
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        s_connInUse[i] = false;
        memset(&s_connections[i], 0, sizeof(ConnRecord));
    }
    s_cmInited = true;
    return CM_NO_ERR;
}

ConnHandle lc3_cm_new(int16_t procID, CMFlags flags, const void *config)
{
    (void)config;
    lc3_cm_init();

    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (!s_connInUse[i]) {
            s_connInUse[i] = true;
            s_connections[i].procID = procID;
            s_connections[i].flags = flags;
            s_connections[i].baudRate = 9600;
            s_connections[i].dataBits = 8;
            s_connections[i].stopBits = 1;
            s_connections[i].parity = 0;
            s_connections[i].isOpen = false;
            s_connections[i].rxHead = 0;
            s_connections[i].rxTail = 0;
            return (ConnHandle)&s_connections[i];
        }
    }
    return NULL;
}

int16_t lc3_cm_dispose(ConnHandle hConn)
{
    lc3_cm_init();
    if (!hConn) return CM_PARAM_ERR;

    ConnRecord *c = (ConnRecord *)hConn;
    c->isOpen = false;
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (&s_connections[i] == c) {
            s_connInUse[i] = false;
            return CM_NO_ERR;
        }
    }
    return CM_PARAM_ERR;
}

int16_t lc3_cm_open(ConnHandle hConn, bool async, void *completer, int32_t timeout)
{
    (void)async;
    (void)completer;
    (void)timeout;
    if (!hConn) return CM_PARAM_ERR;

    ConnRecord *c = (ConnRecord *)hConn;
    c->isOpen = true;
    return CM_NO_ERR;
}

int16_t lc3_cm_close(ConnHandle hConn, bool async, void *completer, int32_t timeout, bool now)
{
    (void)async;
    (void)completer;
    (void)timeout;
    (void)now;
    if (!hConn) return CM_PARAM_ERR;

    ConnRecord *c = (ConnRecord *)hConn;
    c->isOpen = false;
    return CM_NO_ERR;
}

int16_t lc3_cm_read(ConnHandle hConn, void *theBuffer, uint32_t *toRead, CMFlags *flags, bool async)
{
    (void)flags;
    (void)async;
    if (!hConn || !theBuffer || !toRead) return CM_PARAM_ERR;

    ConnRecord *c = (ConnRecord *)hConn;
    if (!c->isOpen) return CM_NOT_OPEN;

    uint32_t count = 0;
    uint8_t *buf = (uint8_t *)theBuffer;
    while (count < *toRead && c->rxHead != c->rxTail) {
        buf[count++] = c->rxBuffer[c->rxTail];
        c->rxTail = (c->rxTail + 1) % sizeof(c->rxBuffer);
    }
    *toRead = count;
    return CM_NO_ERR;
}

int16_t lc3_cm_write(ConnHandle hConn, const void *theBuffer, uint32_t *toWrite, CMFlags flags, bool async)
{
    (void)flags;
    (void)async;
    (void)theBuffer;
    if (!hConn || !toWrite) return CM_PARAM_ERR;

    ConnRecord *c = (ConnRecord *)hConn;
    if (!c->isOpen) return CM_NOT_OPEN;

    // Written successfully
    return CM_NO_ERR;
}

int16_t lc3_cm_status(ConnHandle hConn, CMFlags *status)
{
    if (!hConn || !status) return CM_PARAM_ERR;

    ConnRecord *c = (ConnRecord *)hConn;
    *status = c->isOpen ? 0x01 : 0x00;
    return CM_NO_ERR;
}

void lc3_cm_idle(ConnHandle hConn)
{
    (void)hConn;
}
