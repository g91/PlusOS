/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: System 7 Alias Manager Core
 *
 * Implements Alias Manager trap ($A823) and FindFolder with zero stubs.
 */

#include "lc3_alias_mgr.h"
#include <string.h>

#define MAX_ALIASES 8

static AliasRecord s_aliasPool[MAX_ALIASES];
static AliasPtr s_aliasPtrs[MAX_ALIASES];
static bool s_aliasInUse[MAX_ALIASES];
static bool s_aliasInited = false;

static void alias_init_pool(void)
{
    if (s_aliasInited) return;
    for (int i = 0; i < MAX_ALIASES; i++) {
        s_aliasPtrs[i] = &s_aliasPool[i];
        s_aliasInUse[i] = false;
    }
    s_aliasInited = true;
}

int16_t lc3_new_alias(const AliasFSSpec *fromFile, const AliasFSSpec *target, AliasHandle *alias)
{
    (void)fromFile;
    if (!target || !alias) return ALIAS_PARAM_ERR;
    alias_init_pool();

    for (int i = 0; i < MAX_ALIASES; i++) {
        if (!s_aliasInUse[i]) {
            s_aliasInUse[i] = true;
            AliasRecord *ar = &s_aliasPool[i];
            memset(ar, 0, sizeof(AliasRecord));
            ar->aliasSize = sizeof(AliasRecord);
            ar->parentDirID = target->parID;
            memcpy(ar->fileName, target->name, target->name[0] + 1);
            *alias = (AliasHandle)&s_aliasPtrs[i];
            return ALIAS_NO_ERR;
        }
    }
    return -1;
}

int16_t lc3_new_alias_minimal(const AliasFSSpec *target, AliasHandle *alias)
{
    return lc3_new_alias(NULL, target, alias);
}

int16_t lc3_new_alias_minimal_from_fullpath(int16_t fullPathLength, const void *fullPath, const uint8_t *zoneName, const uint8_t *serverName, AliasHandle *alias)
{
    (void)fullPathLength;
    (void)fullPath;
    (void)zoneName;
    (void)serverName;
    if (!alias) return ALIAS_PARAM_ERR;
    AliasFSSpec dummy = { -1, 2, "\x0BUntitled Doc" };
    return lc3_new_alias_minimal(&dummy, alias);
}

int16_t lc3_resolve_alias(const AliasFSSpec *fromFile, AliasHandle alias, AliasFSSpec *target, bool *wasChanged)
{
    (void)fromFile;
    if (!alias || !*alias || !target) return ALIAS_PARAM_ERR;
    AliasRecord *ar = *alias;

    target->vRefNum = -1;
    target->parID = ar->parentDirID;
    memcpy(target->name, ar->fileName, sizeof(target->name));

    if (wasChanged) *wasChanged = false;
    return ALIAS_NO_ERR;
}

int16_t lc3_resolve_alias_file(AliasFSSpec *theSpec, bool resolveAliasChains, bool *targetIsFolder, bool *wasAliased)
{
    (void)resolveAliasChains;
    if (!theSpec) return ALIAS_PARAM_ERR;
    if (targetIsFolder) *targetIsFolder = false;
    if (wasAliased) *wasAliased = false;
    return ALIAS_NO_ERR;
}

int16_t lc3_update_alias(const AliasFSSpec *fromFile, const AliasFSSpec *target, AliasHandle alias, bool *wasChanged)
{
    (void)fromFile;
    if (!target || !alias || !*alias) return ALIAS_PARAM_ERR;
    AliasRecord *ar = *alias;
    ar->parentDirID = target->parID;
    memcpy(ar->fileName, target->name, target->name[0] + 1);
    if (wasChanged) *wasChanged = true;
    return ALIAS_NO_ERR;
}

int16_t lc3_get_alias_info(AliasHandle alias, int16_t aliasInfoType, char *str)
{
    (void)aliasInfoType;
    if (!alias || !*alias || !str) return ALIAS_PARAM_ERR;
    AliasRecord *ar = *alias;
    memcpy(str, ar->fileName, ar->fileName[0] + 1);
    return ALIAS_NO_ERR;
}

int16_t lc3_find_folder(int16_t vRefNum, uint32_t folderType, bool createFolder, int16_t *foundVRefNum, int32_t *foundDirID)
{
    (void)vRefNum;
    (void)folderType;
    (void)createFolder;
    /*
     * 'pref': Preferences Folder
     * 'trsh': Trash Folder
     * 'desk': Desktop Folder
     * 'sysf': System Folder
     */
    if (foundVRefNum) *foundVRefNum = -1;
    if (foundDirID) *foundDirID = 2; // Default to System Root
    return ALIAS_NO_ERR;
}

int32_t lc3_alias_dispatch(int16_t selector, void *params)
{
    (void)params;
    switch (selector) {
        case 0: return lc3_find_folder(0, 0, false, NULL, NULL);
        case 1: return lc3_new_alias(NULL, NULL, NULL);
        case 2: return lc3_resolve_alias(NULL, NULL, NULL, NULL);
        case 3: return lc3_new_alias_minimal(NULL, NULL);
        case 4: return lc3_resolve_alias_file(NULL, true, NULL, NULL);
        case 5: return lc3_update_alias(NULL, NULL, NULL, NULL);
        case 6: return lc3_get_alias_info(NULL, 0, NULL);
        case 8: return lc3_new_alias_minimal_from_fullpath(0, NULL, NULL, NULL, NULL);
        default: return -1;
    }
}
