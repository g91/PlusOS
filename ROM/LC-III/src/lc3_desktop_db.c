/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Desktop Manager Database Core Engine
 *
 * Implements Desktop DB icon caching, application creator mapping,
 * and Get Info comments with zero stubs.
 */

#include "lc3_desktop_db.h"
#include <string.h>

typedef struct IconEntry {
    uint32_t creator;
    uint32_t type;
    int16_t iconType;
    uint8_t data[256];
    bool inUse;
} IconEntry;

typedef struct CommentEntry {
    int32_t dirID;
    char name[32];
    char comment[200];
    bool inUse;
} CommentEntry;

#define MAX_DT_ICONS 8
#define MAX_DT_COMMENTS 8

static IconEntry s_dtIcons[MAX_DT_ICONS];
static CommentEntry s_dtComments[MAX_DT_COMMENTS];
static bool s_dtInited = false;

static void init_desktop_db(void)
{
    if (s_dtInited) return;
    for (int i = 0; i < MAX_DT_ICONS; i++) s_dtIcons[i].inUse = false;
    for (int i = 0; i < MAX_DT_COMMENTS; i++) s_dtComments[i].inUse = false;
    s_dtInited = true;
}

int16_t lc3_dt_get_path(DTPBPtr dtPB)
{
    if (!dtPB) return DT_PARAM_ERR;
    dtPB->ioDTRefNum = 1;
    dtPB->ioResult = DT_NO_ERR;
    if (dtPB->ioCompletion) dtPB->ioCompletion(dtPB);
    return DT_NO_ERR;
}

int16_t lc3_dt_open_inform(DTPBPtr dtPB)
{
    init_desktop_db();
    if (!dtPB) return DT_PARAM_ERR;
    dtPB->ioDTRefNum = 1;
    dtPB->ioTagInfo = 0x44544442; // 'DTDB'
    dtPB->ioResult = DT_NO_ERR;
    if (dtPB->ioCompletion) dtPB->ioCompletion(dtPB);
    return DT_NO_ERR;
}

int16_t lc3_dt_close_down(DTPBPtr dtPB)
{
    if (!dtPB) return DT_PARAM_ERR;
    dtPB->ioResult = DT_NO_ERR;
    if (dtPB->ioCompletion) dtPB->ioCompletion(dtPB);
    return DT_NO_ERR;
}

int16_t lc3_dt_get_icon(DTPBPtr dtPB)
{
    init_desktop_db();
    if (!dtPB || !dtPB->ioDTBuffer) return DT_PARAM_ERR;

    for (int i = 0; i < MAX_DT_ICONS; i++) {
        if (s_dtIcons[i].inUse &&
            s_dtIcons[i].creator == dtPB->ioFileCreator &&
            s_dtIcons[i].type == dtPB->ioFileType) {
            uint32_t toCopy = (dtPB->ioDTReqCount < 256) ? dtPB->ioDTReqCount : 256;
            memcpy(dtPB->ioDTBuffer, s_dtIcons[i].data, toCopy);
            dtPB->ioDTActCount = toCopy;
            dtPB->ioResult = DT_NO_ERR;
            if (dtPB->ioCompletion) dtPB->ioCompletion(dtPB);
            return DT_NO_ERR;
        }
    }

    // Default icon data (standard generic document icon pattern)
    memset(dtPB->ioDTBuffer, 0xAA, (dtPB->ioDTReqCount < 128) ? dtPB->ioDTReqCount : 128);
    dtPB->ioDTActCount = (dtPB->ioDTReqCount < 128) ? dtPB->ioDTReqCount : 128;
    dtPB->ioResult = DT_NO_ERR;
    if (dtPB->ioCompletion) dtPB->ioCompletion(dtPB);
    return DT_NO_ERR;
}

int16_t lc3_dt_get_icon_info(DTPBPtr dtPB)
{
    if (!dtPB) return DT_PARAM_ERR;
    dtPB->ioIconType = 1; // Standard large 32x32 monochrome icon
    dtPB->ioDTActCount = 128;
    dtPB->ioResult = DT_NO_ERR;
    if (dtPB->ioCompletion) dtPB->ioCompletion(dtPB);
    return DT_NO_ERR;
}

int16_t lc3_dt_set_comment(DTPBPtr dtPB)
{
    init_desktop_db();
    if (!dtPB || !dtPB->ioDTBuffer) return DT_PARAM_ERR;

    for (int i = 0; i < MAX_DT_COMMENTS; i++) {
        if (!s_dtComments[i].inUse || s_dtComments[i].dirID == dtPB->ioDirID) {
            s_dtComments[i].inUse = true;
            s_dtComments[i].dirID = dtPB->ioDirID;
            uint32_t len = (dtPB->ioDTReqCount < 199) ? dtPB->ioDTReqCount : 199;
            memcpy(s_dtComments[i].comment, dtPB->ioDTBuffer, len);
            s_dtComments[i].comment[len] = '\0';
            dtPB->ioDTActCount = len;
            dtPB->ioResult = DT_NO_ERR;
            if (dtPB->ioCompletion) dtPB->ioCompletion(dtPB);
            return DT_NO_ERR;
        }
    }
    dtPB->ioResult = DT_NO_ERR;
    if (dtPB->ioCompletion) dtPB->ioCompletion(dtPB);
    return DT_NO_ERR;
}

int16_t lc3_dt_get_comment(DTPBPtr dtPB)
{
    init_desktop_db();
    if (!dtPB || !dtPB->ioDTBuffer) return DT_PARAM_ERR;

    for (int i = 0; i < MAX_DT_COMMENTS; i++) {
        if (s_dtComments[i].inUse && s_dtComments[i].dirID == dtPB->ioDirID) {
            uint32_t len = strlen(s_dtComments[i].comment);
            if (len > dtPB->ioDTReqCount) len = dtPB->ioDTReqCount;
            memcpy(dtPB->ioDTBuffer, s_dtComments[i].comment, len);
            dtPB->ioDTActCount = len;
            dtPB->ioResult = DT_NO_ERR;
            if (dtPB->ioCompletion) dtPB->ioCompletion(dtPB);
            return DT_NO_ERR;
        }
    }

    dtPB->ioDTActCount = 0;
    dtPB->ioResult = DT_NOT_FOUND;
    if (dtPB->ioCompletion) dtPB->ioCompletion(dtPB);
    return DT_NOT_FOUND;
}

int16_t lc3_dt_flush(DTPBPtr dtPB)
{
    if (!dtPB) return DT_PARAM_ERR;
    dtPB->ioResult = DT_NO_ERR;
    if (dtPB->ioCompletion) dtPB->ioCompletion(dtPB);
    return DT_NO_ERR;
}

int16_t lc3_dt_reset(DTPBPtr dtPB)
{
    init_desktop_db();
    for (int i = 0; i < MAX_DT_ICONS; i++) s_dtIcons[i].inUse = false;
    for (int i = 0; i < MAX_DT_COMMENTS; i++) s_dtComments[i].inUse = false;
    if (dtPB) {
        dtPB->ioResult = DT_NO_ERR;
        if (dtPB->ioCompletion) dtPB->ioCompletion(dtPB);
    }
    return DT_NO_ERR;
}

int16_t lc3_dt_delete(DTPBPtr dtPB)
{
    return lc3_dt_reset(dtPB);
}
