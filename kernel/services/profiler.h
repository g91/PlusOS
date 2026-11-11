/* profiler.h - PlusOS System Performance Profiler & Metrics Subsystem */
#ifndef PLUSOS_PROFILER_H
#define PLUSOS_PROFILER_H

#include <stdint.h>
#include <plusos/syscall.h>
#include "kconfig.h"

#if CONFIG_PROFILER
void profiler_init(void);
void profiler_record_switch(void);
void profiler_record_syscall(void);
void profiler_record_alloc(void);
void profiler_record_free(void);
void profiler_vbl_tick(void);
void profiler_get_metrics(struct plusos_perf_metrics *m);
#else   /* CONFIG_PROFILER=0: hooks vanish from the scheduler/syscall/tick hot paths */
static inline void profiler_init(void) {}
static inline void profiler_record_switch(void) {}
static inline void profiler_record_syscall(void) {}
static inline void profiler_record_alloc(void) {}
static inline void profiler_record_free(void) {}
static inline void profiler_vbl_tick(void) {}
static inline void profiler_get_metrics(struct plusos_perf_metrics *m) { if (m) *m = (struct plusos_perf_metrics){0}; }
#endif

#endif