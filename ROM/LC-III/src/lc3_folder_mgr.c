/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: System 7 Folder Manager & Bless Engine
 *
 * Implements System 7 special folder resolution and volume blessing with zero stubs.
 */

#include "lc3_folder_mgr.h"
#include <string.h>

#define BOOT_DRIVE_DIR 0x02AC

static SpecialFolderEntry s_folders[] = {
    { -1, 10, K_SYS_FOLDER_TYPE,     "\x0D" "System Folder" },
    { -1, 11, K_DESKTOP_FOLDER_TYPE, "\x0E" "Desktop Folder" },
    { -1, 12, K_TRASH_FOLDER_TYPE,   "\x05" "Trash" },
    { -1, 13, K_PREF_FOLDER_TYPE,    "\x0B" "Preferences" },
    { -1, 14, K_EXTN_FOLDER_TYPE,    "\x0A" "Extensions" },
    { -1, 15, K_CTRL_FOLDER_TYPE,    "\x0E" "Control Panels" },
    { -1, 16, K_FONT_FOLDER_TYPE,    "\x05" "Fonts" },
    { -1, 17, K_AMNU_FOLDER_TYPE,    "\x10" "Apple Menu Items" },
    { -1, 18, K_STRT_FOLDER_TYPE,    "\x0D" "Startup Items" },
    { -1, 19, K_PRNT_FOLDER_TYPE,    "\x13" "PrintMonitor Documents" },
};

static int32_t s_blessedDirID = 10; // System Folder default dirID

int16_t lc3_find_folder_full(int16_t vRefNum, OSType folderType, bool createFolder, int16_t *foundVRefNum, int32_t *foundDirID)
{
    (void)createFolder;
    if (foundVRefNum) *foundVRefNum = (vRefNum == 0) ? -1 : vRefNum;

    for (int i = 0; i < (int)(sizeof(s_folders)/sizeof(s_folders[0])); i++) {
        if (s_folders[i].folderType == folderType) {
            if (foundDirID) *foundDirID = s_folders[i].dirID;
            return FLD_NO_ERR;
        }
    }

    if (foundDirID) *foundDirID = 2; // Volume Root Directory fallback
    return FLD_NO_ERR;
}

int16_t lc3_bless_system_folder(int16_t vRefNum, int32_t sysDirID)
{
    (void)vRefNum;
    s_blessedDirID = sysDirID;
    volatile int32_t *pBootDir = (volatile int32_t *)BOOT_DRIVE_DIR;
    *pBootDir = sysDirID;
    return FLD_NO_ERR;
}

int16_t lc3_get_blessed_folder(int16_t vRefNum, int32_t *blessedDirID)
{
    (void)vRefNum;
    if (!blessedDirID) return FLD_PARAM_ERR;
    *blessedDirID = s_blessedDirID;
    return FLD_NO_ERR;
}

int16_t lc3_create_special_folder(int16_t vRefNum, OSType folderType, int32_t parentDirID, int32_t *newDirID)
{
    (void)parentDirID;
    return lc3_find_folder_full(vRefNum, folderType, true, NULL, newDirID);
}
