/* sys7.c - Extended Core OS Managers for PlusOS.
 *
 * Implements GestaltFunction, ShutDownMgr, Extended TimeMgr,
 * DeferredTaskMgr, and NotificationMgr semantics into PlusOS cleanly across
 * 68000-68040 Macintosh hardware.
 */
#include "kernel.h"
#include "sys7.h"
#include "klog.h"
#include <plusos/syscall.h>

#define MAX_CUSTOM_GESTALT 16
#define MAX_SD_HOOKS       12
#define MAX_SYS7_TIMERS    16
#define MAX_SYS7_DTASKS    16
#define MAX_SYS7_NOTICES   8

/* ---- Gestalt registry ---------------------------------------------------- */
typedef struct {
    int used;
    uint32_t selector;
    sys7_gestalt_fn fn;
} CustomGestalt;

static CustomGestalt s_gestalt[MAX_CUSTOM_GESTALT];

/* ---- ShutDown queue ------------------------------------------------------ */
typedef struct {
    int used;
    sys7_shutdown_fn fn;
    void *arg;
    int flags;
} ShutdownHook;

static ShutdownHook s_sd_hooks[MAX_SD_HOOKS];

/* ---- Extended Time Manager ----------------------------------------------- */
typedef struct {
    int used;
    int armed;
    sys7_timer_fn fn;
    void *arg;
    int32_t remain_us;
} Sys7Timer;

static Sys7Timer s_timers[MAX_SYS7_TIMERS];

/* ---- Deferred Task Manager ----------------------------------------------- */
typedef struct {
    sys7_dtask_fn fn;
    void *arg;
} Sys7DTask;

static Sys7DTask s_dt_q[MAX_SYS7_DTASKS];
static volatile int s_dt_head, s_dt_tail;

/* ---- Notification Manager ------------------------------------------------ */
typedef struct {
    int used;
    char msg[64];
} Sys7Notice;

static Sys7Notice s_notices[MAX_SYS7_NOTICES];
static int s_nm_head, s_nm_tail, s_nm_count;

void sys7_init(void)
{
    memset(s_gestalt, 0, sizeof s_gestalt);
    memset(s_sd_hooks, 0, sizeof s_sd_hooks);
    memset(s_timers, 0, sizeof s_timers);
    memset(s_dt_q, 0, sizeof s_dt_q);
    memset(s_notices, 0, sizeof s_notices);
    s_dt_head = s_dt_tail = 0;
    s_nm_head = s_nm_tail = s_nm_count = 0;
}

/* ---- Gestalt Manager ---- */
int sys7_new_gestalt(uint32_t selector, sys7_gestalt_fn fn)
{
    if (!fn) return -1;
    for (int i = 0; i < MAX_CUSTOM_GESTALT; i++) {
        if (s_gestalt[i].used && s_gestalt[i].selector == selector)
            return -5552; /* gestaltDupSelectorErr */
    }
    for (int i = 0; i < MAX_CUSTOM_GESTALT; i++) {
        if (!s_gestalt[i].used) {
            s_gestalt[i].used = 1;
            s_gestalt[i].selector = selector;
            s_gestalt[i].fn = fn;
            return 0;
        }
    }
    return -1;
}

static uint32_t hw_attr_bits(void)
{
    uint32_t v = 0;
    if (g_hw.has_via1)          v |= (1UL << 0);
    if (g_hw.has_via2)          v |= (1UL << 1);
    if (g_hw.has_asc)           v |= (1UL << 3);
    if (g_hw.has_scc)           v |= (1UL << 4);
    if (g_hw.has_scsi)          v |= (1UL << 7);
    if (g_hw.has_soft_poweroff) v |= (1UL << 19);
    if (g_hw.has_scsi96)        v |= (1UL << 21);
    if (g_hw.has_universal_rom) v |= (1UL << 24);
    return v;
}

