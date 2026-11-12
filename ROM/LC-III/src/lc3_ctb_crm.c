/*
 * PlusOS Macintosh LC III ROM Compatibility Layer
 * Module 187: CommToolbox Communications Resource Manager (CRM) (GE)
 */

#include "lc3_ctb_crm.h"
#include <string.h>

void lc3_ctb_crm_init(lc3_crm_table_t *tbl)
{
    if (!tbl) return;
    tbl->head = 0;
    tbl->num_devices = 0;
    tbl->version = 0x0110; /* CRM version 1.1 */
}

int16_t lc3_ctb_crm_install(lc3_crm_table_t *tbl, lc3_crm_rec_t *rec)
{
    if (!tbl || !rec) return -1;
    if (tbl->num_devices >= LC3_CRM_MAX_ENTRIES) return -2;

    rec->next = tbl->head;
    tbl->head = rec;
    tbl->num_devices++;
    return 0;
}

int16_t lc3_ctb_crm_remove(lc3_crm_table_t *tbl, lc3_crm_rec_t *rec)
{
    if (!tbl || !rec || !tbl->head) return -1;

    lc3_crm_rec_t *curr = tbl->head;
    lc3_crm_rec_t *prev = 0;

    while (curr) {
        if (curr == rec) {
            if (prev) {
                prev->next = curr->next;
            } else {
                tbl->head = curr->next;
            }
            curr->next = 0;
            if (tbl->num_devices > 0) tbl->num_devices--;
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }

    return -2; /* Not found */
}

lc3_crm_rec_t *lc3_ctb_crm_search(lc3_crm_table_t *tbl, uint32_t device_type, int16_t device_id)
{
    if (!tbl) return 0;

    lc3_crm_rec_t *curr = tbl->head;
    while (curr) {
        if ((device_type == 0 || curr->device_type == device_type) &&
            (device_id == -1 || curr->device_id == device_id)) {
            return curr;
        }
        curr = curr->next;
    }

    return 0;
}

uint32_t lc3_ctb_crm_get_icon(lc3_crm_table_t *tbl, int16_t device_id)
{
    lc3_crm_rec_t *rec = lc3_ctb_crm_search(tbl, 0, device_id);
    if (rec) {
        return rec->icon_id;
    }
    return 0;
}

uint16_t lc3_ctb_crm_get_version(const lc3_crm_table_t *tbl)
{
    if (!tbl) return 0;
    return tbl->version;
}
