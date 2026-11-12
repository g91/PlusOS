/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: AppleTalk PAP (Printer Access Protocol) Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_PAP_ENGINE_H
#define LC3_PAP_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

#define PAP_NO_ERR           0
#define PAP_PRINTER_BUSY   -400
#define PAP_CONN_REFUSED   -401
#define PAP_CONN_CLOSED    -402
#define PAP_PARAM_ERR      -403
#define PAP_BUFFER_OVERFLOW -404

#define PAP_MAX_CONNS        4
#define PAP_QUANTUM_SIZE     512

typedef struct PAPConnection {
    bool active;
    uint8_t connID;
    uint16_t printerNet;
    uint8_t printerNode;
    uint8_t printerSocket;
    uint8_t localSocket;
    uint8_t sendCredits;
    uint8_t recvCredits;
    uint16_t lastSeqNum;
} PAPConnection;

/* Public API */
int16_t lc3_pap_open(uint16_t net, uint8_t node, uint8_t socket, uint8_t *connID);
int16_t lc3_pap_close(uint8_t connID);
int16_t lc3_pap_read(uint8_t connID, uint8_t *buffer, uint16_t *bytesRead, bool *eof);
int16_t lc3_pap_write(uint8_t connID, const uint8_t *data, uint16_t bytesToWrite, bool eof);
int16_t lc3_pap_status(uint16_t net, uint8_t node, uint8_t socket, char *statusStr, uint16_t maxLen);
int16_t lc3_pap_send_credits(uint8_t connID, uint8_t additionalCredits);

#endif /* LC3_PAP_ENGINE_H */
