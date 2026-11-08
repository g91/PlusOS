/* gc.c - PlusOS Kernel Garbage Collector & Resource Scavenger */
#include "kernel.h"
#include "gc.h"
#include "kmem.h"
#include "proc.h"

extern void pipe_gc(int *closed_pipes);
extern void sync_gc(int *freed_sync);

static uint32_t last_idle_gc = 0;

void gc_init(void)
{
    last_idle_gc = 0;
}

int gc_scavenge(struct plusos_gc_args *args)
{
    uint32_t initial_reclaimed = g_gc_reclaimed_total;
    int chunks = 0;
    int zombies = 0;
    int pipes = 0;
    int syncs = 0;

    /* 1. Free any allocations owned by non-existent processes */
    for (int pid = 1; pid < 256; pid++) {
        int found = 0;
        for (int i = 0; i < MAX_PROC; i++) {
            Proc *p = proc_table_entry(i);
            if (p && p->pid == pid && p->state != PROC_UNUSED && p->state != PROC_ZOMBIE) {
                found = 1;
                break;
            }
        }
        if (!found) {
            chunks += kmem_free_pid(pid);
        }
    }

    /* 2. Collect orphaned/closed pipes and sync objects */
    pipe_gc(&pipes);
    sync_gc(&syncs);

    /* 3. Reap detached/orphaned zombies */
    for (int i = 0; i < MAX_PROC; i++) {
        Proc *p = proc_table_entry(i);
        if (p && p->state == PROC_ZOMBIE) {
            int parent_alive = 0;
            if (p->parent > 0) {
                for (int j = 0; j < MAX_PROC; j++) {
                    Proc *par = proc_table_entry(j);
                    if (par && par->pid == p->parent && par->state != PROC_UNUSED && par->state != PROC_ZOMBIE) {
                        parent_alive = 1;
                        break;
                    }
                }
            }
            if (!parent_alive || p->detached) {
                kfree(p->kstack);
                kfree(p->ustack);
                kfree(p->image);
                p->state = PROC_UNUSED;
                zombies++;
            }
        }
    }

    /* 4. Compact the ROM heap if requested */
    if (args && args->compact_heap) {
        kmem_compact();
    }

    uint32_t bytes_freed = g_gc_reclaimed_total - initial_reclaimed;
    if (args) {
        args->bytes_reclaimed = bytes_freed;
        args->chunks_freed = chunks;
        args->zombies_reaped = zombies;
        args->pipes_closed = pipes;
    }

    return (int)bytes_freed;
}

void gc_on_idle(void)
{
    uint32_t now = tick_count();
    /* Run a light GC pass every 10 seconds (600 ticks) when idle */
    if (now - last_idle_gc >= 600) {
        last_idle_gc = now;
        struct plusos_gc_args a = { 0, 0, 0, 0, 0 };
        gc_scavenge(&a);
    }
}

void gc_proc_exit(int pid)
{
    kmem_free_pid(pid);
}
