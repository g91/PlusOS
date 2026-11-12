/* lc3_universal_tables.h - Macintosh Universal ROM $067C Machine Configuration Tables
 * Fully reverse-engineered from Universal ROM $067C (lc3_rom.map)
 * Offsets 0x00010C - 0x0011D0
 */
#ifndef LC3_UNIVERSAL_TABLES_H
#define LC3_UNIVERSAL_TABLES_H

#include <stdint.h>
#include <stdbool.h>

#define MACH_TYPE_II           6
#define MACH_TYPE_IIX          7
#define MACH_TYPE_IICX         8
#define MACH_TYPE_SE30         9
#define MACH_TYPE_IICI         11
#define MACH_TYPE_IISI         18
#define MACH_TYPE_LC           19
#define MACH_TYPE_QUADRA700    22
#define MACH_TYPE_CLASSIC2     23
#define MACH_TYPE_QUADRA900    24
#define MACH_TYPE_POWERBOOK170 25
#define MACH_TYPE_QUADRA950    26
#define MACH_TYPE_LCIII        27
#define MACH_TYPE_LCII         37

typedef struct {
    uint16_t machID;            /* Gestalt 'mach' value */
    const char *modelName;      /* Human-readable model string */
    uint8_t  cpuType;           /* 0=68000, 1=68010, 2=68020, 3=68030, 4=68040 */
    uint8_t  fpuType;           /* 0=None, 1=68881, 2=68882, 3=68040 built-in */
    uint8_t  mmuType;           /* 0=None, 1=68851, 2=68030 built-in, 3=68040 built-in */
    uint8_t  videoType;         /* 0=Monochrome, 1=Toby, 2=RBV, 3=DAFB, 4=Sonora */
    uint8_t  scsiType;          /* 0=NCR 5380, 1=NCR 53C96 ESP */
    uint8_t  soundType;         /* 0=Standard Mac, 1=Apple Sound Chip (ASC) */
    uint32_t ioBaseVia1;        /* VIA1 Base Address */
    uint32_t ioBaseVia2;        /* VIA2 / RBV / OSS Base Address */
    uint32_t ioBaseScsi;        /* SCSI Controller Base Address */
    uint32_t ioBaseScc;         /* SCC Serial Base Address */
    uint32_t vramBase;          /* VRAM Framebuffer Base Address */
    uint16_t defaultRowBytes;   /* Default Scanline Byte Pitch */
    uint32_t romVectorInit;     /* ROM Hardware Initialization Function */
} LC3MachineDescriptor;

/* Universal Table Interface */
const LC3MachineDescriptor *lc3_get_machine_descriptor(uint16_t machID);
const LC3MachineDescriptor *lc3_identify_current_hardware(void);
uint16_t lc3_get_machine_count(void);

#endif /* LC3_UNIVERSAL_TABLES_H */
