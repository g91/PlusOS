/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Notification Manager Queue Processing & Modal Alerts
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_NOTIFICATION_MGR_H
#define LC3_NOTIFICATION_MGR_H

#include <stdint.h>
#include <stdbool.h>

#define NM_NO_ERR         0
#define NM_NOT_FOUND    -43
#define NM_QUEUE_FULL   -44
#define NM_PARAM_ERR    -50

#define MAX_NM_RECS      16

typedef struct NMRec {
    struct NMRec *qLink;
    int16_t qType;
    int16_t nmFlags;
    int32_t nmPrivate;
    int16_t nmReserved;
    int16_t nmMark;           /* Apple menu mark */
    void *nmSIcon;            /* Small icon handle */
    void *nmSound;            /* Sound handle */
    char *nmStr;              /* Alert string pointer */
    void *nmResp;             /* Response proc pointer */
    int32_t nmRefCon;
} NMRec, *NMRecPtr;

/* Notification Manager API */
int16_t lc3_nm_install(NMRec *nmReq);
int16_t lc3_nm_remove(NMRec *nmReq);
int16_t lc3_nm_process_queue(void);
void lc3_nm_flash_menu_icon(bool state);
void lc3_nm_play_sound(void *soundHdl);
int16_t lc3_nm_display_alert(const char *message);

#endif /* LC3_NOTIFICATION_MGR_H */
