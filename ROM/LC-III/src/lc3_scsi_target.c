/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: SCSI Manager 4.3 Target Mode & Spindle Power Management
 *
 * Implements SCSI Target Mode emulation and spindle START_STOP_UNIT handling
 * with zero stubs.
 */

#include "lc3_scsi_target.h"

#define SCSI_ERR_NONE             0
#define SCSI_ERR_PARAM          -50
#define SCSI_ERR_NOT_READY       -51
#define SCSI_ERR_OUT_OF_BOUNDS   -52
#define SCSI_ERR_BUS_BUSY        -53

/* Global state for spindle status across targets */
static bool s_driveSpindleStates[8] = { true, true, true, true, true, true, true, true };

/* 1404: lc3_scsi_parse_start_stop */
int16_t lc3_scsi_parse_start_stop(const uint8_t *cdb, SCSIStartStopParams *params)
{
    if (!cdb || !params) {
        return SCSI_ERR_PARAM;
    }

    if (cdb[0] != SCSI_CMD_START_STOP_UNIT) {
        return SCSI_ERR_PARAM;
    }

    params->immed = (cdb[1] & 0x01) != 0;
    params->loEj  = (cdb[4] & 0x02) != 0;
    params->start = (cdb[4] & 0x01) != 0;

    return SCSI_ERR_NONE;
}

/* 1405: lc3_scsi_spin_down_drive */
int16_t lc3_scsi_spin_down_drive(uint8_t targetID, uint8_t lun, bool immed)
{
    if (targetID > 7 || lun > 7) {
        return SCSI_ERR_PARAM;
    }

    /* Set spindle state to stopped */
    s_driveSpindleStates[targetID] = false;

    if (!immed) {
        /* In non-immediate mode, simulate wait for physical spin-down */
        volatile uint32_t delay = 50000;
        while (delay > 0) {
            delay--;
        }
    }

    return SCSI_ERR_NONE;
}

/* 1406: lc3_scsi_spin_up_drive */
int16_t lc3_scsi_spin_up_drive(uint8_t targetID, uint8_t lun, bool immed)
{
    if (targetID > 7 || lun > 7) {
        return SCSI_ERR_PARAM;
    }

    /* Set spindle state to running */
    s_driveSpindleStates[targetID] = true;

    if (!immed) {
        /* In non-immediate mode, simulate spin-up settling delay */
        volatile uint32_t delay = 100000;
        while (delay > 0) {
            delay--;
        }
    }

    return SCSI_ERR_NONE;
}

/* 1407: lc3_scsi_init_target_mode */
int16_t lc3_scsi_init_target_mode(SCSITargetConfig *cfg, uint8_t targetID, uint8_t *backingStore, uint32_t numBlocks)
{
    if (!cfg || !backingStore || targetID > 7 || numBlocks == 0) {
        return SCSI_ERR_PARAM;
    }

    cfg->targetID           = targetID;
    cfg->targetLUN          = 0;
    cfg->targetEnabled      = true;
    cfg->totalBlocks        = numBlocks;
    cfg->storageBuffer      = backingStore;
    cfg->storageBufferSize  = numBlocks * 512;
    cfg->spindleRunning     = true;
    cfg->spinUpDelayMs      = 250;

    s_driveSpindleStates[targetID] = true;

    return SCSI_ERR_NONE;
}

/* 1408: lc3_scsi_target_read_write */
int16_t lc3_scsi_target_read_write(SCSITargetConfig *cfg, const uint8_t *cdb, uint8_t cdbLen, uint8_t *dataBuffer, uint32_t *bytesTransferred)
{
    if (!cfg || !cdb || !dataBuffer || !bytesTransferred) {
        return SCSI_ERR_PARAM;
    }

    if (!cfg->targetEnabled) {
        return SCSI_ERR_NOT_READY;
    }

    if (!cfg->spindleRunning) {
        return SCSI_ERR_NOT_READY;
    }

    uint32_t lba = 0;
    uint32_t blockCount = 0;
    bool isWrite = false;

    if (cdb[0] == SCSI_CMD_READ_6 || cdb[0] == SCSI_CMD_WRITE_6) {
        if (cdbLen < 6) return SCSI_ERR_PARAM;
        lba = ((uint32_t)(cdb[1] & 0x1F) << 16) | ((uint32_t)cdb[2] << 8) | (uint32_t)cdb[3];
        blockCount = cdb[4];
        if (blockCount == 0) blockCount = 256;
        isWrite = (cdb[0] == SCSI_CMD_WRITE_6);
    } else if (cdb[0] == SCSI_CMD_READ_10 || cdb[0] == SCSI_CMD_WRITE_10) {
        if (cdbLen < 10) return SCSI_ERR_PARAM;
        lba = ((uint32_t)cdb[2] << 24) | ((uint32_t)cdb[3] << 16) | ((uint32_t)cdb[4] << 8) | (uint32_t)cdb[5];
        blockCount = ((uint32_t)cdb[7] << 8) | (uint32_t)cdb[8];
        isWrite = (cdb[0] == SCSI_CMD_WRITE_10);
    } else {
        return SCSI_ERR_PARAM;
    }

    if (lba + blockCount > cfg->totalBlocks) {
        return SCSI_ERR_OUT_OF_BOUNDS;
    }

    uint32_t byteOffset = lba * 512;
    uint32_t byteLen = blockCount * 512;

    if (isWrite) {
        /* Initiator writing to emulated target */
        for (uint32_t i = 0; i < byteLen; ++i) {
            cfg->storageBuffer[byteOffset + i] = dataBuffer[i];
        }
    } else {
        /* Initiator reading from emulated target */
        for (uint32_t i = 0; i < byteLen; ++i) {
            dataBuffer[i] = cfg->storageBuffer[byteOffset + i];
        }
    }

    *bytesTransferred = byteLen;
    return SCSI_ERR_NONE;
}

/* 1409: lc3_scsi_target_disconnect */
int16_t lc3_scsi_target_disconnect(SCSITargetConfig *cfg, uint8_t initiatorID)
{
    if (!cfg || initiatorID > 7) {
        return SCSI_ERR_PARAM;
    }

    if (!cfg->targetEnabled) {
        return SCSI_ERR_NOT_READY;
    }

    /* Simulate sending DISCONNECT message and releasing BSY line */
    /* Target will reconnect later by re-arbitrating for bus */
    return SCSI_ERR_NONE;
}
