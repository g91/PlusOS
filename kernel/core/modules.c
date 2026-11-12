/* modules.c - the kernel's module table: what is built in, in what order, and
 * how each driver presents itself through the kdriver table.
 *
 * To add a module: write `static int m_foo(void)`, add one MOD() line below
 * (guarded by its CONFIG_ switch if it is optional), and add the CONFIG_ switch
 * to kconfig.h / the MOD_* lists in the Makefile. Nothing else needs to know. */
#include "kernel.h"
#include "proc.h"
#include "kmem.h"
#include "gc.h"
#include "pipe.h"
#include "sync.h"
#include "../proc/signal.h"
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
#include "syscall.h"
#include "users.h"
#include "perms.h"
#include "sys7.h"
#include "vfs.h"
#include "posix.h"

/* ---- drivers' kdriver tables --------------------------------------------- */
static long kbd_drv_read(void *buf, uint32_t off, uint32_t len)
{
    (void)off;
    uint8_t *p = buf;
    uint32_t n = 0;
    while (n < len) {                   /* whatever is queued; never blocks */
        int k = kbd_poll();
        if (k <= 0) break;              /* kbd_poll() returns 0 when nothing is queued */
        p[n++] = (uint8_t)k;
    }
    return (long)n;
}
static long kbd_drv_control(int cmd, void *arg)
{
    (void)arg;
    switch (cmd) {
    case KDRV_KBD_POLL: return kbd_poll();
    case KDRV_KBD_GET:  return kbd_get();
    case KDRV_KBD_MODS: return kbd_mods();
    }
    return -1;
}
static const kdriver kbd_drv = { "kbd", KDRV_INPUT, 0, kbd_drv_read, 0, kbd_drv_control, 0 };

static long video_drv_control(int cmd, void *arg)
{
    switch (cmd) {
    case KDRV_VID_PRESENT:      video_present(); return 0;
    case KDRV_VID_PRESENT_RECT: { int *r = arg; if (!r) return -1; video_present_rect(r[0], r[1]); return 0; }
    case KDRV_VID_FRONT:        return (long)video_front();
    case KDRV_VID_BACK:         return (long)video_back();
    }
    return -1;
}
static const kdriver video_drv = { "video", KDRV_VIDEO, 0, 0, 0, video_drv_control, 0 };

#if CONFIG_SOUND
static long snd_drv_control(int cmd, void *arg)
{
    switch (cmd) {
    case KDRV_SND_BEEP:   sound_beep((int)(intptr_t)arg); return 0;
    case KDRV_SND_TONE:   { int *a = arg; if (!a) return -1; sound_tone(a[0], a[1]); return 0; }
    case KDRV_SND_GETVOL: return sound_get_volume();
    case KDRV_SND_SETVOL: sound_set_volume((int)(intptr_t)arg); return 0;
    }
    return -1;
}
static const kdriver snd_drv = { "snd", KDRV_SOUND, 0, 0, 0, snd_drv_control, 0 };
#endif

static long blk_drv_read(void *buf, uint32_t block, uint32_t count)        { return blk_read(block, count, buf); }
static long blk_drv_write(const void *buf, uint32_t block, uint32_t count) { return blk_write(block, count, buf); }
static const kdriver blk_drv = { "blk", KDRV_BLOCK, 0, blk_drv_read, blk_drv_write, 0, 0 };

static long tmr_drv_control(int cmd, void *arg)
{
    (void)arg;
    return cmd == KDRV_TMR_TICKS ? (long)tick_count() : -1;
}
static const kdriver tmr_drv = { "tick", KDRV_TIMER, 0, 0, 0, tmr_drv_control, 0 };

/* ---- module init wrappers -------------------------------------------------- */
static int m_kmem(void)     { kmem_init(); return 0; }
static int m_gc(void)       { gc_init(); return 0; }
static int m_klog(void)     { klog_init(); return 0; }
static int m_env(void)      { env_init(); return 0; }
static int m_security(void) { security_init(); return 0; }
#if CONFIG_ENTROPY
static int m_entropy(void)  { entropy_init(); return 0; }
#endif
#if CONFIG_PROFILER
static int m_profiler(void) { profiler_init(); return 0; }
#endif
#if CONFIG_USERS
static int m_users(void)    { users_init(); return 0; }
#endif
#if CONFIG_PERMS
static int m_perms(void)    { perms_init(); return 0; }
#endif

static int m_kbd(void)
{
    kbd_init((g_flags & PLUSOS_HAVE_EVENTS) != 0);
    return kdrv_register(&kbd_drv);
}
#if CONFIG_SOUND
static int m_sound(void)    { sound_init(); return kdrv_register(&snd_drv); }
#endif
#if CONFIG_POWER
static int m_power(void)    { power_init(); return 0; }
#endif
#if CONFIG_PRAM
static int m_pram(void)     { pram_init(); return 0; }
#endif
static int m_video(void)    { video_init(); return kdrv_register(&video_drv); }

