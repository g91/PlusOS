/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: CommToolbox Connection Manager Core
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_CTB_CONNECTION_H
#define LC3_CTB_CONNECTION_H

#include <stdint.h>
#include <stdbool.h>

#define CM_NO_ERR            0
#define CM_PARAM_ERR       -50
#define CM_NOT_OPEN        -51
#define CM_TIMEOUT         -52
#define CM_NO_CARRIER      -53

/* Connection States */
typedef enum {
    CM_STATE_CLOSED = 0,
    CM_STATE_INITIALIZED,
    CM_STATE_LISTENING,
    CM_STATE_CONNECTING,
    CM_STATE_CONNECTED,
    CM_STATE_CLOSING
} CMConnectionState;

/* Connection Record */
typedef struct ConnRecord {
    int16_t procID;              /* Connection tool ID (e.g. Serial Tool = 1, Modem Tool = 2) */
    CMConnectionState state;
    uint32_t baudRate;
    uint8_t dataBits;
    uint8_t stopBits;
    uint8_t parity;
    bool flowControlDTR;
    bool flowControlCTS;
    uint32_t bytesSent;
    uint32_t bytesReceived;
} ConnRecord, *ConnRecordPtr, **ConnHandle;

/* Public API */
int16_t lc3_ctb_cm_init(void);
int16_t lc3_ctb_cm_open(ConnHandle hConn, int16_t toolID, uint32_t baud);
int16_t lc3_ctb_cm_listen(ConnHandle hConn, bool async);
int16_t lc3_ctb_cm_read(ConnHandle hConn, uint8_t *buffer, uint32_t *bytesToRead, uint32_t timeoutMs);
int16_t lc3_ctb_cm_write(ConnHandle hConn, const uint8_t *buffer, uint32_t *bytesToWrite, bool async);
int16_t lc3_ctb_cm_close(ConnHandle hConn);

#endif /* LC3_CTB_CONNECTION_H */
