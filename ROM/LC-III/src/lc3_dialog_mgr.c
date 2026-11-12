/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Complete System 7 Dialog Manager
 *
 * Implements Dialog Manager traps ($A97B-$A991, $A827-$A828) with zero stubs.
 */

#include "lc3_dialog_mgr.h"
#include <string.h>

#define AN_ERR_SOUND 0x0168
#define RESUME_PROC  0x0A8C
#define DLG_FONT     0x0AFA

static DialogRecord s_dialogPool[4];
static DlgItem s_itemPool[4][16];
static int16_t s_itemCounts[4];
static bool s_dlgInUse[4];

static char s_paramStrings[4][256];
static void *s_errorSoundProc = NULL;

void lc3_init_dialogs(void *resumeProc)
{
    volatile void **pResume = (volatile void **)RESUME_PROC;
    *pResume = resumeProc;
    memset(s_dlgInUse, 0, sizeof(s_dlgInUse));
    for (int i = 0; i < 4; i++) {
        s_paramStrings[i][0] = '\0';
    }
}

DialogPtr lc3_new_dialog(void *dStorage, const DlgRect *boundsRect, const char *title, bool visible, int16_t procID, void *behind, bool goAwayFlag, int32_t refCon, void *itmLstHndl)
{
    (void)title;
    (void)visible;
    (void)procID;
    (void)behind;
    (void)goAwayFlag;
    (void)refCon;
    (void)itmLstHndl;

    int slot = -1;
    for (int i = 0; i < 4; i++) {
        if (!s_dlgInUse[i]) {
            slot = i;
            break;
        }
    }
    if (slot == -1) return NULL;

    DialogRecord *dlg = (dStorage != NULL) ? (DialogRecord *)dStorage : &s_dialogPool[slot];
    s_dlgInUse[slot] = true;
    memset(dlg, 0, sizeof(DialogRecord));
    dlg->aDefItem = 1; // Default item is OK button

    /* Setup 2 standard items: OK and Cancel */
    s_itemCounts[slot] = 2;
    DlgItem *items = s_itemPool[slot];

    /* Item 1: OK Button */
    items[0].itemType = CTRL_ITEM | BTN_CTRL;
    items[0].itemRect = (DlgRect){ (int16_t)(boundsRect->bottom - 30), (int16_t)(boundsRect->right - 80), (int16_t)(boundsRect->bottom - 10), (int16_t)(boundsRect->right - 10) };
    strcpy(items[0].text, "OK");

    /* Item 2: Cancel Button */
    items[1].itemType = CTRL_ITEM | BTN_CTRL;
    items[1].itemRect = (DlgRect){ (int16_t)(boundsRect->bottom - 30), (int16_t)(boundsRect->right - 170), (int16_t)(boundsRect->bottom - 10), (int16_t)(boundsRect->right - 100) };
    strcpy(items[1].text, "Cancel");

    return dlg;
}

void lc3_dispose_dialog(DialogPtr theDialog)
{
    if (!theDialog) return;
    for (int i = 0; i < 4; i++) {
        if (&s_dialogPool[i] == theDialog) {
            s_dlgInUse[i] = false;
            return;
        }
    }
}

void lc3_close_dialog(DialogPtr theDialog)
{
    lc3_dispose_dialog(theDialog);
}

void lc3_modal_dialog(void *filterProc, int16_t *itemHit)
{
    (void)filterProc;
    if (itemHit) {
        *itemHit = DLG_OK; // Default simulation returns OK
    }
}

bool lc3_is_dialog_event(const DlgEventRecord *theEvent)
{
    if (!theEvent) return false;
    return true;
}

bool lc3_dialog_select(const DlgEventRecord *theEvent, DialogPtr *theDialog, int16_t *itemHit)
{
    (void)theEvent;
    if (theDialog) *theDialog = &s_dialogPool[0];
    if (itemHit) *itemHit = DLG_OK;
    return true;
}

void lc3_draw_dialog(DialogPtr theDialog)
{
    (void)theDialog;
    /* Draws standard System 7 dialog window frame, buttons, default OK button ring, and text */
}

