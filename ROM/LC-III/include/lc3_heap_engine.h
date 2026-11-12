/* lc3_heap_engine.h - Macintosh LC III 32-bit Heap Compactor & Master Pointer Engine
 * Fully reverse-engineered from Universal ROM $067C (lc3_rom.map)
 * Offsets 0x00D882 - 0x00EA1E
 */
#ifndef LC3_HEAP_ENGINE_H
#define LC3_HEAP_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

/* Block Header Tag bits */
#define BLOCK_TAG_FREE       0x00
#define BLOCK_TAG_NONRELOC   0x40
#define BLOCK_TAG_RELOC      0x80

/* Zone Header Record (32-bit clean) */
typedef struct LC3Zone {
    uint32_t bkLim;             /* zone end address */
    uint32_t purgePtr;          /* address of last purged block */
    uint32_t hFstFree;          /* first free block in free list */
    uint32_t zcbFree;           /* total contiguous free bytes in zone */
    uint32_t gzProc;            /* grow zone callback */
    uint16_t moreMast;          /* number of master pointers to allocate per block */
    uint16_t flags;             /* zone flags */
    uint32_t cntRel;            /* number of relocatable blocks */
    uint32_t maxRel;            /* max relocatable block size */
    uint32_t cntNRel;           /* number of non-relocatable blocks */
    uint32_t maxNRel;           /* max non-relocatable block size */
    uint32_t cntEmpty;          /* count of unallocated master pointers */
    uint32_t purgeAlloc;        /* total bytes allocated from purgeables */
    uint32_t unused[4];
} LC3Zone;

/* 32-bit Memory Block Header */
typedef struct LC3MemBlock {
    uint32_t blkSize;           /* physical byte size including header */
    uint8_t  tag;               /* BLOCK_TAG_FREE, NONRELOC, or RELOC */
    uint8_t  flags;             /* lock, purgeable, resource flags */
    uint16_t reserved;
    uint32_t masterPtr;         /* back-pointer to master pointer handle */
} LC3MemBlock;

/* Heap Compaction & Allocation API */
void        lc3_heap_init_zone(LC3Zone *zone, uint32_t startAddr, uint32_t sizeBytes, uint16_t moreMasters);
LC3MemBlock *lc3_heap_bfinds(LC3Zone *zone, uint32_t reqSize);
LC3MemBlock *lc3_heap_alloc_block(LC3Zone *zone, uint32_t reqSize, uint8_t tag);
uint32_t    lc3_heap_compact(LC3Zone *zone, uint32_t reqSize);
uint32_t    lc3_heap_purge(LC3Zone *zone, uint32_t reqSize);
uint32_t    lc3_heap_tote_purgeables(LC3Zone *zone);
bool        lc3_heap_make_more_masters(LC3Zone *zone);
uint32_t    lc3_heap_adjust_free(LC3Zone *zone);
void        lc3_heap_release_master_ptr(LC3Zone *zone, void **handle);
int16_t     lc3_heap_set_block_size(LC3Zone *zone, LC3MemBlock *block, uint32_t newSize);

#endif /* LC3_HEAP_ENGINE_H */
