/* posix.h - PlusOS POSIX Process, Identity, Time, and Terminal Support */
#ifndef PLUSOS_POSIX_H
#define PLUSOS_POSIX_H

#include <plusos/syscall.h>

void posix_init(void);
long posix_syscall(struct plusos_posix_args *a);

int  exec_spawn_fds(const char *name, int argc, char **argv, const int fds_override[3]);

#endif
