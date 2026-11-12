/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: System 7 Alias Manager Core
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_ALIAS_MGR_H
#define LC3_ALIAS_MGR_H

#include <stdint.h>
#include <stdbool.h>

#define ALIAS_NO_ERR         0
#define ALIAS_PARAM_ERR    -50
#define ALIAS_FNF_ERR      -43
#define ALIAS_DIR_NOT_FND -120

typedef struct {
    int16_t vRefNum;
    int32_t parID;
    uint8_t name[64];
} AliasFSSpec;

typedef struct {
    uint32_t userType;
    uint16_t aliasSize;
    uint8_t volumeName[28];
    uint32_t volumeCrDate;
    int16_t volSig;
    int16_t volType;
    int32_t parentDirID;
    uint8_t fileName[64];
    int32_t fileNumber;
    uint32_t fileCrDate;
    uint32_t fileType;
    uint32_t fileCreator;
    int16_t nlvlFrom;
    int16_t nlvlTo;
    uint32_t volumeAttributes;
    int16_t fsFSID;
} AliasRecord, *AliasPtr, **AliasHandle;

/* Alias Manager API */
int16_t lc3_new_alias(const AliasFSSpec *fromFile, const AliasFSSpec *target, AliasHandle *alias);
int16_t lc3_new_alias_minimal(const AliasFSSpec *target, AliasHandle *alias);
int16_t lc3_new_alias_minimal_from_fullpath(int16_t fullPathLength, const void *fullPath, const uint8_t *zoneName, const uint8_t *serverName, AliasHandle *alias);
int16_t lc3_resolve_alias(const AliasFSSpec *fromFile, AliasHandle alias, AliasFSSpec *target, bool *wasChanged);
int16_t lc3_resolve_alias_file(AliasFSSpec *theSpec, bool resolveAliasChains, bool *targetIsFolder, bool *wasAliased);
int16_t lc3_update_alias(const AliasFSSpec *fromFile, const AliasFSSpec *target, AliasHandle alias, bool *wasChanged);
int16_t lc3_get_alias_info(AliasHandle alias, int16_t aliasInfoType, char *str);
int16_t lc3_find_folder(int16_t vRefNum, uint32_t folderType, bool createFolder, int16_t *foundVRefNum, int32_t *foundDirID);

/* Alias Dispatch ($A823) */
int32_t lc3_alias_dispatch(int16_t selector, void *params);

#endif /* LC3_ALIAS_MGR_H */
