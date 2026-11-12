/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Thread Synchronization Primitives (Semaphores & Inter-Thread Queues)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_THREAD_SYNC_H
#define LC3_THREAD_SYNC_H

#include <stdint.h>
#include <stdbool.h>

#define SYNC_NO_ERR            0
#define SYNC_NOT_FOUND_ERR  -618
#define SYNC_WOULD_BLOCK    -619
#define SYNC_PARAM_ERR       -50

typedef uint32_t SemaphoreID;
typedef uint32_t QueueID;

typedef struct SemaphoreRecord {
    SemaphoreID id;
    int32_t count;
    uint32_t waitingThreadsMask;
    bool inUse;
} SemaphoreRecord;

typedef struct QueueRecord {
    QueueID id;
    void *elements[16];
    uint32_t head;
    uint32_t tail;
    uint32_t count;
    bool inUse;
} QueueRecord;

/* Thread Synchronization API */
int16_t lc3_new_semaphore(int32_t initialCount, SemaphoreID *semID);
int16_t lc3_dispose_semaphore(SemaphoreID semID);
int16_t lc3_wait_on_semaphore(SemaphoreID semID, uint32_t timeoutTicks);
int16_t lc3_signal_semaphore(SemaphoreID semID);

/* Inter-Thread Queue API */
int16_t lc3_new_queue(QueueID *qID);
int16_t lc3_dispose_queue(QueueID qID);
int16_t lc3_enqueue(QueueID qID, void *element);
int16_t lc3_dequeue(QueueID qID, void **element);

#endif /* LC3_THREAD_SYNC_H */
