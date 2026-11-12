/* vfs.h - PlusOS POSIX Virtual File System & Per-Process File Descriptors */
#ifndef PLUSOS_VFS_H
#define PLUSOS_VFS_H

#include <stdint.h>
#include <plusos/syscall.h>

struct Proc;

#define MAC_UNIX_EPOCH_DELTA 2082844800UL

void vfs_init(void);
void vfs_proc_init(struct Proc *p, struct Proc *parent, const int fds_override[3]);
void vfs_proc_exit(struct Proc *p);

int  vfs_fd_is_console(int fd);
int  vfs_fd_read(int fd, void *buf, int32_t count);
int  vfs_fd_write(int fd, const void *buf, int32_t count);
int  vfs_fd_dup_to_child(int parent_fd);
int  vfs_pipe_alloc(int *rfd, int *wfd);

int  vfs_normalize_path(const char *in, char *out, int cap, int follow_last_link);
long vfs_syscall(struct plusos_vfs_args *a);

#endif
