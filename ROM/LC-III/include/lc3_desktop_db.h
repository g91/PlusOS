/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Desktop Manager Database Core Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_DESKTOP_DB_H
#define LC3_DESKTOP_DB_H

#include <stdint.h>
#include <stdbool.h>

#define DT_NO_ERR         0
#define DT_NOT_FOUND   -1300
#define DT_PARAM_ERR     -50

typedef struct DTPBRec {
    void *qLink;
    int16_t qType;
    int16_t ioTrap;
    uint8_t *ioCmdAddr;
    void (*ioCompletion)(struct DTPBRec *dtPB);
    int16_t ioResult;
    char *ioNamePtr;
    int16_t ioVRefNum;
    int16_t ioDTRefNum;
    int16_t ioIndex;
    uint32_t ioTagInfo;
    void *ioDTBuffer;
    uint32_t ioDTReqCount;
    uint32_t ioDTActCount;
    int16_t ioIconType;
    uint32_t ioFileCreator;
    uint32_t ioFileType;
    int32_t ioDirID;
} DTPBRec, *DTPBPtr;

/* Desktop Manager API */
int16_t lc3_dt_get_path(DTPBPtr dtPB);
int16_t lc3_dt_open_inform(DTPBPtr dtPB);
int16_t lc3_dt_close_down(DTPBPtr dtPB);
int16_t lc3_dt_get_icon(DTPBPtr dtPB);
int16_t lc3_dt_get_icon_info(DTPBPtr dtPB);
int16_t lc3_dt_set_comment(DTPBPtr dtPB);
int16_t lc3_dt_get_comment(DTPBPtr dtPB);
int16_t lc3_dt_flush(DTPBPtr dtPB);
int16_t lc3_dt_reset(DTPBPtr dtPB);
int16_t lc3_dt_delete(DTPBPtr dtPB);

#endif /* LC3_DESKTOP_DB_H */
