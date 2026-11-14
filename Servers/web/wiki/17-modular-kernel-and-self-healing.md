# Modular Kernel: Drivers, Services & Self-Healing

PlusOS is built as a **modular kernel**. Instead of one monolithic blob that lives or dies together, each subsystem is a self-contained **kernel module** with its own lifecycle, supervised at run time so that the failure of one module cannot take down the machine. This mirrors the way a modern kernel keeps itself alive by isolating and recovering its subsystems rather than panicking.

The framework lives in `kernel/core/module.c` / `module.h`, with the inter-module communication fabric in `kernel/core/kpipe.c` / `kpipe.h`.

---

## Modules: Drivers and Services

Every module is described by a `KModule` record. There are three classes:

| Class | Purpose | Examples |
|---|---|---|
| **`KMOD_CORE`** | Essential kernel service; a permanent failure is fatal | `klog` (event log ring) |
| **`KMOD_DRIVER`** | Hardware driver | `video`, `kbd`, `sound` |
| **`KMOD_SERVICE`** | Kernel-land application | `syslog`, `supervisor` |

A module author provides up to four callbacks; the framework owns everything else:

```c
typedef struct KModule {
    const char *name;        /* unique, short, e.g. "video" */
    const char *desc;
    uint8_t     cls;         /* KMOD_CORE | KMOD_DRIVER | KMOD_SERVICE */
    uint8_t     critical;    /* 1: a permanent failure is fatal        */
    uint8_t     autostart;   /* start during boot                      */
    uint8_t     restart_policy;

    int   (*init)(KModule *);    /* bring the subsystem up (0 = ok)     */
    void  (*fini)(KModule *);    /* tear down for stop/restart          */
    int   (*health)(KModule *);  /* liveness probe, 0 = healthy         */
    void  (*work)(KModule *);    /* a body run as its own kernel task   */
    /* ...runtime fields owned by the framework... */
} KModule;
```

A **driver** typically supplies only `init` (and optionally `health`) and becomes `RUNNING` once it attaches to its hardware. A **service** supplies a `work` body, which the framework runs as a dedicated Ring-0 kernel task — usually a loop that waits on a pipeline.

---

## Lifecycle & State Machine

```text
  register ──▶ REGISTERED ──start──▶ RUNNING ──┬── stop ──▶ STOPPED
                   ▲                            │
                   │                       (fault / unhealthy)
                   │                            ▼
                   └────────── restart ───── FAILED
```

| State | Meaning |
|---|---|
| `KMOD_REGISTERED` | Known to the registry, not yet started |
| `KMOD_RUNNING` | `init()` succeeded; worker (if any) is alive |
| `KMOD_FAILED` | `init()` failed, the worker faulted, or `health()` went unresponsive |
| `KMOD_STOPPED` | Cleanly stopped |

The public API:

```c
int  module_register(KModule *m);
int  module_start(KModule *m);
int  module_stop(KModule *m);
int  module_restart(KModule *m);
void module_start_all(void);        /* start every autostart module at boot */
```

At boot, `kmain()` calls `module_init()`, `module_register_builtins()` and `module_start_all()` before the scheduler is handed control:

```text
[30] module_init: modular kernel (drivers, services, pipelines)...
     module 'supervisor' [service] running
     module 'syslog'     [service] running
     module 'sound'      [driver]  running
     module 'kbd'        [driver]  running
     module 'video'      [driver]  running
     module 'klog'       [core]    running
     modules: 6 registered (supervised)
```

---

## Pipelines (kpipe): How Modules Talk

Modules never call into each other's internals. They communicate over named **pipelines** — bounded, in-kernel message channels — so a module can be restarted without any other module holding a dangling pointer to it.

```c
KPipe *kpipe_create(const char *name);          /* idempotent */
KPipe *kpipe_find(const char *name);
int    kpipe_send(KPipe *p, const void *data, int len);  /* non-blocking */
int    kpipe_tryrecv(KPipe *p, void *buf, int max);      /* non-blocking */
int    kpipe_recv(KPipe *p, void *buf, int max);         /* blocks for a message */
```

