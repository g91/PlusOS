/* sync.h - PlusOS Semaphores and Mutexes Synchronization Primitives */
#ifndef PLUSOS_SYNC_H
#define PLUSOS_SYNC_H

#include <stdint.h>
#include <plusos/syscall.h>

#define MAX_SEMAPHORES 16
#define MAX_MUTEXES    16

void sync_init(void);

int  sem_create(int init_val, int max_val, int owner_pid);
int  sem_wait(int sem_id);
int  sem_post(int sem_id);
int  sem_destroy(int sem_id);

int  mutex_create(int owner_pid);
int  mutex_lock(int mutex_id);
int  mutex_unlock(int mutex_id);
int  mutex_destroy(int mutex_id);

void sync_proc_exit(int pid);
void sync_gc(int *freed_count);

#endif
