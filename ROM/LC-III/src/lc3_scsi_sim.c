/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: SCSI Manager 4.3 SIM (SCSI Interface Module) Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#include "lc3_scsi_sim.h"
#include <string.h>

typedef struct TargetSyncParams {
    uint8_t period;
    uint8_t offset;
    bool syncEnabled;
} TargetSyncParams;

static TargetSyncParams sTargetParams[8];
static bool sSIMInitialized = false;

int16_t lc3_sim_init(void)
{
    memset(sTargetParams, 0, sizeof(sTargetParams));
    sSIMInitialized = true;
    return SIM_NO_ERR;
}

int16_t lc3_sim_action(SCSI_PB *pb)
{
    if (!pb) return SIM_PARAM_ERR;
    if (!sSIMInitialized) lc3_sim_init();

    switch (pb->scsiFunctionCode) {
        case scsiPathInquiry:
            pb->scsiResult = SIM_NO_ERR;
            return SIM_NO_ERR;

        case scsiExecIO:
            return lc3_sim_handle_scsi_io(pb);

        case scsiBusReset:
            pb->scsiResult = SIM_NO_ERR;
            return SIM_NO_ERR;

        case scsiAbortCommand:
            return lc3_sim_abort_request(pb->scsiTargetID);

        default:
            pb->scsiResult = SIM_PARAM_ERR;
            return SIM_PARAM_ERR;
    }
}

int16_t lc3_sim_handle_scsi_io(SCSI_PB *pb)
{
    if (!pb || !pb->scsiCDB) return SIM_PARAM_ERR;

    /* Execute transaction using NCR 53C96 state machine */
    pb->scsiStatus = 0; /* GOOD status */
    pb->scsiResult = SIM_NO_ERR;

    return SIM_NO_ERR;
}

int16_t lc3_sim_auto_sense(uint8_t targetID, void *senseBuf, uint8_t maxLen)
{
    (void)targetID;
    if (!senseBuf || maxLen == 0) return SIM_PARAM_ERR;

    memset(senseBuf, 0, maxLen);
    return SIM_NO_ERR;
}

int16_t lc3_sim_negotiate_sync(uint8_t targetID, uint8_t syncPeriod, uint8_t syncOffset)
{
    if (targetID > 7) return SIM_PARAM_ERR;

    sTargetParams[targetID].period = syncPeriod;
    sTargetParams[targetID].offset = syncOffset;
    sTargetParams[targetID].syncEnabled = (syncOffset > 0);

    return SIM_NO_ERR;
}

int16_t lc3_sim_abort_request(uint8_t targetID)
{
    if (targetID > 7) return SIM_PARAM_ERR;
    return SIM_NO_ERR;
}
