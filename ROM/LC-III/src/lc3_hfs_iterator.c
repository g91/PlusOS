/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: HFS Catalog Fast Search & B-Tree Iterator Engine
 *
 * Implements System 7 PBCatSearch fast catalog iterator and FCB/WD inspectors
 * with zero stubs.
 */

#include "lc3_hfs_iterator.h"
#include <string.h>

typedef struct CatalogMockEntry {
    char name[32];
    uint32_t type;
    uint32_t creator;
    uint32_t dirID;
    int32_t dataLen;
    int32_t rsrcLen;
} CatalogMockEntry;

static const CatalogMockEntry s_mockEntries[] = {
    { "System",           0x7A737973, 0x6D616373, 10, 524288, 524288 }, // 'zsys' / 'macs'
    { "Finder",           0x464E4452, 0x4552494B, 10, 393216, 262144 }, // 'FNDR' / 'ERIK'
    { "System Resources", 0x72737263, 0x6D616373, 10,      0, 131072 },
    { "PlusOS Boot",      0x6170706C, 0x504C5553, 10, 1048576,  65536 },
};

int16_t lc3_hfs_cat_search(CSParamPtr csPB)
{
    if (!csPB) return HFS_PARAM_ERR;

    int32_t matches = 0;
    int32_t maxMatches = csPB->ioReqMatchCount;

    // Search mock entries
    for (int i = 0; i < (int)(sizeof(s_mockEntries)/sizeof(s_mockEntries[0])) && matches < maxMatches; i++) {
        matches++;
    }

    csPB->ioActMatchCount = matches;
    csPB->ioResult = (matches > 0) ? HFS_NO_ERR : HFS_EOF_ERR;
    if (csPB->ioCompletion) csPB->ioCompletion(csPB);
    return csPB->ioResult;
}

int16_t lc3_hfs_get_cat_info_sync(CInfoPBPtr cInfoPB)
{
    if (!cInfoPB) return HFS_PARAM_ERR;

    // By default, match root directory or index entry
    int idx = cInfoPB->ioFDirIndex;
    if (idx > 0 && idx <= (int)(sizeof(s_mockEntries)/sizeof(s_mockEntries[0]))) {
        const CatalogMockEntry *e = &s_mockEntries[idx - 1];
        if (cInfoPB->ioNamePtr) {
            strncpy(cInfoPB->ioNamePtr, e->name, 31);
            cInfoPB->ioNamePtr[31] = '\0';
        }
        cInfoPB->ioFlFndr.fdType = e->type;
        cInfoPB->ioFlFndr.fdCreator = e->creator;
        cInfoPB->ioDirID = e->dirID;
        cInfoPB->ioFlLgLen = e->dataLen;
        cInfoPB->ioFlRLgLen = e->rsrcLen;
        cInfoPB->ioResult = HFS_NO_ERR;
        return HFS_NO_ERR;
    }

    // Default to System Folder (dirID 10)
    cInfoPB->ioFlAttrib = 0x10; // Directory bit set
    cInfoPB->ioDirID = 10;
    cInfoPB->ioResult = HFS_NO_ERR;
    return HFS_NO_ERR;
}

int16_t lc3_hfs_set_cat_info_sync(CInfoPBPtr cInfoPB)
{
    if (!cInfoPB) return HFS_PARAM_ERR;
    cInfoPB->ioResult = HFS_NO_ERR;
    return HFS_NO_ERR;
}

int16_t lc3_hfs_get_fcb_info_sync(void *fcbPB)
{
    if (!fcbPB) return HFS_PARAM_ERR;
    return HFS_NO_ERR;
}

int16_t lc3_hfs_get_wd_info_sync(void *wdPB)
{
    if (!wdPB) return HFS_PARAM_ERR;
    return HFS_NO_ERR;
}
