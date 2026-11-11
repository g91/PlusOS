/* syscall.h - the PlusOS system-call ABI, shared by the kernel and userland.
 *
 * Userland enters the kernel with TRAP #1: D0 = call number, A0 = argument
 * (a scalar or a pointer to an argument block), result in D0. Every other
 * register is preserved. TRAP is not privileged, so it is the safe way for a
 * ring-2 app to ask the kernel for anything that needs supervisor mode
 * (scheduling, process control, blocking input, memory, disks, power).
 * ROM Toolbox calls (A-line traps) may be made directly: the kernel's A-line
 * gate runs them on the caller's own stack and returns to user mode.
 */
#ifndef PLUSOS_SYSCALL_H
#define PLUSOS_SYSCALL_H
#include <stdint.h>

enum {
    SYS_exit = 1,      /* a0 = exit code                 -> no return */
    SYS_yield,         /*                                -> 0 */
    SYS_sleep,         /* a0 = ticks                     -> 0 */
    SYS_getpid,        /*                                -> pid */
    SYS_spawn,         /* a0 = &{const char*name;int argc;char**argv} -> pid or <0 */
    SYS_wait,          /* a0 = pid                       -> exit code or <0 */
    SYS_getkey,        /* blocking: next key             -> key */
    SYS_pollkey,       /* non-blocking: key or 0         -> key */
    SYS_readline,      /* a0 = &{char*buf;int size}      -> length */
    SYS_reboot,        /*                                -> no return */
    SYS_shutdown,      /*                                -> no return */
    SYS_malloc,        /* a0 = size                      -> pointer (ROM heap) */
    SYS_free,          /* a0 = pointer                   -> 0 */
    SYS_memfree,       /*                                -> free bytes in the ROM heap */
    SYS_blkio,         /* a0 = plusos_blkio_args*        -> 0 or OSErr */
    SYS_chainboot,     /* a0 = SCSI id                   -> no return, or <0 error */
    SYS_getcfg,        /* a0 = PLUSOS_CFG_* key          -> value */
    SYS_setcfg,        /* a0 = plusos_cfg_args*          -> 0 or <0 (saved to disk) */
    SYS_sysinfo,       /* a0 = plusos_sysinfo*           -> 0 */
    SYS_kill,          /* a0 = pid                       -> 0 or <0 */
    SYS_reapany,       /* a0 = plusos_reap*              -> code; pid in *a0 (0 = none) */
    SYS_proclist,      /* a0 = plusos_proclist_args*     -> number of processes */
    SYS_regget,        /* a0 = plusos_reg_args* (key -> vbuf)        -> length or -1 */
    SYS_regset,        /* a0 = plusos_reg_args* (key, value; "" deletes) -> 0 or -1 */
    SYS_regenum,       /* a0 = plusos_reg_args* (index -> kbuf, vbuf) -> 0, or -1 past the end */
    SYS_console,       /* a0 = plusos_con_args*  (text console; see PLUSOS_CON_*) */
    SYS_win,           /* a0 = plusos_win_args*  (window server; see PLUSOS_WIN_*) */
    SYS_termspawn,     /* a0 = plusos_spawn_args* -> pid; the program's console is a window */
    SYS_thread,        /* a0 = plusos_thread_args* -> thread id (a pid), or -1 */
    SYS_detach,        /* a0 = thread/process id: free it automatically when it ends */
    SYS_history,       /* a0 = plusos_history_args* -> persistent 20-cmd history */
    SYS_sound,         /* a0 = plusos_sound_args*   -> hardware sound synthesizer */
    SYS_scrap,         /* a0 = plusos_scrap_args*   -> system-wide clipboard/scrap */

