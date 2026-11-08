/* kmem.c - Dynamic RAM management, slab allocator, and heap sentinel for PlusOS. */
#include "kernel.h"
#include "kmem.h"
#include "gc.h"
#include "proc.h"

#define SLAB_CLASSES 5
static const uint16_t slab_sizes[SLAB_CLASSES] = { 32, 64, 128, 256, 512 };
#define SLAB_COUNTS_0 16
#define SLAB_COUNTS_1 16
#define SLAB_COUNTS_2 8
#define SLAB_COUNTS_3 4
#define SLAB_COUNTS_4 4

static uint8_t slab_buf_0[SLAB_COUNTS_0 * 32];
static uint8_t slab_buf_1[SLAB_COUNTS_1 * 64];
static uint8_t slab_buf_2[SLAB_COUNTS_2 * 128];
static uint8_t slab_buf_3[SLAB_COUNTS_3 * 256];
static uint8_t slab_buf_4[SLAB_COUNTS_4 * 512];

static uint32_t slab_bitmap[SLAB_CLASSES];

static KMemChunk *chunk_head = 0;
static uint32_t total_alloc_bytes = 0;
static uint32_t active_chunks_count = 0;
static uint32_t corruption_count = 0;
uint32_t g_gc_reclaimed_total = 0;

void kmem_init(void)
{
    chunk_head = 0;
    total_alloc_bytes = 0;
    active_chunks_count = 0;
    corruption_count = 0;
    for (int i = 0; i < SLAB_CLASSES; i++) slab_bitmap[i] = 0;
}

static int slab_pick_class(uint32_t size)
{
    for (int i = 0; i < SLAB_CLASSES; i++) {
        if (size <= slab_sizes[i]) return i;
    }
    return -1;
}

static void *slab_alloc(int cls, int pid)
{
    (void)pid;
    int max_count = 0;
    uint8_t *buf = 0;
    switch (cls) {
    case 0: max_count = SLAB_COUNTS_0; buf = slab_buf_0; break;
    case 1: max_count = SLAB_COUNTS_1; buf = slab_buf_1; break;
    case 2: max_count = SLAB_COUNTS_2; buf = slab_buf_2; break;
    case 3: max_count = SLAB_COUNTS_3; buf = slab_buf_3; break;
    case 4: max_count = SLAB_COUNTS_4; buf = slab_buf_4; break;
    default: return 0;
    }

    uint32_t bm = slab_bitmap[cls];
    for (int bit = 0; bit < max_count; bit++) {
        if (!(bm & (1UL << bit))) {
            slab_bitmap[cls] |= (1UL << bit);
            return buf + (bit * slab_sizes[cls]);
        }
    }
    return 0;
}

static int slab_free(void *p)
{
    uint8_t *addr = (uint8_t *)p;
    if (addr >= slab_buf_0 && addr < slab_buf_0 + sizeof(slab_buf_0)) {
        int idx = (int)(addr - slab_buf_0) / 32;
        slab_bitmap[0] &= ~(1UL << idx);
        return 1;
    }
    if (addr >= slab_buf_1 && addr < slab_buf_1 + sizeof(slab_buf_1)) {
        int idx = (int)(addr - slab_buf_1) / 64;
        slab_bitmap[1] &= ~(1UL << idx);
        return 1;
    }
    if (addr >= slab_buf_2 && addr < slab_buf_2 + sizeof(slab_buf_2)) {
        int idx = (int)(addr - slab_buf_2) / 128;
        slab_bitmap[2] &= ~(1UL << idx);
        return 1;
    }
    if (addr >= slab_buf_3 && addr < slab_buf_3 + sizeof(slab_buf_3)) {
        int idx = (int)(addr - slab_buf_3) / 256;
        slab_bitmap[3] &= ~(1UL << idx);
        return 1;
    }
    if (addr >= slab_buf_4 && addr < slab_buf_4 + sizeof(slab_buf_4)) {
        int idx = (int)(addr - slab_buf_4) / 512;
        slab_bitmap[4] &= ~(1UL << idx);
        return 1;
    }
    return 0;
}

