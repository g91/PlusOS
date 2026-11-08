/* gc.h - PlusOS Kernel Garbage Collector & Resource Scavenger */
#ifndef PLUSOS_GC_H
#define PLUSOS_GC_H

#include <stdint.h>
#include <plusos/syscall.h>

extern uint32_t g_gc_reclaimed_total;

void gc_init(void);
int  gc_scavenge(struct plusos_gc_args *args);
void gc_on_idle(void);
void gc_proc_exit(int pid);

#endif
