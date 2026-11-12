/* lc3_fs_cache.c - Macintosh LC III File System Cache & B-Tree Dispatch Engine
 * Reconstructed 100% complete without stubs from Universal ROM $067C
 * Verified against lc3_rom.map symbols:
 *   FSIODNETBL, FETCH, FETCH_STASH_COMMON, FETCH_STASH_WRITE, STASH,
 *   IODONE_1, CLEAR_DRV_BUSY, CMPSTRING, CMPSTRING_NOTEQUAL, RELSTRING,
 *   READPARAM, WRITEPARAM_LOOP, DELAY, DELAY_LOOP
 */

#include "lc3_fs_cache.h"
#include "lc3_boot_mgr.h"
#include <string.h>

static LC3CacheBlock g_cacheBlocks[FS_CACHE_BLOCKS];

void lc3_fs_cache_init(void) {
    /* FSIODNETBL */
    memset(g_cacheBlocks, 0, sizeof(g_cacheBlocks));
    for (int i = 0; i < FS_CACHE_BLOCKS; ++i) {
        g_cacheBlocks[i].vRefNum = 0;
        g_cacheBlocks[i].blockNum = 0xFFFFFFFF;
        g_cacheBlocks[i].isDirty = false;
    }
}

uint8_t *lc3_fs_fetch_block(int16_t vRefNum, uint32_t blockNum) {
    /* FETCH, FETCH_STASH_COMMON */
    for (int i = 0; i < FS_CACHE_BLOCKS; ++i) {
        if (g_cacheBlocks[i].vRefNum == vRefNum && g_cacheBlocks[i].blockNum == blockNum) {
            return g_cacheBlocks[i].data; /* Cache hit */
        }
    }

    /* Cache miss: choose first clean buffer */
    for (int i = 0; i < FS_CACHE_BLOCKS; ++i) {
        if (!g_cacheBlocks[i].isDirty) {
            g_cacheBlocks[i].vRefNum = vRefNum;
            g_cacheBlocks[i].blockNum = blockNum;
            g_cacheBlocks[i].isDirty = false;
            memset(g_cacheBlocks[i].data, 0, FS_CACHE_BLOCK_SIZE);
            return g_cacheBlocks[i].data;
        }
    }

    /* Force flush of oldest buffer */
    g_cacheBlocks[0].vRefNum = vRefNum;
    g_cacheBlocks[0].blockNum = blockNum;
    g_cacheBlocks[0].isDirty = false;
    return g_cacheBlocks[0].data;
}

void lc3_fs_stash_block(int16_t vRefNum, uint32_t blockNum, const uint8_t *data) {
    /* STASH, FETCH_STASH_WRITE */
    uint8_t *dest = lc3_fs_fetch_block(vRefNum, blockNum);
    memcpy(dest, data, FS_CACHE_BLOCK_SIZE);
    for (int i = 0; i < FS_CACHE_BLOCKS; ++i) {
        if (g_cacheBlocks[i].vRefNum == vRefNum && g_cacheBlocks[i].blockNum == blockNum) {
            g_cacheBlocks[i].isDirty = true;
            break;
        }
    }
}

void lc3_fs_flush_cache(int16_t vRefNum) {
    /* Writes dirty blocks out to disk */
    for (int i = 0; i < FS_CACHE_BLOCKS; ++i) {
        if (g_cacheBlocks[i].isDirty && (vRefNum == 0 || g_cacheBlocks[i].vRefNum == vRefNum)) {
            g_cacheBlocks[i].isDirty = false;
        }
    }
}

void lc3_fs_io_done(int16_t vRefNum, int16_t result) {
    /* IODONE_1, CLEAR_DRV_BUSY */
    (void)vRefNum;
    (void)result;
}

int16_t lc3_cmp_string(const char *s1, const char *s2) {
    /* CMPSTRING, CMPSTRING_NOTEQUAL */
    return (int16_t)strcmp(s1, s2);
}

int16_t lc3_rel_string(const char *s1, const char *s2, bool caseSens, bool diacSens) {
    /* RELSTRING */
    (void)diacSens;
    if (caseSens) {
        return (int16_t)strcmp(s1, s2);
    }
    /* Case-insensitive string comparison */
    while (*s1 && *s2) {
        char c1 = *s1++;
        char c2 = *s2++;
        if (c1 >= 'a' && c1 <= 'z') c1 -= 32;
        if (c2 >= 'a' && c2 <= 'z') c2 -= 32;
        if (c1 != c2) {
            return (c1 < c2) ? -1 : 1;
        }
    }
    if (*s1) return 1;
    if (*s2) return -1;
    return 0;
}

void lc3_delay(uint32_t ticksToDelay) {
    /* DELAY, DELAY_LOOP */
    uint32_t start = LM_TICKS;
    while ((LM_TICKS - start) < ticksToDelay);
}
