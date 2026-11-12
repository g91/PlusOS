/* tick.c - the PlusOS time source and preemption signal.
 *
 * We install a VBL task through the ROM's Vertical Retrace Manager (_VInstall).
 * It fires at the ~60.15 Hz vertical-blank interrupt, which is our scheduler
 * tick: it counts time, wakes timed sleepers and raises want_resched. The low
 * byte of Ticks (0x16A) also advances, but our own counter is independent of
 * anything else that might reset it.
 */
#include "kernel.h"
#include "proc.h"
#include "profiler.h"
#include "sys7.h"
#include <plusrom/VRetraceMgr.h>

extern void tick_vbl(void);              /* tick.S */
extern void sched_on_tick(void);         /* proc.c */
extern void timer_vbl_tick(void);

static volatile uint32_t ticks;

/* The VBL task record the ROM manager walks. Must stay allocated forever. */
static VBLTask vbl;

uint32_t tick_count(void) { return ticks; }

/* called from tick.S at interrupt level */
void tick_c(void)
{
    ticks++;
    /* NOTE: we do NOT touch StkLowPt ($0110) here. The ROM's own VBL stack
     * sniffer raises SysError 28 ("stack ran into heap")
     * whenever the stack pointer is below the application zone's BkLim. In
     * PlusOS every process runs on a kernel/user stack that we kmalloc from the
     * application heap, so the SP is *always* below BkLim by design - the
     * sniffer's invariant simply does not apply to us. The sniffer is gated on
     * StkLowPt != 0 (it self-skips when StkLowPt is 0), and the ROM maintains
     * StkLowPt itself, so writing our heap-resident SP into it only armed the
     * sniffer to fire against our own stacks. tick_init() clears StkLowPt once
     * to keep the sniffer disabled (the same `clr.l StkLowPt` idiom the ROM
     * uses in SCSIMgr/BTreeMgr to suspend sniffing). */
    kwd_tick();                         /* watchdog: counts ticks since the heartbeat last ran */
    sound_vbl_tick();                   /* (no-op unless CONFIG_SOUND) */
    sys7_vbl_tick();                    /* Extended TimeMgr / DeferredTasks */
    sched_on_tick();
    timer_vbl_tick();
    profiler_vbl_tick();                /* (no-op unless CONFIG_PROFILER) */
}

void tick_init(void)
{
    /* Disable the ROM's VBL stack sniffer: PlusOS process stacks live inside
     * the application heap, so its "stack below heap => SysError 28" check must
     * never run. A zero StkLowPt makes the sniffer self-skip every frame. */
    LM_LONG(0x110) = 0;                   /* StkLowPt */

    memset(&vbl, 0, sizeof vbl);
    vbl.qType = 1;                       /* vType: a VBL task */
    vbl.vblAddr = (ProcPtr)tick_vbl;
    vbl.vblCount = 1;                    /* fire on the next frame */
    vbl.vblPhase = 0;
    VInstall(&vbl);
}
