/* thread.h - PlusOS threads: several flows of execution inside one program.
 *
 * Threads share all of the program's memory (globals, heap) and each has its
 * own stack. The kernel schedules every thread on its own and preempts it when
 * its time slice runs out, so a busy thread never stalls the others. When the
 * program's main thread ends, its other threads end too.
 *
 *   static gmutex_t lock = GMUTEX_INIT;
 *   static void worker(void *arg) { ... gmutex_lock(&lock); ... gmutex_unlock(&lock); }
 *   gthread_t t = gthread_create(worker, 0);
 *   ...
 *   gthread_join(t);
 *
 * Shared data must be protected with a gmutex_t. Mutexes are recursive: the
 * thread that holds one can lock it again (unlock the same number of times).
 * PlusGFX keeps one drawing target and clip per program, so threads that draw
 * should take a common lock around their drawing.
 */
#ifndef PLUSOS_THREAD_H
#define PLUSOS_THREAD_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int gthread_t;

typedef struct {
    volatile uint8_t locked;        /* set atomically (bset) */
    volatile int     owner;         /* thread id of the holder */
    volatile int     depth;         /* recursion count */
} gmutex_t;
#define GMUTEX_INIT { 0, 0, 0 }

/* Start fn(arg) on a new thread with a 4 KB stack. Returns its id, or -1.
 * Join it with gthread_join, or gthread_detach it so it cleans up by itself. */
gthread_t gthread_create(void (*fn)(void *), void *arg);
gthread_t gthread_create_ex(void (*fn)(void *), void *arg, int stack_bytes, int detached);
int       gthread_join(gthread_t t);        /* wait for it to end; its exit code */
void      gthread_detach(gthread_t t);
void      gthread_exit(int code) __attribute__((noreturn));
void      gthread_yield(void);
void      gthread_sleep(int ticks);         /* 60 ticks = 1 second */
gthread_t gthread_self(void);

void      gmutex_init(gmutex_t *m);
void      gmutex_lock(gmutex_t *m);
int       gmutex_trylock(gmutex_t *m);      /* 1 if acquired */
void      gmutex_unlock(gmutex_t *m);

#ifdef __cplusplus
}
#endif
#endif
