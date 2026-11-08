/* kmem.h - PlusOS dynamic RAM management, slab allocator, and heap sentinel */
#ifndef PLUSOS_KMEM_H
#define PLUSOS_KMEM_H

#include <stdint.h>
#include <stddef.h>
#include <plusos/syscall.h>

#define KMEM_MAGIC_HEAD 0x504C5553  /* "PLUS" */
#define KMEM_MAGIC_TAIL 0x444F4E45  /* "DONE" */

typedef struct KMemChunk {
    uint32_t magic;
    uint32_t size;
    uint32_t raw_size;
    int16_t  owner_pid;
    uint8_t  slab_idx;
    uint8_t  flags;
    struct KMemChunk *next;
    struct KMemChunk *prev;
} KMemChunk;

void  kmem_init(void);
void *kmem_alloc(uint32_t size, int pid);
void  kmem_free(void *p);
int   kmem_free_pid(int pid);
int   kmem_check(void);
void  kmem_stats(struct plusos_ram_stats *st);
void  kmem_compact(void);

#endif
