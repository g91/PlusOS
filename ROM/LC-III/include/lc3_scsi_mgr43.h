/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: SCSI Manager 4.3 (Asynchronous SCSI Architecture / XPT / SIM)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_SCSI_MGR43_H
#define LC3_SCSI_MGR43_H

#include <stdint.h>
#include <stdbool.h>

#define SCSI_REQ_CMP          0x00 // Request completed without error
#define SCSI_BUS_FREE         0x01 // Bus free detected
#define SCSI_SEL_TO           0x02 // Selection timeout
#define SCSI_DATA_RUN_ERR     0x03 // Data overrun/underrun
#define SCSI_SEQUENCE_FLT     0x04 // Phase sequence failure
#define SCSI_REQUEST_IN_PROG  0xFF // Request in progress

typedef uint16_t SCSIBusID;

typedef struct SCSI_PB {
    void *qLink;
    uint32_t scsiReserved1;
    void (*scsiCompletion)(struct SCSI_PB *scsiPB);
    int16_t scsiFlags;
    int16_t scsiDriverStorage;
    int16_t scsiResult;
    uint8_t scsiFunctionCode;
    uint8_t scsiReserved2;
    uint8_t scsiDevice; // Target ID (0-7)
    uint8_t scsiLun;    // LUN (0-7)
    uint8_t *scsiDataPtr;
    uint32_t scsiDataLength;
    uint8_t *scsiCDB;
    uint8_t scsiCDBLength;
} SCSI_PB, *SCSI_PBPtr;

typedef struct SCSIBusDescriptor {
    SCSIBusID busID;
    char busName[32];
    uint8_t initiatorID;
    bool isRegistered;
} SCSIBusDescriptor;

/* SCSI Manager 4.3 API */
int16_t lc3_scsi_action(SCSI_PBPtr paramBlock);
int16_t lc3_scsi_register_bus(SCSIBusID busID, const char *busName, uint8_t initiatorID);
int16_t lc3_scsi_deregister_bus(SCSIBusID busID);
int16_t lc3_scsi_kill_io(SCSI_PBPtr paramBlock);
int16_t lc3_scsi_scan_bus(SCSIBusID busID, uint8_t *activeTargetsBitmap);
uint32_t lc3_scsi_mgr43_version(void);

#endif /* LC3_SCSI_MGR43_H */
