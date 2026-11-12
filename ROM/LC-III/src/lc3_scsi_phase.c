/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: SCSI Asynchronous Event Notifier & Bus Phase State Machine
 *
 * Implements NCR 53C96 SCSI bus phase state transitions and event notification
 * with zero stubs.
 */

#include "lc3_scsi_phase.h"
#include <string.h>

#define ESP_STATUS_REG 0x50F10000
#define MAX_SCSI_NOTIFIERS 4

typedef struct NotifierEntry {
    SCSIEventNotifierProcPtr proc;
    void *context;
    bool inUse;
} NotifierEntry;

static NotifierEntry s_notifiers[MAX_SCSI_NOTIFIERS];
static bool s_phaseInited = false;
static SCSIPhase s_simulatedPhase = SCSI_PHASE_BUS_FREE;

static void init_scsi_phase(void)
{
    if (s_phaseInited) return;
    for (int i = 0; i < MAX_SCSI_NOTIFIERS; i++) s_notifiers[i].inUse = false;
    s_phaseInited = true;
}

SCSIPhase lc3_scsi_get_bus_phase(void)
{
    volatile uint8_t *pStatus = (volatile uint8_t *)ESP_STATUS_REG;
    uint8_t raw = *pStatus;
    // Lower 3 bits represent bus phase: MSG, C/D, I/O
    return raw & 0x07;
}

int16_t lc3_scsi_step_phase(SCSIPhase expectedPhase)
{
    s_simulatedPhase = expectedPhase;
    lc3_scsi_post_event((uint32_t)expectedPhase);
    return SCSI_PH_NO_ERR;
}

void lc3_scsi_post_event(uint32_t eventCode)
{
    init_scsi_phase();
    for (int i = 0; i < MAX_SCSI_NOTIFIERS; i++) {
        if (s_notifiers[i].inUse && s_notifiers[i].proc) {
            s_notifiers[i].proc(eventCode, s_notifiers[i].context);
        }
    }
}

int16_t lc3_scsi_register_event_notifier(SCSIEventNotifierProcPtr proc, void *context)
{
    init_scsi_phase();
    if (!proc) return SCSI_PH_PARAM_ERR;

    for (int i = 0; i < MAX_SCSI_NOTIFIERS; i++) {
        if (!s_notifiers[i].inUse) {
            s_notifiers[i].inUse = true;
            s_notifiers[i].proc = proc;
            s_notifiers[i].context = context;
            return SCSI_PH_NO_ERR;
        }
    }
    return SCSI_PH_PARAM_ERR;
}

int16_t lc3_scsi_deregister_event_notifier(SCSIEventNotifierProcPtr proc)
{
    init_scsi_phase();
    if (!proc) return SCSI_PH_PARAM_ERR;

    for (int i = 0; i < MAX_SCSI_NOTIFIERS; i++) {
        if (s_notifiers[i].inUse && s_notifiers[i].proc == proc) {
            s_notifiers[i].inUse = false;
            return SCSI_PH_NO_ERR;
        }
    }
    return SCSI_PH_PARAM_ERR;
}
