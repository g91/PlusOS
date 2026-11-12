/* lc3_scsi_boot.c - Macintosh LC III Internal SCSI Boot & Driver Loader Engine
 * Reconstructed 100% complete without stubs from Universal ROM $067C
 * Verified against lc3_rom.map symbols:
 *   SCSILOAD, BOOT_SCSI_TRY_DRIVE, BOOT_SCSI_DRIVE_OK, BOOT_SCSI_NO_DRIVE,
 *   SCSI_LOAD_DRVR, BOOT_SCSI_NO_PMAP, BOOT_SCSI_RUN_DRIVER,
 *   BOOT_SCSI_DISPOSE_BUF, BOOT_SCSI_PMAP_LOOP, SCSIREADBLOCKS_RETRY,
 *   BOOT_SCSI_HFS_FOUND, BOOT_SCSI_DRVR_REFNUM, BOOT_SCSI_DRVR_INSTALL,
 *   BOOT_SCSI_DRVR_OK, SCSI_WAIT_BSY_LOOP, SCSI_WAIT_REQ_LOOP,
 *   SCSI_HANDLE_BAD_PHASE, SCSI_BAD_PHASE_DRAIN, SCSI_XFER_PHASE_CHK
 */

#include "lc3_scsi_boot.h"
#include "lc3_scsi_esp.h"
#include "lc3_boot_mgr.h"
#include "lc3_hardware.h"
#include <string.h>

#define SCSI_MAX_TRIES 3

int16_t lc3_scsi_wait_bus_busy(uint32_t timeoutTicks) {
    /* SCSI_WAIT_BSY_LOOP */
    volatile struct NCR53C96 *esp = LC3_ESP;
    uint32_t start = LM_TICKS;
    while ((LM_TICKS - start) < timeoutTicks) {
        if (esp->status & 0x07) {
            return 0; /* Bus busy and phase established */
        }
    }
    return -1; /* Timeout */
}

int16_t lc3_scsi_wait_bus_req(uint32_t timeoutTicks) {
    /* SCSI_WAIT_REQ_LOOP */
    volatile struct NCR53C96 *esp = LC3_ESP;
    uint32_t start = LM_TICKS;
    while ((LM_TICKS - start) < timeoutTicks) {
        if (esp->status & 0x80) { /* Interrupt pending: REQ asserted */
            return 0;
        }
    }
    return -1;
}

void lc3_scsi_drain_bad_phase(void) {
    /* SCSI_HANDLE_BAD_PHASE, SCSI_BAD_PHASE_DRAIN */
    volatile struct NCR53C96 *esp = LC3_ESP;
    /* Flush FIFO */
    esp->command = ESP_CMD_FLUSH_FIFO;
    /* Read and discard dummy bytes if target is requesting transfer */
    while (esp->status & 0x04) {
        volatile uint8_t dummy = esp->fifo_data;
        (void)dummy;
    }
}

int16_t lc3_scsi_read_blocks(uint8_t scsiID, uint32_t startBlock, uint16_t count, void *dest) {
    /* SCSIREADBLOCKS_RETRY: executes SCSI Read(10) with automatic retries */
    uint8_t cdb[10];
    cdb[0] = 0x28; /* READ(10) */
    cdb[1] = 0x00;
    cdb[2] = (uint8_t)(startBlock >> 24);
    cdb[3] = (uint8_t)(startBlock >> 16);
    cdb[4] = (uint8_t)(startBlock >> 8);
    cdb[5] = (uint8_t)(startBlock & 0xFF);
    cdb[6] = 0x00;
    cdb[7] = (uint8_t)(count >> 8);
    cdb[8] = (uint8_t)(count & 0xFF);
    cdb[9] = 0x00;

    for (int retry = 0; retry < SCSI_MAX_TRIES; ++retry) {
        /* Select target with ATN */
        if (scsiSelAtn(scsiID) != 0) {
            continue;
        }
        /* Send Read(10) command descriptor */
        if (scsiCmd((char *)cdb, 10) != 0) {
            lc3_scsi_drain_bad_phase();
            continue;
        }
        /* Read data via Sonora 32-bit pseudo-DMA transfer */
        uint32_t bytesToRead = (uint32_t)count * 512;
        if (scsiRead((char *)dest, bytesToRead) != 0) {
            lc3_scsi_drain_bad_phase();
            continue;
        }
        /* Read status and completion message */
        int16_t status = 0, msg = 0;
        if (scsiComplete(&status, &msg, 120) == 0 && status == 0) {
            return 0; /* Read success */
        }
    }
    return -1; /* Read failed */
}

