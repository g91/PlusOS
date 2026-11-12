/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Complete System 7 Dialog Manager
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_DIALOG_MGR_H
#define LC3_DIALOG_MGR_H

#include <stdint.h>
#include <stdbool.h>

#define DLG_OK           1
#define DLG_CANCEL       2

#define CTRL_ITEM        4
#define BTN_CTRL         0
#define CHK_CTRL         1
#define RAD_CTRL         2
#define RES_CTRL         3
#define STAT_TEXT        8
#define EDIT_TEXT        16
#define ICON_ITEM        32
#define PICT_ITEM        64
#define USER_ITEM        0
#define ITEM_DISABLE     128

typedef struct {
    int16_t top;
    int16_t left;
    int16_t bottom;
    int16_t right;
} DlgRect;

typedef struct {
    int16_t v;
    int16_t h;
} DlgPoint;

typedef struct {
    int16_t what;
    uint32_t message;
    uint32_t when;
    DlgPoint where;
    uint16_t modifiers;
} DlgEventRecord;

typedef struct {
    void *window; // Underlying WindowRecord / GrafPort
    void *items;  // Handle to DITL items
    void *teHandle;
    int16_t editField;
    int16_t editOpen;
    int16_t aDefItem;
} DialogRecord, *DialogPtr;

typedef struct {
    int16_t itemType;
    void *itemHandle;
    DlgRect itemRect;
    char text[64];
} DlgItem;

/* Dialog Manager API */
void lc3_init_dialogs(void *resumeProc);
DialogPtr lc3_new_dialog(void *dStorage, const DlgRect *boundsRect, const char *title, bool visible, int16_t procID, void *behind, bool goAwayFlag, int32_t refCon, void *itmLstHndl);
void lc3_dispose_dialog(DialogPtr theDialog);
void lc3_close_dialog(DialogPtr theDialog);
void lc3_modal_dialog(void *filterProc, int16_t *itemHit);
bool lc3_is_dialog_event(const DlgEventRecord *theEvent);
bool lc3_dialog_select(const DlgEventRecord *theEvent, DialogPtr *theDialog, int16_t *itemHit);
void lc3_draw_dialog(DialogPtr theDialog);
void lc3_get_d_item(DialogPtr theDialog, int16_t itemNo, int16_t *itemType, void **itemHandle, DlgRect *box);
void lc3_set_d_item(DialogPtr theDialog, int16_t itemNo, int16_t itemType, void *itemHandle, const DlgRect *box);
void lc3_hide_d_item(DialogPtr theDialog, int16_t itemNo);
void lc3_show_d_item(DialogPtr theDialog, int16_t itemNo);
void lc3_sel_i_text(DialogPtr theDialog, int16_t itemNo, int16_t strtSel, int16_t endSel);
void lc3_get_i_text(void *itemHandle, char *text);
void lc3_set_i_text(void *itemHandle, const char *text);
int16_t lc3_find_d_item(DialogPtr theDialog, DlgPoint pt);
int16_t lc3_alert(int16_t alertID, void *filterProc);
int16_t lc3_stop_alert(int16_t alertID, void *filterProc);
int16_t lc3_note_alert(int16_t alertID, void *filterProc);
int16_t lc3_caution_alert(int16_t alertID, void *filterProc);
void lc3_param_text(const char *param0, const char *param1, const char *param2, const char *param3);
void lc3_error_sound(void *soundProc);

#endif /* LC3_DIALOG_MGR_H */
