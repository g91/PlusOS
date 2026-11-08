/* plusnet/scsimgr.h - the ROM SCSI Manager (Macintosh Plus and SE).
 *
 * Every call is one trap, _SCSIDispatch ($A815), with a selector word pushed
 * last (the classic Inside Macintosh IV calling sequence):
 *     move.w #selector,-(sp) ; _SCSIDispatch
 * They are stack-based pascal functions returning OSErr, declared here in the
 * same `pascal ... = { words }` inline form the generated ROM headers use.
 * This header is SDK-neutral (no plusrom/serom includes) so one library
 * builds for both machines.
 *
 * Sequence for one SCSI command:
 *     SCSIGet()  SCSISelect(id)  SCSICmd(cdb,6)  [SCSIRead/SCSIWrite(tib)]
 *     SCSIComplete(&stat,&msg,timeoutTicks)
 */
#ifndef PLUSNET_SCSIMGR_H
#define PLUSNET_SCSIMGR_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Transfer Instruction Block opcodes */
enum { scInc = 1, scNoInc = 2, scAdd = 3, scMove = 4, scLoop = 5, scNop = 6, scStop = 7, scComp = 8 };

/* SCSI Manager result codes */
enum {
    scCommErr      = 2,     /* communications error (no device at that ID / bus error) */
    scArbNBErr     = 3,     /* arbitration failed: bus busy */
    scBadParmsErr  = 4,
    scPhaseErr     = 5,     /* the target changed phase before the TIB finished (short data) */
    scCompareErr   = 6,
    scMgrBusyErr   = 7,
    scSequenceErr  = 8,
    scBusTOErr     = 9,     /* bus timeout */
    scComplPhaseErr = 10
};

typedef struct {
    uint16_t scOpcode;
    uint32_t scParam1;      /* scInc/scNoInc: buffer address */
    uint32_t scParam2;      /* scInc/scNoInc: byte count */
} __attribute__((packed)) SCSIInstr;

#define PLUSNET_SCSI_TRAP(sel) = { 0x3F3C, sel, 0xA815 }

pascal int16_t SCSIReset(void)                                            PLUSNET_SCSI_TRAP(0x0000);
pascal int16_t SCSIGet(void)                                              PLUSNET_SCSI_TRAP(0x0001);
pascal int16_t SCSISelect(int16_t targetID)                               PLUSNET_SCSI_TRAP(0x0002);
pascal int16_t SCSICmd(void *buffer, int16_t count)                       PLUSNET_SCSI_TRAP(0x0003);
pascal int16_t SCSIComplete(int16_t *stat, int16_t *message, uint32_t wait) PLUSNET_SCSI_TRAP(0x0004);
pascal int16_t SCSIRead(void *tibPtr)                                     PLUSNET_SCSI_TRAP(0x0005);
pascal int16_t SCSIWrite(void *tibPtr)                                    PLUSNET_SCSI_TRAP(0x0006);
pascal int16_t SCSIRBlind(void *tibPtr)                                   PLUSNET_SCSI_TRAP(0x0008);
pascal int16_t SCSIWBlind(void *tibPtr)                                   PLUSNET_SCSI_TRAP(0x0009);
pascal int16_t SCSIStat(void)                                             PLUSNET_SCSI_TRAP(0x000A);

#ifdef __cplusplus
}
#endif
#endif /* PLUSNET_SCSIMGR_H */
