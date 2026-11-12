/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Standard File Package 3 (System 7 Navigation & FSSpec)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_STANDARD_FILE_H
#define LC3_STANDARD_FILE_H

#include <stdint.h>
#include <stdbool.h>

typedef uint32_t OSType;

typedef struct {
    int16_t vRefNum;
    int32_t parID;
    uint8_t name[64];
} FSSpec;

typedef struct {
    bool sfGood;
    bool sfReplacing;
    OSType sfType;
    FSSpec sfFile;
    int16_t sfScript;
    int16_t sfFlags;
    bool sfIsFolder;
    bool sfIsVolume;
    int32_t sfReserved1;
    int32_t sfReserved2;
} StandardFileReply;

typedef struct {
    bool good;
    bool copy;
    OSType fType;
    int16_t vRefNum;
    int16_t version;
    uint8_t fName[64];
} SFReply;

typedef struct {
    int16_t v;
    int16_t h;
} SFPoint;

/* Package 3 API */
void lc3_standard_get_file(void *fileFilter, int16_t numTypes, const OSType *typeList, StandardFileReply *reply);
void lc3_standard_put_file(const uint8_t *prompt, const uint8_t *defaultName, StandardFileReply *reply);
void lc3_custom_get_file(void *fileFilter, int16_t numTypes, const OSType *typeList, StandardFileReply *reply, int16_t dlgID, SFPoint where, void *dlgHook, void *filterProc, const int16_t *activeList, void *activateProc, void *yourDataPtr);
void lc3_custom_put_file(const uint8_t *prompt, const uint8_t *defaultName, StandardFileReply *reply, int16_t dlgID, SFPoint where, void *dlgHook, void *filterProc, const int16_t *activeList, void *activateProc, void *yourDataPtr);
void lc3_sf_get_file(SFPoint where, const uint8_t *prompt, void *fileFilter, int16_t numTypes, const OSType *typeList, void *dlgHook, SFReply *reply);
void lc3_sf_put_file(SFPoint where, const uint8_t *prompt, const uint8_t *origName, void *dlgHook, SFReply *reply);

/* Package 3 Dispatcher */
void lc3_pack3_dispatch(int16_t selector, void *params);

#endif /* LC3_STANDARD_FILE_H */
