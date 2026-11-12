/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Serial Driver Core (SCC 85C30 / Modem & Printer Ports)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_SERIAL_SCC_H
#define LC3_SERIAL_SCC_H

#include <stdint.h>
#include <stdbool.h>

#define SER_PORT_A         0 // Modem Port
#define SER_PORT_B         1 // Printer / LocalTalk Port

#define SER_BAUD_300       300
#define SER_BAUD_1200     1200
#define SER_BAUD_2400     2400
#define SER_BAUD_9600     9600
#define SER_BAUD_19200   19200
#define SER_BAUD_38400   38400
#define SER_BAUD_57600   57600

#define SER_NO_ERR         0
#define SER_PORT_BUSY     -1
#define SER_BUF_FULL      -2

typedef struct {
    int32_t baudRate;
    uint8_t dataBits;
    uint8_t stopBits;
    uint8_t parity;
    bool ctsHandshake;
    bool dtrFlow;
} SerialConfig;

/* Serial Driver Core API */
int16_t lc3_ser_open(int16_t port, const SerialConfig *config);
int16_t lc3_ser_close(int16_t port);
int16_t lc3_ser_write(int16_t port, const void *data, int32_t length, int32_t *actCount);
int16_t lc3_ser_read(int16_t port, void *data, int32_t length, int32_t *actCount);
int32_t lc3_ser_status(int16_t port);
void lc3_ser_reset(int16_t port);

#endif /* LC3_SERIAL_SCC_H */
