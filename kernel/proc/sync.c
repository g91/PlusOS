/* sync.c - PlusOS Semaphores and Mutexes Synchronization Primitives */
#include "kernel.h"
#include "proc.h"
#include "sync.h"

typedef struct {
    int  val;
    int  max_val;
    int  active;
    int  owner_pid;
} Semaphore;

typedef struct {
    int  locked;
    int  holder_pid;
    int  active;
    int  owner_pid;
} Mutex;

static Semaphore sems[MAX_SEMAPHORES];
static Mutex     mutexes[MAX_MUTEXES];

void sync_init(void)
{
    memset(sems, 0, sizeof sems);
    memset(mutexes, 0, sizeof mutexes);
}

int sem_create(int init_val, int max_val, int owner_pid)
{
    uint16_t sr = int_disable();
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (!sems[i].active) {
            sems[i].active = 1;
            sems[i].val = init_val;
            sems[i].max_val = max_val > 0 ? max_val : 32767;
            sems[i].owner_pid = owner_pid;
            int_restore(sr);
            return i;
        }
    }
    int_restore(sr);
    return -1;
}

int sem_wait(int sem_id)
{
    if (sem_id < 0 || sem_id >= MAX_SEMAPHORES) return -1;
    Semaphore *s = &sems[sem_id];
    for (;;) {
        uint16_t sr = int_disable();
        if (!s->active) { int_restore(sr); return -1; }
        if (s->val > 0) {
            s->val--;
            int_restore(sr);
            return 0;
        }
        proc_sleep(s);
        int_restore(sr);
    }
}

int sem_post(int sem_id)
{
    if (sem_id < 0 || sem_id >= MAX_SEMAPHORES) return -1;
    Semaphore *s = &sems[sem_id];
    uint16_t sr = int_disable();
    if (!s->active) { int_restore(sr); return -1; }
    if (s->val < s->max_val) s->val++;
    proc_wakeup(s);
    int_restore(sr);
    return 0;
}

int sem_destroy(int sem_id)
{
    if (sem_id < 0 || sem_id >= MAX_SEMAPHORES) return -1;
    uint16_t sr = int_disable();
    sems[sem_id].active = 0;
    proc_wakeup(&sems[sem_id]);
    int_restore(sr);
    return 0;
}

int mutex_create(int owner_pid)
{
    uint16_t sr = int_disable();
    for (int i = 0; i < MAX_MUTEXES; i++) {
        if (!mutexes[i].active) {
            mutexes[i].active = 1;
            mutexes[i].locked = 0;
            mutexes[i].holder_pid = 0;
            mutexes[i].owner_pid = owner_pid;
            int_restore(sr);
            return i;
        }
    }
    int_restore(sr);
    return -1;
}

int mutex_lock(int mutex_id)
{
    if (mutex_id < 0 || mutex_id >= MAX_MUTEXES) return -1;
    Mutex *m = &mutexes[mutex_id];
    int cur_pid = proc_current ? proc_current->pid : 0;
    for (;;) {
        uint16_t sr = int_disable();
        if (!m->active) { int_restore(sr); return -1; }
        if (!m->locked) {
            m->locked = 1;
            m->holder_pid = cur_pid;
            int_restore(sr);
            return 0;
        }
        if (m->holder_pid == cur_pid) {
            int_restore(sr);
            return 0; /* Recursive re-entry */
        }
        proc_sleep(m);
        int_restore(sr);
    }
}

int mutex_unlock(int mutex_id)
{
    if (mutex_id < 0 || mutex_id >= MAX_MUTEXES) return -1;
    Mutex *m = &mutexes[mutex_id];
    uint16_t sr = int_disable();
    if (!m->active || !m->locked) { int_restore(sr); return -1; }
    int cur_pid = proc_current ? proc_current->pid : 0;
    if (m->holder_pid != cur_pid && cur_pid != 0) {
        int_restore(sr);
        return -1;
    }
    m->locked = 0;
    m->holder_pid = 0;
    proc_wakeup(m);
    int_restore(sr);
    return 0;
}

int mutex_destroy(int mutex_id)
{
    if (mutex_id < 0 || mutex_id >= MAX_MUTEXES) return -1;
    uint16_t sr = int_disable();
    mutexes[mutex_id].active = 0;
    proc_wakeup(&mutexes[mutex_id]);
    int_restore(sr);
    return 0;
}

void sync_proc_exit(int pid)
{
    uint16_t sr = int_disable();
    for (int i = 0; i < MAX_MUTEXES; i++) {
        if (mutexes[i].active && mutexes[i].holder_pid == pid) {
            mutexes[i].locked = 0;
            mutexes[i].holder_pid = 0;
            proc_wakeup(&mutexes[i]);
        }
        if (mutexes[i].active && mutexes[i].owner_pid == pid) {
            mutexes[i].active = 0;
            proc_wakeup(&mutexes[i]);
        }
    }
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (sems[i].active && sems[i].owner_pid == pid) {
            sems[i].active = 0;
            proc_wakeup(&sems[i]);
        }
    }
    int_restore(sr);
}

void sync_gc(int *freed_count)
{
    int freed = 0;
    uint16_t sr = int_disable();
    for (int i = 0; i < MAX_MUTEXES; i++) {
        if (mutexes[i].active && mutexes[i].owner_pid > 0) {
            int alive = 0;
            for (int p = 0; p < MAX_PROC; p++) {
                Proc *pr = proc_table_entry(p);
                if (pr && pr->pid == mutexes[i].owner_pid && pr->state != PROC_UNUSED && pr->state != PROC_ZOMBIE) {
                    alive = 1; break;
                }
            }
            if (!alive) {
                mutexes[i].active = 0;
                proc_wakeup(&mutexes[i]);
                freed++;
            }
        }
    }
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (sems[i].active && sems[i].owner_pid > 0) {
            int alive = 0;
            for (int p = 0; p < MAX_PROC; p++) {
                Proc *pr = proc_table_entry(p);
                if (pr && pr->pid == sems[i].owner_pid && pr->state != PROC_UNUSED && pr->state != PROC_ZOMBIE) {
                    alive = 1; break;
                }
            }
            if (!alive) {
                sems[i].active = 0;
                proc_wakeup(&sems[i]);
                freed++;
            }
        }
    }
    int_restore(sr);
    if (freed_count) *freed_count = freed;
}
