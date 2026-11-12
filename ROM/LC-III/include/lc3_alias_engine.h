/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Alias Manager Fast File Tracking & Dynamic Resolution Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_ALIAS_ENGINE_H
#define LC3_ALIAS_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

#define ALIAS_NO_ERR         0
#define ALIAS_NOT_FOUND    -43
#define ALIAS_PARAM_ERR    -50

typedef struct AliasRecord {
    uint32_t userType;
    uint16_t aliasSize;
    int16_t volumeVRefNum;
    uint32_t volumeCreationDate;
    char volumeName[32];
    int32_t parentDirID;
    char targetName[32];
    int32_t fileID;
    uint32_t targetCreationDate;
    uint32_t fileType;
    uint32_t fileCreator;
    bool isDirectory;
} AliasRecord, *AliasPtr, **AliasHandle;

/* Alias Manager API */
int16_t lc3_alias_create(int16_t vRefNum, int32_t dirID, const char *fileName, AliasRecord *outAlias);
int16_t lc3_alias_resolve(const AliasRecord *alias, int16_t *vRefNum, int32_t *dirID, char *fileName, bool *wasChanged);
int16_t lc3_alias_match(const AliasRecord *alias, int32_t rulesMask, int16_t *matchedVRef, int32_t *matchedDirID);
int16_t lc3_alias_update(AliasRecord *alias, int16_t newVRef, int32_t newDirID, const char *newFileName);
int16_t lc3_alias_get_info(const AliasRecord *alias, char *targetName, char *volName, uint32_t *fileType);
int16_t lc3_alias_dispatch(int16_t selector, void *params);

#endif /* LC3_ALIAS_ENGINE_H */
