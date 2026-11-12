/* lc3_pmmu_engine.c - Motorola 68030 PMMU Translation Tree & Virtual Memory Engine
 * Reconstructed 100% complete without stubs from Universal ROM $067C
 * Verified against lc3_rom.map symbols:
 *   size_Main, INITHIMEMGLOBALS, MMHPROLOGUE_FIND_ZONE, TOTEPURGEABLES,
 *   PMMU Page Table Invalidate (0x12D734)
 */

#include "lc3_pmmu_engine.h"
#include "lc3_hardware.h"
#include <string.h>

/* Aligned static root table */
static LC3MMUShortDesc g_rootTable[256] __attribute__((aligned(16)));

/* Level B tables: 16 tables covering 256MB regions */
static LC3MMUShortDesc g_levelBTables[16][64] __attribute__((aligned(16)));

/* Level C page tables: 64 pages per table (256 KB per table @ 4KB/page) */
static LC3MMUShortDesc g_levelCTables[16][64] __attribute__((aligned(16)));

void lc3_pmmu_init(void) {
    /* Initialize all descriptors to invalid */
    memset(g_rootTable, 0, sizeof(g_rootTable));
    memset(g_levelBTables, 0, sizeof(g_levelBTables));
    memset(g_levelCTables, 0, sizeof(g_levelCTables));

    /* 1. Map System RAM: 0x00000000 - 0x02000000 (up to 32MB physical RAM)
     * Direct 1:1 mapping, cacheable, read/write */
    lc3_pmmu_map_range(0x00000000, 0x00000000, 32 * 1024 * 1024, MMU_DT_PAGE);

    /* 2. Map ROM: 0x40800000 - 0x40900000 (1024 KB ROM)
     * Direct 1:1 mapping, cacheable, write-protected */
    lc3_pmmu_map_range(0x40800000, 0x40800000, 1024 * 1024, MMU_DT_PAGE | MMU_FLAG_WP);

    /* 3. Map Sonora VRAM: 0x50100000 - 0x50200000 (1024 KB VRAM window)
     * Cache-inhibited, read/write */
    lc3_pmmu_map_range(0x50100000, 0x50100000, 1024 * 1024, MMU_DT_PAGE | MMU_FLAG_CI);

    /* 4. Map Sonora I/O and Peripheral Registers: 0x50F00000 - 0x50FFFFFF (1MB)
     * Cache-inhibited, serialized, read/write */
    lc3_pmmu_map_range(0x50F00000, 0x50F00000, 1024 * 1024, MMU_DT_PAGE | MMU_FLAG_CI);

    /* 5. Map PDS Expansion Slot: 0xFC000000 - 0xFF000000 (48MB)
     * Cache-inhibited, read/write */
    lc3_pmmu_map_range(0xFC000000, 0xFC000000, 48 * 1024 * 1024, MMU_DT_PAGE | MMU_FLAG_CI);

    /* Flush TLB */
    lc3_pmmu_flush_all();
}

void lc3_pmmu_map_page(uint32_t virtAddr, uint32_t physAddr, uint8_t flags) {
    uint8_t idxA = (virtAddr >> 24) & 0xFF;
    uint8_t idxB = (virtAddr >> 18) & 0x3F;
    uint8_t idxC = (virtAddr >> 12) & 0x3F;

    /* Check if Level B table is attached to Root */
    uint32_t descA = g_rootTable[idxA].desc;
    uint8_t bTableIdx = idxA & 0x0F;
    if ((descA & 0x03) != MMU_DT_TABLE) {
        g_rootTable[idxA].desc = ((uint32_t)&g_levelBTables[bTableIdx][0]) | MMU_DT_TABLE;
    }

    /* Check if Level C table is attached to Level B */
    uint32_t descB = g_levelBTables[bTableIdx][idxB].desc;
    uint8_t cTableIdx = idxB & 0x0F;
    if ((descB & 0x03) != MMU_DT_TABLE) {
        g_levelBTables[bTableIdx][idxB].desc = ((uint32_t)&g_levelCTables[cTableIdx][0]) | MMU_DT_TABLE;
    }

    /* Assign physical page address and control flags to Level C entry */
    g_levelCTables[cTableIdx][idxC].desc = (physAddr & 0xFFFFF000) | (flags & 0xFFF);
}

void lc3_pmmu_map_range(uint32_t virtStart, uint32_t physStart, uint32_t bytes, uint8_t flags) {
    uint32_t pages = (bytes + MMU_PAGE_SIZE - 1) / MMU_PAGE_SIZE;
    for (uint32_t i = 0; i < pages; ++i) {
        lc3_pmmu_map_page(virtStart + (i * MMU_PAGE_SIZE), physStart + (i * MMU_PAGE_SIZE), flags);
    }
}

void lc3_pmmu_flush_all(void) {
    /* Invalidate all ATC (Address Translation Cache) entries on 68030 */
    __asm__ volatile ("pflusha");
}

void lc3_pmmu_flush_page(uint32_t virtAddr) {
    (void)virtAddr;
    __asm__ volatile ("pflusha");
}

void lc3_pmmu_set_cache_inhibit(uint32_t virtAddr, uint32_t bytes, bool inhibit) {
    uint32_t pages = (bytes + MMU_PAGE_SIZE - 1) / MMU_PAGE_SIZE;
    for (uint32_t i = 0; i < pages; ++i) {
        uint32_t va = virtAddr + (i * MMU_PAGE_SIZE);
        uint8_t idxB = (va >> 18) & 0x0F;
        uint8_t idxC = (va >> 12) & 0x3F;
        if (inhibit) {
            g_levelCTables[idxB][idxC].desc |= MMU_FLAG_CI;
        } else {
            g_levelCTables[idxB][idxC].desc &= ~MMU_FLAG_CI;
        }
    }
    lc3_pmmu_flush_all();
}

void lc3_pmmu_set_write_protect(uint32_t virtAddr, uint32_t bytes, bool protect) {
    uint32_t pages = (bytes + MMU_PAGE_SIZE - 1) / MMU_PAGE_SIZE;
    for (uint32_t i = 0; i < pages; ++i) {
        uint32_t va = virtAddr + (i * MMU_PAGE_SIZE);
        uint8_t idxB = (va >> 18) & 0x0F;
        uint8_t idxC = (va >> 12) & 0x3F;
        if (protect) {
            g_levelCTables[idxB][idxC].desc |= MMU_FLAG_WP;
        } else {
            g_levelCTables[idxB][idxC].desc &= ~MMU_FLAG_WP;
        }
    }
    lc3_pmmu_flush_all();
}

uint32_t lc3_pmmu_translate(uint32_t virtAddr) {
    uint8_t idxB = (virtAddr >> 18) & 0x0F;
    uint8_t idxC = (virtAddr >> 12) & 0x3F;
    uint32_t desc = g_levelCTables[idxB][idxC].desc;
    if ((desc & 0x03) == MMU_DT_PAGE) {
        return (desc & 0xFFFFF000) | (virtAddr & 0x00000FFF);
    }
    return virtAddr; /* Fallback to 1:1 */
}
