/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Standard File System 7 Custom Hook & QuickTime Preview Architecture
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_STDFILE_CUSTOM_H
#define LC3_STDFILE_CUSTOM_H

#include <stdint.h>
#include <stdbool.h>

#define SF_NO_ERR         0
#define SF_USER_CANCEL    1
#define SF_PARAM_ERR    -50

typedef struct StandardFileReply {
    bool sfGood;
    bool sfReplacing;
    uint32_t sfType;
    struct {
        int16_t vRefNum;
        int32_t parID;
        char name[64];
    } sfFile;
    uint32_t sfScript;
    int16_t sfFlags;
    bool sfIsFolder;
    bool sfIsVolume;
    int32_t sfReserved1;
    int16_t sfReserved2;
} StandardFileReply;

typedef bool (*FileFilterYDProcPtr)(void *pb, void *yourDataPtr);
typedef int16_t (*DlgHookYDProcPtr)(int16_t item, void *theDialog, void *yourDataPtr);
typedef bool (*ModalFilterYDProcPtr)(void *theDialog, void *theEvent, int16_t *itemHit, void *yourDataPtr);
typedef void (*ActivateYDProcPtr)(void *theDialog, int16_t item, bool activating, void *yourDataPtr);

typedef struct Point {
    int16_t v;
    int16_t h;
} Point;

/* Custom Standard File API */
void lc3_custom_get_file(FileFilterYDProcPtr fileFilter, int16_t numTypes, const uint32_t *typeList,
                         StandardFileReply *reply, int16_t dlgID, Point where,
                         DlgHookYDProcPtr dlgHook, ModalFilterYDProcPtr filterProc,
                         const int16_t *activeList, ActivateYDProcPtr activateProc, void *yourDataPtr);
void lc3_custom_put_file(const char *prompt, const char *defaultName, StandardFileReply *reply,
                         int16_t dlgID, Point where,
                         DlgHookYDProcPtr dlgHook, ModalFilterYDProcPtr filterProc,
                         const int16_t *activeList, ActivateYDProcPtr activateProc, void *yourDataPtr);
void lc3_standard_get_file_preview(FileFilterYDProcPtr fileFilter, int16_t numTypes, const uint32_t *typeList,
                                   StandardFileReply *reply);
bool lc3_std_file_modal_filter(void *theDialog, void *theEvent, int16_t *itemHit);
void lc3_std_file_activate_hook(void *theDialog, int16_t item, bool activating);
int16_t lc3_std_file_dispatch(int16_t selector, void *params);

#endif /* LC3_STDFILE_CUSTOM_H */
