/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Desk Manager (Apple Menu & Desk Accessory DA Loader)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_DESK_MGR_H
#define LC3_DESK_MGR_H

#include <stdint.h>
#include <stdbool.h>

#define DESK_NO_ERR         0
#define DESK_NOT_FOUND    -43
#define DESK_FULL_ERR     -44
#define DESK_PARAM_ERR    -50

#define MAX_ACTIVE_DAS     16

typedef struct EventRecord {
    uint16_t what;
    uint32_t message;
    uint32_t when;
    int16_t whereV;
    int16_t whereH;
    uint16_t modifiers;
} EventRecord;

typedef struct DeskAccRecord {
    int16_t dRefNum;          /* Negative driver refnum: ~unitNum */
    char daName[32];
    void *windowPtr;
    bool inUse;
    uint32_t lastTaskTick;
} DeskAccRecord;

/* Desk Manager API */
int16_t lc3_desk_init(void);
int16_t lc3_open_desk_acc(const char *deskAccName);
void lc3_close_desk_acc(int16_t dRefNum);
void lc3_system_task(void);
bool lc3_system_click(const EventRecord *theEvent, void *theWindow);
bool lc3_system_menu(int32_t menuResult);

#endif /* LC3_DESK_MGR_H */
