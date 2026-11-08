/* timer.c - PlusOS Software Timer and Alarm Subsystem */
#include "kernel.h"
#include "proc.h"
#include "timer.h"
#include "signal.h"

typedef struct {
    int  active;
    int  expire_tick;
    int  interval_ticks;
    int  signum;
    int  owner_pid;
} SoftTimer;

static SoftTimer timers[MAX_TIMERS];

void timer_subsys_init(void)
{
    memset(timers, 0, sizeof timers);
}

int timer_set(int delay_ticks, int interval_ticks, int signum, int owner_pid)
{
    if (delay_ticks <= 0) return -1;
    uint16_t sr = int_disable();
    for (int i = 0; i < MAX_TIMERS; i++) {
        if (!timers[i].active) {
            timers[i].active = 1;
            timers[i].expire_tick = (int)tick_count() + delay_ticks;
            timers[i].interval_ticks = interval_ticks > 0 ? interval_ticks : 0;
            timers[i].signum = signum > 0 ? signum : PLUSOS_SIGALRM;
            timers[i].owner_pid = owner_pid;
            int_restore(sr);
            return i;
        }
    }
    int_restore(sr);
    return -1;
}

int timer_cancel(int timer_id, int owner_pid)
{
    if (timer_id < 0 || timer_id >= MAX_TIMERS) return -1;
    uint16_t sr = int_disable();
    if (timers[timer_id].active && (timers[timer_id].owner_pid == owner_pid || owner_pid == 0)) {
        timers[timer_id].active = 0;
        int_restore(sr);
        return 0;
    }
    int_restore(sr);
    return -1;
}

int timer_alarm(int seconds, int owner_pid)
{
    /* Cancel existing alarm for this process */
    uint16_t sr = int_disable();
    int remaining = 0;
    int now = (int)tick_count();
    for (int i = 0; i < MAX_TIMERS; i++) {
        if (timers[i].active && timers[i].owner_pid == owner_pid && timers[i].signum == PLUSOS_SIGALRM) {
            remaining = (timers[i].expire_tick - now) / 60;
            if (remaining < 0) remaining = 0;
            timers[i].active = 0;
        }
    }
    if (seconds > 0) {
        timer_set(seconds * 60, 0, PLUSOS_SIGALRM, owner_pid);
    }
    int_restore(sr);
    return remaining;
}

void timer_vbl_tick(void)
{
    int now = (int)tick_count();
    for (int i = 0; i < MAX_TIMERS; i++) {
        if (timers[i].active && now - timers[i].expire_tick >= 0) {
            signal_send(timers[i].owner_pid, timers[i].signum);
            if (timers[i].interval_ticks > 0) {
                timers[i].expire_tick = now + timers[i].interval_ticks;
            } else {
                timers[i].active = 0;
            }
        }
    }
}

void timer_proc_exit(int pid)
{
    uint16_t sr = int_disable();
    for (int i = 0; i < MAX_TIMERS; i++) {
        if (timers[i].active && timers[i].owner_pid == pid) {
            timers[i].active = 0;
        }
    }
    int_restore(sr);
}