    /* --- Extended Kernel Subsystems & Features (34+) --- */
    SYS_ramstats,      /* 34: a0 = plusos_ram_stats* -> 0 */
    SYS_gc,            /* 35: a0 = plusos_gc_args*   -> reclaimed bytes */
    SYS_heapcheck,     /* 36:                        -> 0 if healthy, or corrupt count */
    SYS_pipecreate,    /* 37: a0 = plusos_pipe_args* -> pipe_id or -1 */
    SYS_piperead,      /* 38: a0 = plusos_pipe_io*   -> bytes read or -1 */
    SYS_pipewrite,     /* 39: a0 = plusos_pipe_io*   -> bytes written or -1 */
    SYS_pipeclose,     /* 40: a0 = pipe_id           -> 0 or -1 */
    SYS_seminit,       /* 41: a0 = plusos_sem_args*  -> sem_id or -1 */
    SYS_semwait,       /* 42: a0 = sem_id            -> 0 or -1 */
    SYS_sempost,       /* 43: a0 = sem_id            -> 0 or -1 */
    SYS_semdestroy,    /* 44: a0 = sem_id            -> 0 or -1 */
    SYS_mutexinit,     /* 45:                        -> mutex_id or -1 */
    SYS_mutexlock,     /* 46: a0 = mutex_id          -> 0 or -1 */
    SYS_mutexunlock,   /* 47: a0 = mutex_id          -> 0 or -1 */
    SYS_signal,        /* 48: a0 = plusos_sig_args*  -> 0 or -1 */
    SYS_alarm,         /* 49: a0 = seconds           -> remaining seconds */
    SYS_timer,         /* 50: a0 = plusos_timer_args*-> timer_id or -1 */
    SYS_klog,          /* 51: a0 = plusos_klog_args* -> bytes written or -1 */
    SYS_dmesg,         /* 52: a0 = plusos_dmesg_args*-> bytes returned or -1 */
    SYS_getenv,        /* 53: a0 = plusos_env_args*  -> length or -1 */
    SYS_setenv,        /* 54: a0 = plusos_env_args*  -> 0 or -1 */
    SYS_ramdisk,       /* 55: a0 = plusos_ramdisk_args* -> 0 or -1 */
    SYS_shm,           /* 56: a0 = plusos_shm_args*  -> shmid or ptr or -1 */
    SYS_power,         /* 57: a0 = plusos_power_args*-> 0 or -1 */
    SYS_pram,          /* 58: a0 = plusos_pram_args* -> 0 or -1 */
    SYS_getrandom,     /* 59: a0 = plusos_rand_args* -> 0 or -1 */
    SYS_perfmetrics,   /* 60: a0 = plusos_perf_metrics* -> 0 */
    SYS_priority,      /* 61: a0 = plusos_prio_args* -> old_prio or 0 */
    SYS_watchdog,      /* 62: a0 = plusos_watchdog_args* -> 0 */
    SYS_procpgid,      /* 63: a0 = plusos_pgid_args* -> pgid or 0 */
    SYS_users,         /* 64: a0 = plusos_users_args* -> see op */
    SYS_perms,         /* 65: a0 = plusos_perms_args* -> see op */
};

/* SYS_users: user accounts backed by /etc/passwd and /etc/shadow. One call
 * dispatches on an op field; reasonable ops and what they return:
 *
 *   PLUSOS_USERS_FIRSTRUN   -> 1 if no accounts exist, else 0
 *   PLUSOS_USERS_COUNT      -> number of accounts
 *   PLUSOS_USERS_AT         (index  -> name/uid/gid/home/flags in *a0) -> 0 or -1
 *   PLUSOS_USERS_FIND       (name   -> uid/gid/home/flags in *a0)      -> 0 or -1
 *   PLUSOS_USERS_FIND_UID   (uid    -> name/gid/home/flags in *a0)     -> 0 or -1
 *   PLUSOS_USERS_AUTH       (name + pw)                                -> 1 ok, 0 bad
 *   PLUSOS_USERS_SETPW      (name + pw)                                -> 0 or -1
 *   PLUSOS_USERS_ADD        (name + pw + flags + is_root)              -> 0 or -1
 *   PLUSOS_USERS_WHOAMI     ()                                         -> current uid
 *   PLUSOS_USERS_SETCURRENT (uid; also stamps proc_current)            -> 0
 */
enum {
    PLUSOS_USERS_FIRSTRUN = 1,
    PLUSOS_USERS_COUNT,
    PLUSOS_USERS_AT,
    PLUSOS_USERS_FIND,
    PLUSOS_USERS_FIND_UID,
    PLUSOS_USERS_AUTH,
    PLUSOS_USERS_SETPW,
    PLUSOS_USERS_ADD,
    PLUSOS_USERS_WHOAMI,
    PLUSOS_USERS_SETCURRENT,
    PLUSOS_USERS_IS_SUDOER,
    PLUSOS_USERS_SUDO_ELEVATE,
    PLUSOS_USERS_DELETE,
};

