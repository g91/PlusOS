/* utime.h - POSIX file timestamps */
#ifndef PLUSOS_UTIME_H
#define PLUSOS_UTIME_H
#include <sys/types.h>
#ifdef __cplusplus
extern "C" {
#endif

struct utimbuf {
    time_t actime;
    time_t modtime;
};

int utime(const char *filename, const struct utimbuf *times);

#ifdef __cplusplus
}
#endif
#endif
