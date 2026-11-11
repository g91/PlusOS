/* shm.h - PlusOS Shared Memory Subsystem */
#ifndef PLUSOS_SHM_H
#define PLUSOS_SHM_H

#include <stdint.h>
#include <plusos/syscall.h>
#include "kconfig.h"

#define MAX_SHM_REGIONS 8
#define MAX_SHM_SIZE    4096

#if CONFIG_SHM
void  shm_init(void);
int   shm_get(int key, uint32_t size, int owner_pid);
void *shm_at(int shmid);
int   shm_dt(int shmid);
void  shm_proc_exit(int pid);
#else
static inline void  shm_init(void) {}
static inline int   shm_get(int k, uint32_t s, int p) { (void)k; (void)s; (void)p; return -1; }
static inline void *shm_at(int id) { (void)id; return 0; }
static inline int   shm_dt(int id) { (void)id; return -1; }
static inline void  shm_proc_exit(int p) { (void)p; }
#endif

#endif