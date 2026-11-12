/* sys/utsname.h - POSIX system identification */
#ifndef PLUSOS_SYS_UTSNAME_H
#define PLUSOS_SYS_UTSNAME_H
#ifdef __cplusplus
extern "C" {
#endif

struct utsname {
    char sysname[24];
    char nodename[32];
    char release[24];
    char version[48];
    char machine[24];
};

int uname(struct utsname *buf);

#ifdef __cplusplus
}
#endif
#endif
