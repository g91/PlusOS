/* timer.h - PlusOS Software Timer and Alarm Subsystem */
#ifndef PLUSOS_TIMER_H
#define PLUSOS_TIMER_H

#include <stdint.h>
#include <plusos/syscall.h>

#define MAX_TIMERS 16

void timer_subsys_init(void);
int  timer_set(int delay_ticks, int interval_ticks, int signum, int owner_pid);
int  timer_cancel(int timer_id, int owner_pid);
int  timer_alarm(int seconds, int owner_pid);
void timer_vbl_tick(void);
void timer_proc_exit(int pid);

#endif
