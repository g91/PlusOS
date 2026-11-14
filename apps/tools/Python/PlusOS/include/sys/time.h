#ifndef PLUSOS_SYS_TIME_H
#define PLUSOS_SYS_TIME_H
#include <time.h>
struct timeval { long tv_sec; long tv_usec; };
struct timezone { int tz_minuteswest; int tz_dsttime; };
struct itimerval { struct timeval it_interval; struct timeval it_value; };
#define ITIMER_REAL 0
int gettimeofday(struct timeval *tv, struct timezone *tz);
int clock_gettime(int clk, struct timespec *ts);
int clock_getres(int clk, struct timespec *ts);
#endif
