/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Communications Toolbox Connection Manager (CM)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_COMM_CONN_H
#define LC3_COMM_CONN_H

#include <stdint.h>
#include <stdbool.h>

#define CM_NO_ERR             0
#define CM_GENERIC_ERR     -200
#define CM_NOT_OPEN        -201
#define CM_PARAM_ERR        -50

typedef void *ConnHandle;
typedef uint32_t CMFlags;

typedef struct ConnRecord {
    int16_t procID;
    CMFlags flags;
    int16_t errCode;
    int32_t refCon;
    void *userData;
    bool isOpen;
    bool isListening;
    uint32_t baudRate;
    uint8_t dataBits;
    uint8_t stopBits;
    uint8_t parity;
    uint8_t rxBuffer[512];
    uint32_t rxHead;
    uint32_t rxTail;
} ConnRecord;

/* Connection Manager API */
int16_t lc3_cm_init(void);
ConnHandle lc3_cm_new(int16_t procID, CMFlags flags, const void *config);
int16_t lc3_cm_dispose(ConnHandle hConn);
int16_t lc3_cm_open(ConnHandle hConn, bool async, void *completer, int32_t timeout);
int16_t lc3_cm_close(ConnHandle hConn, bool async, void *completer, int32_t timeout, bool now);
int16_t lc3_cm_read(ConnHandle hConn, void *theBuffer, uint32_t *toRead, CMFlags *flags, bool async);
int16_t lc3_cm_write(ConnHandle hConn, const void *theBuffer, uint32_t *toWrite, CMFlags flags, bool async);
int16_t lc3_cm_status(ConnHandle hConn, CMFlags *status);
void lc3_cm_idle(ConnHandle hConn);

#endif /* LC3_COMM_CONN_H */
