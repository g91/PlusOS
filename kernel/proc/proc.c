/* proc.c - PlusOS process table and scheduler.
 *
 * Cooperative round-robin with a timer tick for timekeeping and timed sleeps;
 * blocking syscalls yield, so the system stays responsive. Context switches
 * (switch.S) run only in supervisor mode. A real preemptive tick is a later
 * hardening step - the hook is tick_on_interrupt() -> want_resched.
 */
#include "kernel.h"
#include "proc.h"

extern void ctx_switch(uint32_t *old_ksp, uint32_t new_ksp);
extern void ctx_start_user(uint32_t usp, uint32_t entry);

static Proc  procs[MAX_PROC];
static Proc  idle_proc;                 /* runs when nothing else is ready */
static Proc *run_head, *run_tail;       /* ready queue (FIFO) */
Proc        *proc_current;
static int   next_pid = 1;
volatile int want_resched;              /* set by the tick, honoured at safe points */
volatile int sched_active;              /* 1 once sched_run() has started */
volatile int want_preempt;              /* irq.S: switch away from the user process now */
static int   slice;                     /* ticks the current process has run */
#define QUANTUM 3                       /* ~50 ms per turn when others are waiting */

/* ---- ready queue ---------------------------------------------------------- */
static void rq_push(Proc *p)
{
    p->next = 0;
    if (run_tail) run_tail->next = p; else run_head = p;
    run_tail = p;
    p->state = PROC_READY;
}

static Proc *rq_pop(void)
{
    Proc *p = run_head;
    if (p) { run_head = p->next; if (!run_head) run_tail = 0; }
    return p;
}

/* ---- the switch ----------------------------------------------------------- */
/* Pick the next runnable process and switch to it. Caller must be in a state
 * where leaving is safe (it has already queued itself if it wants to run on). */
/* USP is one register but each ring-2 process has its own user stack, so it
 * must be swapped on every context switch or a resumed user process runs with
 * the wrong user stack. (Supervisor-only instructions; resched is supervisor.) */
static inline uint32_t usp_get(void)
{
    uint32_t v;
    __asm__ volatile("move.l %%usp, %0" : "=a"(v));
    return v;
}
static inline void usp_set(uint32_t v)
{
    __asm__ volatile("move.l %0, %%usp" : : "a"(v));
}

static void resched(void)
{
    want_resched = 0;
    Proc *next = rq_pop();
    if (!next) next = &idle_proc;
    Proc *prev = proc_current;
    proc_current = next;
    next->state = PROC_RUNNING;
    slice = 0;                                  /* a fresh time slice */
    want_preempt = 0;
    if (next != prev) {
        if (prev->ring == RING_USER) prev->usp_save = usp_get();
        ctx_switch(&prev->ksp, next->ksp);
        /* regained control as prev (== proc_current now); restore our USP */
        if (prev->ring == RING_USER) usp_set(prev->usp_save);
    }
}

/* If the running process has been killed, end it. Called at safe points (every
 * blocking primitive, and on entry to a syscall) so a kill takes effect as soon
 * as the target next yields or asks the kernel for anything. */
void proc_check_killed(void)
{
    if (proc_current->killed && proc_current->ring == RING_USER)
        proc_exit(-9);
}

void proc_yield(void)
{
    uint16_t sr = int_disable();
    if (proc_current != &idle_proc)
        rq_push(proc_current);
    resched();
    int_restore(sr);
    proc_check_killed();
}

void proc_sleep(const void *chan)
{
    uint16_t sr = int_disable();
    proc_current->wait_chan = chan;
    proc_current->state = PROC_BLOCKED;
    resched();
    int_restore(sr);
    proc_check_killed();
}

void proc_wakeup(const void *chan)
{
    uint16_t sr = int_disable();
    for (int i = 0; i < MAX_PROC; i++)
        if (procs[i].state == PROC_BLOCKED && procs[i].wait_chan == chan) {
            procs[i].wait_chan = 0;
            procs[i].wake_tick = 0;
            rq_push(&procs[i]);
        }
    int_restore(sr);
}

void proc_sleep_ticks(int ticks)
{
    uint16_t sr = int_disable();
    proc_current->wake_tick = (int)tick_count() + (ticks < 1 ? 1 : ticks);
    proc_current->wait_chan = 0;
    proc_current->state = PROC_BLOCKED;
    resched();
    int_restore(sr);
    proc_check_killed();
}

/* Called from the tick: account CPU to the running process, wake timed
 * sleepers, flag preemption. */
