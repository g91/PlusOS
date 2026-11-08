/* profiler.h - PlusOS System Performance Profiler & Metrics Subsystem */
#ifndef PLUSOS_PROFILER_H
#define PLUSOS_PROFILER_H

#include <stdint.h>
#include <plusos/syscall.h>

void profiler_init(void);
void profiler_record_switch(void);
void profiler_record_syscall(void);
void profiler_record_alloc(void);
void profiler_record_free(void);
void profiler_vbl_tick(void);
void profiler_get_metrics(struct plusos_perf_metrics *m);

#endif
