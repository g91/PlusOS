/*
 * PlusOS Macintosh LC III ROM Compatibility Layer
 * Module 187: CommToolbox Communications Resource Manager (CRM) (GE)
 */

#ifndef LC3_CTB_CRM_H
#define LC3_CTB_CRM_H

#include <stdint.h>
#include <stdbool.h>

#define LC3_CRM_MAX_ENTRIES     32
#define LC3_CRM_DEVICE_TYPE_SER 0x53455249 /* 'SERI' Serial port */
#define LC3_CRM_DEVICE_TYPE_MOD 0x4D4F444D /* 'MODM' Modem */
#define LC3_CRM_DEVICE_TYPE_NET 0x4E455457 /* 'NETW' Network */

typedef struct lc3_crm_rec {
    uint32_t device_type;
    int16_t  device_id;
    char     device_name[32];
    uint32_t icon_id;
    uint32_t flags;
    struct lc3_crm_rec *next;
} lc3_crm_rec_t;

typedef struct {
    lc3_crm_rec_t *head;
    uint16_t num_devices;
    uint16_t version;
} lc3_crm_table_t;

void lc3_ctb_crm_init(lc3_crm_table_t *tbl);
int16_t lc3_ctb_crm_install(lc3_crm_table_t *tbl, lc3_crm_rec_t *rec);
int16_t lc3_ctb_crm_remove(lc3_crm_table_t *tbl, lc3_crm_rec_t *rec);
lc3_crm_rec_t *lc3_ctb_crm_search(lc3_crm_table_t *tbl, uint32_t device_type, int16_t device_id);
uint32_t lc3_ctb_crm_get_icon(lc3_crm_table_t *tbl, int16_t device_id);
uint16_t lc3_ctb_crm_get_version(const lc3_crm_table_t *tbl);

#endif /* LC3_CTB_CRM_H */