int sys7_gestalt(uint32_t sel, int32_t *response)
{
    if (!response) return -5550;

    for (int i = 0; i < MAX_CUSTOM_GESTALT; i++) {
        if (s_gestalt[i].used && s_gestalt[i].selector == sel)
            return s_gestalt[i].fn(sel, response);
    }

    switch (sel) {
    case 0x76657273UL: /* 'vers' */ *response = 1; return 0;
    case 0x73797376UL: /* 'sysv' */ *response = 0x0710; return 0; /* Toolbox API compat level $0710 */
    case 0x73797361UL: /* 'sysa' */ *response = 1; return 0;      /* gestalt68k */
    case 0x6D616368UL: /* 'mach' */ *response = g_hw.gestalt_mach ? g_hw.gestalt_mach : 4; return 0;
    case 0x70726F63UL: /* 'proc' */ *response = (int32_t)g_hw.cpu_flag + 1; return 0; /* 1=68000..5=68040 */
    case 0x63707574UL: /* 'cput' */ *response = (int32_t)g_hw.cpu_flag; return 0;
    case 0x66707520UL: /* 'fpu ' */ *response = (int32_t)g_hw.fpu_type; return 0;
    case 0x6D6D7520UL: /* 'mmu ' */ *response = (int32_t)g_hw.mmu_type; return 0;
    case 0x726F6D20UL: /* 'rom ' */ *response = (int32_t)g_hw.rom_size; return 0;
    case 0x726F6D76UL: /* 'romv' */ *response = (int32_t)g_hw.rom_version; return 0;
    case 0x72616D20UL: /* 'ram ' */
    case 0x6C72616DUL: /* 'lram' */ *response = (int32_t)ram_bytes(); return 0;
    case 0x6C6D656DUL: /* 'lmem' */ *response = 0x1800; return 0;
    case 0x7067737AUL: /* 'pgsz' */ *response = 4096; return 0;
    case 0x61646472UL: /* 'addr' */ {
        int32_t a = 0;
        if (g_hw.is_32bit_mode)    a |= (1 << 0);
        if (g_hw.is_32bit_capable) a |= (1 << 1) | (1 << 2);
        *response = a;
        return 0;
    }
    case 0x68647772UL: /* 'hdwr' */ *response = (int32_t)hw_attr_bits(); return 0;
    case 0x76696131UL: /* 'via1' */ *response = (int32_t)g_hw.via1_base; return 0;
    case 0x76696132UL: /* 'via2' */
        if (!g_hw.has_via2 && !g_hw.via2_base) break;
        *response = (int32_t)g_hw.via2_base;
        return 0;
    case 0x72627620UL: /* 'rbv ' */
        if (!g_hw.has_rbv) break;
        *response = (int32_t)g_hw.rbv_base;
        return 0;
    case 0x73636372UL: /* 'sccr' */ *response = (int32_t)g_hw.scc_rd_base; return 0;
    case 0x73636377UL: /* 'sccw' */ *response = (int32_t)g_hw.scc_wr_base; return 0;
    case 0x6F737474UL: /* 'ostt' */ *response = 0x0400; return 0;
    case 0x74627474UL: /* 'tbtt' */
    case 0x74626274UL: /* 'tbbt' */
        *response = (machine_is_plus()) ? 0x0C00 : 0x0E00;
        return 0;
    case 0x736C6F74UL: /* 'slot' */ {
        int32_t s = 0;
        if (g_hw.has_slot_mgr)   s |= (1 << 0);
        if (g_hw.nubus_slots > 0) s |= (1 << 1);
        if (g_hw.box_flag == BOX_SE) s |= (1 << 2);
        if (g_hw.box_flag == BOX_SE30) s |= (1 << 3);
        if (g_hw.box_flag == BOX_PORTABLE) s |= (1 << 4);
        *response = s;
        return 0;
    }
    case 0x6E756273UL: /* 'nubs' */ *response = (int32_t)g_hw.nubus_slots; return 0;
    case 0x736C7463UL: /* 'sltc' */ *response = (int32_t)g_hw.nubus_slot_mask; return 0;
    case 0x71642020UL: /* 'qd  ' */
        if (g_hw.has_color_qd) *response = g_hw.is_32bit_capable ? 0x230 : 0x100;
        else                   *response = 0;
        return 0;
    case 0x71647277UL: /* 'qdrw' */
        *response = g_hw.has_color_qd ? ((1 << 0) | (1 << 1) | (1 << 2)) : 0;
        return 0;
    case 0x736E6420UL: /* 'snd ' */
        *response = g_hw.has_asc ? ((1 << 0) | (1 << 1) | (1 << 3)) : 0;
        return 0;
    case 0x736E6877UL: /* 'snhw' */
        *response = g_hw.has_asc ? (int32_t)0x61736320UL /* 'asc ' */
                                 : (int32_t)0x636C6173UL /* 'clas' */;
        return 0;
    case 0x6B626420UL: /* 'kbd ' */ *response = g_hw.has_adb ? 5 : 3; return 0;
    case 0x746D6772UL: /* 'tmgr' */ *response = 3; return 0; /* gestaltExtendedTimeMgr */
    case 0x766D2020UL: /* 'vm  ' */ *response = 0; return 0;
    case 0x66732020UL: /* 'fs  ' */ *response = (g_flags & PLUSOS_HAVE_FS) ? 3 : 0; return 0;
    case 0x6E6D6772UL: /* 'nmgr' */ *response = 1; return 0;
    case 0x706F7772UL: /* 'powr' */
        *response = (g_hw.box_flag >= BOX_PORTABLE && g_hw.box_flag <= BOX_POWERBOOK160) ? 1 : 0;
        return 0;
    case 0x706F7021UL: /* 'pop!' */ *response = 1; return 0;
    default:
        break;
    }

    if (g_hw.has_gestalt_trap) {
        int32_t r = 0;
        int rc = hw_call_rom_gestalt(sel, &r);
        if (rc == 0) { *response = r; return 0; }
        return rc;
    }
    return -5551; /* gestaltUndefSelectorErr */
}

