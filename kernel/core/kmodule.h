/* kmodule.h - the PlusOS kernel module registry and driver table.
 *
 * Everything that is not the bare scheduler/trap core is a *module*: it has a
 * name, a type, a boot stage, optional dependencies, and init/fini entry
 * points. The boot code walks the stages in order and initialises each module
 * whose dependencies are up; one failing module is recorded and skipped
 * instead of taking the whole boot down.
 *
 * Drivers additionally register a `kdriver` operations table (init/read/write/
 * control/irq) under a name, so services and syscalls can find a device
 * without hard-wiring its functions. Hot paths keep direct calls; look the
 * driver up once and cache the pointer.
 *
 * Without an MMU a module cannot be memory-isolated. What this gives instead:
 * a clean interface, ordered/dependent bring-up, failure containment at init,
 * and per-module state that the crash handler and `lsmod` can report. */
#ifndef PLUSOS_KMODULE_H
#define PLUSOS_KMODULE_H

#include <stdint.h>
#include <stddef.h>

/* ---- module registry ---------------------------------------------------- */
enum kmod_type  { KMOD_CORE = 0, KMOD_DRIVER, KMOD_SERVICE };

enum kmod_state {
    KMOD_REGISTERED = 0,        /* known, not started yet */
    KMOD_RUNNING,               /* init() returned 0 */
    KMOD_FAILED,                /* init() returned < 0 */
    KMOD_SKIPPED,               /* a dependency was not running */
    KMOD_STOPPED,               /* fini() ran */
};

/* flags */
#define KMOD_F_ESSENTIAL  0x01  /* the system cannot work without it */

/* Boot stages, in order. */
enum kmod_stage {
    KSTAGE_MEMORY = 0,          /* heaps and allocators */
    KSTAGE_BASE,                /* logging, environment, security, counters */
    KSTAGE_DRIVERS,             /* hardware drivers */
    KSTAGE_SERVICES,            /* services built on the drivers */
    KSTAGE_CORE,                /* scheduler, IPC, syscalls, tick */
    KSTAGE_COUNT
};

typedef struct kmodule {
    const char *name;
    uint8_t     type;           /* enum kmod_type */
    uint8_t     stage;          /* enum kmod_stage */
    uint8_t     flags;          /* KMOD_F_* */
    int       (*init)(void);    /* 0 = ok, < 0 = failed */
    void      (*fini)(void);    /* optional; run in reverse order at shutdown */
    const char *deps;           /* space-separated module names, or 0 */
    /* filled in at run time */
    uint8_t     state;          /* enum kmod_state */
    int16_t     err;            /* init() result when FAILED */
    uint16_t    init_ticks;     /* 60 Hz ticks init() took */
} kmodule;

/* Register a module table (array of `n` entries, in boot order within each
 * stage). The table must stay allocated (use a static array). */
void kmod_register_table(kmodule *tab, int n);

/* Bring up every module of `stage`. Returns the number of modules that did not
 * start (failed or skipped). Prints one line per problem. */
int  kmod_boot_stage(int stage);

/* Run fini() of every running module, last started first. */
void kmod_shutdown(void);

kmodule    *kmod_find(const char *name);
int         kmod_running(const char *name);     /* 1 if RUNNING */
int         kmod_count(void);
kmodule    *kmod_at(int index);
const char *kmod_state_name(int state);
const char *kmod_type_name(int type);
const char *kmod_stage_name(int stage);

/* What the kernel was last doing, for the crash handler. */
extern const char *kmod_init_cur;       /* module being initialised, or 0 */
extern volatile int kmod_last_syscall;  /* number of the last syscall entered */

/* ---- driver table ------------------------------------------------------- */
enum kdrv_class { KDRV_INPUT = 0, KDRV_VIDEO, KDRV_SOUND, KDRV_BLOCK, KDRV_NET, KDRV_TIMER };

typedef struct kdriver {
    const char *name;           /* "kbd", "video", "snd", "blk", ... */
    uint8_t     cls;            /* enum kdrv_class */
    int       (*init)(void);
    long      (*read)(void *buf, uint32_t offset, uint32_t len);
    long      (*write)(const void *buf, uint32_t offset, uint32_t len);
    long      (*control)(int cmd, void *arg);
    void      (*irq)(void);     /* called from the tick if non-null */
} kdriver;

#define KDRV_MAX 12

/* control() commands of the built-in drivers (arg is a pointer, or an integer
 * cast to a pointer, as noted). */
enum {
    KDRV_KBD_POLL = 1,          /* -> key, or 0 if none queued; never blocks */
    KDRV_KBD_GET,               /* -> key, blocks */
    KDRV_KBD_MODS,              /* -> modifier word */
    KDRV_VID_PRESENT = 1,       /* flip the whole frame */
    KDRV_VID_PRESENT_RECT,      /* arg = int[2]: top, bottom */
    KDRV_VID_FRONT,             /* -> live screen address */
    KDRV_VID_BACK,              /* -> back buffer address */
    KDRV_SND_BEEP = 1,          /* arg = (intptr_t)ticks */
    KDRV_SND_TONE,              /* arg = int[2]: hz, ticks */
    KDRV_SND_GETVOL,
    KDRV_SND_SETVOL,            /* arg = (intptr_t)volume */
    KDRV_TMR_TICKS = 1,         /* -> 60 Hz tick count */
};

int            kdrv_register(const kdriver *d);   /* 0, or -1 if full/duplicate */
const kdriver *kdrv_find(const char *name);
const kdriver *kdrv_find_class(int cls);
int            kdrv_count(void);
const kdriver *kdrv_at(int index);
void           kdrv_irq_all(void);                /* run every irq hook */
long           kdrv_control(const char *name, int cmd, void *arg);  /* -1 if absent */

/* ---- watchdog ----------------------------------------------------------- */
/* tick_c() calls kwd_tick() every VBL; the heartbeat task calls kwd_pet() once
 * a second. If the heartbeat does not run for KWD_LIMIT_TICKS the watchdog
 * records a stall (it cannot safely log from interrupt level); kwd_pet()
 * returns the stall length in ticks when it finally runs again, else 0. */
#define KWD_LIMIT_TICKS  (60 * 8)
void     kwd_tick(void);
uint32_t kwd_pet(void);
uint32_t kwd_stalls(void);              /* total stalls seen since boot */
uint32_t kwd_worst(void);               /* longest stall, in ticks */

#endif
