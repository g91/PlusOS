/* syscall.c - TRAP #1 dispatch and the service-table trampolines. */
#include "kernel.h"
#include "proc.h"
#include "syscall.h"
#include "kmem.h"
#include "gc.h"
#include "pipe.h"
#include "sync.h"
#include "signal.h"
#include "timer.h"
#include "klog.h"
#include "environ.h"
#include "ramdisk.h"
#include "shm.h"
#include "power.h"
#include "pram.h"
#include "entropy.h"
#include "profiler.h"
#include "security.h"

extern void trap1_entry(void);

long syscall_dispatch(long num, void *arg)
{
    kmod_last_syscall = (int)num;                    /* for the crash report */
    profiler_record_syscall();
    if (num != SYS_exit) proc_check_killed();        /* a killed process dies here */
    switch (num) {
    case SYS_exit:
        proc_exit((int)(intptr_t)arg);              /* no return */
        return 0;
    case SYS_yield:
        proc_yield();
        return 0;
    case SYS_sleep:
        proc_sleep_ticks((int)(intptr_t)arg);
        return 0;
    case SYS_getpid:
        return proc_current->pid;
    case SYS_spawn: {
        struct plusos_spawn_args *s = arg;
        return exec_spawn(s->name, s->argc, s->argv);
    }
    case SYS_wait:
        return proc_wait((int)(intptr_t)arg);
    case SYS_getkey: {                              /* a terminal window, or the keyboard */
        int k;
        if (term_getkey(1, &k)) return k;
        return kbd_get();
    }
    case SYS_pollkey: {
        int k;
        if (term_getkey(0, &k)) return k;
        return kbd_poll();
    }
    case SYS_readline: {
        struct plusos_readline_args *r = arg;
        int n;
        if (term_readline(r->buf, r->size, &n)) return n;
        return kbd_line(r->buf, r->size);
    }
    case SYS_thread: {
        struct plusos_thread_args *t = arg;
        return proc_create_thread((uint32_t)t->entry, (uint32_t)t->fn, (uint32_t)t->arg,
                                  (uint32_t)t->stack_bytes);
    }
    case SYS_detach:
        return proc_detach((int)(intptr_t)arg);
    case SYS_win:
        return winsrv_call(arg);
    case SYS_termspawn: {
        struct plusos_spawn_args *s = arg;
        return winsrv_termspawn(s->name, s->argc, s->argv);
    }
    case SYS_reboot:
        sys_reboot();                               /* no return */
        return 0;
    case SYS_shutdown:
        sys_shutdown();                             /* no return */
        return 0;
    case SYS_malloc:
        return (long)kmem_alloc((uint32_t)(intptr_t)arg, proc_current ? proc_current->pid : 0);
    case SYS_free:
        kmem_free(arg);
        return 0;
    case SYS_memfree:
        return (long)FreeMem();
    case SYS_blkio: {
        struct plusos_blkio_args *b = arg;
        return b->write ? blk_write(b->block, b->count, b->buf) : blk_read(b->block, b->count, b->buf);
    }
    case SYS_chainboot:
        if (!machine_is_se()) return PLUSOS_EBOOT_NOTSE;
        return chainboot((int)(intptr_t)arg);       /* restarts on success */
    case SYS_getcfg:
        return cfg_get((int)(intptr_t)arg);
    case SYS_setcfg: {
        struct plusos_cfg_args *c = arg;
        return cfg_set(c->key, c->value);
    }
    case SYS_sysinfo: {
        struct plusos_sysinfo *s = arg;
        memset(s, 0, sizeof *s);
        strlcpy(s->machine, plusrom_machine_name(), sizeof s->machine);
        strlcpy(s->cpu, cpu_name(), sizeof s->cpu);
        s->ram_bytes = ram_bytes();
        s->free_bytes = (unsigned long)FreeMem();
        s->ticks = tick_count();
        s->nprocs = proc_count();
        s->is_se = machine_is_se();
        s->startup_id = s->is_se ? chainboot_get_default() : -1;
        strlcpy(s->volume, g_volname, sizeof s->volume);
        return 0;
    }
    case SYS_kill:
        return proc_kill((int)(intptr_t)arg);
    case SYS_reapany: {
        struct plusos_reap *r = arg;
        return proc_reap_child(proc_current->pid, &r->pid);
    }
    case SYS_proclist: {
        struct plusos_proclist_args *pl = arg;
        int n = 0;
        for (int i = 0; i < 16 && n < pl->max; i++) {
            Proc *p = proc_table_entry(i);
            if (p->state == 0) continue;
            struct plusos_procinfo *d = &pl->buf[n++];
            d->pid = p->pid; d->parent = p->parent; d->ring = p->ring; d->state = p->state;
            d->cpu_ticks = p->cpu_ticks; d->mem_bytes = p->mem_bytes;
            d->priority = p->priority; d->pgid = p->pgid;
            strlcpy(d->name, p->name, sizeof d->name);
        }
        return n;
    }
    case SYS_regget: {
        struct plusos_reg_args *r = arg;
        return reg_get(r->key, r->vbuf, r->vsize);
    }
    case SYS_regset: {
        struct plusos_reg_args *r = arg;
        return reg_set(r->key, r->value);
    }
    case SYS_regenum: {
        struct plusos_reg_args *r = arg;
        return reg_enum(r->index, r->kbuf, r->ksize, r->vbuf, r->vsize);
    }
    case SYS_console:
        return console_op(arg);
    case SYS_history: {
        struct plusos_history_args *h = arg;
        if (!h) return -1;
        switch (h->op) {
        case PLUSOS_HIST_COUNT:
            return kbd_history_count();
        case PLUSOS_HIST_GET: {
            const char *s = kbd_history_get(h->index);
            if (!s || !h->buf || h->size <= 0) return -1;
            strlcpy(h->buf, s, (size_t)h->size);
            return 0;
        }
        case PLUSOS_HIST_ADD:
            if (h->buf) kbd_history_add(h->buf);
            return 0;
        case PLUSOS_HIST_CLEAR:
            kbd_history_clear();
            return 0;
        }
        return -1;
    }
    case SYS_sound:
        return sound_syscall(arg);
    case SYS_scrap:
        return scrap_syscall(arg);

    /* --- New Systems & Features --- */
    case SYS_ramstats:
        kmem_stats((struct plusos_ram_stats *)arg);
        return 0;
    case SYS_gc:
        return gc_scavenge((struct plusos_gc_args *)arg);
    case SYS_heapcheck:
        return kmem_check();
    case SYS_pipecreate: {
        struct plusos_pipe_args *p = arg;
        return pipe_create(&p->read_fd, &p->write_fd, proc_current->pid);
    }
    case SYS_piperead: {
        struct plusos_pipe_io *p = arg;
        return pipe_read(p->pipe_id, p->buf, p->count, p->nonblock);
    }
    case SYS_pipewrite: {
        struct plusos_pipe_io *p = arg;
        return pipe_write(p->pipe_id, p->buf, p->count, p->nonblock);
    }
    case SYS_pipeclose:
        return pipe_close((int)(intptr_t)arg);
    case SYS_seminit: {
        struct plusos_sem_args *s = arg;
        return sem_create(s->init_val, s->max_val, proc_current->pid);
    }
    case SYS_semwait:
        return sem_wait((int)(intptr_t)arg);
    case SYS_sempost:
        return sem_post((int)(intptr_t)arg);
    case SYS_semdestroy:
        return sem_destroy((int)(intptr_t)arg);
    case SYS_mutexinit:
        return mutex_create(proc_current->pid);
    case SYS_mutexlock:
        return mutex_lock((int)(intptr_t)arg);
    case SYS_mutexunlock:
        return mutex_unlock((int)(intptr_t)arg);
    case SYS_signal: {
        struct plusos_sig_args *s = arg;
        return signal_send(s->pid, s->signum);
    }
    case SYS_alarm:
        return timer_alarm((int)(intptr_t)arg, proc_current->pid);
    case SYS_timer: {
        struct plusos_timer_args *t = arg;
        if (t->op == PLUSOS_TIMER_SET)
            return timer_set(t->delay_ticks, t->interval_ticks, t->signum, proc_current->pid);
        return timer_cancel(t->timer_id, proc_current->pid);
    }
    case SYS_klog: {
        struct plusos_klog_args *k = arg;
        klog_write(k->level, k->msg, k->len);
        return k->len;
    }
    case SYS_dmesg: {
        struct plusos_dmesg_args *d = arg;
        return klog_read(d->buf, d->max_len, d->offset);
    }
    case SYS_getenv: {
        struct plusos_env_args *e = arg;
        return env_get(e->key, e->value, e->val_size);
    }
    case SYS_setenv: {
        struct plusos_env_args *e = arg;
        if (e->op == PLUSOS_ENV_UNSET) return env_unset(e->key);
        if (e->op == PLUSOS_ENV_LIST) return env_list(e->value, e->val_size);
        return env_set(e->key, e->value);
    }
    case SYS_ramdisk: {
        struct plusos_ramdisk_args *r = arg;
        if (r->op == PLUSOS_RAMDISK_READ) return ramdisk_read(r->block, r->count, r->buf);
        if (r->op == PLUSOS_RAMDISK_WRITE) return ramdisk_write(r->block, r->count, r->buf);
        if (r->op == PLUSOS_RAMDISK_INFO) { ramdisk_status(&r->total_blocks, &r->used_blocks); return 0; }
        if (r->op == PLUSOS_RAMDISK_CLEAR) { ramdisk_clear(); return 0; }
        return -1;
    }
    case SYS_shm: {
        struct plusos_shm_args *s = arg;
        if (s->op == PLUSOS_SHM_GET) return shm_get(s->key, s->size, proc_current->pid);
        if (s->op == PLUSOS_SHM_AT) { s->addr = shm_at(s->shmid); return s->addr ? 0 : -1; }
        if (s->op == PLUSOS_SHM_DT) return shm_dt(s->shmid);
        return -1;
    }
    case SYS_power:
        return power_status((struct plusos_power_args *)arg);
    case SYS_pram:
        return pram_dispatch((struct plusos_pram_args *)arg);
    case SYS_getrandom: {
        struct plusos_rand_args *r = arg;
        return entropy_get_bytes(r->buf, r->len);
    }
    case SYS_perfmetrics:
        profiler_get_metrics((struct plusos_perf_metrics *)arg);
        return 0;
    case SYS_priority: {
        struct plusos_prio_args *p = arg;
        if (p->op == PLUSOS_PRIO_GET) return proc_get_priority(p->pid ? p->pid : proc_current->pid);
        return proc_set_priority(p->pid ? p->pid : proc_current->pid, p->priority);
    }
    case SYS_watchdog: {
        struct plusos_watchdog_args *w = arg;
        (void)w;
        return 0;
    }
    case SYS_procpgid: {
        struct plusos_pgid_args *p = arg;
        if (p->op == PLUSOS_PGID_GET) return proc_get_pgid(p->pid ? p->pid : proc_current->pid);
        return proc_set_pgid(p->pid ? p->pid : proc_current->pid, p->pgid);
    }
    default:
        return -1;
    }
}

