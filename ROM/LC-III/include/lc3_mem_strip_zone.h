/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Memory Manager 32-Bit Clean StripAddress & Heap Zone Walker
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_MEM_STRIP_ZONE_H
#define LC3_MEM_STRIP_ZONE_H

#include <stdint.h>
#include <stdbool.h>

#define MM_NO_ERR         0
#define MM_CORRUPT_ZONE -109
#define MM_PARAM_ERR     -50

typedef struct BlockHeader {
    uint32_t tagAndSize; // High 8 bits: tag/flags; lower 24 bits: size
    void **handle;       // Pointer to master pointer if relocatable
} BlockHeader;

typedef struct HeapZone {
    void *bkLim;
    void *purgePtr;
    void *hFstFree;
    int32_t zcbFree;
    void *gzProc;
    int16_t moreMast;
    int16_t flags;
} HeapZone;

/* Memory Manager Extended API */
void *lc3_strip_address(void *address);
void *lc3_translate_24_to_32(void *addr24);
void *lc3_translate_32_to_24(void *addr32);
int16_t lc3_walk_heap_zone(HeapZone *zone, uint32_t *blockCount, uint32_t *freeBytes);
bool lc3_check_heap_consistency(HeapZone *zone);
int32_t lc3_compact_and_purge_zone(HeapZone *zone, int32_t bytesNeeded);

#endif /* LC3_MEM_STRIP_ZONE_H */
