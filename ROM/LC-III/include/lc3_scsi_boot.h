/* lc3_scsi_boot.h - Macintosh LC III Internal SCSI Boot & Driver Loader Engine
 * Fully reverse-engineered from Universal ROM $067C (lc3_rom.map)
 * Offsets 0x0071F0 - 0x0094D2
 */
#ifndef LC3_SCSI_BOOT_H
#define LC3_SCSI_BOOT_H

#include <stdint.h>
#include <stdbool.h>

#define SCSI_DRVR_SIGNATURE  0x4552 /* 'ER' */
#define SCSI_PMAP_SIGNATURE  0x504D /* 'PM' */

/* Block 0 Driver Descriptor Record */
typedef struct {
    uint16_t sbSig;             /* device signature: 0x4552 */
    uint16_t sbBlkSize;         /* block size in bytes (usually 512) */
    uint32_t sbBlkCount;        /* total blocks on physical device */
    uint16_t sbDevType;         /* device type */
    uint16_t sbDevId;           /* device ID */
    uint32_t sbData;            /* driver data */
    uint16_t sbDrvrCount;       /* number of driver entries */
    struct {
        uint32_t ddBlock;       /* first block of driver image */
        uint16_t ddSize;        /* size of driver image in 512-byte blocks */
        uint16_t ddType;        /* system operating type: 1 = Mac OS */
    } ddMap[4];
} LC3ScsiDDR;

/* Partition Map Entry (Block 1+) */
typedef struct {
    uint16_t pmSig;             /* partition signature: 0x504D */
    uint16_t pmSigPad;
    uint32_t pmMapBlkCnt;       /* total partition map entries */
    uint32_t pmPyPartStart;     /* physical start block of partition */
    uint32_t pmPartBlkCnt;      /* length of partition in blocks */
    char     pmPartName[32];    /* partition name string */
    char     pmPartType[32];    /* partition type string (e.g. "Apple_HFS") */
    uint32_t pmLgDataStart;     /* logical start block */
    uint32_t pmDataCnt;         /* data block count */
    uint32_t pmPartStatus;      /* partition status flags (bootable, etc.) */
    uint32_t pmBootBlock;       /* logical block of boot code */
    uint32_t pmBootSize;        /* size of boot code */
    uint32_t pmBootLoad;        /* memory address to load boot code */
    uint32_t pmBootEntry;       /* jump address of boot code */
} LC3ScsiPartition;

/* Internal SCSI Boot & Driver API */
bool     lc3_scsi_boot_load(void);
bool     lc3_scsi_try_drive(uint8_t scsiID);
int16_t  lc3_scsi_read_blocks(uint8_t scsiID, uint32_t startBlock, uint16_t count, void *dest);
bool     lc3_scsi_load_driver(uint8_t scsiID, const LC3ScsiDDR *ddr);
bool     lc3_scsi_find_hfs_partition(uint8_t scsiID, LC3ScsiPartition *outPart);
int16_t  lc3_scsi_wait_bus_busy(uint32_t timeoutTicks);
int16_t  lc3_scsi_wait_bus_req(uint32_t timeoutTicks);
void     lc3_scsi_drain_bad_phase(void);

#endif /* LC3_SCSI_BOOT_H */