void syscall_init(void)
{
    *(void **)0x84 = (void *)trap1_entry;           /* TRAP #1 = exception vector 33 */
    /* A-line gate: ROM traps from ring 2 run on the caller's stack (aline.S) */
    extern void aline_entry(void);
    extern void *aline_orig;
    if (*(void **)0x28 != (void *)aline_entry) {
        aline_orig = *(void **)0x28;
        *(void **)0x28 = (void *)aline_entry;
    }
}

/* ---- trampolines (user-safe: they only issue TRAP #1) ---- */
int ktrap_getkey(void)                            { return (int)plusos_syscall(SYS_getkey, 0); }
int ktrap_readline(char *buf, int size)
{
    struct plusos_readline_args a = { buf, size };
    return (int)plusos_syscall(SYS_readline, &a);
}
int ktrap_spawn(const char *name, int argc, char **argv)
{
    struct plusos_spawn_args a = { name, argc, argv };
    return (int)plusos_syscall(SYS_spawn, &a);
}
__attribute__((noreturn)) void ktrap_reboot(void)   { plusos_syscall(SYS_reboot, 0); for (;;) {} }
__attribute__((noreturn)) void ktrap_shutdown(void) { plusos_syscall(SYS_shutdown, 0); for (;;) {} }
void *ktrap_malloc(uint32_t n) { return (void *)plusos_syscall(SYS_malloc, (void *)(intptr_t)n); }
void  ktrap_free(void *p)      { plusos_syscall(SYS_free, p); }
uint32_t ktrap_memfree(void)   { return (uint32_t)plusos_syscall(SYS_memfree, 0); }
static int ktrap_blkio(int w, uint32_t blk, uint32_t n, void *buf)
{
    struct plusos_blkio_args a = { w, blk, n, buf };
    return (int)plusos_syscall(SYS_blkio, &a);
}
/* ---- console: every program's text output goes through here, in the kernel
 * (supervisor, never preempted), so two programs printing at once can't
 * tangle the console's state. ---- */
