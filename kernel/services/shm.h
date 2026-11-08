/* shm.h - PlusOS Shared Memory Subsystem */
#ifndef PLUSOS_SHM_H
#define PLUSOS_SHM_H

#include <stdint.h>
#include <plusos/syscall.h>

#define MAX_SHM_REGIONS 8
#define MAX_SHM_SIZE    4096

void  shm_init(void);
int   shm_get(int key, uint32_t size, int owner_pid);
void *shm_at(int shmid);
int   shm_dt(int shmid);
void  shm_proc_exit(int pid);

#endif
