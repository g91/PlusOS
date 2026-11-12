/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: HFS Catalog Fast Search & B-Tree Iterator Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_HFS_ITERATOR_H
#define LC3_HFS_ITERATOR_H

#include <stdint.h>
#include <stdbool.h>

#define HFS_NO_ERR         0
#define HFS_FNF_ERR      -43 // File not found
#define HFS_EOF_ERR      -39 // End of catalog
#define HFS_PARAM_ERR    -50

typedef struct CInfoPBRec {
    void *qLink;
    int16_t qType;
    int16_t ioTrap;
    uint8_t *ioCmdAddr;
    void (*ioCompletion)(struct CInfoPBRec *pb);
    int16_t ioResult;
    char *ioNamePtr;
    int16_t ioVRefNum;
    int16_t ioFRefNum;
    int8_t  ioFVersNum;
    int8_t  filler1;
    int16_t ioFDirIndex;
    int8_t  ioFlAttrib;
    int8_t  ioFlVersNum;
    struct {
        uint32_t fdType;
        uint32_t fdCreator;
        uint16_t fdFlags;
        int16_t  fdLocationV;
        int16_t  fdLocationH;
        int16_t  fdFldr;
    } ioFlFndr;
    uint32_t ioDirID;
    uint16_t ioFlStBlk;
    int32_t  ioFlLgLen;
    int32_t  ioFlPyLen;
    uint16_t ioFlRStBlk;
    int32_t  ioFlRLgLen;
    int32_t  ioFlRPyLen;
    uint32_t ioFlCrDat;
    uint32_t ioFlMdDat;
} CInfoPBRec, *CInfoPBPtr;

typedef struct CSParam {
    void *qLink;
    int16_t qType;
    int16_t ioTrap;
    uint8_t *ioCmdAddr;
    void (*ioCompletion)(struct CSParam *pb);
    int16_t ioResult;
    char *ioNamePtr;
    int16_t ioVRefNum;
    int32_t ioMatchPtr;
    int32_t ioReqMatchCount;
    int32_t ioActMatchCount;
    int32_t ioSearchBits;
    CInfoPBPtr ioSearchInfo1;
    CInfoPBPtr ioSearchInfo2;
    uint32_t ioSearchTime;
    uint32_t ioCatPosition[4];
} CSParam, *CSParamPtr;

/* HFS Iterator API */
int16_t lc3_hfs_cat_search(CSParamPtr csPB);
int16_t lc3_hfs_get_cat_info_sync(CInfoPBPtr cInfoPB);
int16_t lc3_hfs_set_cat_info_sync(CInfoPBPtr cInfoPB);
int16_t lc3_hfs_get_fcb_info_sync(void *fcbPB);
int16_t lc3_hfs_get_wd_info_sync(void *wdPB);

#endif /* LC3_HFS_ITERATOR_H */
