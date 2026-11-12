/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: SCSI Manager 4.3 Parameter Block Validator & Error Mapping
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_SCSI_ERR_MAP_H
#define LC3_SCSI_ERR_MAP_H

#include <stdint.h>
#include <stdbool.h>

#define SCSI_NO_ERR         0
#define SCSI_PARAM_ERR    -50

/* Mac OS Disk OSErr definitions */
#define ioErr             -36
#define fnOpnErr          -38
#define eofErr            -39
#define posErr            -40
#define mFulErr           -41
#define tmfoErr           -42
#define wPrErr            -44
#define fLckdErr          -45
#define nsDrvErr          -56
#define noMacDskErr       -57
#define offLinErr         -65

typedef struct SCSIExtendedSense {
    uint8_t errorCode;        /* 0x70 or 0x71 */
    uint8_t segmentNum;
    uint8_t senseKey;         /* 0x0 to 0xF */
    uint8_t infoBytes[4];
    uint8_t addSenseLength;
    uint8_t cmdSpecific[4];
    uint8_t asc;              /* Additional Sense Code */
    uint8_t ascq;             /* Additional Sense Code Qualifier */
    uint8_t fruc;
    uint8_t senseKeySpecific[3];
} SCSIExtendedSense;

typedef void (*SCSICompletionProcPtr)(void *pb);

typedef struct SCSIValidationPB {
    uint8_t targetID;
    uint8_t lun;
    uint8_t cdbLength;
    const uint8_t *cdb;
    void *buffer;
    uint32_t bufferLength;
    SCSICompletionProcPtr completion;
} SCSIValidationPB;

/* SCSI Error Mapping API */
int16_t lc3_scsi_validate_pb(const SCSIValidationPB *pb);
int16_t lc3_scsi_map_status_to_oserr(uint8_t scsiStatus);
int16_t lc3_scsi_decode_sense_key(uint8_t senseKey);
int16_t lc3_scsi_map_asc_ascq(uint8_t asc, uint8_t ascq);
bool lc3_scsi_verify_buffer_clean(const void *bufPtr, uint32_t length);
void lc3_scsi_invoke_completion(SCSICompletionProcPtr proc, void *pb);

#endif /* LC3_SCSI_ERR_MAP_H */
