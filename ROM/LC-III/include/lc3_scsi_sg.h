/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: SCSI Manager 4.3 Virtual Memory Scatter-Gather DMA Driver
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_SCSI_SG_H
#define LC3_SCSI_SG_H

#include <stdint.h>
#include <stdbool.h>

#define SCSI_SG_NO_ERR         0
#define SCSI_SG_PAGE_FAULT    -1
#define SCSI_SG_PARAM_ERR    -50

#define MAX_SG_SEGMENTS       16
#define MAC_PAGE_SIZE       4096

typedef struct ScatterGatherEntry {
    void *physAddr;
    uint32_t byteCount;
} ScatterGatherEntry;

typedef struct ScatterGatherContext {
    ScatterGatherEntry entries[MAX_SG_SEGMENTS];
    uint16_t numEntries;
    uint32_t totalBytes;
    uint32_t residualBytes;
} ScatterGatherContext;

/* Scatter-Gather DMA API */
int16_t lc3_scsi_sg_init(ScatterGatherContext *ctx);
int16_t lc3_scsi_sg_add_segment(ScatterGatherContext *ctx, void *physAddr, uint32_t byteCount);
int16_t lc3_scsi_sg_execute_read(ScatterGatherContext *ctx, uint8_t targetID);
int16_t lc3_scsi_sg_execute_write(ScatterGatherContext *ctx, uint8_t targetID);
uint32_t lc3_scsi_sg_calc_residual(const ScatterGatherContext *ctx, uint32_t bytesTransferred);
int16_t lc3_scsi_sg_lock_pages(const ScatterGatherContext *ctx);

#endif /* LC3_SCSI_SG_H */