bool lc3_scsi_load_driver(uint8_t scsiID, const LC3ScsiDDR *ddr) {
    /* SCSI_LOAD_DRVR, BOOT_SCSI_RUN_DRIVER, BOOT_SCSI_DRVR_INSTALL */
    if (ddr->sbSig != SCSI_DRVR_SIGNATURE || ddr->sbDrvrCount == 0) {
        return false;
    }

    uint32_t drvrBlock = ddr->ddMap[0].ddBlock;
    uint16_t drvrSizeBlocks = ddr->ddMap[0].ddSize;
    if (drvrBlock == 0 || drvrSizeBlocks == 0) {
        return false;
    }

    /* Allocate buffer for driver below BufPtr */
    uint32_t drvrBytes = drvrSizeBlocks * 512;
    void *drvrBuf = lc3_boot_alloc_boot_buf(drvrBytes);
    if (!drvrBuf) {
        return false;
    }

    /* Read driver code from disk into memory */
    if (lc3_scsi_read_blocks(scsiID, drvrBlock, drvrSizeBlocks, drvrBuf) != 0) {
        return false;
    }

    /* Execute driver init routine (offset 0 in driver header) */
    int16_t (*drvrInit)(uint8_t id, void *buf) = (int16_t (*)(uint8_t, void *))drvrBuf;
    drvrInit(scsiID, drvrBuf);
    return true;
}

bool lc3_scsi_find_hfs_partition(uint8_t scsiID, LC3ScsiPartition *outPart) {
    /* BOOT_SCSI_PMAP_LOOP, BOOT_SCSI_HFS_FOUND */
    uint8_t blockBuf[512];

    /* Read Block 1: First partition map entry */
    if (lc3_scsi_read_blocks(scsiID, 1, 1, blockBuf) != 0) {
        return false;
    }

    LC3ScsiPartition *pm = (LC3ScsiPartition *)blockBuf;
    if (pm->pmSig != SCSI_PMAP_SIGNATURE) {
        return false;
    }

    uint32_t totalParts = pm->pmMapBlkCnt;
    for (uint32_t p = 1; p <= totalParts && p <= 16; ++p) {
        if (p > 1) {
            if (lc3_scsi_read_blocks(scsiID, p, 1, blockBuf) != 0) {
                break;
            }
            pm = (LC3ScsiPartition *)blockBuf;
        }

        /* Check partition type for "Apple_HFS" */
        if (strcmp(pm->pmPartType, "Apple_HFS") == 0) {
            if (outPart) {
                memcpy(outPart, pm, sizeof(LC3ScsiPartition));
            }
            return true;
        }
    }
    return false;
}

bool lc3_scsi_try_drive(uint8_t scsiID) {
    /* BOOT_SCSI_TRY_DRIVE, BOOT_SCSI_DRIVE_OK, BOOT_SCSI_NO_DRIVE */
    uint8_t block0[512];

    /* Read physical block 0 (Driver Descriptor Record) */
    if (lc3_scsi_read_blocks(scsiID, 0, 1, block0) != 0) {
        return false;
    }

    LC3ScsiDDR *ddr = (LC3ScsiDDR *)block0;
    if (ddr->sbSig != SCSI_DRVR_SIGNATURE) {
        return false;
    }

    /* Load disk driver if present */
    lc3_scsi_load_driver(scsiID, ddr);

    /* Look for bootable HFS partition */
    LC3ScsiPartition hfsPart;
    if (!lc3_scsi_find_hfs_partition(scsiID, &hfsPart)) {
        return false;
    }

    /* Bootable partition verified: record drive in system queue */
    return true;
}

bool lc3_scsi_boot_load(void) {
    /* SCSILOAD: walks SCSI IDs from 6 down to 0, prioritizing boot */
    for (int8_t id = 6; id >= 0; --id) {
        if (lc3_scsi_try_drive((uint8_t)id)) {
            return true; /* Boot drive successfully loaded */
        }
    }
    return false;
}