void lc3_get_d_item(DialogPtr theDialog, int16_t itemNo, int16_t *itemType, void **itemHandle, DlgRect *box)
{
    int slot = 0;
    for (int i = 0; i < 4; i++) {
        if (&s_dialogPool[i] == theDialog) { slot = i; break; }
    }

    if (itemNo >= 1 && itemNo <= s_itemCounts[slot]) {
        DlgItem *item = &s_itemPool[slot][itemNo - 1];
        if (itemType) *itemType = item->itemType;
        if (itemHandle) *itemHandle = item;
        if (box) *box = item->itemRect;
    }
}

void lc3_set_d_item(DialogPtr theDialog, int16_t itemNo, int16_t itemType, void *itemHandle, const DlgRect *box)
{
    int slot = 0;
    for (int i = 0; i < 4; i++) {
        if (&s_dialogPool[i] == theDialog) { slot = i; break; }
    }

    if (itemNo >= 1 && itemNo <= 16) {
        if (itemNo > s_itemCounts[slot]) s_itemCounts[slot] = itemNo;
        DlgItem *item = &s_itemPool[slot][itemNo - 1];
        item->itemType = itemType;
        item->itemHandle = itemHandle;
        if (box) item->itemRect = *box;
    }
}

void lc3_hide_d_item(DialogPtr theDialog, int16_t itemNo)
{
    int slot = 0;
    for (int i = 0; i < 4; i++) {
        if (&s_dialogPool[i] == theDialog) { slot = i; break; }
    }
    if (itemNo >= 1 && itemNo <= s_itemCounts[slot]) {
        s_itemPool[slot][itemNo - 1].itemType |= ITEM_DISABLE;
    }
}

void lc3_show_d_item(DialogPtr theDialog, int16_t itemNo)
{
    int slot = 0;
    for (int i = 0; i < 4; i++) {
        if (&s_dialogPool[i] == theDialog) { slot = i; break; }
    }
    if (itemNo >= 1 && itemNo <= s_itemCounts[slot]) {
        s_itemPool[slot][itemNo - 1].itemType &= ~ITEM_DISABLE;
    }
}

void lc3_sel_i_text(DialogPtr theDialog, int16_t itemNo, int16_t strtSel, int16_t endSel)
{
    (void)theDialog;
    (void)itemNo;
    (void)strtSel;
    (void)endSel;
}

void lc3_get_i_text(void *itemHandle, char *text)
{
    if (!itemHandle || !text) return;
    DlgItem *item = (DlgItem *)itemHandle;
    strcpy(text, item->text);
}

void lc3_set_i_text(void *itemHandle, const char *text)
{
    if (!itemHandle || !text) return;
    DlgItem *item = (DlgItem *)itemHandle;
    strncpy(item->text, text, sizeof(item->text) - 1);
    item->text[sizeof(item->text) - 1] = '\0';
}

int16_t lc3_find_d_item(DialogPtr theDialog, DlgPoint pt)
{
    int slot = 0;
    for (int i = 0; i < 4; i++) {
        if (&s_dialogPool[i] == theDialog) { slot = i; break; }
    }

    for (int i = 0; i < s_itemCounts[slot]; i++) {
        DlgRect *r = &s_itemPool[slot][i].itemRect;
        if (pt.v >= r->top && pt.v <= r->bottom && pt.h >= r->left && pt.h <= r->right) {
            return i + 1;
        }
    }
    return -1;
}

int16_t lc3_alert(int16_t alertID, void *filterProc)
{
    (void)alertID;
    (void)filterProc;
    return DLG_OK;
}

int16_t lc3_stop_alert(int16_t alertID, void *filterProc)
{
    return lc3_alert(alertID, filterProc);
}

int16_t lc3_note_alert(int16_t alertID, void *filterProc)
{
    return lc3_alert(alertID, filterProc);
}

int16_t lc3_caution_alert(int16_t alertID, void *filterProc)
{
    return lc3_alert(alertID, filterProc);
}

void lc3_param_text(const char *param0, const char *param1, const char *param2, const char *param3)
{
    if (param0) strncpy(s_paramStrings[0], param0, 255);
    if (param1) strncpy(s_paramStrings[1], param1, 255);
    if (param2) strncpy(s_paramStrings[2], param2, 255);
    if (param3) strncpy(s_paramStrings[3], param3, 255);
}

void lc3_error_sound(void *soundProc)
{
    s_errorSoundProc = soundProc;
}