void sched_on_tick(void)
{
    if (proc_current) proc_current->cpu_ticks++;
    int now = (int)tick_count();
    for (int i = 0; i < MAX_PROC; i++)
        if (procs[i].state == PROC_BLOCKED && procs[i].wake_tick && now - procs[i].wake_tick >= 0) {
            procs[i].wake_tick = 0;
            rq_push(&procs[i]);
        }
    want_resched = 1;
    /* Preemption: a user process that has used its slice while others are
     * ready gets switched out when the interrupt returns (irq.S). */
    if (proc_current && proc_current->ring == RING_USER && run_head && ++slice >= QUANTUM)
        want_preempt = 1;
}

/* Called from irq.S on the way back to an interrupted user program. */
void preempt_user(void)
{
    want_preempt = 0;
    if (proc_current && proc_current->ring == RING_USER) proc_yield();
}

void preempt_init(void)
{
    extern void irq1_entry(void);
    extern void *irq1_orig;
    uint16_t sr = int_disable();
    if (*(void **)0x64 != (void *)irq1_entry) {
        irq1_orig = *(void **)0x64;             /* the ROM's level-1 handler */
        *(void **)0x64 = (void *)irq1_entry;
    }
    int_restore(sr);
}

/* ---- creation ------------------------------------------------------------- */
static Proc *proc_alloc(void)
{
    for (int i = 0; i < MAX_PROC; i++)
        if (procs[i].state == PROC_UNUSED) {
            memset(&procs[i], 0, sizeof(Proc));
            procs[i].pid = next_pid++;
            return &procs[i];
        }
    return 0;
}

/* First thing a brand-new task runs (reached by the initial ctx_switch rts). */
static void proc_launch(void)
{
    int_enable();                       /* switches start with interrupts off */
    Proc *p = proc_current;
    if (p->ring == RING_USER)
        ctx_start_user(p->user_sp, p->user_entry);   /* drop to ring 2; returns via exit */
    else
        p->entry(p->arg);               /* ring-0/1 task body */
    proc_exit(0);                       /* body returned: exit */
}

/* Prime a kernel stack so the first ctx_switch "returns" into proc_launch. */
static void prime_stack(Proc *p)
{
    uint32_t *sp = (uint32_t *)(p->kstack + KSTACK_SIZE);
    *--sp = (uint32_t)proc_launch;      /* ctx_switch's rts target */
    for (int i = 0; i < 11; i++) *--sp = 0;   /* d2-d7/a2-a6 placeholders */
    /* a5 (slot 9: d2..d7 = 0..5, a2..a6 = 6..10) must point at the QuickDraw
     * globals for every process - the ROM Toolbox finds its globals via A5. */
    sp[9] = LM_LONG(0x904);             /* CurrentA5 */
    p->ksp = (uint32_t)sp;
}

int proc_create_kernel(const char *name, void (*fn)(void *), void *arg)
{
    uint16_t sr = int_disable();
    Proc *p = proc_alloc();
    if (!p) { int_restore(sr); return -1; }
    p->kstack = kmalloc(KSTACK_SIZE);
    if (!p->kstack) { p->state = PROC_UNUSED; int_restore(sr); return -1; }
    p->ring = RING_KERNEL;
    p->entry = fn;
    p->arg = arg;
    p->parent = proc_current ? proc_current->pid : 0;
    p->mem_bytes = KSTACK_SIZE;
    strlcpy(p->name, name, PROC_NAME);
    prime_stack(p);
    rq_push(p);
    int_restore(sr);
    return p->pid;
}

