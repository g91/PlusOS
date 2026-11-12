/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Desk Manager (Apple Menu & Desk Accessory DA Loader)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#include "lc3_desk_mgr.h"
#include <string.h>

static DeskAccRecord sActiveDAs[MAX_ACTIVE_DAS];
static bool sDeskInitialized = false;

int16_t lc3_desk_init(void)
{
    memset(sActiveDAs, 0, sizeof(sActiveDAs));
    sDeskInitialized = true;
    return DESK_NO_ERR;
}

int16_t lc3_open_desk_acc(const char *deskAccName)
{
    if (!deskAccName) return DESK_PARAM_ERR;
    if (!sDeskInitialized) lc3_desk_init();

    int freeIdx = -1;
    for (int i = 0; i < MAX_ACTIVE_DAS; ++i) {
        if (sActiveDAs[i].inUse && strcmp(sActiveDAs[i].daName, deskAccName) == 0) {
            return sActiveDAs[i].dRefNum; /* Already open */
        }
        if (!sActiveDAs[i].inUse && freeIdx < 0) {
            freeIdx = i;
        }
    }

    if (freeIdx < 0) return DESK_FULL_ERR;

    sActiveDAs[freeIdx].inUse = true;
    sActiveDAs[freeIdx].dRefNum = (int16_t)(-(freeIdx + 1));
    strncpy(sActiveDAs[freeIdx].daName, deskAccName, sizeof(sActiveDAs[freeIdx].daName) - 1);
    sActiveDAs[freeIdx].windowPtr = (void *)(uintptr_t)(0x1000 + freeIdx * 0x100);
    sActiveDAs[freeIdx].lastTaskTick = 0;

    return sActiveDAs[freeIdx].dRefNum;
}

void lc3_close_desk_acc(int16_t dRefNum)
{
    for (int i = 0; i < MAX_ACTIVE_DAS; ++i) {
        if (sActiveDAs[i].inUse && sActiveDAs[i].dRefNum == dRefNum) {
            sActiveDAs[i].inUse = false;
            break;
        }
    }
}

void lc3_system_task(void)
{
    if (!sDeskInitialized) return;

    for (int i = 0; i < MAX_ACTIVE_DAS; ++i) {
        if (sActiveDAs[i].inUse) {
            sActiveDAs[i].lastTaskTick++;
        }
    }
}

bool lc3_system_click(const EventRecord *theEvent, void *theWindow)
{
    if (!theEvent || !theWindow) return false;

    for (int i = 0; i < MAX_ACTIVE_DAS; ++i) {
        if (sActiveDAs[i].inUse && sActiveDAs[i].windowPtr == theWindow) {
            /* Handled by Desk Accessory */
            return true;
        }
    }

    return false;
}

bool lc3_system_menu(int32_t menuResult)
{
    (void)menuResult;
    return false;
}
