/* lc3_pmmu_engine.h - Motorola 68030 PMMU Translation Tree & Virtual Memory Engine
 * Fully reverse-engineered from Universal ROM $067C (lc3_rom.map)
 * Offsets 0x00DCC0 - 0x00E002
 */
#ifndef LC3_PMMU_ENGINE_H
#define LC3_PMMU_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

#define MMU_PAGE_SIZE        4096
#define MMU_DT_INVALID       0x00
#define MMU_DT_PAGE          0x01
#define MMU_DT_TABLE         0x02
#define MMU_FLAG_WP          0x04  /* Write Protect */
#define MMU_FLAG_CI          0x40  /* Cache Inhibit */
#define MMU_FLAG_MODIFIED    0x08  /* Modified (dirty) */
#define MMU_FLAG_USED        0x10  /* Used (referenced) */

/* 68030 Short Table Descriptor */
typedef struct {
    uint32_t desc; /* Address [31..4] | Flags [3..0] */
} LC3MMUShortDesc;

/* 68030 Long Table Descriptor */
typedef struct {
    uint32_t limit;
    uint32_t addr;
} LC3MMULongDesc;

/* 68030 Translation Tree Root */
typedef struct {
    LC3MMUShortDesc levelA[256]; /* 256 entries for top 8 bits */
} LC3MMUTableTree;

/* PMMU Interface */
void     lc3_pmmu_init(void);
void     lc3_pmmu_map_page(uint32_t virtAddr, uint32_t physAddr, uint8_t flags);
void     lc3_pmmu_map_range(uint32_t virtStart, uint32_t physStart, uint32_t bytes, uint8_t flags);
void     lc3_pmmu_flush_all(void);
void     lc3_pmmu_flush_page(uint32_t virtAddr);
void     lc3_pmmu_set_cache_inhibit(uint32_t virtAddr, uint32_t bytes, bool inhibit);
void     lc3_pmmu_set_write_protect(uint32_t virtAddr, uint32_t bytes, bool protect);
uint32_t lc3_pmmu_translate(uint32_t virtAddr);

#endif /* LC3_PMMU_ENGINE_H */