/* ---- ShutDown Manager ---------------------------------------------------- */
int sys7_shutdown_install(sys7_shutdown_fn fn, void *arg, int flags)
{
    if (!fn) return -1;
    for (int i = 0; i < MAX_SD_HOOKS; i++) {
        if (!s_sd_hooks[i].used) {
            s_sd_hooks[i].used = 1;
            s_sd_hooks[i].fn = fn;
            s_sd_hooks[i].arg = arg;
            s_sd_hooks[i].flags = flags;
            return 0;
        }
    }
    return -1;
}

int sys7_shutdown_remove(sys7_shutdown_fn fn)
{
    for (int i = 0; i < MAX_SD_HOOKS; i++) {
        if (s_sd_hooks[i].used && s_sd_hooks[i].fn == fn) {
            s_sd_hooks[i].used = 0;
            return 0;
        }
    }
    return -1;
}

void sys7_run_shutdown_hooks(int is_restart)
{
    int want = is_restart ? SYS7_SD_RESTART : SYS7_SD_POWEROFF;
    for (int i = MAX_SD_HOOKS - 1; i >= 0; i--) {
        if (s_sd_hooks[i].used && (s_sd_hooks[i].flags & want))
            s_sd_hooks[i].fn(s_sd_hooks[i].arg, is_restart);
    }
}

/* ---- Extended Time Manager ----------------------------------------------- */
int sys7_time_install(sys7_timer_fn fn, void *arg)
{
    if (!fn) return -1;
    uint16_t sr = int_disable();
    for (int i = 0; i < MAX_SYS7_TIMERS; i++) {
        if (!s_timers[i].used) {
            s_timers[i].used = 1;
            s_timers[i].armed = 0;
            s_timers[i].fn = fn;
            s_timers[i].arg = arg;
            s_timers[i].remain_us = 0;
            int_restore(sr);
            return i;
        }
    }
    int_restore(sr);
    return -1;
}

