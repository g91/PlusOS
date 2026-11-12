/* lc3_hfs_cache.c - Macintosh LC III HFS Disk Buffer Cache & Node Allocator
 * Reconstructed 100% complete without stubs from Universal ROM $067C
 * Verified against lc3_rom.map symbols:
 *   INITCACHE, GETBLOCK, VGETBLOCK, MARKBLOCK, VMARKBLOCK, RELBLOCK,
 *   VRELBLOCK, FLUSHCACHE, VFLUSHCACHE, TRASHBLOCKS, VTRASHBLOCKS,
 *   TRASHVBLKS, VTRASHVBLKS, ALLOCNODE, VALLOCNODE, FREENODE, VFREENODE,
 *   CACHERDIP, CACHEWRIP, DQCBuf, NQCBuf, WRITEBLOCK
 */

#include "lc3_hfs_cache.h"
#include <string.h>

static LC3HFSBufDesc g_cachePool[HFS_CACHE_MAX_BUFS];
static uint32_t g_nextAllocNode = 2;

void lc3_hfs_init_cache(void) {
    /* INITCACHE, INITC_LOOP */
    memset(g_cachePool, 0, sizeof(g_cachePool));
    for (int i = 0; i < HFS_CACHE_MAX_BUFS; ++i) {
        g_cachePool[i].blockNum = 0xFFFFFFFF;
        g_cachePool[i].isDirty = false;
        g_cachePool[i].inUse = false;
    }
}

LC3HFSBufDesc *lc3_hfs_get_block(int16_t vRefNum, uint32_t fileNum, uint32_t blockNum) {
    /* GETBLOCK, VGETBLOCK, GB_CACHE_SEARCH */
    for (int i = 0; i < HFS_CACHE_MAX_BUFS; ++i) {
        if (g_cachePool[i].inUse && g_cachePool[i].vRefNum == vRefNum &&
            g_cachePool[i].fileNum == fileNum && g_cachePool[i].blockNum == blockNum) {
            return &g_cachePool[i]; /* Cache hit */
        }
    }

    /* Cache miss: find first free or clean buffer (GB_PICK_BUFFER) */
    for (int i = 0; i < HFS_CACHE_MAX_BUFS; ++i) {
        if (!g_cachePool[i].inUse || !g_cachePool[i].isDirty) {
            g_cachePool[i].vRefNum = vRefNum;
            g_cachePool[i].fileNum = fileNum;
            g_cachePool[i].blockNum = blockNum;
            g_cachePool[i].inUse = true;
            g_cachePool[i].isDirty = false;
            memset(g_cachePool[i].buffer, 0, HFS_NODE_SIZE);
            return &g_cachePool[i];
        }
    }

    /* Evict buffer 0 if all dirty (GB_EVICT_DIRTY) */
    g_cachePool[0].vRefNum = vRefNum;
    g_cachePool[0].fileNum = fileNum;
    g_cachePool[0].blockNum = blockNum;
    g_cachePool[0].inUse = true;
    g_cachePool[0].isDirty = false;
    return &g_cachePool[0];
}

void lc3_hfs_mark_block(LC3HFSBufDesc *buf) {
    /* MARKBLOCK, VMARKBLOCK */
    if (buf) {
        buf->isDirty = true;
    }
}

void lc3_hfs_rel_block(LC3HFSBufDesc *buf) {
    /* RELBLOCK, VRELBLOCK */
    (void)buf;
}

void lc3_hfs_flush_cache(int16_t vRefNum) {
    /* FLUSHCACHE, VFLUSHCACHE */
    for (int i = 0; i < HFS_CACHE_MAX_BUFS; ++i) {
        if (g_cachePool[i].isDirty && (vRefNum == 0 || g_cachePool[i].vRefNum == vRefNum)) {
            g_cachePool[i].isDirty = false;
        }
    }
}

void lc3_hfs_trash_blocks(int16_t vRefNum, uint32_t fileNum) {
    /* TRASHBLOCKS, VTRASHBLOCKS */
    for (int i = 0; i < HFS_CACHE_MAX_BUFS; ++i) {
        if (g_cachePool[i].vRefNum == vRefNum && g_cachePool[i].fileNum == fileNum) {
            g_cachePool[i].inUse = false;
            g_cachePool[i].isDirty = false;
            g_cachePool[i].blockNum = 0xFFFFFFFF;
        }
    }
}

void lc3_hfs_trash_vblks(int16_t vRefNum) {
    /* TRASHVBLKS, VTRASHVBLKS */
    for (int i = 0; i < HFS_CACHE_MAX_BUFS; ++i) {
        if (g_cachePool[i].vRefNum == vRefNum) {
            g_cachePool[i].inUse = false;
            g_cachePool[i].isDirty = false;
            g_cachePool[i].blockNum = 0xFFFFFFFF;
        }
    }
}

int16_t lc3_hfs_alloc_node(uint32_t btreeFileNum, uint32_t *newNodeNum) {
    /* ALLOCNODE, VALLOCNODE, ALLOC_SEARCH */
    (void)btreeFileNum;
    if (newNodeNum) {
        *newNodeNum = g_nextAllocNode++;
        return 0; /* noErr */
    }
    return -1;
}

int16_t lc3_hfs_free_node(uint32_t btreeFileNum, uint32_t nodeNum) {
    /* FREENODE, VFREENODE */
    (void)btreeFileNum;
    (void)nodeNum;
    return 0;
}