int proc_create_user(const char *name, uint8_t *image, uint32_t entry_off,
                     int argc, char **argv)
{
    uint16_t sr = int_disable();
    Proc *p = proc_alloc();
    if (!p) { int_restore(sr); return -1; }
    p->kstack = kmalloc(KSTACK_SIZE);
    p->ustack = kmalloc(USTACK_SIZE);
    if (!p->kstack || !p->ustack) {
        kfree(p->kstack); kfree(p->ustack);
        p->state = PROC_UNUSED; int_restore(sr); return -1;
    }
    p->ring = RING_USER;
    p->image = image;
    p->user_entry = (uint32_t)image + entry_off;
    p->parent = proc_current ? proc_current->pid : 0;
    p->con_win = proc_current ? proc_current->con_win : 0;   /* children share the console */
    p->mem_bytes = KSTACK_SIZE + USTACK_SIZE;        /* + image size, added by exec.c */
    strlcpy(p->name, name, PROC_NAME);

    /* Build the user stack: argv strings, the argv array, then (api,argv,argc)
     * for the app crt0 (int entry(argc, argv, api)). */
    uint8_t *utop = p->ustack + USTACK_SIZE;
    uint32_t uaddr = (uint32_t)utop;
    uint32_t uargv[MAX_ARGS + 1];
    if (argc > MAX_ARGS) argc = MAX_ARGS;
    for (int i = argc - 1; i >= 0; i--) {
        uint32_t len = (uint32_t)strlen(argv[i]) + 1;
        uaddr -= len;
        memcpy((void *)uaddr, argv[i], len);
        uargv[i] = uaddr;
    }
    uaddr &= ~1u;
    uargv[argc] = 0;
    uaddr -= (uint32_t)(argc + 1) * 4;
    uint32_t uargv_ptr = uaddr;
    memcpy((void *)uaddr, uargv, (uint32_t)(argc + 1) * 4);
    uaddr &= ~3u;
    /* crt0 is entered like a called function int entry(argc, argv, api): it
     * reads argc/argv/api just above a return-address slot. Exit is via syscall,
     * so the return address is never used (0). */
    uaddr -= 16;
    ((uint32_t *)uaddr)[0] = 0;                  /* return address (unused) */
    ((uint32_t *)uaddr)[1] = (uint32_t)argc;
    ((uint32_t *)uaddr)[2] = uargv_ptr;
    ((uint32_t *)uaddr)[3] = (uint32_t)&g_api;
    p->user_sp = uaddr;

    prime_stack(p);
    rq_push(p);
    int_restore(sr);
    return p->pid;
}

int proc_create_thread(uint32_t entry, uint32_t fn, uint32_t arg, uint32_t stack_bytes)
{
    if (stack_bytes < 1024) stack_bytes = 4096;
    if (stack_bytes > 32768) stack_bytes = 32768;
    stack_bytes &= ~3u;
    Proc *me = proc_current;
    if (me->ring != RING_USER) return -1;
    uint16_t sr = int_disable();
    Proc *p = proc_alloc();
    if (!p) { int_restore(sr); return -1; }
    p->kstack = kmalloc(KSTACK_SIZE);
    p->ustack = kmalloc(stack_bytes);
    if (!p->kstack || !p->ustack) {
        kfree(p->kstack); kfree(p->ustack);
        p->state = PROC_UNUSED; int_restore(sr); return -1;
    }
    p->ring = RING_USER;
    p->image = 0;                               /* shares the program's code */
    p->user_entry = entry;
    p->parent = me->pid;
    p->thread_of = me->thread_of ? me->thread_of : me->pid;
    p->con_win = me->con_win;
    p->mem_bytes = KSTACK_SIZE + stack_bytes;
    strlcpy(p->name, me->name, PROC_NAME - 2);      /* "desktop+" = a thread of desktop */
    {   int n = (int)strlen(p->name); if (n < PROC_NAME - 2) { p->name[n] = '+'; p->name[n + 1] = 0; } }
    /* entered like a C call entry(fn, arg), return address never used */
    uint32_t u = ((uint32_t)p->ustack + stack_bytes) & ~3u;
    u -= 12;
    ((uint32_t *)u)[0] = 0;
    ((uint32_t *)u)[1] = fn;
    ((uint32_t *)u)[2] = arg;
    p->user_sp = u;
    prime_stack(p);
    rq_push(p);
    int_restore(sr);
    return p->pid;
}

int proc_detach(int pid)
{
    for (int i = 0; i < MAX_PROC; i++)
        if (procs[i].pid == pid && procs[i].state != PROC_UNUSED) { procs[i].detached = 1; return 0; }
    return -1;
}

static volatile int zombies_pending;             /* idle reaps orphaned/detached zombies */

/* ---- exit / wait ---------------------------------------------------------- */
__attribute__((noreturn)) void proc_exit(int code)
{
    if (proc_current->ring == RING_USER) {
        if (!proc_current->thread_of)                /* a program ends: so do its threads */
            for (int i = 0; i < MAX_PROC; i++)
                if (procs[i].thread_of == proc_current->pid &&
                    procs[i].state != PROC_UNUSED && procs[i].state != PROC_ZOMBIE)
                    proc_kill(procs[i].pid);
        winsrv_proc_exit(proc_current->pid);
    }
    zombies_pending = 1;
    uint16_t sr = int_disable();
    Proc *p = proc_current;
    p->exit_code = code;
    p->state = PROC_ZOMBIE;
    proc_wakeup(p);                     /* wake anyone in proc_wait on us */
    resched();                          /* never returns */
    int_restore(sr);
    for (;;) {}
}

