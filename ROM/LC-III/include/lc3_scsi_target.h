/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: SCSI Manager 4.3 Target Mode & Spindle Power Management
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_SCSI_TARGET_H
#define LC3_SCSI_TARGET_H

#include <stdint.h>
#include <stdbool.h>

#define SCSI_STATUS_GOOD                 0x00
#define SCSI_STATUS_CHECK_CONDITION      0x02
#define SCSI_STATUS_BUSY                 0x08

#define SCSI_CMD_TEST_UNIT_READY         0x00
#define SCSI_CMD_REQUEST_SENSE           0x03
#define SCSI_CMD_READ_6                  0x08
#define SCSI_CMD_WRITE_6                 0x0A
#define SCSI_CMD_START_STOP_UNIT         0x1B
#define SCSI_CMD_READ_10                 0x28
#define SCSI_CMD_WRITE_10                0x2A

#define SCSI_PHASE_DATA_OUT              0
#define SCSI_PHASE_DATA_IN               1
#define SCSI_PHASE_COMMAND               2
#define SCSI_PHASE_STATUS                3
#define SCSI_PHASE_MSG_OUT               6
#define SCSI_PHASE_MSG_IN                7

#define SCSI_MSG_COMMAND_COMPLETE        0x00
#define SCSI_MSG_SAVE_DATA_POINTER       0x02
#define SCSI_MSG_RESTORE_POINTERS        0x03
#define SCSI_MSG_DISCONNECT              0x04

typedef struct SCSITargetConfig {
    uint8_t targetID;            /* Local target ID (0-7) */
    uint8_t targetLUN;           /* Logical Unit Number */
    bool targetEnabled;          /* True if target mode active */
    uint32_t totalBlocks;        /* Emulated drive capacity in 512-byte blocks */
    uint8_t *storageBuffer;      /* Memory backing emulated disk */
    uint32_t storageBufferSize;  /* Backing store size in bytes */
    bool spindleRunning;         /* True if drive spindle is spinning */
    uint16_t spinUpDelayMs;      /* Simulated spin-up latency */
} SCSITargetConfig;

typedef struct SCSIStartStopParams {
    bool immed;                  /* Return status immediately before completion */
    bool loEj;                   /* Load or Eject medium */
    bool start;                  /* 1 = spin up, 0 = spin down */
} SCSIStartStopParams;

/* Public API */
int16_t lc3_scsi_parse_start_stop(const uint8_t *cdb, SCSIStartStopParams *params);
int16_t lc3_scsi_spin_down_drive(uint8_t targetID, uint8_t lun, bool immed);
int16_t lc3_scsi_spin_up_drive(uint8_t targetID, uint8_t lun, bool immed);
int16_t lc3_scsi_init_target_mode(SCSITargetConfig *cfg, uint8_t targetID, uint8_t *backingStore, uint32_t numBlocks);
int16_t lc3_scsi_target_read_write(SCSITargetConfig *cfg, const uint8_t *cdb, uint8_t cdbLen, uint8_t *dataBuffer, uint32_t *bytesTransferred);
int16_t lc3_scsi_target_disconnect(SCSITargetConfig *cfg, uint8_t initiatorID);

#endif /* LC3_SCSI_TARGET_H */
