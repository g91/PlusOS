/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: SCSI Manager Blind Transfer Handshake & FIFO Optimizations
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_SCSI_BLIND_H
#define LC3_SCSI_BLIND_H

#include <stdint.h>
#include <stdbool.h>

#define SCSI_BLIND_NO_ERR         0
#define SCSI_BLIND_PHASE_ERR   -1091
#define SCSI_BLIND_TIMEOUT     -1090
#define SCSI_BLIND_PARAM_ERR     -50

typedef struct SCSITransferContext {
    uint8_t targetID;
    uint8_t currentPhase;
    uint32_t bytesRemaining;
    void *currentBuffer;
    bool disconnected;
} SCSITransferContext;

/* SCSI Blind Transfer API */
int16_t lc3_scsi_blind_read_unrolled(void *destBuffer, uint32_t sectorCount);
int16_t lc3_scsi_blind_write_unrolled(const void *srcBuffer, uint32_t sectorCount);
bool lc3_scsi_check_fifo_threshold(uint8_t minBytes);
int16_t lc3_scsi_save_disconnect_context(SCSITransferContext *ctx);
int16_t lc3_scsi_restore_reselect_context(const SCSITransferContext *ctx);
int16_t lc3_scsi_handle_phase_mismatch(uint8_t expectedPhase, uint8_t actualPhase);

#endif /* LC3_SCSI_BLIND_H */
