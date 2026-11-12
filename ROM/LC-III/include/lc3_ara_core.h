/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Apple Remote Access (ARA) Serial Tool & Remote Networking
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_ARA_CORE_H
#define LC3_ARA_CORE_H

#include <stdint.h>
#include <stdbool.h>

#define ARA_NO_ERR         0
#define ARA_NOT_CONNECTED -3000
#define ARA_BUSY          -3001
#define ARA_PARAM_ERR       -50

typedef struct ARAConnStatus {
    bool isConnected;
    bool isConnecting;
    uint32_t connectSpeed;
    uint32_t bytesSent;
    uint32_t bytesReceived;
    uint32_t connectDurationTicks;
} ARAConnStatus;

/* ARA API */
int16_t lc3_ara_serial_open(int16_t portNum, uint32_t baudRate);
int16_t lc3_ara_serial_close(int16_t portNum);
int16_t lc3_ara_ppp_connect(const char *dialString);
int16_t lc3_ara_ppp_disconnect(void);
int16_t lc3_ara_get_status(ARAConnStatus *status);

#endif /* LC3_ARA_CORE_H */
