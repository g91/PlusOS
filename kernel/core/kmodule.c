/* kmodule.c - kernel module registry, driver table and watchdog. */
#ifdef KMOD_HOST                        /* host-side unit test (tools/tests/test_kmodule.c) */
#include <stdio.h>
#include <string.h>
#include "kmodule.h"
extern uint32_t fake_now;               /* the test's fake 60 Hz clock */
#else
#include "kernel.h"
#include "kmodule.h"
#include "kconfig.h"
#include "../services/klog.h"
#endif

/* Boot-stage events are logged to the kernel event log (and thence /boot.log)
 * as well as printed to the console. The host-side unit test has no klog. */
#ifdef KMOD_HOST
#define KMOD_LOG(level, ...) ((void)0)
#else
#define KMOD_LOG(level, ...) klog((level), __VA_ARGS__)
#endif

#ifndef KMOD_NOW
#define KMOD_NOW()  ((uint32_t)LM_LONG(0x16A))     /* Ticks: 60 Hz, valid before tick_init */
#endif

#define KMOD_MAX 48

static kmodule *mod_tab[KMOD_MAX];
static int      mod_n;
static int      boot_order[KMOD_MAX];   /* indices in the order they started, for shutdown */
static int      boot_n;

const char *kmod_init_cur;
volatile int kmod_last_syscall;

/* ---- registry ------------------------------------------------------------ */
void kmod_register_table(kmodule *tab, int n)
{
    for (int i = 0; i < n && mod_n < KMOD_MAX; i++) {
        tab[i].state = KMOD_REGISTERED;
        tab[i].err = 0;
        tab[i].init_ticks = 0;
        mod_tab[mod_n++] = &tab[i];
    }
}

kmodule *kmod_find(const char *name)
{
    for (int i = 0; i < mod_n; i++)
        if (strcmp(mod_tab[i]->name, name) == 0) return mod_tab[i];
    return 0;
}

int kmod_running(const char *name)
{
    kmodule *m = kmod_find(name);
    return m && m->state == KMOD_RUNNING;
}

int kmod_count(void) { return mod_n; }
kmodule *kmod_at(int i) { return (i >= 0 && i < mod_n) ? mod_tab[i] : 0; }

/* First dependency of m that is not running, copied into out; 0 if all are up. */
static int missing_dep(const kmodule *m, char *out, int outsz)
{
    const char *p = m->deps;
    while (p && *p) {
        while (*p == ' ') p++;
        if (!*p) break;
        int n = 0;
        char name[24];
        while (p[n] && p[n] != ' ' && n < (int)sizeof name - 1) { name[n] = p[n]; n++; }
        name[n] = 0;
        p += n;
        while (*p && *p != ' ') p++;                     /* skip an over-long tail */
        if (n && !kmod_running(name)) {
            strncpy(out, name, (size_t)outsz - 1);
            out[outsz - 1] = 0;
            return 1;
        }
    }
    return 0;
}

int kmod_boot_stage(int stage)
{
    int bad = 0;
    for (int i = 0; i < mod_n; i++) {
        kmodule *m = mod_tab[i];
        if (m->stage != stage || m->state != KMOD_REGISTERED) continue;
        char dep[24];
        if (missing_dep(m, dep, sizeof dep)) {
            m->state = KMOD_SKIPPED;
            printf("  module %s skipped (needs %s)\n", m->name, dep);
            KMOD_LOG(4, "module %s skipped (needs %s)", m->name, dep);
            bad++;
            continue;
        }
        kmod_init_cur = m->name;                        /* the crash handler reports this */
        uint32_t t0 = KMOD_NOW();
        int rc = m->init ? m->init() : 0;
        uint32_t dt = KMOD_NOW() - t0;
        kmod_init_cur = 0;
        m->init_ticks = dt > 0xFFFF ? 0xFFFF : (uint16_t)dt;
        if (rc < 0) {
            m->state = KMOD_FAILED;
            m->err = (int16_t)rc;
            printf("  module %s FAILED (%d)%s\n", m->name, rc,
                   (m->flags & KMOD_F_ESSENTIAL) ? " - essential!" : "");
            KMOD_LOG(3, "module %s FAILED (%d)%s", m->name, rc,
                     (m->flags & KMOD_F_ESSENTIAL) ? " - essential!" : "");
            bad++;
        } else {
            m->state = KMOD_RUNNING;
            KMOD_LOG(6, "module %s started (%u tick%s)", m->name,
                     (unsigned)m->init_ticks, m->init_ticks == 1 ? "" : "s");
            if (boot_n < KMOD_MAX) boot_order[boot_n++] = i;
        }
    }
    return bad;
}

