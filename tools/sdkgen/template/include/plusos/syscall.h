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
