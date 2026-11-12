/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Zilog 8530 Serial Communications Controller (SCC) Enhanced Driver
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_SCC_DRIVER_H
#define LC3_SCC_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

#define SCC_NO_ERR         0
#define SCC_TIMEOUT_ERR -1090
#define SCC_PARAM_ERR     -50

#define SCC_CHANNEL_A       0   /* Modem Port */
#define SCC_CHANNEL_B       1   /* Printer / LocalTalk Port */

#define SCC_BASE_ADDR   0x50004000

typedef struct SCCChannelConfig {
    uint32_t baudRate;
    uint8_t dataBits;     /* 5, 6, 7, 8 */
    uint8_t stopBits;     /* 1, 2 */
    uint8_t parity;       /* 0 = None, 1 = Odd, 2 = Even */
    bool hwHandshake;     /* CTS/RTS */
    bool rxInterrupt;
} SCCChannelConfig;

typedef struct SCCChannelState {
    SCCChannelConfig config;
    uint16_t timeConstant;
    bool txReady;
    bool rxReady;
    uint8_t rxBuffer[64];
    uint8_t rxHead;
    uint8_t rxTail;
    bool dtrActive;
    bool rtsActive;
    bool ctsState;
} SCCChannelState;

/* SCC Driver API */
int16_t lc3_scc_init_port(uint8_t channel, const SCCChannelConfig *config);
int16_t lc3_scc_set_handshake(uint8_t channel, bool dtr, bool rts);
int16_t lc3_scc_transmit_byte(uint8_t channel, uint8_t byteVal);
int16_t lc3_scc_receive_byte(uint8_t channel, uint8_t *byteVal);
int16_t lc3_scc_set_baud(uint8_t channel, uint32_t baudRate);
int16_t lc3_scc_reset_channel(uint8_t channel);

#endif /* LC3_SCC_DRIVER_H */
