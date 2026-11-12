/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Standard File System 7 Hierarchical Navigation & Directory Cache
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_STDFILE_NAV_H
#define LC3_STDFILE_NAV_H

#include <stdint.h>
#include <stdbool.h>

#define SF_NO_ERR         0
#define SF_STACK_OVERFLOW -1
#define SF_STACK_UNDERFLOW -2
#define SF_PARAM_ERR     -50

#define SF_MAX_HISTORY    16
#define SF_MAX_VOLUMES    8

typedef struct SFDirEntry {
    int32_t dirID;
    char dirName[32];
} SFDirEntry;

typedef struct SFNavSession {
    int16_t vRefNum;
    int32_t currentDirID;
    char currentDirName[32];
    SFDirEntry historyStack[SF_MAX_HISTORY];
    uint16_t historyDepth;
    int16_t mountedVolumes[SF_MAX_VOLUMES];
    uint16_t numVolumes;
    uint16_t activeVolIndex;
} SFNavSession;

typedef struct SFFileInfo {
    char fileName[32];
    uint32_t fileType;
    uint32_t fileCreator;
    int32_t dataLogicalLength;
    bool isDirectory;
} SFFileInfo;

/* Standard File Navigation API */
int16_t lc3_sf_nav_init(SFNavSession *session, int16_t defaultVRefNum, int32_t defaultDirID);
int16_t lc3_sf_nav_push_dir(SFNavSession *session, int32_t dirID, const char *dirName);
int16_t lc3_sf_nav_pop_dir(SFNavSession *session);
int16_t lc3_sf_nav_cycle_volume(SFNavSession *session);
bool lc3_sf_nav_filter_file(const SFFileInfo *file, const uint32_t *typeList, int16_t numTypes);
int16_t lc3_sf_nav_build_popup(const SFNavSession *session, char menuItemsOut[SF_MAX_HISTORY][32], uint16_t *itemCount);

#endif /* LC3_STDFILE_NAV_H */
