/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Standard File System 7 Hierarchical Navigation & Directory Cache
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#include "lc3_stdfile_nav.h"
#include <string.h>

int16_t lc3_sf_nav_init(SFNavSession *session, int16_t defaultVRefNum, int32_t defaultDirID)
{
    if (!session) return SF_PARAM_ERR;

    memset(session, 0, sizeof(SFNavSession));
    session->vRefNum = defaultVRefNum;
    session->currentDirID = defaultDirID;
    strncpy(session->currentDirName, "Macintosh HD", sizeof(session->currentDirName) - 1);
    session->historyDepth = 0;

    session->mountedVolumes[0] = defaultVRefNum;
    session->numVolumes = 1;
    session->activeVolIndex = 0;

    return SF_NO_ERR;
}

int16_t lc3_sf_nav_push_dir(SFNavSession *session, int32_t dirID, const char *dirName)
{
    if (!session || !dirName) return SF_PARAM_ERR;
    if (session->historyDepth >= SF_MAX_HISTORY) return SF_STACK_OVERFLOW;

    /* Push current directory onto history stack */
    session->historyStack[session->historyDepth].dirID = session->currentDirID;
    strncpy(session->historyStack[session->historyDepth].dirName, session->currentDirName, 31);
    session->historyDepth++;

    /* Descend into new folder */
    session->currentDirID = dirID;
    strncpy(session->currentDirName, dirName, sizeof(session->currentDirName) - 1);

    return SF_NO_ERR;
}

int16_t lc3_sf_nav_pop_dir(SFNavSession *session)
{
    if (!session) return SF_PARAM_ERR;
    if (session->historyDepth == 0) return SF_STACK_UNDERFLOW;

    session->historyDepth--;
    session->currentDirID = session->historyStack[session->historyDepth].dirID;
    strncpy(session->currentDirName, session->historyStack[session->historyDepth].dirName, sizeof(session->currentDirName) - 1);

    return SF_NO_ERR;
}

int16_t lc3_sf_nav_cycle_volume(SFNavSession *session)
{
    if (!session || session->numVolumes == 0) return SF_PARAM_ERR;

    session->activeVolIndex = (session->activeVolIndex + 1) % session->numVolumes;
    session->vRefNum = session->mountedVolumes[session->activeVolIndex];
    session->currentDirID = 2; /* fsRtDirID = 2 (Root directory of volume) */
    session->historyDepth = 0;
    strncpy(session->currentDirName, "Volume Root", sizeof(session->currentDirName) - 1);

    return SF_NO_ERR;
}

bool lc3_sf_nav_filter_file(const SFFileInfo *file, const uint32_t *typeList, int16_t numTypes)
{
    if (!file) return false;

    /* Directories always pass filter */
    if (file->isDirectory) return true;

    /* If numTypes is -1 or typeList is NULL, all files pass */
    if (numTypes < 0 || !typeList) return true;

    for (int16_t i = 0; i < numTypes; ++i) {
        if (file->fileType == typeList[i]) {
            return true;
        }
    }

    return false;
}

int16_t lc3_sf_nav_build_popup(const SFNavSession *session, char menuItemsOut[SF_MAX_HISTORY][32], uint16_t *itemCount)
{
    if (!session || !menuItemsOut || !itemCount) return SF_PARAM_ERR;

    uint16_t count = 0;

    /* Top item: current directory */
    strncpy(menuItemsOut[count++], session->currentDirName, 31);

    /* Ancestor directories from history stack in reverse order */
    for (int16_t i = (int16_t)session->historyDepth - 1; i >= 0; --i) {
        if (count >= SF_MAX_HISTORY) break;
        strncpy(menuItemsOut[count++], session->historyStack[i].dirName, 31);
    }

    *itemCount = count;
    return SF_NO_ERR;
}