void *kmem_alloc(uint32_t size, int pid)
{
    if (size == 0) return 0;

    int cls = slab_pick_class(size);
    if (cls >= 0) {
        void *sp = slab_alloc(cls, pid);
        if (sp) return sp;
    }

    /* Direct ROM heap allocation with header and canary */
    uint32_t aligned_size = (size + 3) & ~3u;
    uint32_t total_needed = sizeof(KMemChunk) + aligned_size + sizeof(uint32_t);

    Ptr raw = NewPtr((Size)total_needed);
    if (!raw) {
        /* Out of memory or fragmented heap: run GC and heap compaction, then retry */
        kmem_compact();
        raw = NewPtr((Size)total_needed);
        if (!raw) return 0;
    }

    KMemChunk *chunk = (KMemChunk *)raw;
    chunk->magic = KMEM_MAGIC_HEAD;
    chunk->size = size;
    chunk->raw_size = total_needed;
    chunk->owner_pid = (int16_t)pid;
    chunk->slab_idx = 0;
    chunk->flags = 1;

    uint8_t *payload = (uint8_t *)(chunk + 1);
    uint32_t *tail_canary = (uint32_t *)(payload + aligned_size);
    *tail_canary = KMEM_MAGIC_TAIL;

    /* Insert into active chunks list */
    uint16_t sr = int_disable();
    chunk->prev = 0;
    chunk->next = chunk_head;
    if (chunk_head) chunk_head->prev = chunk;
    chunk_head = chunk;
    active_chunks_count++;
    total_alloc_bytes += size;
    int_restore(sr);

    return (void *)payload;
}

void kmem_free(void *p)
{
    if (!p) return;

    if (slab_free(p)) return;

    KMemChunk *chunk = ((KMemChunk *)p) - 1;
    if (chunk->magic != KMEM_MAGIC_HEAD) {
        DisposePtr((Ptr)p);
        return;
    }

    uint32_t aligned_size = (chunk->size + 3) & ~3u;
    uint32_t *tail_canary = (uint32_t *)((uint8_t *)p + aligned_size);
    if (*tail_canary != KMEM_MAGIC_TAIL) {
        corruption_count++;
    }

    uint16_t sr = int_disable();
    if (chunk->prev) chunk->prev->next = chunk->next;
    else chunk_head = chunk->next;
    if (chunk->next) chunk->next->prev = chunk->prev;

    if (total_alloc_bytes >= chunk->size) total_alloc_bytes -= chunk->size;
    if (active_chunks_count > 0) active_chunks_count--;
    int_restore(sr);

    chunk->magic = 0;
    DisposePtr((Ptr)chunk);
}

int kmem_free_pid(int pid)
{
    if (pid <= 0) return 0;
    int freed_count = 0;
    uint16_t sr = int_disable();
    KMemChunk *curr = chunk_head;
    while (curr) {
        KMemChunk *nxt = curr->next;
        if (curr->owner_pid == pid && curr->magic == KMEM_MAGIC_HEAD) {
            uint32_t sz = curr->size;
            if (curr->prev) curr->prev->next = curr->next;
            else chunk_head = curr->next;
            if (curr->next) curr->next->prev = curr->prev;

            if (total_alloc_bytes >= sz) total_alloc_bytes -= sz;
            if (active_chunks_count > 0) active_chunks_count--;
            g_gc_reclaimed_total += sz;
            curr->magic = 0;
            DisposePtr((Ptr)curr);
            freed_count++;
        }
        curr = nxt;
    }
    int_restore(sr);
    return freed_count;
}

int kmem_check(void)
{
    int corrupt = 0;
    uint16_t sr = int_disable();
    KMemChunk *curr = chunk_head;
    while (curr) {
        if (curr->magic != KMEM_MAGIC_HEAD) {
            corrupt++;
            break;
        }
        uint32_t aligned_size = (curr->size + 3) & ~3u;
        uint32_t *tail_canary = (uint32_t *)((uint8_t *)(curr + 1) + aligned_size);
        if (*tail_canary != KMEM_MAGIC_TAIL) {
            corrupt++;
        }
        curr = curr->next;
    }
    int_restore(sr);
    return corrupt + corruption_count;
}

void kmem_compact(void)
{
    if (g_flags & PLUSOS_HAVE_HEAP) {
        CompactMem(0x10000);
        PurgeMem(0x10000);
    }
}

void kmem_stats(struct plusos_ram_stats *st)
{
    if (!st) return;
    st->total_ram = ram_bytes();
    st->free_ram = (uint32_t)FreeMem();
    st->kernel_load_addr = __plusos_load_addr;
    st->kernel_stack_size = KERNEL_STACK;
    st->heap_alloc_bytes = total_alloc_bytes;
    st->heap_chunks_active = active_chunks_count;
    
    uint32_t slab_used = 0;
    uint32_t slab_free = 0;
    for (int i = 0; i < SLAB_CLASSES; i++) {
        uint32_t bm = slab_bitmap[i];
        int max_c = (i == 0 || i == 1) ? 16 : (i == 2 ? 8 : 4);
        for (int b = 0; b < max_c; b++) {
            if (bm & (1UL << b)) slab_used += slab_sizes[i];
            else slab_free++;
        }
    }
    st->slab_alloc_bytes = slab_used;
    st->slab_free_slots = slab_free;
    st->largest_free_block = (uint32_t)MaxBlock();
    st->gc_reclaimed_total = g_gc_reclaimed_total;
    st->corruption_detected = kmem_check();
}
