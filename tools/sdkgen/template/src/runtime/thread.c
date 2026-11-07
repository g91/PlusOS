/* thread.c - PlusOS threads and mutexes (see plusos/thread.h). */
#include <plusos/plusos.h>
#include <plusos/syscall.h>
#include <plusos/thread.h>

/* The kernel starts every thread here, as if called as entry(fn, arg). */
static void __plusos_thread_start(void (*fn)(void *), void *arg)
{
    fn(arg);
    plusos_syscall(SYS_exit, 0);
    for (;;) {}
}

gthread_t gthread_create_ex(void (*fn)(void *), void *arg, int stack_bytes, int detached)
{
    struct plusos_thread_args a = { (void *)__plusos_thread_start, (void *)fn, arg, stack_bytes };
    int t = (int)plusos_syscall(SYS_thread, &a);
    if (t > 0 && detached) plusos_syscall(SYS_detach, (void *)(intptr_t)t);
    return t;
}

gthread_t gthread_create(void (*fn)(void *), void *arg) { return gthread_create_ex(fn, arg, 4096, 0); }
int  gthread_join(gthread_t t)     { return (int)plusos_syscall(SYS_wait, (void *)(intptr_t)t); }
void gthread_detach(gthread_t t)   { plusos_syscall(SYS_detach, (void *)(intptr_t)t); }
void gthread_yield(void)           { plusos_syscall(SYS_yield, 0); }
void gthread_sleep(int ticks)      { plusos_syscall(SYS_sleep, (void *)(intptr_t)ticks); }
gthread_t gthread_self(void)       { return (gthread_t)plusos_syscall(SYS_getpid, 0); }

void gthread_exit(int code)
{
    plusos_syscall(SYS_exit, (void *)(intptr_t)code);
    for (;;) {}
}

/* ---- mutexes ---------------------------------------------------------------
 * bset is a single instruction, so it can't be split by an interrupt: on a
 * one-CPU machine it is an atomic test-and-set. A thread that finds the lock
 * taken yields so the holder can run and release it. */
static inline int test_and_set(volatile uint8_t *p)
{
    uint8_t was_clear;
    __asm__ volatile("bset #7, %1\n\tseq %0" : "=d"(was_clear), "+m"(*p) : : "cc");
    return was_clear != 0;
}

void gmutex_init(gmutex_t *m) { m->locked = 0; m->owner = 0; m->depth = 0; }

int gmutex_trylock(gmutex_t *m)
{
    int me = gthread_self();
    if (m->locked && m->owner == me) { m->depth++; return 1; }
    if (!test_and_set(&m->locked)) return 0;
    m->owner = me;
    m->depth = 1;
    return 1;
}

void gmutex_lock(gmutex_t *m)
{
    int me = gthread_self();
    if (m->locked && m->owner == me) { m->depth++; return; }
    while (!test_and_set(&m->locked)) gthread_yield();
    m->owner = me;
    m->depth = 1;
}

void gmutex_unlock(gmutex_t *m)
{
    if (--m->depth > 0) return;
    m->depth = 0;
    m->owner = 0;
    m->locked = 0;
}
