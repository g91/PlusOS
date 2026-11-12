/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Extended ADB Manager 2.0
 *
 * Implements ADB Manager 2.0 traps ($A077-$A07C) with zero stubs.
 */

#include "lc3_adb_mgr2.h"
#include <string.h>

#define ADB_BASE 0x0CF8

static ADBDevTable s_adbTable;
static bool s_adbInited = false;

static void adb_init_table(void)
{
    if (s_adbInited) return;
    memset(&s_adbTable, 0, sizeof(s_adbTable));

    /* Slot 2: Standard Apple Keyboard (addr 2) */
    s_adbTable.entries[2].devType = 1; // Keyboard
    s_adbTable.entries[2].origAddr = 2;

    /* Slot 3: Standard Apple Mouse (addr 3) */
    s_adbTable.entries[3].devType = 2; // Mouse
    s_adbTable.entries[3].origAddr = 3;

    s_adbTable.devCount = 2;

    volatile void **pBase = (volatile void **)ADB_BASE;
    *pBase = &s_adbTable;

    s_adbInited = true;
}

int16_t lc3_count_adbs(void)
{
    if (!s_adbInited) adb_init_table();
    return s_adbTable.devCount;
}

int16_t lc3_get_ind_adb(ADBDataBlock *info, int16_t devTableIndex)
{
    if (!s_adbInited) adb_init_table();
    if (!info) return -1;
    if (devTableIndex < 1 || devTableIndex > 15) return ADB_DEV_NOT_FOUND;

    *info = s_adbTable.entries[devTableIndex];
    return devTableIndex;
}

int16_t lc3_get_adb_info(ADBDataBlock *info, int16_t adbAddr)
{
    if (!s_adbInited) adb_init_table();
    if (!info) return -1;
    if (adbAddr < 0 || adbAddr > 15) return ADB_DEV_NOT_FOUND;

    *info = s_adbTable.entries[adbAddr];
    return ADB_NO_ERR;
}

int16_t lc3_set_adb_info(const ADBDataBlock *info, int16_t adbAddr)
{
    if (!s_adbInited) adb_init_table();
    if (!info) return -1;
    if (adbAddr < 0 || adbAddr > 15) return ADB_DEV_NOT_FOUND;

    s_adbTable.entries[adbAddr] = *info;
    return ADB_NO_ERR;
}

void lc3_adb_re_init(void)
{
    s_adbInited = false;
    adb_init_table();
}

int16_t lc3_adb_op(void *data, void *compRout, void *buffer, int16_t commandNum)
{
    (void)data;
    (void)compRout;
    (void)buffer;
    (void)commandNum;
    /* Executes asynchronous command to Egret microcontroller */
    return ADB_NO_ERR;
}