static int m_api(void)      { api_init(); return kdrv_register(&blk_drv); }
static int m_sys7(void)     { sys7_init(); return 0; }
static int m_vfs(void)      { vfs_init(); return 0; }
static int m_posix(void)    { posix_init(); return 0; }
#if CONFIG_RAMDISK
static int m_ramdisk(void)  { ramdisk_init(); return 0; }
#endif
#if CONFIG_SHM
static int m_shm(void)      { shm_init(); return 0; }
#endif

static int m_sched(void)    { sched_init(); return 0; }
static int m_pipe(void)     { pipe_init(); return 0; }
static int m_sync(void)     { sync_init(); return 0; }
static int m_signal(void)   { signal_init(); return 0; }
static int m_timer(void)    { timer_subsys_init(); return 0; }
static int m_syscall(void)  { syscall_init(); return 0; }
static int m_tick(void)     { tick_init(); return kdrv_register(&tmr_drv); }
static int m_preempt(void)  { preempt_init(); return 0; }

/* ---- the table: boot order within a stage is the order written here ---------- */
#define ESS KMOD_F_ESSENTIAL
#define MOD(name, type, stage, flags, init, deps) { name, type, stage, flags, init, 0, deps, 0, 0, 0 }

static kmodule k_modules[] = {
    /* memory */
    MOD("kmem",     KMOD_CORE,    KSTAGE_MEMORY,   ESS, m_kmem,     0),
    MOD("gc",       KMOD_SERVICE, KSTAGE_MEMORY,   0,   m_gc,       "kmem"),
    /* base */
    MOD("klog",     KMOD_SERVICE, KSTAGE_BASE,     0,   m_klog,     "kmem"),
    MOD("env",      KMOD_SERVICE, KSTAGE_BASE,     0,   m_env,      "kmem"),
#if CONFIG_ENTROPY
    MOD("entropy",  KMOD_SERVICE, KSTAGE_BASE,     0,   m_entropy,  0),
#endif
#if CONFIG_PROFILER
    MOD("profiler", KMOD_SERVICE, KSTAGE_BASE,     0,   m_profiler, 0),
#endif
    MOD("security", KMOD_SERVICE, KSTAGE_BASE,     0,   m_security, 0),
    /* drivers */
    MOD("kbd",      KMOD_DRIVER,  KSTAGE_DRIVERS,  ESS, m_kbd,      "kmem"),
#if CONFIG_SOUND
    MOD("sound",    KMOD_DRIVER,  KSTAGE_DRIVERS,  0,   m_sound,    0),
#endif
#if CONFIG_POWER
    MOD("power",    KMOD_SERVICE, KSTAGE_DRIVERS,  0,   m_power,    0),
#endif
#if CONFIG_PRAM
    MOD("pram",     KMOD_DRIVER,  KSTAGE_DRIVERS,  0,   m_pram,     0),
#endif
    MOD("video",    KMOD_DRIVER,  KSTAGE_DRIVERS,  ESS, m_video,    "kmem"),
    /* services (api wires the video buffers into the app service table) */
    MOD("api",      KMOD_SERVICE, KSTAGE_SERVICES, ESS, m_api,      "video"),
#if CONFIG_RAMDISK
    MOD("ramdisk",  KMOD_SERVICE, KSTAGE_SERVICES, 0,   m_ramdisk,  "kmem"),
#endif
#if CONFIG_SHM
    MOD("shm",      KMOD_SERVICE, KSTAGE_SERVICES, 0,   m_shm,      "kmem"),
#endif
#if CONFIG_USERS
    MOD("users",    KMOD_SERVICE, KSTAGE_SERVICES, 0,   m_users,    "kmem"),
#endif
#if CONFIG_PERMS
    MOD("perms",    KMOD_SERVICE, KSTAGE_SERVICES, 0,   m_perms,    "kmem"),
#endif
    MOD("sys7",     KMOD_SERVICE, KSTAGE_SERVICES, 0,   m_sys7,     "video"),
    MOD("vfs",      KMOD_SERVICE, KSTAGE_SERVICES, ESS, m_vfs,      "kmem"),
    MOD("posix",    KMOD_SERVICE, KSTAGE_SERVICES, ESS, m_posix,    "vfs"),
    /* core: scheduler, IPC, syscall gate, tick */
    MOD("sched",    KMOD_CORE,    KSTAGE_CORE,     ESS, m_sched,    "kmem"),
    MOD("pipe",     KMOD_CORE,    KSTAGE_CORE,     0,   m_pipe,     "sched"),
    MOD("sync",     KMOD_CORE,    KSTAGE_CORE,     0,   m_sync,     "sched"),
    MOD("signal",   KMOD_CORE,    KSTAGE_CORE,     0,   m_signal,   "sched"),
    MOD("timer",    KMOD_CORE,    KSTAGE_CORE,     0,   m_timer,    "sched"),
    MOD("syscall",  KMOD_CORE,    KSTAGE_CORE,     ESS, m_syscall,  "sched"),
    MOD("tick",     KMOD_CORE,    KSTAGE_CORE,     ESS, m_tick,     "sched"),
    MOD("preempt",  KMOD_CORE,    KSTAGE_CORE,     ESS, m_preempt,  "tick"),
};

void modules_register(void)
{
    kmod_register_table(k_modules, (int)(sizeof k_modules / sizeof k_modules[0]));
}