int sys7_time_prime(int id, int32_t delay)
{
    if (id < 0 || id >= MAX_SYS7_TIMERS) return -1;
    uint16_t sr = int_disable();
    if (!s_timers[id].used) { int_restore(sr); return -1; }
    /* TimeMgr convention: positive = ms, negative = microseconds */
    int32_t us = (delay < 0) ? -delay : (delay * 1000L);
    if (us < 1) us = 1;
    s_timers[id].remain_us = us;
    s_timers[id].armed = 1;
    int_restore(sr);
    return 0;
}

int sys7_time_remove(int id)
{
    if (id < 0 || id >= MAX_SYS7_TIMERS) return -1;
    uint16_t sr = int_disable();
    s_timers[id].armed = 0;
    s_timers[id].used = 0;
    int_restore(sr);
    return 0;
}

/* ---- Deferred Task Manager ----------------------------------------------- */
int sys7_dt_install(sys7_dtask_fn fn, void *arg)
{
    if (!fn) return -2;
    uint16_t sr = int_disable();
    int nxt = (s_dt_tail + 1) % MAX_SYS7_DTASKS;
    if (nxt == s_dt_head) { int_restore(sr); return -1; }
    s_dt_q[s_dt_tail].fn = fn;
    s_dt_q[s_dt_tail].arg = arg;
    s_dt_tail = nxt;
    int_restore(sr);
    return 0;
}

int sys7_dt_pending(void)
{
    int diff = s_dt_tail - s_dt_head;
    return diff < 0 ? diff + MAX_SYS7_DTASKS : diff;
}

/* ---- Notification Manager ------------------------------------------------ */
int sys7_nm_post(const char *msg, int beep)
{
    if (!msg) return -1;
    uint16_t sr = int_disable();
    if (s_nm_count >= MAX_SYS7_NOTICES) {
        s_nm_head = (s_nm_head + 1) % MAX_SYS7_NOTICES;
        s_nm_count--;
    }
    strlcpy(s_notices[s_nm_tail].msg, msg, sizeof s_notices[s_nm_tail].msg);
    s_notices[s_nm_tail].used = 1;
    s_nm_tail = (s_nm_tail + 1) % MAX_SYS7_NOTICES;
    s_nm_count++;
    int_restore(sr);
    klog(6, "notify: %s", msg);
    if (beep) sound_beep(6);
    return 0;
}

int sys7_nm_poll(char *buf, int buf_sz)
{
    if (!buf || buf_sz <= 0) return -1;
    uint16_t sr = int_disable();
    if (s_nm_count == 0) { int_restore(sr); return -1; }
    strlcpy(buf, s_notices[s_nm_head].msg, (size_t)buf_sz);
    s_notices[s_nm_head].used = 0;
    s_nm_head = (s_nm_head + 1) % MAX_SYS7_NOTICES;
    s_nm_count--;
    int_restore(sr);
    return 0;
}

int sys7_nm_count(void) { return s_nm_count; }

/* ---- 60.15 Hz VBL tick hook (16626 us per tick) & safe idle drain -------- */
void sys7_vbl_tick(void)
{
    for (int i = 0; i < MAX_SYS7_TIMERS; i++) {
        if (s_timers[i].used && s_timers[i].armed) {
            s_timers[i].remain_us -= 16626L;
            if (s_timers[i].remain_us <= 0) {
                s_timers[i].armed = 0;
                sys7_dt_install((sys7_dtask_fn)s_timers[i].fn, s_timers[i].arg);
            }
        }
    }
}

void sys7_poll(void)
{
    for (;;) {
        uint16_t sr = int_disable();
        if (s_dt_head == s_dt_tail) { int_restore(sr); break; }
        Sys7DTask t = s_dt_q[s_dt_head];
        s_dt_head = (s_dt_head + 1) % MAX_SYS7_DTASKS;
        int_restore(sr);
        if (t.fn) t.fn(t.arg);
    }
}
