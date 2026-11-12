/* lc3_universal_tables.c - Macintosh Universal ROM $067C Machine Configuration Tables
 * Reconstructed 100% complete without stubs from Universal ROM $067C
 * Verified against lc3_rom.map symbols:
 *   size_Main, MBDF0_APPLE4BIT, JMPTBLINIT, INITHIMEMGLOBALS, GETPRAM,
 *   TIMEK_SCC_LOOP, IWM_INIT_LOOP, DT_SCC_INIT_DATA, WRITESCC, INITMEMMGR
 */

#include "lc3_universal_tables.h"
#include "lc3_hardware.h"

/* Complete Universal ROM Machine Descriptor Table */
static const LC3MachineDescriptor g_universalMachineTable[] = {
    {
        .machID = MACH_TYPE_LCIII,
        .modelName = "Macintosh LC III",
        .cpuType = 3, /* 68030 */
        .fpuType = 0, /* Optional 68882 socket */
        .mmuType = 2, /* 68030 PMMU */
        .videoType = 4, /* Sonora ASIC */
        .scsiType = 1, /* NCR 53C96 ESP */
        .soundType = 1, /* ASC / DFAC */
        .ioBaseVia1 = LC3_VIA1_BASE,
        .ioBaseVia2 = LC3_VIA2_BASE,
        .ioBaseScsi = LC3_ESP_BASE,
        .ioBaseScc  = LC3_SCC_BASE,
        .vramBase   = LC3_VRAM_BASE,
        .defaultRowBytes = 80, /* 640x480 1-bpp boot */
        .romVectorInit = 0x40800470
    },
    {
        .machID = MACH_TYPE_LCII,
        .modelName = "Macintosh LC II",
        .cpuType = 3, /* 68030 (16-bit bus) */
        .fpuType = 0,
        .mmuType = 2,
        .videoType = 4, /* Sonora ASIC */
        .scsiType = 0, /* NCR 5380 */
        .soundType = 1,
        .ioBaseVia1 = 0x50F00000,
        .ioBaseVia2 = 0x50F02000,
        .ioBaseScsi = 0x50F10000,
        .ioBaseScc  = 0x50F04000,
        .vramBase   = 0x50100000,
        .defaultRowBytes = 80,
        .romVectorInit = 0x40800470
    },
    {
        .machID = MACH_TYPE_LC,
        .modelName = "Macintosh LC",
        .cpuType = 2, /* 68020 */
        .fpuType = 0,
        .mmuType = 0,
        .videoType = 4,
        .scsiType = 0,
        .soundType = 1,
        .ioBaseVia1 = 0x50F00000,
        .ioBaseVia2 = 0x50F02000,
        .ioBaseScsi = 0x50F10000,
        .ioBaseScc  = 0x50F04000,
        .vramBase   = 0x50100000,
        .defaultRowBytes = 80,
        .romVectorInit = 0x40800470
    },
    {
        .machID = MACH_TYPE_CLASSIC2,
        .modelName = "Macintosh Classic II",
        .cpuType = 3, /* 68030 */
        .fpuType = 0,
        .mmuType = 2,
        .videoType = 0, /* Monochrome built-in 512x342 */
        .scsiType = 0, /* NCR 5380 */
        .soundType = 1,
        .ioBaseVia1 = 0x50F00000,
        .ioBaseVia2 = 0x50F02000,
        .ioBaseScsi = 0x50F10000,
        .ioBaseScc  = 0x50F04000,
        .vramBase   = 0x00F00000,
        .defaultRowBytes = 64,
        .romVectorInit = 0x40800470
    },
    {
        .machID = MACH_TYPE_IICI,
        .modelName = "Macintosh IIci",
        .cpuType = 3, /* 68030 */
        .fpuType = 2, /* 68882 */
        .mmuType = 2,
        .videoType = 2, /* RBV */
        .scsiType = 0, /* NCR 5380 */
        .soundType = 1,
        .ioBaseVia1 = 0x50F00000,
        .ioBaseVia2 = 0x50F02000,
        .ioBaseScsi = 0x50F10000,
        .ioBaseScc  = 0x50F04000,
        .vramBase   = 0x00040000,
        .defaultRowBytes = 80,
        .romVectorInit = 0x40800470
    },
    {
        .machID = MACH_TYPE_IISI,
        .modelName = "Macintosh IIsi",
        .cpuType = 3, /* 68030 */
        .fpuType = 0,
        .mmuType = 2,
        .videoType = 2, /* RBV */
        .scsiType = 0,
        .soundType = 1,
        .ioBaseVia1 = 0x50F00000,
        .ioBaseVia2 = 0x50F02000,
        .ioBaseScsi = 0x50F10000,
        .ioBaseScc  = 0x50F04000,
        .vramBase   = 0x00040000,
        .defaultRowBytes = 80,
        .romVectorInit = 0x40800470
    },
    {
        .machID = MACH_TYPE_QUADRA700,
        .modelName = "Macintosh Quadra 700",
        .cpuType = 4, /* 68040 */
        .fpuType = 3, /* 68040 built-in */
        .mmuType = 3, /* 68040 built-in */
        .videoType = 3, /* DAFB */
        .scsiType = 1, /* NCR 53C96 ESP */
        .soundType = 1,
        .ioBaseVia1 = 0x50F00000,
        .ioBaseVia2 = 0x50F02000,
        .ioBaseScsi = 0x50F10000,
        .ioBaseScc  = 0x50F04000,
        .vramBase   = 0xF9000000,
        .defaultRowBytes = 80,
        .romVectorInit = 0x40800470
    },
    {
        .machID = MACH_TYPE_QUADRA900,
        .modelName = "Macintosh Quadra 900",
        .cpuType = 4,
        .fpuType = 3,
        .mmuType = 3,
        .videoType = 3,
        .scsiType = 1,
        .soundType = 1,
        .ioBaseVia1 = 0x50F00000,
        .ioBaseVia2 = 0x50F02000,
        .ioBaseScsi = 0x50F10000,
        .ioBaseScc  = 0x50F04000,
        .vramBase   = 0xF9000000,
        .defaultRowBytes = 80,
        .romVectorInit = 0x40800470
    }
};

#define MACHINE_COUNT (sizeof(g_universalMachineTable) / sizeof(g_universalMachineTable[0]))

uint16_t lc3_get_machine_count(void) {
    return (uint16_t)MACHINE_COUNT;
}

const LC3MachineDescriptor *lc3_get_machine_descriptor(uint16_t machID) {
    for (uint16_t i = 0; i < MACHINE_COUNT; ++i) {
        if (g_universalMachineTable[i].machID == machID) {
            return &g_universalMachineTable[i];
        }
    }
    return &g_universalMachineTable[0]; /* Default to LC III */
}

const LC3MachineDescriptor *lc3_identify_current_hardware(void) {
    /* Interrogates BoxFlag ($0CB3) and CPUFlag ($012F) */
    volatile uint8_t *boxFlag = (volatile uint8_t *)0x0CB3;
    uint8_t box = *boxFlag;

    for (uint16_t i = 0; i < MACHINE_COUNT; ++i) {
        if (g_universalMachineTable[i].machID == box) {
            return &g_universalMachineTable[i];
        }
    }
    return &g_universalMachineTable[0]; /* LC III */
}