struct plusos_users_args {
    int      op;
    int      index;
    int      uid;
    int      gid;
    int      flags;
    int      is_root;
    char     name [16];
    char     home [40];
    char     shell[16];
    char     pw   [32];
};

enum {
    PLUSOS_PERMS_GET = 1,
    PLUSOS_PERMS_SET,
    PLUSOS_PERMS_CHMOD,
    PLUSOS_PERMS_CHOWN,
    PLUSOS_PERMS_CHECK,
    PLUSOS_PERMS_COUNT,
    PLUSOS_PERMS_AT,
};
struct plusos_perms_args {
    int      op;
    int      index;
    int      mode;
    int      uid;
    int      gid;
    int      want;
    char     path[48];
};

/* SYS_thread: run entry(fn, arg) as a new thread of the calling program (use
 * <plusos/thread.h>, which supplies entry). Threads share the program's memory,
 * are preempted like any process, and end when the program ends. */
struct plusos_thread_args { void *entry; void *fn; void *arg; int stack_bytes; };

/* ---- window server --------------------------------------------------------
 * A program owns windows; it draws into a window's bitmap (with PlusGFX) and
 * calls UPDATE. The desktop composites every window on screen and sends the
 * owner its mouse and keyboard events. Windows go away when their owner exits. */
enum { PLUSOS_WIN_CREATE, PLUSOS_WIN_UPDATE, PLUSOS_WIN_EVENT, PLUSOS_WIN_DESTROY,
       PLUSOS_WIN_TITLE, PLUSOS_WIN_LIST, PLUSOS_WIN_POST, PLUSOS_WIN_MOVE };
enum { PLUSOS_EV_NONE, PLUSOS_EV_MOUSEDOWN, PLUSOS_EV_MOUSEUP, PLUSOS_EV_KEY, PLUSOS_EV_CLOSE };

struct plusos_event { int type, x, y, key; };       /* x, y: content coordinates */
struct plusos_wininfo {
    int id, owner, term, dead;      /* term: a text console; dead: its program has ended */
    int x, y, w, h;                 /* placement hint, content size */
    unsigned long seq;              /* bumped on every update */
    unsigned char *bits; int rowbytes;
    char title[32];
};
struct plusos_win_args {
    int op, id;
    int x, y, w, h;
    const char *title;
    struct plusos_event *ev; int wait;
    unsigned char *bits; int rowbytes;               /* out: CREATE */
    struct plusos_wininfo *list; int max;            /* LIST */
    unsigned long seq;                               /* out: LIST - changes on any change */
};

/* SYS_console operations. The console is a process's text screen: the whole
 * display, or a terminal window on the desktop. */
enum { PLUSOS_CON_PUTC, PLUSOS_CON_WRITE, PLUSOS_CON_CLEAR, PLUSOS_CON_GOTO,
       PLUSOS_CON_GETPOS, PLUSOS_CON_INVERT, PLUSOS_CON_CURSOR,
       PLUSOS_CON_GETSIZE };       /* -> *pa = columns, *pb = rows */
struct plusos_con_args { int op; int a, b; const char *s; int n; int *pa, *pb; };

/* registry calls (the system-wide key=value store, see `reg`) */
struct plusos_reg_args {
    const char *key;
    const char *value;
    char       *kbuf;  int ksize;
    char       *vbuf;  int vsize;
    int         index;
};

/* one entry per process, for the task manager (SYS_proclist) */
struct plusos_procinfo {
    int           pid, parent, ring, state;     /* state: 0 unused,1 ready,2 run,3 blocked,4 zombie */
    unsigned long cpu_ticks;                    /* ticks spent running */
    unsigned long mem_bytes;                    /* stacks + image */
    int           priority;                     /* -20 to +19 */
    int           pgid;                         /* process group id */
    char          name[16];
};
struct plusos_proclist_args { struct plusos_procinfo *buf; int max; };
struct plusos_reap { int pid; };                /* SYS_reapany writes the finished pid here */

/* SYS_chainboot errors */
#define PLUSOS_EBOOT_NOTSE   (-1)   /* switching Systems needs a Mac SE */
#define PLUSOS_EBOOT_NODISK  (-2)   /* no SCSI disk at that ID */

