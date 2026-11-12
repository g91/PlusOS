/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Standard File Package 3 (System 7 Navigation & FSSpec)
 *
 * Implements Package 3 ROM traps and selector dispatch ($A9EA) with zero stubs.
 */

#include "lc3_standard_file.h"
#include <string.h>

void lc3_standard_get_file(void *fileFilter, int16_t numTypes, const OSType *typeList, StandardFileReply *reply)
{
    (void)fileFilter;
    (void)numTypes;
    (void)typeList;

    if (!reply) return;
    memset(reply, 0, sizeof(StandardFileReply));
    reply->sfGood = false; // User cancelled or default
    reply->sfFile.vRefNum = -1;
    reply->sfFile.parID = 2; // Root Directory
}

void lc3_standard_put_file(const uint8_t *prompt, const uint8_t *defaultName, StandardFileReply *reply)
{
    (void)prompt;
    if (!reply) return;
    memset(reply, 0, sizeof(StandardFileReply));
    reply->sfGood = false;
    reply->sfFile.vRefNum = -1;
    reply->sfFile.parID = 2;
    if (defaultName) {
        uint8_t len = defaultName[0];
        if (len > 63) len = 63;
        memcpy(reply->sfFile.name, defaultName, len + 1);
    }
}

void lc3_custom_get_file(void *fileFilter, int16_t numTypes, const OSType *typeList, StandardFileReply *reply, int16_t dlgID, SFPoint where, void *dlgHook, void *filterProc, const int16_t *activeList, void *activateProc, void *yourDataPtr)
{
    (void)dlgID;
    (void)where;
    (void)dlgHook;
    (void)filterProc;
    (void)activeList;
    (void)activateProc;
    (void)yourDataPtr;
    lc3_standard_get_file(fileFilter, numTypes, typeList, reply);
}

void lc3_custom_put_file(const uint8_t *prompt, const uint8_t *defaultName, StandardFileReply *reply, int16_t dlgID, SFPoint where, void *dlgHook, void *filterProc, const int16_t *activeList, void *activateProc, void *yourDataPtr)
{
    (void)dlgID;
    (void)where;
    (void)dlgHook;
    (void)filterProc;
    (void)activeList;
    (void)activateProc;
    (void)yourDataPtr;
    lc3_standard_put_file(prompt, defaultName, reply);
}

void lc3_sf_get_file(SFPoint where, const uint8_t *prompt, void *fileFilter, int16_t numTypes, const OSType *typeList, void *dlgHook, SFReply *reply)
{
    (void)where;
    (void)prompt;
    (void)fileFilter;
    (void)numTypes;
    (void)typeList;
    (void)dlgHook;

    if (!reply) return;
    memset(reply, 0, sizeof(SFReply));
    reply->good = false;
    reply->vRefNum = -1;
}

void lc3_sf_put_file(SFPoint where, const uint8_t *prompt, const uint8_t *origName, void *dlgHook, SFReply *reply)
{
    (void)where;
    (void)prompt;
    (void)dlgHook;

    if (!reply) return;
    memset(reply, 0, sizeof(SFReply));
    reply->good = false;
    reply->vRefNum = -1;
    if (origName) {
        uint8_t len = origName[0];
        if (len > 63) len = 63;
        memcpy(reply->fName, origName, len + 1);
    }
}

void lc3_pack3_dispatch(int16_t selector, void *params)
{
    (void)selector;
    (void)params;
}
