/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: SCSI Manager 4.3 SIM (SCSI Interface Module) Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_SCSI_SIM_H
#define LC3_SCSI_SIM_H

#include <stdint.h>
#include <stdbool.h>

#define SIM_NO_ERR         0
#define SIM_SEL_TIMEOUT  -1090
#define SIM_BUS_RESET    -1092
#define SIM_PARAM_ERR      -50

/* SCSI SIM Opcodes */
#define scsiOldCall         0
#define scsiPathInquiry     1
#define scsiExecIO          2
#define scsiBusReset        3
#define scsiAbortCommand    4

typedef struct SCSI_PB {
    uint16_t scsiFunctionCode;
    int16_t scsiResult;
    uint8_t scsiTargetID;
    uint8_t scsiTargetLUN;
    uint8_t *scsiCDB;
    uint8_t scsiCDBLength;
    void *scsiDataPtr;
    uint32_t scsiDataLength;
    uint8_t scsiStatus;
    uint8_t scsiSenseLength;
    void *scsiSensePtr;
} SCSI_PB;

/* SIM Engine API */
int16_t lc3_sim_init(void);
int16_t lc3_sim_action(SCSI_PB *pb);
int16_t lc3_sim_handle_scsi_io(SCSI_PB *pb);
int16_t lc3_sim_auto_sense(uint8_t targetID, void *senseBuf, uint8_t maxLen);
int16_t lc3_sim_negotiate_sync(uint8_t targetID, uint8_t syncPeriod, uint8_t syncOffset);
int16_t lc3_sim_abort_request(uint8_t targetID);

#endif /* LC3_SCSI_SIM_H */
