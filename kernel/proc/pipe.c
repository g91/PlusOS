/* pipe.c - PlusOS Kernel FIFO Inter-Process Communication Pipes */
#include "kernel.h"
#include "proc.h"
#include "pipe.h"

typedef struct {
    uint8_t  buf[PIPE_BUF_SIZE];
    uint16_t head;
    uint16_t tail;
    uint16_t count;
    uint8_t  active;
    uint8_t  readers;
    uint8_t  writers;
    int16_t  owner_pid;
} Pipe;

static Pipe pipes[MAX_PIPES];

void pipe_init(void)
{
    memset(pipes, 0, sizeof pipes);
}

int pipe_create(int *read_fd, int *write_fd, int owner_pid)
{
    if (!read_fd || !write_fd) return -1;
    uint16_t sr = int_disable();
    for (int i = 0; i < MAX_PIPES; i++) {
        if (!pipes[i].active) {
            pipes[i].active = 1;
            pipes[i].head = 0;
            pipes[i].tail = 0;
            pipes[i].count = 0;
            pipes[i].readers = 1;
            pipes[i].writers = 1;
            pipes[i].owner_pid = (int16_t)owner_pid;
            *read_fd = i;
            *write_fd = i;
            int_restore(sr);
            return i;
        }
    }
    int_restore(sr);
    return -1;
}

int pipe_read(int pipe_id, void *buf, int count, int nonblock)
{
    if (pipe_id < 0 || pipe_id >= MAX_PIPES || !buf || count <= 0) return -1;
    Pipe *p = &pipes[pipe_id];
    if (!p->active) return -1;

    uint8_t *dst = (uint8_t *)buf;
    int read_bytes = 0;

    while (read_bytes < count) {
        uint16_t sr = int_disable();
        if (p->count > 0) {
            dst[read_bytes++] = p->buf[p->head];
            p->head = (p->head + 1) % PIPE_BUF_SIZE;
            p->count--;
            proc_wakeup(&p->count);
            int_restore(sr);
        } else {
            if (p->writers == 0) {
                int_restore(sr);
                break; /* EOF */
            }
            if (nonblock || read_bytes > 0) {
                int_restore(sr);
                break;
            }
            proc_sleep(&p->count);
            int_restore(sr);
        }
    }

    return read_bytes;
}

int pipe_write(int pipe_id, const void *buf, int count, int nonblock)
{
    if (pipe_id < 0 || pipe_id >= MAX_PIPES || !buf || count <= 0) return -1;
    Pipe *p = &pipes[pipe_id];
    if (!p->active) return -1;
    if (p->readers == 0) return -1; /* Broken pipe */

    const uint8_t *src = (const uint8_t *)buf;
    int written_bytes = 0;

    while (written_bytes < count) {
        uint16_t sr = int_disable();
        if (p->count < PIPE_BUF_SIZE) {
            p->buf[p->tail] = src[written_bytes++];
            p->tail = (p->tail + 1) % PIPE_BUF_SIZE;
            p->count++;
            proc_wakeup(&p->count);
            int_restore(sr);
        } else {
            if (p->readers == 0) {
                int_restore(sr);
                return -1;
            }
            if (nonblock || written_bytes > 0) {
                int_restore(sr);
                break;
            }
            proc_sleep(&p->count);
            int_restore(sr);
        }
    }

    return written_bytes;
}

int pipe_close(int pipe_id)
{
    if (pipe_id < 0 || pipe_id >= MAX_PIPES) return -1;
    uint16_t sr = int_disable();
    Pipe *p = &pipes[pipe_id];
    if (p->active) {
        p->active = 0;
        p->readers = 0;
        p->writers = 0;
        p->count = 0;
        proc_wakeup(&p->count);
    }
    int_restore(sr);
    return 0;
}

void pipe_proc_exit(int pid)
{
    uint16_t sr = int_disable();
    for (int i = 0; i < MAX_PIPES; i++) {
        if (pipes[i].active && pipes[i].owner_pid == pid) {
            pipes[i].active = 0;
            proc_wakeup(&pipes[i].count);
        }
    }
    int_restore(sr);
}

void pipe_gc(int *closed_count)
{
    int closed = 0;
    uint16_t sr = int_disable();
    for (int i = 0; i < MAX_PIPES; i++) {
        if (pipes[i].active) {
            int owner_alive = 0;
            for (int p = 0; p < MAX_PROC; p++) {
                Proc *pr = proc_table_entry(p);
                if (pr && pr->pid == pipes[i].owner_pid && pr->state != PROC_UNUSED && pr->state != PROC_ZOMBIE) {
                    owner_alive = 1;
                    break;
                }
            }
            if (!owner_alive && pipes[i].owner_pid > 0) {
                pipes[i].active = 0;
                proc_wakeup(&pipes[i].count);
                closed++;
            }
        }
    }
    int_restore(sr);
    if (closed_count) *closed_count = closed;
}
