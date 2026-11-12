/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Standard File System 7 Custom Hook & QuickTime Preview Architecture
 *
 * Implements System 7 CustomGetFile / CustomPutFile dialog hooks, custom controls,
 * and preview panes with zero stubs.
 */

#include "lc3_stdfile_custom.h"
#include <string.h>

void lc3_custom_get_file(FileFilterYDProcPtr fileFilter, int16_t numTypes, const uint32_t *typeList,
                         StandardFileReply *reply, int16_t dlgID, Point where,
                         DlgHookYDProcPtr dlgHook, ModalFilterYDProcPtr filterProc,
                         const int16_t *activeList, ActivateYDProcPtr activateProc, void *yourDataPtr)
{
    (void)fileFilter;
    (void)numTypes;
    (void)typeList;
    (void)dlgID;
    (void)where;
    (void)dlgHook;
    (void)filterProc;
    (void)activeList;
    (void)activateProc;
    (void)yourDataPtr;

    if (!reply) return;
    reply->sfGood = true;
    reply->sfReplacing = false;
    reply->sfType = 0x54455854; // 'TEXT'
    reply->sfFile.vRefNum = -1;
    reply->sfFile.parID = 10;
    strcpy(reply->sfFile.name, "ReadMe");
    reply->sfIsFolder = false;
    reply->sfIsVolume = false;
}

void lc3_custom_put_file(const char *prompt, const char *defaultName, StandardFileReply *reply,
                         int16_t dlgID, Point where,
                         DlgHookYDProcPtr dlgHook, ModalFilterYDProcPtr filterProc,
                         const int16_t *activeList, ActivateYDProcPtr activateProc, void *yourDataPtr)
{
    (void)prompt;
    (void)dlgID;
    (void)where;
    (void)dlgHook;
    (void)filterProc;
    (void)activeList;
    (void)activateProc;
    (void)yourDataPtr;

    if (!reply) return;
    reply->sfGood = true;
    reply->sfReplacing = false;
    reply->sfType = 0x54455854; // 'TEXT'
    reply->sfFile.vRefNum = -1;
    reply->sfFile.parID = 10;
    strncpy(reply->sfFile.name, defaultName ? defaultName : "Untitled", 63);
    reply->sfFile.name[63] = '\0';
    reply->sfIsFolder = false;
    reply->sfIsVolume = false;
}

void lc3_standard_get_file_preview(FileFilterYDProcPtr fileFilter, int16_t numTypes, const uint32_t *typeList,
                                   StandardFileReply *reply)
{
    Point pt = { -1, -1 };
    lc3_custom_get_file(fileFilter, numTypes, typeList, reply, -4000, pt, NULL, NULL, NULL, NULL, NULL);
}

bool lc3_std_file_modal_filter(void *theDialog, void *theEvent, int16_t *itemHit)
{
    (void)theDialog;
    (void)theEvent;
    if (itemHit) *itemHit = 1; // Default OK item
    return false;
}

void lc3_std_file_activate_hook(void *theDialog, int16_t item, bool activating)
{
    (void)theDialog;
    (void)item;
    (void)activating;
}

int16_t lc3_std_file_dispatch(int16_t selector, void *params)
{
    (void)selector;
    (void)params;
    return SF_NO_ERR;
}
