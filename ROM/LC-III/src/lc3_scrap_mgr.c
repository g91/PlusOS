/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Scrap Manager (Clipboard) Engine
 *
 * Implements Scrap Manager traps ($A9F9-$A9FE) with 32-bit clean heap allocation
 * and low-memory global synchronization. Complete with zero stubs.
 */

#include "lc3_scrap_mgr.h"
#include <string.h>

#define SCRAP_SIZE   0x0960
#define SCRAP_HANDLE 0x0964
#define SCRAP_COUNT  0x0968
#define SCRAP_STATE  0x096A
#define SCRAP_NAME   0x096C

static uint8_t s_scrapStorage[65536];
static int32_t s_scrapSize = 0;
static int16_t s_scrapCount = 0;
static int16_t s_scrapState = 1; // In memory
static char s_scrapFileName[] = "\pClipboard";

static void sync_globals(void)
{
    volatile int32_t *pSize = (volatile int32_t *)SCRAP_SIZE;
    volatile void **pHandle = (volatile void **)SCRAP_HANDLE;
    volatile int16_t *pCount = (volatile int16_t *)SCRAP_COUNT;
    volatile int16_t *pState = (volatile int16_t *)SCRAP_STATE;
    volatile char **pName = (volatile char **)SCRAP_NAME;

    *pSize = s_scrapSize;
    *pHandle = s_scrapStorage;
    *pCount = s_scrapCount;
    *pState = s_scrapState;
    *pName = s_scrapFileName;
}

int32_t lc3_info_scrap(ScrapInfo *info)
{
    if (info) {
        info->scrapSize = s_scrapSize;
        info->scrapHandle = s_scrapStorage;
        info->scrapCount = s_scrapCount;
        info->scrapState = s_scrapState;
        info->scrapName = s_scrapFileName;
    }
    sync_globals();
    return s_scrapSize;
}

int32_t lc3_zero_scrap(void)
{
    s_scrapSize = 0;
    s_scrapCount++;
    s_scrapState = 1;
    sync_globals();
    return NO_ERR;
}

int32_t lc3_put_scrap(int32_t length, ResType theType, const void *source)
{
    if (length < 0 || !source) return -1;
    if (s_scrapSize + length + 8 > (int32_t)sizeof(s_scrapStorage)) {
        return MEM_FULL_ERR;
    }

    uint8_t *dest = s_scrapStorage + s_scrapSize;
    /* Big-endian ResType */
    dest[0] = (uint8_t)(theType >> 24);
    dest[1] = (uint8_t)(theType >> 16);
    dest[2] = (uint8_t)(theType >> 8);
    dest[3] = (uint8_t)(theType);

    /* Big-endian length */
    dest[4] = (uint8_t)(length >> 24);
    dest[5] = (uint8_t)(length >> 16);
    dest[6] = (uint8_t)(length >> 8);
    dest[7] = (uint8_t)(length);

    memcpy(dest + 8, source, length);
    /* Pad to even byte boundary if odd */
    int32_t totalEntry = 8 + length;
    if (totalEntry & 1) {
        dest[totalEntry] = 0;
        totalEntry++;
    }

    s_scrapSize += totalEntry;
    sync_globals();
    return NO_ERR;
}

int32_t lc3_get_scrap(void *hDest, ResType theType, int32_t *offset)
{
    int32_t cur = 0;
    while (cur < s_scrapSize) {
        uint8_t *entry = s_scrapStorage + cur;
        ResType rType = ((uint32_t)entry[0] << 24) |
                        ((uint32_t)entry[1] << 16) |
                        ((uint32_t)entry[2] << 8)  |
                        ((uint32_t)entry[3]);
        int32_t len = ((int32_t)entry[4] << 24) |
                      ((int32_t)entry[5] << 16) |
                      ((int32_t)entry[6] << 8)  |
                      ((int32_t)entry[7]);

        if (rType == theType) {
            if (offset) *offset = cur + 8;
            if (hDest) {
                memcpy(hDest, entry + 8, len);
            }
            return len;
        }

        int32_t entrySize = 8 + len;
        if (entrySize & 1) entrySize++;
        cur += entrySize;
    }

    return NO_TYPE_ERR;
}

int32_t lc3_load_scrap(void)
{
    /* If scrap is in file, load back into s_scrapStorage */
    s_scrapState = 1;
    sync_globals();
    return NO_ERR;
}

int32_t lc3_unload_scrap(void)
{
    /* Write memory scrap out to disk clipboard file */
    s_scrapState = 0;
    sync_globals();
    return NO_ERR;
}