Each pipe is a fixed ring of small, length-delimited messages. `kpipe_send` is non-blocking and wakes any blocked reader; `kpipe_recv` sleeps on the pipe (via the scheduler's `proc_sleep` / `proc_wakeup`) until a message arrives. Interrupts are masked around the ring update so a driver can post an event from an interrupt context safely.

**Worked example — the system log pipeline.** The built-in `syslog` service does nothing but drain the `"log"` pipe into the kernel event log:

```c
static void syslog_work(KModule *m)
{
    KPipe *in = kpipe_find("log");
    char msg[KPIPE_MSG_MAX + 1];
    for (;;) {
        int n = kpipe_recv(in, msg, KPIPE_MSG_MAX);
        if (n < 0) { proc_sleep_ticks(1); continue; }
        msg[n] = 0;
        klog(PLUSOS_LOG_INFO, "syslog: %s", msg);
    }
}
```

Any driver or service can now log by posting to a pipe it never has to know the owner of:

```c
const char s[] = "supervisor online";
kpipe_send(kpipe_find("log"), s, sizeof s - 1);
```

which appears in the boot log, proving the end-to-end path `supervisor → "log" pipe → syslog`:

```text
[0.68] syslog: supervisor online
```

---

## The Supervisor: Self-Healing

The **`supervisor`** is itself a service module — the kernel supervises itself as just another module. Once per second it probes every module and recovers any that failed:

```c
void module_supervisor_tick(void)
{
    uint32_t now = tick_count();
    for (KModule *m = ...; m; m = m->next) {
        if (m->state == KMOD_RUNNING && m->health) {
            if (m->health(m) == 0) m->last_beat = now;
            else if (now - m->last_beat > 300)      /* ~5 s unresponsive */
                m->state = KMOD_FAILED;
        }
        if (m->state == KMOD_FAILED &&
            m->restart_policy != KMOD_RESTART_NEVER) {
            if (m->restarts < 10) module_restart(m);  /* bounded: no restart storm */
        }
    }
}
```

Restart policies are `KMOD_RESTART_NEVER`, `KMOD_RESTART_ONFAIL`, and `KMOD_RESTART_ALWAYS`. The supervisor caps automatic restarts at **10** attempts, after which a persistently-failing module is left in `FAILED` rather than looping forever.

### Demonstrated recovery

A built-in self-test module (`canary`, compiled in with `-DPLUSOS_MODTEST`) reports a failure every two seconds. The log shows the supervisor isolating and restarting it over and over while the shell and every other module keep running:

```text
[2.73] canary: reporting a module failure
[3.48] module 'canary' restarting (attempt 1) → running
[5.53] canary: reporting a module failure
[5.56] module 'canary' restarting (attempt 2) → running
[7.65] canary: reporting a module failure
[7.66] module 'canary' restarting (attempt 3) → running
...
```

---

## Fault Isolation: A Crash Is Not a Panic

Module recovery is tied into the kernel's exception handler (`kernel/cpu/crash.c`). When a fault strikes:

- **A Ring-2 application** (desktop, shell, a game) is terminated and reaped; the kernel keeps running every other process and `init` respawns the shell.
- **A module's worker task** (Ring-0) is flagged via `module_mark_failed(pid)` so the supervisor restarts it — the rest of the kernel is untouched.
- Only a fault with nothing left to isolate drops to the recovery shell.

The handler is **re-entrancy guarded**: if rendering the on-screen panel or writing the crash dump itself faults, the guard drops the risky step and still recovers, instead of spiralling into a double fault and resetting the machine. See **[18. In-Kernel Crash Debugger & Post-Mortem Dumps](/wiki/18-crash-debugger-and-post-mortem-dumps)**.

---

## Why This Matters on a 68000

The compact Macs have no MMU, so PlusOS cannot rely on hardware memory protection to wall modules off from each other. The modular framework provides the next best thing: **structural** isolation. Each subsystem has a defined boundary (its callbacks and its pipes), a known health signal, and a supervisor empowered to restart it. The result is a kernel that treats the failure of a driver or service as a recoverable event rather than a fatal one — the behavior you expect from a modern operating system, running on 1986 hardware.

---

## Navigation

- **Previous Chapter**: [← 16. Macintosh LC III ROM Reverse-Engineering & Port](/wiki/16-macintosh-lc3-rom-decompilation-and-port)
- **Next Chapter**: [18. In-Kernel Crash Debugger & Post-Mortem Dumps →](/wiki/18-crash-debugger-and-post-mortem-dumps)
- **Table of Contents**: [Return to Getting Started](/wiki/01-getting-started)
