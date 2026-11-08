/* shm.c - PlusOS Shared Memory Subsystem */
#include "kernel.h"
#include "kmem.h"
#include "shm.h"

typedef struct {
    int      key;
    uint32_t size;
    void    *ptr;
    int      refcount;
    int      active;
    int      owner_pid;
} ShmRegion;

static ShmRegion shm_table[MAX_SHM_REGIONS];

void shm_init(void)
{
    memset(shm_table, 0, sizeof shm_table);
}

int shm_get(int key, uint32_t size, int owner_pid)
{
    if (size == 0 || size > MAX_SHM_SIZE) return -1;
    uint16_t sr = int_disable();

    /* Find existing by key */
    for (int i = 0; i < MAX_SHM_REGIONS; i++) {
        if (shm_table[i].active && shm_table[i].key == key) {
            shm_table[i].refcount++;
            int_restore(sr);
            return i;
        }
    }

    /* Create new */
    for (int i = 0; i < MAX_SHM_REGIONS; i++) {
        if (!shm_table[i].active) {
            void *p = kmem_alloc(size, 0);
            if (!p) {
                int_restore(sr);
                return -1;
            }
            memset(p, 0, size);
            shm_table[i].active = 1;
            shm_table[i].key = key;
            shm_table[i].size = size;
            shm_table[i].ptr = p;
            shm_table[i].refcount = 1;
            shm_table[i].owner_pid = owner_pid;
            int_restore(sr);
            return i;
        }
    }

    int_restore(sr);
    return -1;
}

void *shm_at(int shmid)
{
    if (shmid < 0 || shmid >= MAX_SHM_REGIONS) return 0;
    if (!shm_table[shmid].active) return 0;
    return shm_table[shmid].ptr;
}

int shm_dt(int shmid)
{
    if (shmid < 0 || shmid >= MAX_SHM_REGIONS) return -1;
    uint16_t sr = int_disable();
    if (shm_table[shmid].active) {
        if (shm_table[shmid].refcount > 0) shm_table[shmid].refcount--;
        if (shm_table[shmid].refcount == 0) {
            kmem_free(shm_table[shmid].ptr);
            shm_table[shmid].active = 0;
            shm_table[shmid].ptr = 0;
        }
    }
    int_restore(sr);
    return 0;
}

void shm_proc_exit(int pid)
{
    uint16_t sr = int_disable();
    for (int i = 0; i < MAX_SHM_REGIONS; i++) {
        if (shm_table[i].active && shm_table[i].owner_pid == pid) {
            shm_dt(i);
        }
    }
    int_restore(sr);
}
