/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: SCSI Manager Pseudo-DMA Handshake & 32-Bit Burst Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_SCSI_PDMA_H
#define LC3_SCSI_PDMA_H

#include <stdint.h>
#include <stdbool.h>

#define SCSI_PDMA_NO_ERR         0
#define SCSI_PDMA_TIMEOUT     -1090
#define SCSI_PDMA_PARAM_ERR     -50

/* SCSI Pseudo-DMA API */
int16_t lc3_scsi_pdma_read_blind512(void *destBuffer);
int16_t lc3_scsi_pdma_write_blind512(const void *srcBuffer);
int16_t lc3_scsi_pdma_read_handshake(void *destBuffer, uint32_t byteCount);
int16_t lc3_scsi_pdma_write_handshake(const void *srcBuffer, uint32_t byteCount);
void lc3_scsi_reset_bus(void);

#endif /* LC3_SCSI_PDMA_H */
