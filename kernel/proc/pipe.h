/* pipe.h - PlusOS Kernel FIFO Inter-Process Communication Pipes */
#ifndef PLUSOS_PIPE_H
#define PLUSOS_PIPE_H

#include <stdint.h>
#include <plusos/syscall.h>

#define MAX_PIPES 4
#define PIPE_BUF_SIZE 256

void pipe_init(void);
int  pipe_create(int *read_fd, int *write_fd, int owner_pid);
int  pipe_read(int pipe_id, void *buf, int count, int nonblock);
int  pipe_write(int pipe_id, const void *buf, int count, int nonblock);
int  pipe_close(int pipe_id);
void pipe_proc_exit(int pid);
void pipe_gc(int *closed_count);

#endif
