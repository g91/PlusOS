/* time.h - POSIX time & date conversion */
#ifndef PLUSOS_TIME_H
#define PLUSOS_TIME_H
#include <sys/types.h>
#ifdef __cplusplus
extern "C" {
#endif

struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

struct timespec {
    time_t tv_sec;
    long   tv_nsec;
};

time_t time(time_t *tloc);
int    stime(const time_t *t);
struct tm *gmtime(const time_t *timep);
struct tm *localtime(const time_t *timep);
time_t mktime(struct tm *tm);
size_t strftime(char *s, size_t max, const char *fmt, const struct tm *tm);
int    nanosleep(const struct timespec *req, struct timespec *rem);

#ifdef __cplusplus
}
#endif
#endif