/* configuration keys (SYS_getcfg / SYS_setcfg) */
#define PLUSOS_CFG_BOOT      1      /* first program at boot: */
#define PLUSOS_BOOT_SHELL    0      /*   the shell (default) */
#define PLUSOS_BOOT_DESKTOP  1      /*   the desktop */
#define PLUSOS_BOOT_MENU     2      /*   the multi-boot menu */

struct plusos_spawn_args { const char *name; int argc; char **argv; };
struct plusos_readline_args { char *buf; int size; };
struct plusos_blkio_args { int write; unsigned long block, count; void *buf; };
struct plusos_cfg_args { int key; int value; };
struct plusos_sysinfo {
    char          machine[24];      /* "Macintosh Plus", "Macintosh SE" */
    char          cpu[16];
    unsigned long ram_bytes;
    unsigned long free_bytes;       /* ROM heap */
    unsigned long ticks;            /* 60 Hz since boot */
    int           nprocs;
    int           is_se;            /* can chain-boot other Systems */
    int           startup_id;       /* default SCSI startup ID, -1 = none */
    char          volume[32];       /* boot volume name */
};

/* SYS_history: persistent 20-command shell history stored in "PlusOS History" */
enum { PLUSOS_HIST_COUNT, PLUSOS_HIST_GET, PLUSOS_HIST_ADD, PLUSOS_HIST_CLEAR };
struct plusos_history_args {
    int   op;
    int   index;
    char *buf;
    int   size;
};

/* SYS_sound: Compact Mac 22.25 kHz PWM sound driver & VIA volume control */
enum { PLUSOS_SND_BEEP, PLUSOS_SND_TONE, PLUSOS_SND_VOL_GET, PLUSOS_SND_VOL_SET,
       PLUSOS_SND_NOTES };
struct plusos_sound_args {
    int             op;
    int             freq_hz;        /* tone frequency in Hz (e.g. 440) */
    int             duration_ticks; /* duration in 1/60s ticks */
    int             volume;         /* 0..7 (0 = silent menu-bar flash) */
    const uint16_t *notes;          /* pairs of (freq_hz, duration_ticks) */
    int             count;          /* number of note pairs */
};

/* SYS_scrap: system-wide clipboard ("Clipboard File" + low-memory ScrapVars) */
enum { PLUSOS_SCRAP_GET, PLUSOS_SCRAP_SET, PLUSOS_SCRAP_CLEAR, PLUSOS_SCRAP_INFO };
struct plusos_scrap_args {
    int      op;
    uint32_t type;                  /* 'TEXT' = 0x54455854, 'PICT' = 0x50494354 */
    void    *buf;
    int      size;
    int      count;                 /* out: ScrapCount edition number */
};

/* RAM Management stats (SYS_ramstats) */
struct plusos_ram_stats {
    uint32_t total_ram;
    uint32_t free_ram;
    uint32_t kernel_load_addr;
    uint32_t kernel_stack_size;
    uint32_t heap_alloc_bytes;
    uint32_t heap_chunks_active;
    uint32_t slab_alloc_bytes;
    uint32_t slab_free_slots;
    uint32_t largest_free_block;
    uint32_t gc_reclaimed_total;
    uint32_t corruption_detected;
};

/* Garbage Collection args (SYS_gc) */
struct plusos_gc_args {
    int      compact_heap;     /* 1 = run ROM CompactMem & PurgeMem */
    uint32_t bytes_reclaimed;  /* out: bytes freed by GC */
    int      chunks_freed;     /* out: memory chunks freed */
    int      zombies_reaped;   /* out: zombie procs reaped */
    int      pipes_closed;     /* out: orphan pipes closed */
};

/* Pipe args (SYS_pipecreate, SYS_piperead, SYS_pipewrite) */
struct plusos_pipe_args {
    int read_fd;
    int write_fd;
};
struct plusos_pipe_io {
    int   pipe_id;
    void *buf;
    int   count;
    int   nonblock;
};

/* Semaphore args (SYS_seminit) */
struct plusos_sem_args {
    int init_val;
    int max_val;
};

/* Signal constants and args (SYS_signal) */
enum {
    PLUSOS_SIGINT  = 2,
    PLUSOS_SIGKILL = 9,
    PLUSOS_SIGUSR1 = 10,
    PLUSOS_SIGUSR2 = 12,
    PLUSOS_SIGALRM = 14,
    PLUSOS_SIGTERM = 15,
};
struct plusos_sig_args {
    int pid;
    int signum;
};

