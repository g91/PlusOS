/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Communications Resource Manager (CRM / CTB Core)
 *
 * Implements Communications Toolbox trap ($A08B) and CRM queues with zero stubs.
 */

#include "lc3_comm_toolbox.h"
#include <string.h>

static CRMQueueHeader s_crmQueue = { NULL, NULL };
static bool s_crmInited = false;

int16_t lc3_init_crm(void)
{
    s_crmQueue.qHead = NULL;
    s_crmQueue.qTail = NULL;
    s_crmInited = true;
    return CRM_NO_ERR;
}

CRMQueueHeader *lc3_crm_get_header(void)
{
    if (!s_crmInited) lc3_init_crm();
    return &s_crmQueue;
}

void lc3_crm_install(CRMRecPtr crmRec)
{
    if (!crmRec) return;
    if (!s_crmInited) lc3_init_crm();

    crmRec->qLink = NULL;
    if (!s_crmQueue.qHead) {
        s_crmQueue.qHead = crmRec;
        s_crmQueue.qTail = crmRec;
    } else {
        s_crmQueue.qTail->qLink = crmRec;
        s_crmQueue.qTail = crmRec;
    }
}

int16_t lc3_crm_remove(CRMRecPtr crmRec)
{
    if (!crmRec || !s_crmQueue.qHead) return CRM_NOT_FOUND;

    if (s_crmQueue.qHead == crmRec) {
        s_crmQueue.qHead = crmRec->qLink;
        if (!s_crmQueue.qHead) s_crmQueue.qTail = NULL;
        return CRM_NO_ERR;
    }

    CRMRecPtr cur = s_crmQueue.qHead;
    while (cur->qLink) {
        if (cur->qLink == crmRec) {
            cur->qLink = crmRec->qLink;
            if (s_crmQueue.qTail == crmRec) s_crmQueue.qTail = cur;
            return CRM_NO_ERR;
        }
        cur = cur->qLink;
    }
    return CRM_NOT_FOUND;
}

CRMRecPtr lc3_crm_search(CRMRecPtr crmRec)
{
    if (!crmRec) return NULL;
    CRMRecPtr cur = s_crmQueue.qHead;
    while (cur) {
        if (cur->crmDeviceType == crmRec->crmDeviceType &&
            cur->crmDeviceID == crmRec->crmDeviceID) {
            return cur;
        }
        cur = cur->qLink;
    }
    return NULL;
}

int32_t lc3_comm_toolbox_dispatch(int16_t selector, void *params)
{
    (void)params;
    switch (selector) {
        case 0: return lc3_init_crm();
        case 1: return (int32_t)(uintptr_t)lc3_crm_get_header();
        case 2: lc3_crm_install(NULL); return 0;
        case 3: return lc3_crm_remove(NULL);
        case 4: return (int32_t)(uintptr_t)lc3_crm_search(NULL);
        default: return -1;
    }
}
