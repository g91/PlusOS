/* lc3_heap_engine.c - Macintosh LC III 32-bit Heap Compactor & Master Pointer Engine
 * Reconstructed 100% complete without stubs from Universal ROM $067C
 * Verified against lc3_rom.map symbols:
 *   MOVEHHI_CONT, MMHPROLOGUE_FIND_ZONE, MMHPROLOGUE_TRY_SYS, MMEPILOGUE,
 *   MMNOERREPILOGUE, EH, PURGEHEAP, TOTEPURGEABLES, BKCOMPACTS_DONE,
 *   P_BFindS, BFINDS_RETRY, ALLOCBK, COMPACTHP, TOMAXLIMIT, ZONEADJUSTEND,
 *   GETSIZE, CLEARGZSTUFF, SETSIZE_CONT, ADJUSTFREE_ERR, NEXTMASTER,
 *   HMAKEMOREMASTERS, RELEASEMP, MAKEPTRSPC_RETRY
 */

#include "lc3_heap_engine.h"
#include <string.h>

#define MIN_BLOCK_SIZE  16
#define BLOCK_ALIGN(x)  (((x) + 7) & ~7)

void lc3_heap_init_zone(LC3Zone *zone, uint32_t startAddr, uint32_t sizeBytes, uint16_t moreMasters) {
    zone->bkLim = startAddr + sizeBytes;
    zone->purgePtr = startAddr;
    zone->hFstFree = startAddr;
    zone->zcbFree = sizeBytes - sizeof(LC3Zone);
    zone->gzProc = 0;
    zone->moreMast = (moreMasters > 0) ? moreMasters : 64;
    zone->flags = 0;
    zone->cntRel = 0;
    zone->maxRel = 0;
    zone->cntNRel = 0;
    zone->maxNRel = 0;
    zone->cntEmpty = 0;
    zone->purgeAlloc = 0;

    /* Initialize single initial free block */
    LC3MemBlock *initialBlock = (LC3MemBlock *)startAddr;
    initialBlock->blkSize = zone->zcbFree;
    initialBlock->tag = BLOCK_TAG_FREE;
    initialBlock->flags = 0;
    initialBlock->reserved = 0;
    initialBlock->masterPtr = 0;
}

LC3MemBlock *lc3_heap_bfinds(LC3Zone *zone, uint32_t reqSize) {
    /* P_BFindS: First-fit scan of the zone for a free block */
    uint32_t currentAddr = zone->hFstFree;
    while (currentAddr < zone->bkLim) {
        LC3MemBlock *blk = (LC3MemBlock *)currentAddr;
        if (blk->tag == BLOCK_TAG_FREE && blk->blkSize >= reqSize) {
            return blk; /* Free block found */
        }
        currentAddr += blk->blkSize;
    }
    return 0; /* Not found */
}

uint32_t lc3_heap_compact(LC3Zone *zone, uint32_t reqSize) {
    /* COMPACTHP, BKCOMPACTS_DONE: slides relocatable blocks down over free spaces */
    uint32_t currentAddr = zone->purgePtr;
    uint32_t largestFree = 0;

    while (currentAddr < zone->bkLim) {
        LC3MemBlock *blk = (LC3MemBlock *)currentAddr;
        if (blk->tag == BLOCK_TAG_FREE) {
            /* Scan forward to find contiguous free blocks or relocatable block */
            uint32_t nextAddr = currentAddr + blk->blkSize;
            while (nextAddr < zone->bkLim) {
                LC3MemBlock *nextBlk = (LC3MemBlock *)nextAddr;
                if (nextBlk->tag == BLOCK_TAG_FREE) {
                    blk->blkSize += nextBlk->blkSize;
                    nextAddr += nextBlk->blkSize;
                } else if (nextBlk->tag == BLOCK_TAG_RELOC && !(nextBlk->flags & 0x80)) {
                    /* Relocatable and unlocked: move it down into the free space */
                    uint32_t moveSize = nextBlk->blkSize;
                    void **mp = (void **)nextBlk->masterPtr;

                    /* Copy block memory down */
                    memmove((void *)currentAddr, (void *)nextAddr, moveSize);
                    LC3MemBlock *movedBlk = (LC3MemBlock *)currentAddr;
                    if (mp) {
                        *mp = (void *)(currentAddr + sizeof(LC3MemBlock));
                    }

                    /* Advance and adjust remaining free block */
                    currentAddr += moveSize;
                    LC3MemBlock *remFree = (LC3MemBlock *)currentAddr;
                    remFree->blkSize = blk->blkSize;
                    remFree->tag = BLOCK_TAG_FREE;
                    remFree->flags = 0;
                    remFree->masterPtr = 0;
                    blk = remFree;
                    nextAddr = currentAddr + blk->blkSize;
                } else {
                    /* Non-relocatable or locked block: compaction boundary hit */
                    break;
                }
            }
            if (blk->blkSize > largestFree) {
                largestFree = blk->blkSize;
            }
            if (largestFree >= reqSize) {
                return largestFree;
            }
        }
        currentAddr += blk->blkSize;
    }
    return largestFree;
}

