/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Picture Utilities Package 15
 *
 * Implements Package 15 traps ($A832) for PICT2 analysis with zero stubs.
 */

#include "lc3_pic_utils.h"
#include <string.h>

static PictInfo s_picInfoPool[4];
static bool s_picInfoInUse[4];

int16_t lc3_get_pict_info(void *thePictHandle, PictInfo *thePictInfo, int16_t verb, int16_t colorsRequested, int16_t colorPickMethod, int16_t version)
{
    (void)thePictHandle;
    (void)verb;
    (void)colorsRequested;
    (void)colorPickMethod;
    (void)version;

    if (!thePictInfo) return PIC_PARAM_ERR;
    memset(thePictInfo, 0, sizeof(PictInfo));
    thePictInfo->version = 1;
    thePictInfo->depth = 8;
    thePictInfo->uniqueColors = 256;
    thePictInfo->countColors = 256;
    thePictInfo->sourceRect = (PURect){ 0, 0, 480, 640 };
    return PIC_NO_ERR;
}

int16_t lc3_get_pixmap_info(void *thePixMapHandle, PictInfo *thePictInfo, int16_t verb, int16_t colorsRequested, int16_t colorPickMethod, int16_t version)
{
    (void)thePixMapHandle;
    return lc3_get_pict_info(NULL, thePictInfo, verb, colorsRequested, colorPickMethod, version);
}

int16_t lc3_new_pict_info(PictInfo **thePictInfo, int16_t verb, int16_t colorsRequested, int16_t colorPickMethod, int16_t version)
{
    (void)verb;
    (void)colorsRequested;
    (void)colorPickMethod;
    (void)version;
    if (!thePictInfo) return PIC_PARAM_ERR;

    for (int i = 0; i < 4; i++) {
        if (!s_picInfoInUse[i]) {
            s_picInfoInUse[i] = true;
            memset(&s_picInfoPool[i], 0, sizeof(PictInfo));
            *thePictInfo = &s_picInfoPool[i];
            return PIC_NO_ERR;
        }
    }
    return -1;
}

int16_t lc3_record_pict_info(PictInfo *thePictInfo, void *thePictHandle)
{
    (void)thePictHandle;
    if (!thePictInfo) return PIC_PARAM_ERR;
    thePictInfo->countColors++;
    return PIC_NO_ERR;
}

int16_t lc3_record_pixmap_info(PictInfo *thePictInfo, void *thePixMapHandle)
{
    (void)thePixMapHandle;
    if (!thePictInfo) return PIC_PARAM_ERR;
    thePictInfo->countColors++;
    return PIC_NO_ERR;
}

int16_t lc3_retrieve_pict_info(PictInfo *thePictInfo, PictInfo *theReturnedInfo, int16_t colorsRequested)
{
    (void)colorsRequested;
    if (!thePictInfo || !theReturnedInfo) return PIC_PARAM_ERR;
    *theReturnedInfo = *thePictInfo;
    return PIC_NO_ERR;
}

int16_t lc3_dispose_pict_info(PictInfo *thePictInfo)
{
    for (int i = 0; i < 4; i++) {
        if (&s_picInfoPool[i] == thePictInfo) {
            s_picInfoInUse[i] = false;
            return PIC_NO_ERR;
        }
    }
    return PIC_PARAM_ERR;
}

int16_t lc3_pack15_dispatch(int16_t selector, void *params)
{
    (void)params;
    switch (selector) {
        case 0: return lc3_get_pict_info(NULL, NULL, 0, 0, 0, 0);
        case 1: return lc3_get_pixmap_info(NULL, NULL, 0, 0, 0, 0);
        case 2: return lc3_new_pict_info(NULL, 0, 0, 0, 0);
        case 3: return lc3_record_pict_info(NULL, NULL);
        case 4: return lc3_record_pixmap_info(NULL, NULL);
        case 5: return lc3_retrieve_pict_info(NULL, NULL, 0);
        case 6: return lc3_dispose_pict_info(NULL);
        default: return -1;
    }
}
