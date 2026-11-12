/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: System 7 Folder Manager & Bless Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_FOLDER_MGR_H
#define LC3_FOLDER_MGR_H

#include <stdint.h>
#include <stdbool.h>

#define FLD_NO_ERR           0
#define FLD_PARAM_ERR      -50
#define FLD_DIR_NOT_FOUND -120

#define K_SYS_FOLDER_TYPE     0x6D616373 // 'macs'
#define K_DESKTOP_FOLDER_TYPE 0x6465736B // 'desk'
#define K_TRASH_FOLDER_TYPE   0x74727368 // 'trsh'
#define K_PREF_FOLDER_TYPE    0x70726566 // 'pref'
#define K_EXTN_FOLDER_TYPE    0x6578746E // 'extn'
#define K_CTRL_FOLDER_TYPE    0x6374726C // 'ctrl'
#define K_FONT_FOLDER_TYPE    0x666F6E74 // 'font'
#define K_AMNU_FOLDER_TYPE    0x616D6E75 // 'amnu'
#define K_STRT_FOLDER_TYPE    0x73747274 // 'strt'
#define K_PRNT_FOLDER_TYPE    0x70726E74 // 'prnt'

typedef uint32_t OSType;

typedef struct {
    int16_t vRefNum;
    int32_t dirID;
    OSType folderType;
    char folderName[32];
} SpecialFolderEntry;

/* Folder Manager API */
int16_t lc3_find_folder_full(int16_t vRefNum, OSType folderType, bool createFolder, int16_t *foundVRefNum, int32_t *foundDirID);
int16_t lc3_bless_system_folder(int16_t vRefNum, int32_t sysDirID);
int16_t lc3_get_blessed_folder(int16_t vRefNum, int32_t *blessedDirID);
int16_t lc3_create_special_folder(int16_t vRefNum, OSType folderType, int32_t parentDirID, int32_t *newDirID);

#endif /* LC3_FOLDER_MGR_H */
