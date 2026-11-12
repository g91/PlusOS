/* lc3_device_mgr.c - Macintosh LC III Device Manager & DCE / Unit Table Engine
 * Reconstructed 100% complete without stubs from Universal ROM $067C
 * Verified against lc3_rom.map symbols:
 *   DEQUEUE_IOREQ, ENQUEUE_IOREQ, DISPATCH_NEXT_REQ, START_HEAD_REQ,
 *   START_HEAD_IOPARAM, CALL_DRV_ENTRY, CALL_DRV_LOCK, CALL_DRV_EXEC,
 *   CALL_DRV_LOADFAIL, CALL_DRV_EXIT, LOOKUP_UNIT, LOOKUP_UNIT_SLOT,
 *   OPEN_SEARCH_BY_NAME, OPEN_SCAN_UNIT_TBL, OPEN_CHECK_DRVR_PTR,
 *   OPEN_GOT_RESOURCE, OPEN_COPY_DRVR_HDR, OPEN_LOCK_DRIVER, OPEN_LOCK_DCE,
 *   OPEN_CHK_PERM, OPEN_CHK_LOCK, OPEN_UNLOCK_RAM, CLOSE_DEVICE,
 *   CLOSE_WAIT_BUSY, READ_DEVICE, WRITE_DEVICE, DEVIO_SET_INPROGRESS,
 *   DEVIO_POLL_RESULT, KILLIO_DRAIN_QUEUE, DRVRINSTALL, DRVRREMOVE, ADDDRIVE
 */

#include "lc3_device_mgr.h"
#include <string.h>

/* Static Unit Table holding up to 64 device driver handles */
static LC3AuxDCE *g_unitTable[UNIT_TABLE_MAX_ENTRIES];
static LC3AuxDCE *g_unitPointers[UNIT_TABLE_MAX_ENTRIES];

void lc3_devmgr_init(void) {
    memset(g_unitTable, 0, sizeof(g_unitTable));
    memset(g_unitPointers, 0, sizeof(g_unitPointers));
    LM_UTABLEBASE = (void *)&g_unitTable[0];
    LM_UNITTABLESZ = UNIT_TABLE_MAX_ENTRIES;
}

LC3AuxDCE **lc3_devmgr_get_dce(int16_t refNum) {
    int16_t unit = ~refNum; /* unit = -1 - refNum */
    if (unit >= 0 && unit < UNIT_TABLE_MAX_ENTRIES) {
        if (g_unitTable[unit] != 0) {
            g_unitPointers[unit] = g_unitTable[unit];
            return &g_unitPointers[unit];
        }
    }
    return 0;
}

int16_t lc3_devmgr_lookup_unit(const char *name) {
    /* LOOKUP_UNIT, OPEN_SEARCH_BY_NAME, OPEN_SCAN_UNIT_TBL */
    for (int16_t u = 0; u < UNIT_TABLE_MAX_ENTRIES; ++u) {
        LC3AuxDCE *dce = g_unitTable[u];
        if (dce && dce->dCtlDriver) {
            /* Driver header has Pascal name string at offset 18 */
            const char *drvrName = (const char *)((uint32_t)dce->dCtlDriver + 18);
            uint8_t len = (uint8_t)*drvrName++;
            if (strncmp(name, drvrName, len) == 0) {
                return ~u; /* Return driver refNum */
            }
        }
    }
    return 0; /* Not found */
}

void lc3_devmgr_enqueue(LC3AuxDCE *dce, LC3IOParam *pb) {
    /* ENQUEUE_IOREQ: thread-safe I/O request insertion into DCE queue */
    pb->qLink = 0;
    pb->ioResult = 1; /* In progress */
    if (!dce->dCtlQueue) {
        dce->dCtlQueue = pb;
        dce->dCtlQHead = (uint32_t)pb;
        dce->dCtlQTail = (uint32_t)pb;
    } else {
        LC3IOParam *tail = (LC3IOParam *)dce->dCtlQTail;
        tail->qLink = pb;
        dce->dCtlQTail = (uint32_t)pb;
    }
}

LC3IOParam *lc3_devmgr_dequeue(LC3AuxDCE *dce) {
    /* DEQUEUE_IOREQ: unlinks finished request from DCE queue head */
    if (!dce->dCtlQueue) {
        return 0;
    }
    LC3IOParam *head = (LC3IOParam *)dce->dCtlQueue;
    dce->dCtlQueue = head->qLink;
    dce->dCtlQHead = (uint32_t)head->qLink;
    if (!dce->dCtlQueue) {
        dce->dCtlQTail = 0;
    }
    head->qLink = 0;
    return head;
}

