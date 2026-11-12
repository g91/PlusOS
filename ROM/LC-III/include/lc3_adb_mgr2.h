/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Extended ADB Manager 2.0
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_ADB_MGR2_H
#define LC3_ADB_MGR2_H

#include <stdint.h>
#include <stdbool.h>

#define ADB_NO_ERR          0
#define ADB_DEV_NOT_FOUND -1
#define ADB_BUS_ERROR     -2

typedef struct {
    int8_t devType;
    int8_t origAddr;
    void *dbServiceRtPtr;
    void *dbDataAreaAddr;
} ADBDataBlock;

typedef struct {
    ADBDataBlock entries[16];
    int16_t devCount;
} ADBDevTable;

typedef struct {
    void *dataBuffer;
    void *compRout;
    void *bufferLimits;
} ADBOpBlock;

/* ADB Manager 2.0 API */
int16_t lc3_count_adbs(void);
int16_t lc3_get_ind_adb(ADBDataBlock *info, int16_t devTableIndex);
int16_t lc3_get_adb_info(ADBDataBlock *info, int16_t adbAddr);
int16_t lc3_set_adb_info(const ADBDataBlock *info, int16_t adbAddr);
void lc3_adb_re_init(void);
int16_t lc3_adb_op(void *data, void *compRout, void *buffer, int16_t commandNum);

#endif /* LC3_ADB_MGR2_H */
