/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Communications Resource Manager (CRM / CTB Core)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_COMM_TOOLBOX_H
#define LC3_COMM_TOOLBOX_H

#include <stdint.h>
#include <stdbool.h>

#define CRM_NO_ERR          0
#define CRM_NOT_FOUND      -1

typedef uint32_t OSType;

typedef struct CRMRec {
    struct CRMRec *qLink;
    int16_t qType;
    OSType crmDeviceType;
    OSType crmDeviceID;
    int32_t crmAttributes;
    int32_t crmStatus;
    int32_t crmRefCon;
} CRMRec, *CRMRecPtr;

typedef struct {
    CRMRecPtr qHead;
    CRMRecPtr qTail;
} CRMQueueHeader;

/* CRM Core API */
int16_t lc3_init_crm(void);
CRMQueueHeader *lc3_crm_get_header(void);
void lc3_crm_install(CRMRecPtr crmRec);
int16_t lc3_crm_remove(CRMRecPtr crmRec);
CRMRecPtr lc3_crm_search(CRMRecPtr crmRec);

/* CommToolbox Dispatch ($A08B) */
int32_t lc3_comm_toolbox_dispatch(int16_t selector, void *params);

#endif /* LC3_COMM_TOOLBOX_H */
