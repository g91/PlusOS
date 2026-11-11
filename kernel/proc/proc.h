/* proc.h - PlusOS processes and the cooperative/preemptive scheduler. */
#ifndef PLUSOS_PROC_H
#define PLUSOS_PROC_H
#include <stdint.h>

#define MAX_PROC      16
#define PROC_NAME     16
#define KSTACK_SIZE   16384         /* per-process supervisor (kernel) stack */

enum proc_state {
    PROC_UNUSED = 0,
    PROC_READY,                     /* on the run queue */
    PROC_RUNNING,
    PROC_BLOCKED,                   /* waiting on a channel (see proc_sleep) */
    PROC_ZOMBIE,                    /* exited, waiting to be reaped */
};

enum proc_ring {
    RING_KERNEL = 0,                /* supervisor task (ring 0/1) */
    RING_USER = 2,                  /* user-mode application (ring 2) */
};

typedef struct Proc {
    uint32_t        ksp;            /* saved supervisor stack pointer (when not running) */
    uint8_t        *kstack;         /* base of the kernel stack (for free) */
    uint8_t        *ustack;         /* base of the user stack, 0 for kernel tasks */
    uint8_t        *image;          /* loaded program image, freed on exit (0 if none) */
    enum proc_state state;
    enum proc_ring  ring;
    int             pid;
    int             parent;         /* pid of the process that spawned this one */
    volatile int    killed;         /* set by SYS_kill; the process exits at a safe point */
    uint32_t        cpu_ticks;      /* ticks for which this process was the running one */
    uint32_t        mem_bytes;      /* kernel+user stack + image, for the task manager */
    int             con_win;        /* console: 0 = screen, >0 = terminal window id,
                                       -2 = terminal not opened yet, -1 = gone */
    int             thread_of;      /* 0 for a process; else the pid of the program it belongs to */
    int             detached;       /* reap automatically when it ends (nobody will join) */
    int             exit_code;
    const void     *wait_chan;      /* what a BLOCKED process is waiting for */
    int             wake_tick;      /* for timed sleeps (0 = none) */
    struct Proc    *next;           /* run-queue / free-list link */
    void          (*entry)(void *); /* kernel task body (first run) */
    void           *arg;            /* argument to entry, or user entry address */
    uint32_t        user_entry;     /* ring 2: entry PC in user mode */
    uint32_t        user_sp;        /* ring 2: initial user stack pointer */
    uint32_t        usp_save;       /* ring 2: USP saved across context switches */
    int             priority;       /* -20 (high) to +19 (low), default 0 */
    int             pgid;           /* process group id */
    uint32_t        sig_pending;    /* bitmask of pending signals */
    uint32_t        sig_mask;       /* bitmask of blocked signals */
    uint32_t        heap_bytes;     /* RAM allocated dynamically via kmem */
    uint16_t        uid, gid;       /* Phase 2: user accounts (0 = root) */
    char            name[PROC_NAME];
} Proc;

extern Proc *proc_current;

void  sched_init(void);
/* Create a ring-0 kernel task that runs fn(arg). Returns its pid, or -1. */
int   proc_create_kernel(const char *name, void (*fn)(void *), void *arg);
/* Create a ring-2 user process whose code image is at `image` (owned by the
 * process, freed on exit), entry at image+entry_off, with argc/argv copied in. */
int   proc_create_user(const char *name, uint8_t *image, uint32_t entry_off,
                       int argc, char **argv);

int   proc_get_priority(int pid);
int   proc_set_priority(int pid, int prio);
int   proc_get_pgid(int pid);
int   proc_set_pgid(int pid, int pgid);

void  sched_run(void) __attribute__((noreturn));   /* start scheduling; never returns */
void  proc_yield(void);                             /* give up the CPU, stay ready */
void  proc_sleep(const void *chan);                 /* block until proc_wakeup(chan) */
void  proc_wakeup(const void *chan);                /* wake everything sleeping on chan */
void  proc_sleep_ticks(int ticks);                  /* block for ~ticks/60 s */
__attribute__((noreturn)) void proc_exit(int code); /* current process exits */
int   proc_wait(int pid);                           /* wait for pid to exit; its code */

int   proc_count(void);
Proc *proc_table_entry(int i);                      /* i in [0,MAX_PROC); for `ps` */
int   proc_kill(int pid);                           /* ask pid to exit; 0 or <0 */
int   proc_reap_child(int parent, int *pid_out);    /* reap any finished child; code or 0 */
void  proc_check_killed(void);                      /* exits the current proc if killed */
void  preempt_init(void);                           /* time-slice user processes (irq.S) */
/* A thread of the current program: runs entry(fn, arg) in user mode on its own
 * stacks, sharing the program's memory. Returns its id (a pid), or -1. */
int   proc_create_thread(uint32_t entry, uint32_t fn, uint32_t arg, uint32_t stack_bytes);
int   proc_detach(int pid);

/* winsrv.c: the window server */
struct plusos_win_args;
struct plusos_con_args;
long  winsrv_call(struct plusos_win_args *a);
int   winsrv_termspawn(const char *name, int argc, char **argv);
void  winsrv_proc_exit(int pid);
int   term_console_op(struct plusos_con_args *c, long *result);
int   term_getkey(int wait, int *key);
int   term_readline(char *buf, int size, int *len);

/* tick.c: the preemption/time source (VBL). */
void      tick_init(void);
uint32_t  tick_count(void);       /* 60 Hz ticks since boot */
void      tick_on_interrupt(void);/* called from the VBL task */

#endif
