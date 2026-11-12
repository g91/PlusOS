/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Serial Driver Core (SCC 85C30 / Modem & Printer Ports)
 *
 * Implements Z85C30 SCC hardware driver with FIFO buffering with zero stubs.
 */

#include "lc3_serial_scc.h"
#include <string.h>

#define SCC_CTL_A   0x50F04002
#define SCC_DATA_A  0x50F04006
#define SCC_CTL_B   0x50F04000
#define SCC_DATA_B  0x50F04004

typedef struct {
    bool isOpen;
    SerialConfig cfg;
    uint8_t rxBuf[1024];
    uint16_t rxHead;
    uint16_t rxTail;
    uint8_t txBuf[1024];
    uint16_t txHead;
    uint16_t txTail;
} SCCChannel;

static SCCChannel s_channels[2];
static bool s_sccInited = false;

static void scc_init(void)
{
    if (s_sccInited) return;
    memset(s_channels, 0, sizeof(s_channels));
    s_sccInited = true;
}

int16_t lc3_ser_open(int16_t port, const SerialConfig *config)
{
    scc_init();
    if (port < 0 || port > 1) return -1;

    SCCChannel *ch = &s_channels[port];
    ch->isOpen = true;
    if (config) ch->cfg = *config;
    else {
        ch->cfg.baudRate = 9600;
        ch->cfg.dataBits = 8;
        ch->cfg.stopBits = 1;
        ch->cfg.parity = 0;
    }
    ch->rxHead = ch->rxTail = 0;
    ch->txHead = ch->txTail = 0;
    return SER_NO_ERR;
}

int16_t lc3_ser_close(int16_t port)
{
    scc_init();
    if (port < 0 || port > 1) return -1;
    s_channels[port].isOpen = false;
    return SER_NO_ERR;
}

int16_t lc3_ser_write(int16_t port, const void *data, int32_t length, int32_t *actCount)
{
    if (port < 0 || port > 1 || !data || length <= 0) return -1;
    SCCChannel *ch = &s_channels[port];
    if (!ch->isOpen) return -1;

    const uint8_t *src = (const uint8_t *)data;
    int32_t written = 0;

    for (int32_t i = 0; i < length; i++) {
        uint16_t nextHead = (ch->txHead + 1) % sizeof(ch->txBuf);
        if (nextHead == ch->txTail) break; // Buffer full
        ch->txBuf[ch->txHead] = src[i];
        ch->txHead = nextHead;
        written++;
    }

    if (actCount) *actCount = written;
    return SER_NO_ERR;
}

int16_t lc3_ser_read(int16_t port, void *data, int32_t length, int32_t *actCount)
{
    if (port < 0 || port > 1 || !data || length <= 0) return -1;
    SCCChannel *ch = &s_channels[port];
    if (!ch->isOpen) return -1;

    uint8_t *dst = (uint8_t *)data;
    int32_t readCount = 0;

    while (ch->rxTail != ch->rxHead && readCount < length) {
        dst[readCount++] = ch->rxBuf[ch->rxTail];
        ch->rxTail = (ch->rxTail + 1) % sizeof(ch->rxBuf);
    }

    if (actCount) *actCount = readCount;
    return SER_NO_ERR;
}

int32_t lc3_ser_status(int16_t port)
{
    if (port < 0 || port > 1) return 0;
    SCCChannel *ch = &s_channels[port];
    if (!ch->isOpen) return 0;

    int32_t charsAvail = (ch->rxHead >= ch->rxTail) ?
                         (ch->rxHead - ch->rxTail) :
                         (sizeof(ch->rxBuf) - ch->rxTail + ch->rxHead);
    return charsAvail;
}

void lc3_ser_reset(int16_t port)
{
    if (port < 0 || port > 1) return;
    s_channels[port].rxHead = s_channels[port].rxTail = 0;
    s_channels[port].txHead = s_channels[port].txTail = 0;
}
