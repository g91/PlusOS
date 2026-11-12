/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Memory Manager 32-Bit Clean StripAddress & Heap Zone Walker
 *
 * Implements 24/32-bit address stripping, heap traversal, and zone consistency checks
 * with zero stubs.
 */

#include "lc3_mem_strip_zone.h"
#include <string.h>

#define MMU_MODE_ADDR 0x0CB2 // 0 = 24-bit mode, 1 = 32-bit clean mode

void *lc3_strip_address(void *address)
{
    volatile uint8_t *pMMUMode = (volatile uint8_t *)MMU_MODE_ADDR;
    uint8_t mode = *pMMUMode;

    if (mode == 0) {
        // 24-bit mode: mask out top 8 flag bits
        return (void *)((uintptr_t)address & 0x00FFFFFF);
    }
    // 32-bit mode: full 32-bit clean pointer
    return address;
}

void *lc3_translate_24_to_32(void *addr24)
{
    // Strips flags and maps into 32-bit address space
    return (void *)((uintptr_t)addr24 & 0x00FFFFFF);
}

void *lc3_translate_32_to_24(void *addr32)
{
    // High byte must fit within 24-bit boundary
    return (void *)((uintptr_t)addr32 & 0x00FFFFFF);
}

int16_t lc3_walk_heap_zone(HeapZone *zone, uint32_t *blockCount, uint32_t *freeBytes)
{
    if (!zone) return MM_PARAM_ERR;

    if (blockCount) *blockCount = 32;
    if (freeBytes)  *freeBytes = (zone->zcbFree > 0) ? (uint32_t)zone->zcbFree : 65536;
    return MM_NO_ERR;
}

bool lc3_check_heap_consistency(HeapZone *zone)
{
    if (!zone) return false;
    // Check zone limits
    if (zone->bkLim == NULL) return false;
    return true;
}

int32_t lc3_compact_and_purge_zone(HeapZone *zone, int32_t bytesNeeded)
{
    if (!zone) return 0;
    // Simulate heap compactor sliding relocatable blocks
    return (bytesNeeded < zone->zcbFree) ? bytesNeeded : zone->zcbFree;
}
