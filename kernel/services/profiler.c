/* profiler.c - PlusOS System Performance Profiler & Metrics Subsystem */
#include "kernel.h"
#include "proc.h"
#include "profiler.h"

static uint32_t ctx_switch_count = 0;
static uint32_t syscall_total_count = 0;
static uint32_t interrupt_total_ticks = 0;
static uint32_t mem_allocs_count = 0;
static uint32_t mem_frees_count = 0;

static uint32_t load_1m = 0; /* 16.16 fixed point */
static uint32_t load_5m = 0;

void profiler_init(void)
{
    ctx_switch_count = 0;
    syscall_total_count = 0;
    interrupt_total_ticks = 0;
    mem_allocs_count = 0;
    mem_frees_count = 0;
    load_1m = 0;
    load_5m = 0;
}

void profiler_record_switch(void)
{
    ctx_switch_count++;
}

void profiler_record_syscall(void)
{
    syscall_total_count++;
}

void profiler_record_alloc(void)
{
    mem_allocs_count++;
}

void profiler_record_free(void)
{
    mem_frees_count++;
}

void profiler_vbl_tick(void)
{
    interrupt_total_ticks++;
    /* Update exponential moving average for load average every 60 ticks (1 sec) */
    if ((interrupt_total_ticks % 60) == 0) {
        int active = 0;
        for (int i = 0; i < MAX_PROC; i++) {
            Proc *p = proc_table_entry(i);
            if (p && (p->state == PROC_RUNNING || p->state == PROC_READY)) active++;
        }
        uint32_t active_fp = ((uint32_t)active) << 16;
        /* load_1m = load_1m * 0.983 + active * 0.016 */
        load_1m = ((load_1m * 60) + (active_fp * 1)) / 61;
        load_5m = ((load_5m * 300) + (active_fp * 1)) / 301;
    }
}

void profiler_get_metrics(struct plusos_perf_metrics *m)
{
    if (!m) return;
    m->context_switches = ctx_switch_count;
    m->syscall_count = syscall_total_count;
    m->interrupt_ticks = interrupt_total_ticks;
    m->mem_alloc_count = mem_allocs_count;
    m->mem_free_count = mem_frees_count;
    m->load_1m_fixed = load_1m;
    m->load_5m_fixed = load_5m;
}