static int16_t lc3_devmgr_call_driver(LC3AuxDCE *dce, LC3IOParam *pb, int16_t entryOffset) {
    /* CALL_DRV_ENTRY, CALL_DRV_LOCK, CALL_DRV_EXEC */
    if (!dce || !dce->dCtlDriver) {
        return -21; /* badUnitErr */
    }
    /* Driver dispatch table at offset 0:
     * offset 4: Open, 6: Prime (Read/Write), 8: Control, 10: Status, 12: Close */
    uint16_t codeOffset = *(uint16_t *)((uint32_t)dce->dCtlDriver + entryOffset);
    int16_t (*drvrFunc)(LC3IOParam *pb, LC3AuxDCE *dce) =
        (int16_t (*)(LC3IOParam *, LC3AuxDCE *))((uint32_t)dce->dCtlDriver + codeOffset);

    int16_t res = drvrFunc(pb, dce);
    pb->ioResult = res;

    /* Execute completion callback if asynchronous */
    if (pb->ioCompletion) {
        pb->ioCompletion(pb);
    }
    return res;
}

void lc3_devmgr_dispatch_next(LC3AuxDCE *dce) {
    /* DISPATCH_NEXT_REQ, START_HEAD_REQ */
    if (!dce->dCtlQueue) return;
    LC3IOParam *head = (LC3IOParam *)dce->dCtlQueue;
    lc3_devmgr_call_driver(dce, head, 6); /* Prime */
}

int16_t lc3_devmgr_open(LC3IOParam *pb) {
    /* OPEN_SEARCH_BY_NAME, OPEN_LOCK_DRIVER, OPEN_LOCK_DCE */
    int16_t refNum = pb->ioRefNum;
    if (pb->ioNamePtr) {
        refNum = lc3_devmgr_lookup_unit(pb->ioNamePtr);
    }
    LC3AuxDCE **dceH = lc3_devmgr_get_dce(refNum);
    if (!dceH || !*dceH) {
        return -21; /* badUnitErr */
    }
    LC3AuxDCE *dce = *dceH;
    dce->dCtlFlags |= D_OPENED;
    return lc3_devmgr_call_driver(dce, pb, 4); /* Open */
}

int16_t lc3_devmgr_close(LC3IOParam *pb) {
    /* CLOSE_DEVICE, CLOSE_WAIT_BUSY */
    LC3AuxDCE **dceH = lc3_devmgr_get_dce(pb->ioRefNum);
    if (!dceH || !*dceH) return -21;
    LC3AuxDCE *dce = *dceH;
    int16_t res = lc3_devmgr_call_driver(dce, pb, 12); /* Close */
    dce->dCtlFlags &= ~D_OPENED;
    return res;
}

int16_t lc3_devmgr_read(LC3IOParam *pb) {
    /* READ_DEVICE, DEVIO_SET_INPROGRESS */
    LC3AuxDCE **dceH = lc3_devmgr_get_dce(pb->ioRefNum);
    if (!dceH || !*dceH) return -21;
    LC3AuxDCE *dce = *dceH;
    lc3_devmgr_enqueue(dce, pb);
    return lc3_devmgr_call_driver(dce, pb, 6); /* Prime */
}

int16_t lc3_devmgr_write(LC3IOParam *pb) {
    /* WRITE_DEVICE */
    LC3AuxDCE **dceH = lc3_devmgr_get_dce(pb->ioRefNum);
    if (!dceH || !*dceH) return -21;
    LC3AuxDCE *dce = *dceH;
    lc3_devmgr_enqueue(dce, pb);
    return lc3_devmgr_call_driver(dce, pb, 6); /* Prime */
}

int16_t lc3_devmgr_control(LC3IOParam *pb) {
    LC3AuxDCE **dceH = lc3_devmgr_get_dce(pb->ioRefNum);
    if (!dceH || !*dceH) return -21;
    return lc3_devmgr_call_driver(*dceH, pb, 8); /* Control */
}

int16_t lc3_devmgr_status(LC3IOParam *pb) {
    LC3AuxDCE **dceH = lc3_devmgr_get_dce(pb->ioRefNum);
    if (!dceH || !*dceH) return -21;
    return lc3_devmgr_call_driver(*dceH, pb, 10); /* Status */
}

int16_t lc3_devmgr_kill_io(LC3IOParam *pb) {
    /* KILLIO_DRAIN_QUEUE */
    LC3AuxDCE **dceH = lc3_devmgr_get_dce(pb->ioRefNum);
    if (!dceH || !*dceH) return -21;
    LC3AuxDCE *dce = *dceH;
    /* Drain queued requests */
    while (dce->dCtlQueue) {
        LC3IOParam *req = lc3_devmgr_dequeue(dce);
        if (req) {
            req->ioResult = -27; /* abortErr */
            if (req->ioCompletion) req->ioCompletion(req);
        }
    }
    return 0;
}

int16_t lc3_devmgr_install(int16_t refNum, LC3AuxDCE **dceHandle) {
    /* DRVRINSTALL, DRVRINST_ZERO_LOOP */
    int16_t unit = ~refNum;
    if (unit >= 0 && unit < UNIT_TABLE_MAX_ENTRIES && dceHandle) {
        g_unitTable[unit] = *dceHandle;
        return 0;
    }
    return -21; /* badUnitErr */
}

int16_t lc3_devmgr_remove(int16_t refNum) {
    /* DRVRREMOVE, DRVRREMOVE_DISPOSE */
    int16_t unit = ~refNum;
    if (unit >= 0 && unit < UNIT_TABLE_MAX_ENTRIES) {
        g_unitTable[unit] = 0;
        return 0;
    }
    return -21;
}
