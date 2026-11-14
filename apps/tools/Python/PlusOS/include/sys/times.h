#ifndef PLUSOS_SYS_TIMES_H
#define PLUSOS_SYS_TIMES_H
#include <time.h>
struct tms { clock_t tms_utime, tms_stime, tms_cutime, tms_cstime; };
clock_t times(struct tms *buf);
#endif
