/* main.c - PlusOS kernel entry (ring 0). crt0 has relocated us, cleared BSS,
 * put the stack just below the image and pointed A5 at __plusos_a5world.
 *
 * Boot brings up the ROM subsystems, then the scheduler, then starts the first
 * tasks. This boot process becomes the idle task. */
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

uint32_t  __plusos_load_addr;
uint32_t  __plusos_a5world[4];      /* (A5) -> QuickDraw globals (set by InitGraf) */
QDGlobals qd;
GrafPort  g_port;
uint32_t  g_flags;
int16_t   g_boot_drive, g_boot_refnum, g_boot_vref;
char      g_volname[32];

volatile uint32_t heartbeat;        /* bumped once a second by the heartbeat task */

extern void __plusos_run_ctors(void);

/* ring-0 init task: launch the first userland application (the shell by
 * default) and keep it running. If it can't be started, fall back to the
 * built-in kernel menu so the machine is still usable. */
static int start_app(const char *name)
{
    char *av[] = { (char *)name, 0 };
    return exec_spawn(name, 1, av);
}

static void init_task(void *arg)
{
    (void)arg;
    /* The boot setting picks the first program. Holding Option at boot forces
     * the shell, in case the desktop is broken. */
    int option_down = (LM_BYTE(0x174 + 7) & 0x04) != 0;        /* KeyMap: Option = key $3A */
    int boot_cfg = cfg_get(PLUSOS_CFG_BOOT);
    if (boot_cfg == PLUSOS_BOOT_MENU && !option_down) {
        menu_main();
    }
    int want_desktop = (boot_cfg == PLUSOS_BOOT_DESKTOP) && !option_down;
    const char *first = want_desktop ? "desktop" : "shell";
    const char *other = want_desktop ? "shell" : "desktop";
    for (;;) {
        int pid = start_app(first);
        if (pid < 0) {
            printf("init: cannot start the %s (%s).\n", first, exec_error(pid));
            pid = start_app(other);
        }
        if (pid < 0) {
            printf("init: no userland programs; starting the recovery console.\n");
            menu_main();            /* never returns */
        }
        proc_wait(pid);             /* runs in ring 2 until it exits ... */
        const char *t = first;      /* ... then switch: exit the shell -> desktop, */
        first = other;              /* quit the desktop -> shell */
        other = t;
    }
}

/* ring-0 task: proof of concurrency - ticks a counter once a second while the
 * shell is blocked waiting for keys. Visible via `uptime`. */
static void heartbeat_task(void *arg)
{
    (void)arg;
    for (;;) {
        heartbeat++;
        proc_sleep_ticks(60);
    }
}

__attribute__((noreturn)) void __plusos_kernel_start(void)
{
    __plusos_run_ctors();
    con_init();
    printf("PlusOS %s by the1Domo on the %s - %s, %luK RAM\n", PLUSOS_VERSION, plusrom_machine_name(),
           cpu_name(), (unsigned long)(ram_bytes() >> 10));
    exc_install();
    printf("Starting the ROM:\n");
    rom_init();
    kmem_init();
    gc_init();
    klog_init();
    env_init();
    entropy_init();
    profiler_init();
    security_init();
    kbd_init((g_flags & PLUSOS_HAVE_EVENTS) != 0);
    sound_init();
    power_init();
    pram_init();
    video_init();                   /* allocate the back buffer before api_init wires it */
    api_init();
    ramdisk_init();
    shm_init();

    printf("Starting the kernel: scheduler, timer, IPC, syscalls\n");
    sched_init();
    pipe_init();
    sync_init();
    signal_init();
    timer_subsys_init();
    syscall_init();
    tick_init();
    preempt_init();                 /* user programs can no longer hog the CPU */
    if (proc_create_kernel("init", init_task, 0) < 0 ||
        proc_create_kernel("heartbeat", heartbeat_task, 0) < 0)
        printf("  WARNING: could not create the startup tasks\n");

    sched_run();                    /* this context becomes idle; never returns */
}