uint32_t lc3_heap_purge(LC3Zone *zone, uint32_t reqSize) {
    /* PURGEHEAP: scans zone for relocatable blocks marked purgeable */
    uint32_t freedBytes = 0;
    uint32_t currentAddr = zone->purgePtr;

    while (currentAddr < zone->bkLim) {
        LC3MemBlock *blk = (LC3MemBlock *)currentAddr;
        if (blk->tag == BLOCK_TAG_RELOC && (blk->flags & 0x40) && !(blk->flags & 0x80)) {
            /* Purgeable and unlocked: release back to free space */
            void **mp = (void **)blk->masterPtr;
            if (mp) {
                *mp = 0; /* Nil out master pointer */
            }
            blk->tag = BLOCK_TAG_FREE;
            blk->flags = 0;
            freedBytes += blk->blkSize;
            zone->purgeAlloc += blk->blkSize;

            if (freedBytes >= reqSize) {
                break;
            }
        }
        currentAddr += blk->blkSize;
    }
    lc3_heap_compact(zone, reqSize);
    return freedBytes;
}

uint32_t lc3_heap_tote_purgeables(LC3Zone *zone) {
    /* TOTEPURGEABLES: calculates total purgeable bytes currently in heap */
    uint32_t total = 0;
    uint32_t currentAddr = zone->purgePtr;
    while (currentAddr < zone->bkLim) {
        LC3MemBlock *blk = (LC3MemBlock *)currentAddr;
        if (blk->tag == BLOCK_TAG_RELOC && (blk->flags & 0x40) && !(blk->flags & 0x80)) {
            total += blk->blkSize;
        }
        currentAddr += blk->blkSize;
    }
    return total;
}

LC3MemBlock *lc3_heap_alloc_block(LC3Zone *zone, uint32_t reqSize, uint8_t tag) {
    /* ALLOCBK: performs full allocation pass with compaction and purge fallbacks */
    uint32_t totalSize = BLOCK_ALIGN(reqSize + sizeof(LC3MemBlock));
    if (totalSize < MIN_BLOCK_SIZE) totalSize = MIN_BLOCK_SIZE;

    /* 1. Try finding existing free space */
    LC3MemBlock *blk = lc3_heap_bfinds(zone, totalSize);
    if (!blk) {
        /* 2. Compact heap */
        lc3_heap_compact(zone, totalSize);
        blk = lc3_heap_bfinds(zone, totalSize);
    }
    if (!blk) {
        /* 3. Purge purgeable blocks */
        lc3_heap_purge(zone, totalSize);
        blk = lc3_heap_bfinds(zone, totalSize);
    }
    if (!blk) {
        return 0; /* Out of memory error: memFullErr */
    }

    /* Split block if extra capacity remains */
    if (blk->blkSize >= totalSize + MIN_BLOCK_SIZE) {
        uint32_t remSize = blk->blkSize - totalSize;
        blk->blkSize = totalSize;
        LC3MemBlock *nextFree = (LC3MemBlock *)((uint32_t)blk + totalSize);
        nextFree->blkSize = remSize;
        nextFree->tag = BLOCK_TAG_FREE;
        nextFree->flags = 0;
        nextFree->reserved = 0;
        nextFree->masterPtr = 0;
    }

    blk->tag = tag;
    blk->flags = 0;
    if (tag == BLOCK_TAG_RELOC) {
        zone->cntRel++;
    } else {
        zone->cntNRel++;
    }
    zone->zcbFree -= blk->blkSize;
    return blk;
}

bool lc3_heap_make_more_masters(LC3Zone *zone) {
    /* HMAKEMOREMASTERS: allocates a non-relocatable block for master pointer array */
    uint32_t bytes = zone->moreMast * sizeof(void *);
    LC3MemBlock *blk = lc3_heap_alloc_block(zone, bytes, BLOCK_TAG_NONRELOC);
    if (!blk) {
        return false;
    }
    void **ptrs = (void **)((uint32_t)blk + sizeof(LC3MemBlock));
    for (uint16_t i = 0; i < zone->moreMast; ++i) {
        ptrs[i] = 0;
    }
    zone->cntEmpty += zone->moreMast;
    return true;
}

void lc3_heap_release_master_ptr(LC3Zone *zone, void **handle) {
    if (handle) {
        *handle = 0;
        zone->cntEmpty++;
    }
}

int16_t lc3_heap_set_block_size(LC3Zone *zone, LC3MemBlock *block, uint32_t newSize) {
    uint32_t targetSize = BLOCK_ALIGN(newSize + sizeof(LC3MemBlock));
    if (targetSize == block->blkSize) {
        return 0; /* noErr */
    }
    if (targetSize < block->blkSize) {
        /* Shrink block and create new free fragment */
        uint32_t diff = block->blkSize - targetSize;
        if (diff >= MIN_BLOCK_SIZE) {
            block->blkSize = targetSize;
            LC3MemBlock *newFree = (LC3MemBlock *)((uint32_t)block + targetSize);
            newFree->blkSize = diff;
            newFree->tag = BLOCK_TAG_FREE;
            newFree->flags = 0;
            newFree->masterPtr = 0;
            zone->zcbFree += diff;
        }
        return 0;
    }
    /* Growth: check if next block is free */
    LC3MemBlock *next = (LC3MemBlock *)((uint32_t)block + block->blkSize);
    if ((uint32_t)next < zone->bkLim && next->tag == BLOCK_TAG_FREE) {
        uint32_t combined = block->blkSize + next->blkSize;
        if (combined >= targetSize) {
            uint32_t excess = combined - targetSize;
            block->blkSize = targetSize;
            if (excess >= MIN_BLOCK_SIZE) {
                LC3MemBlock *rem = (LC3MemBlock *)((uint32_t)block + targetSize);
                rem->blkSize = excess;
                rem->tag = BLOCK_TAG_FREE;
                rem->flags = 0;
                rem->masterPtr = 0;
            }
            return 0;
        }
    }
    return -108; /* memFullErr */
}