int proc_wait(int pid)
{
    for (;;) {
        uint16_t sr = int_disable();
        Proc *target = 0;
        for (int i = 0; i < MAX_PROC; i++)
            if (procs[i].pid == pid && procs[i].state != PROC_UNUSED) target = &procs[i];
        if (!target) { int_restore(sr); return -1; }
        if (target->state == PROC_ZOMBIE) {
            int code = target->exit_code;
            kfree(target->kstack);
            kfree(target->ustack);
            kfree(target->image);
            target->state = PROC_UNUSED;
            int_restore(sr);
            return code;
        }
        int_restore(sr);
        proc_sleep(target);
    }
}

/* Reap any finished child of `parent` without blocking. Returns its exit code
 * and sets *pid_out, or returns 0 with *pid_out = 0 when none has finished. */
int proc_reap_child(int parent, int *pid_out)
{
    uint16_t sr = int_disable();
    for (int i = 0; i < MAX_PROC; i++)
        if (procs[i].state == PROC_ZOMBIE && procs[i].parent == parent) {
            int code = procs[i].exit_code;
            if (pid_out) *pid_out = procs[i].pid;
            kfree(procs[i].kstack);
            kfree(procs[i].ustack);
            kfree(procs[i].image);
            procs[i].state = PROC_UNUSED;
            int_restore(sr);
            return code;
        }
    int_restore(sr);
    if (pid_out) *pid_out = 0;
    return 0;
}

/* Ask a process to exit. It really exits the next time it blocks or makes a
 * syscall (cooperative kill). The idle, init and heartbeat tasks are protected. */
int proc_kill(int pid)
{
    uint16_t sr = int_disable();
    for (int i = 0; i < MAX_PROC; i++)
        if (procs[i].pid == pid && procs[i].state != PROC_UNUSED) {
            if (procs[i].ring != RING_USER) { int_restore(sr); return -1; }  /* no killing kernel tasks */
            procs[i].killed = 1;
            if (procs[i].state == PROC_BLOCKED) {        /* unblock so it runs and dies */
                procs[i].wait_chan = 0;
                procs[i].wake_tick = 0;
                rq_push(&procs[i]);
            }
            int_restore(sr);
            return 0;
        }
    int_restore(sr);
    return -1;
}

/* ---- idle + startup ------------------------------------------------------- */
static int alive(int pid)
{
    if (pid <= 0) return 0;
    for (int i = 0; i < MAX_PROC; i++)
        if (procs[i].pid == pid) return procs[i].state != PROC_UNUSED && procs[i].state != PROC_ZOMBIE;
    return 0;
}

/* Free zombies nobody will wait for: detached threads, and anything whose
 * parent has ended. Runs on the idle (boot) stack, never on the zombie's own. */
static void reap_orphans(void)
{
    zombies_pending = 0;
    uint16_t sr = int_disable();
    for (int i = 0; i < MAX_PROC; i++) {
        Proc *p = &procs[i];
        if (p->state != PROC_ZOMBIE || (!p->detached && alive(p->parent))) continue;
        kfree(p->kstack); kfree(p->ustack); kfree(p->image);
        p->state = PROC_UNUSED;
    }
    int_restore(sr);
}

static void idle_body(void)
{
    for (;;) {
        /* Idle MUST run with interrupts enabled, or the VBL tick never fires to
         * wake timed sleepers and the machine deadlocks. Force the mask low
         * every iteration regardless of how we got back here. */
        int_enable();
        if (zombies_pending) reap_orphans();
        if (run_head) {
            uint16_t sr = int_disable();
            resched();
            int_restore(sr);
        }
    }
}

int proc_count(void)
{
    int n = 0;
    for (int i = 0; i < MAX_PROC; i++)
        if (procs[i].state != PROC_UNUSED) n++;
    return n;
}

Proc *proc_table_entry(int i) { return &procs[i]; }

void sched_init(void)
{
    memset(procs, 0, sizeof procs);
    memset(&idle_proc, 0, sizeof idle_proc);
    idle_proc.state = PROC_RUNNING;
    idle_proc.ring = RING_KERNEL;
    idle_proc.pid = 0;
    strlcpy(idle_proc.name, "idle", PROC_NAME);
    proc_current = &idle_proc;
}

/* Begin scheduling. The caller becomes the idle process. */
void sched_run(void)
{
    sched_active = 1;
    idle_body();
    __builtin_unreachable();
}