long console_op(void *arg)
{
    struct plusos_con_args *c = arg;
    long r;
    if (term_console_op(c, &r)) return r;           /* the process's console is a window */
    switch (c->op) {
    case PLUSOS_CON_GETSIZE: if (c->pa) *c->pa = CON_COLS; if (c->pb) *c->pb = CON_ROWS - 1; return 0;
    case PLUSOS_CON_PUTC:   con_putc(c->a); return 0;
    case PLUSOS_CON_WRITE:  con_write(c->s, (size_t)c->n); return 0;
    case PLUSOS_CON_CLEAR:  con_clear(); return 0;
    case PLUSOS_CON_GOTO:   con_goto(c->a, c->b); return 0;
    case PLUSOS_CON_GETPOS: con_getpos(c->pa, c->pb); return 0;
    case PLUSOS_CON_INVERT: con_set_invert(c->a); return 0;
    case PLUSOS_CON_CURSOR: con_set_cursor(c->a); return 0;
    }
    return -1;
}

static void kcon(int op, int a, int b, const char *s, int n, int *pa, int *pb)
{
    struct plusos_con_args c = { op, a, b, s, n, pa, pb };
    plusos_syscall(SYS_console, &c);
}
void ktrap_con_putc(int ch)                { kcon(PLUSOS_CON_PUTC, ch, 0, 0, 0, 0, 0); }
void ktrap_con_write(const char *s, size_t n) { kcon(PLUSOS_CON_WRITE, 0, 0, s, (int)n, 0, 0); }
void ktrap_con_clear(void)                 { kcon(PLUSOS_CON_CLEAR, 0, 0, 0, 0, 0, 0); }
void ktrap_con_goto(int c, int r)          { kcon(PLUSOS_CON_GOTO, c, r, 0, 0, 0, 0); }
void ktrap_con_getpos(int *c, int *r)      { kcon(PLUSOS_CON_GETPOS, 0, 0, 0, 0, c, r); }
void ktrap_con_invert(int on)              { kcon(PLUSOS_CON_INVERT, on, 0, 0, 0, 0, 0); }
void ktrap_con_cursor(int on)              { kcon(PLUSOS_CON_CURSOR, on, 0, 0, 0, 0, 0); }
int  ktrap_pollkey(void)                   { return (int)plusos_syscall(SYS_pollkey, 0); }
void ktrap_sleep(int ticks)                { plusos_syscall(SYS_sleep, (void *)(intptr_t)ticks); }

int ktrap_blk_read(uint32_t b, uint32_t n, void *buf)        { return ktrap_blkio(0, b, n, buf); }
int ktrap_blk_write(uint32_t b, uint32_t n, const void *buf) { return ktrap_blkio(1, b, n, (void *)buf); }
