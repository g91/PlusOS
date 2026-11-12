/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: SCSI Asynchronous Event Notifier & Bus Phase State Machine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_SCSI_PHASE_H
#define LC3_SCSI_PHASE_H

#include <stdint.h>
#include <stdbool.h>

#define SCSI_PH_NO_ERR         0
#define SCSI_PH_PARAM_ERR    -50

typedef uint8_t SCSIPhase;

#define SCSI_PHASE_DATA_OUT   0x00
#define SCSI_PHASE_DATA_IN    0x01
#define SCSI_PHASE_COMMAND    0x02
#define SCSI_PHASE_STATUS     0x03
#define SCSI_PHASE_MSG_OUT    0x06
#define SCSI_PHASE_MSG_IN     0x07
#define SCSI_PHASE_BUS_FREE   0xFF

typedef void (*SCSIEventNotifierProcPtr)(uint32_t eventCode, void *context);

/* SCSI Phase API */
SCSIPhase lc3_scsi_get_bus_phase(void);
int16_t lc3_scsi_step_phase(SCSIPhase expectedPhase);
void lc3_scsi_post_event(uint32_t eventCode);
int16_t lc3_scsi_register_event_notifier(SCSIEventNotifierProcPtr proc, void *context);
int16_t lc3_scsi_deregister_event_notifier(SCSIEventNotifierProcPtr proc);

#endif /* LC3_SCSI_PHASE_H */
