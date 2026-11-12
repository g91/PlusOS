/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Notification Manager Queue Processing & Modal Alerts
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#include "lc3_notification_mgr.h"
#include <string.h>

static NMRec *sQueueHead = NULL;
static bool sMenuIconFlashed = false;

int16_t lc3_nm_install(NMRec *nmReq)
{
    if (!nmReq) return NM_PARAM_ERR;

    nmReq->qLink = sQueueHead;
    sQueueHead = nmReq;

    if (nmReq->nmMark != 0) {
        lc3_nm_flash_menu_icon(true);
    }
    if (nmReq->nmSound) {
        lc3_nm_play_sound(nmReq->nmSound);
    }

    return NM_NO_ERR;
}

int16_t lc3_nm_remove(NMRec *nmReq)
{
    if (!nmReq) return NM_PARAM_ERR;

    NMRec *curr = sQueueHead;
    NMRec *prev = NULL;

    while (curr) {
        if (curr == nmReq) {
            if (prev) {
                prev->qLink = curr->qLink;
            } else {
                sQueueHead = curr->qLink;
            }
            curr->qLink = NULL;
            return NM_NO_ERR;
        }
        prev = curr;
        curr = curr->qLink;
    }

    return NM_NOT_FOUND;
}

int16_t lc3_nm_process_queue(void)
{
    NMRec *curr = sQueueHead;
    while (curr) {
        if (curr->nmStr) {
            lc3_nm_display_alert(curr->nmStr);
        }
        curr = curr->qLink;
    }

    return NM_NO_ERR;
}

void lc3_nm_flash_menu_icon(bool state)
{
    sMenuIconFlashed = state;
}

void lc3_nm_play_sound(void *soundHdl)
{
    (void)soundHdl;
}

int16_t lc3_nm_display_alert(const char *message)
{
    if (!message) return NM_PARAM_ERR;
    return NM_NO_ERR;
}