void kmod_shutdown(void)
{
    for (int k = boot_n - 1; k >= 0; k--) {
        kmodule *m = mod_tab[boot_order[k]];
        if (m->state != KMOD_RUNNING) continue;
        if (m->fini) m->fini();
        m->state = KMOD_STOPPED;
    }
    boot_n = 0;
}

const char *kmod_state_name(int s)
{
    switch (s) {
    case KMOD_REGISTERED: return "pending";
    case KMOD_RUNNING:    return "running";
    case KMOD_FAILED:     return "FAILED";
    case KMOD_SKIPPED:    return "skipped";
    case KMOD_STOPPED:    return "stopped";
    }
    return "?";
}

const char *kmod_type_name(int t)
{
    return t == KMOD_CORE ? "core" : t == KMOD_DRIVER ? "driver" : "service";
}

const char *kmod_stage_name(int s)
{
    static const char *n[] = { "memory", "base", "drivers", "services", "core" };
    return (s >= 0 && s < KSTAGE_COUNT) ? n[s] : "?";
}

/* ---- driver table --------------------------------------------------------- */
static const kdriver *drv_tab[KDRV_MAX];
static int            drv_n;

int kdrv_register(const kdriver *d)
{
    if (!d || !d->name || drv_n >= KDRV_MAX || kdrv_find(d->name)) return -1;
    drv_tab[drv_n++] = d;
    return 0;
}

const kdriver *kdrv_find(const char *name)
{
    for (int i = 0; i < drv_n; i++)
        if (strcmp(drv_tab[i]->name, name) == 0) return drv_tab[i];
    return 0;
}

const kdriver *kdrv_find_class(int cls)
{
    for (int i = 0; i < drv_n; i++)
        if (drv_tab[i]->cls == cls) return drv_tab[i];
    return 0;
}

int kdrv_count(void) { return drv_n; }
const kdriver *kdrv_at(int i) { return (i >= 0 && i < drv_n) ? drv_tab[i] : 0; }

void kdrv_irq_all(void)
{
    for (int i = 0; i < drv_n; i++)
        if (drv_tab[i]->irq) drv_tab[i]->irq();
}

long kdrv_control(const char *name, int cmd, void *arg)
{
    const kdriver *d = kdrv_find(name);
    return (d && d->control) ? d->control(cmd, arg) : -1;
}

/* ---- watchdog -------------------------------------------------------------- */
#if !defined(KMOD_HOST) && !CONFIG_WATCHDOG
void     kwd_tick(void)   {}
uint32_t kwd_pet(void)    { return 0; }
uint32_t kwd_stalls(void) { return 0; }
uint32_t kwd_worst(void)  { return 0; }
#else
static volatile uint32_t wd_since;      /* ticks since the heartbeat last ran */
static volatile int      wd_tripped;
static uint32_t          wd_stalls, wd_worst;

void kwd_tick(void)                     /* interrupt level: keep it tiny */
{
    uint32_t s = ++wd_since;
    if (s >= KWD_LIMIT_TICKS) {
        if (!wd_tripped) { wd_tripped = 1; wd_stalls++; }
        if (s > wd_worst) wd_worst = s;
    }
}

uint32_t kwd_pet(void)
{
    uint32_t s = wd_tripped ? wd_since : 0;
    wd_since = 0;
    wd_tripped = 0;
    return s;
}

uint32_t kwd_stalls(void) { return wd_stalls; }
uint32_t kwd_worst(void)  { return wd_worst; }
#endif
