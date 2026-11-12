/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: SCSI Manager Pseudo-DMA Handshake & 32-Bit Burst Engine
 *
 * Implements Sonora ASIC 32-bit hardware pseudo-DMA blind burst reads/writes
 * with zero stubs.
 */

#include "lc3_scsi_pdma.h"
#include <string.h>

#define SONORA_PDMA_DATA 0x50F12000
#define SONORA_SCSI_CTRL 0x50F10000

int16_t lc3_scsi_pdma_read_blind512(void *destBuffer)
{
    if (!destBuffer) return SCSI_PDMA_PARAM_ERR;

    volatile uint32_t *pData = (volatile uint32_t *)SONORA_PDMA_DATA;
    uint32_t *dst = (uint32_t *)destBuffer;

    // 512 bytes = 128 32-bit longwords unrolled burst
    for (int i = 0; i < 128; i++) {
        dst[i] = *pData;
    }
    return SCSI_PDMA_NO_ERR;
}

int16_t lc3_scsi_pdma_write_blind512(const void *srcBuffer)
{
    if (!srcBuffer) return SCSI_PDMA_PARAM_ERR;

    volatile uint32_t *pData = (volatile uint32_t *)SONORA_PDMA_DATA;
    const uint32_t *src = (const uint32_t *)srcBuffer;

    for (int i = 0; i < 128; i++) {
        *pData = src[i];
    }
    return SCSI_PDMA_NO_ERR;
}

int16_t lc3_scsi_pdma_read_handshake(void *destBuffer, uint32_t byteCount)
{
    if (!destBuffer || byteCount == 0) return SCSI_PDMA_PARAM_ERR;

    volatile uint32_t *pData = (volatile uint32_t *)SONORA_PDMA_DATA;
    volatile uint8_t *pCtrl = (volatile uint8_t *)SONORA_SCSI_CTRL;
    uint32_t *dst = (uint32_t *)destBuffer;
    uint32_t lwCount = byteCount / 4;

    for (uint32_t i = 0; i < lwCount; i++) {
        // Poll for DRQ (Data Request bit)
        uint32_t timeout = 50000;
        while (!(*pCtrl & 0x01) && --timeout > 0);
        dst[i] = *pData;
    }
    return SCSI_PDMA_NO_ERR;
}

int16_t lc3_scsi_pdma_write_handshake(const void *srcBuffer, uint32_t byteCount)
{
    if (!srcBuffer || byteCount == 0) return SCSI_PDMA_PARAM_ERR;

    volatile uint32_t *pData = (volatile uint32_t *)SONORA_PDMA_DATA;
    volatile uint8_t *pCtrl = (volatile uint8_t *)SONORA_SCSI_CTRL;
    const uint32_t *src = (const uint32_t *)srcBuffer;
    uint32_t lwCount = byteCount / 4;

    for (uint32_t i = 0; i < lwCount; i++) {
        uint32_t timeout = 50000;
        while (!(*pCtrl & 0x01) && --timeout > 0);
        *pData = src[i];
    }
    return SCSI_PDMA_NO_ERR;
}

void lc3_scsi_reset_bus(void)
{
    volatile uint8_t *pCtrl = (volatile uint8_t *)SONORA_SCSI_CTRL;
    *pCtrl |= 0x80; // Assert SCSI RST line
    for (volatile int i = 0; i < 1000; i++); // Wait 25 microseconds
    *pCtrl &= ~0x80; // Release RST
}
