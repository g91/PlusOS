/* exe.h - the PlusOS executable format (.pex)
 *
 * Every PlusOS binary (the kernel and every app) is linked at address 0 and
 * shipped with a relocation table, so it can be loaded anywhere in RAM. Its
 * start-up code (crt0) patches itself for the address it was loaded at.
 *
 *   offset  size  field
 *   0x00    4     bra.w to the start-up code
 *   0x04    4     magic 'PLOS'
 *   0x08    2     format version (1)
 *   0x0A    2     kind: 1 = kernel, 2 = app
 *   0x0C    4     file_size     bytes stored on disk (image + relocation table)
 *   0x10    4     mem_size      bytes to allocate (max(file_size, image + BSS))
 *   0x14    4     reloc_offset  where the relocation table starts (= image size)
 *   0x18    4     reloc_count   number of 32-bit words to fix up
 *   0x1C    4     stack_size    stack the program wants (kernel: its own stack)
 *   0x20    4     reserved
 *   0x24   12     name (NUL padded)
 *   0x30          code ...
 *   [reloc_offset]: reloc_count x u32 image offsets; each long there gets the
 *                   load address added.
 */
#ifndef PLUSOS_EXE_H
#define PLUSOS_EXE_H
#include <stdint.h>

#define PEX_MAGIC        0x504C4F53UL   /* 'PLOS' */
#define PEX_FORMAT       1
#define PEX_KIND_KERNEL  1
#define PEX_KIND_APP     2
#define PEX_HEADER_SIZE  0x30

typedef struct PexHeader {
    uint16_t bra_op;          /* 0x6000 */
    uint16_t bra_disp;
    uint32_t magic;
    uint16_t format;
    uint16_t kind;
    uint32_t file_size;
    uint32_t mem_size;
    uint32_t reloc_offset;
    uint32_t reloc_count;
    uint32_t stack_size;
    uint32_t reserved;
    char     name[12];
} PexHeader;

#endif