/* Timer args (SYS_timer) */
enum { PLUSOS_TIMER_SET, PLUSOS_TIMER_CANCEL };
struct plusos_timer_args {
    int op;
    int timer_id;
    int delay_ticks;
    int interval_ticks;
    int signum;
};

/* Kernel log & dmesg args (SYS_klog, SYS_dmesg) */
enum {
    PLUSOS_LOG_EMERG = 0,
    PLUSOS_LOG_ERR   = 3,
    PLUSOS_LOG_WARN  = 4,
    PLUSOS_LOG_INFO  = 6,
    PLUSOS_LOG_DEBUG = 7,
};
struct plusos_klog_args {
    int         level;
    const char *msg;
    int         len;
};
struct plusos_dmesg_args {
    char *buf;
    int   max_len;
    int   offset;
};

/* Environment args (SYS_getenv, SYS_setenv) */
enum { PLUSOS_ENV_GET, PLUSOS_ENV_SET, PLUSOS_ENV_UNSET, PLUSOS_ENV_LIST };
struct plusos_env_args {
    int         op;
    const char *key;
    char       *value;
    int         val_size;
};

/* RAM disk args (SYS_ramdisk) */
enum { PLUSOS_RAMDISK_READ, PLUSOS_RAMDISK_WRITE, PLUSOS_RAMDISK_INFO, PLUSOS_RAMDISK_CLEAR };
struct plusos_ramdisk_args {
    int      op;
    uint32_t block;
    uint32_t count;
    void    *buf;
    uint32_t total_blocks;
    uint32_t used_blocks;
};

/* Shared memory args (SYS_shm) */
enum { PLUSOS_SHM_GET, PLUSOS_SHM_AT, PLUSOS_SHM_DT };
struct plusos_shm_args {
    int      op;
    int      key;
    uint32_t size;
    int      shmid;
    void    *addr;
};

/* Power management args (SYS_power) */
enum { PLUSOS_PWR_STATUS, PLUSOS_PWR_BLANK, PLUSOS_PWR_SET_TIMEOUT };
struct plusos_power_args {
    int      op;
    int      blank_on;
    int      timeout_ticks;
    uint32_t idle_ticks;
};

/* Parameter RAM args (SYS_pram) */
enum { PLUSOS_PRAM_READ, PLUSOS_PRAM_WRITE };
struct plusos_pram_args {
    int     op;
    int     offset;
    uint8_t value;
};

/* Random args (SYS_getrandom) */
struct plusos_rand_args {
    void *buf;
    int   len;
};

/* Performance metrics (SYS_perfmetrics) */
struct plusos_perf_metrics {
    uint32_t context_switches;
    uint32_t syscall_count;
    uint32_t interrupt_ticks;
    uint32_t mem_alloc_count;
    uint32_t mem_free_count;
    uint32_t load_1m_fixed;   /* 16.16 fixed point */
    uint32_t load_5m_fixed;   /* 16.16 fixed point */
};

/* Priority args (SYS_priority) */
enum { PLUSOS_PRIO_GET, PLUSOS_PRIO_SET };
struct plusos_prio_args {
    int op;
    int pid;
    int priority; /* -20 to +19 */
};

/* Watchdog args (SYS_watchdog) */
enum { PLUSOS_WD_PING, PLUSOS_WD_STATUS };
struct plusos_watchdog_args {
    int op;
    int timeout_ticks;
    int tripped_count;
};

/* Process group args (SYS_procpgid) */
enum { PLUSOS_PGID_GET, PLUSOS_PGID_SET };
struct plusos_pgid_args {
    int op;
    int pid;
    int pgid;
};

#ifdef __m68k__
/* Issue a syscall. D0 = result; the kernel preserves everything else, the
 * clobbers below are conservative. */
static inline long plusos_syscall(long num, void *arg)
{
    register long d0 __asm__("d0") = num;
    register void *a0 __asm__("a0") = arg;
    __asm__ volatile(".short 0x4E41"            /* trap #1 */
                     : "+d"(d0), "+a"(a0) :
                     : "d1", "d2", "a1", "cc", "memory");
    return d0;
}
#endif

#endif
