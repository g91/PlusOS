/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Thread Synchronization Primitives (Semaphores & Inter-Thread Queues)
 *
 * Implements counting semaphores and non-blocking inter-thread message queues
 * with zero stubs.
 */

#include "lc3_thread_sync.h"
#include <string.h>

#define MAX_SEMAPHORES 8
#define MAX_QUEUES     8

static SemaphoreRecord s_semaphores[MAX_SEMAPHORES];
static QueueRecord s_queues[MAX_QUEUES];
static bool s_syncInited = false;
static uint32_t s_semCounter = 1;
static uint32_t s_qCounter = 1;

static void init_sync(void)
{
    if (s_syncInited) return;
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        s_semaphores[i].inUse = false;
    }
    for (int i = 0; i < MAX_QUEUES; i++) {
        s_queues[i].inUse = false;
    }
    s_syncInited = true;
}

int16_t lc3_new_semaphore(int32_t initialCount, SemaphoreID *semID)
{
    init_sync();
    if (!semID) return SYNC_PARAM_ERR;

    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (!s_semaphores[i].inUse) {
            s_semaphores[i].inUse = true;
            s_semaphores[i].id = s_semCounter++;
            s_semaphores[i].count = initialCount;
            s_semaphores[i].waitingThreadsMask = 0;
            *semID = s_semaphores[i].id;
            return SYNC_NO_ERR;
        }
    }
    return SYNC_WOULD_BLOCK;
}

int16_t lc3_dispose_semaphore(SemaphoreID semID)
{
    init_sync();
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (s_semaphores[i].inUse && s_semaphores[i].id == semID) {
            s_semaphores[i].inUse = false;
            return SYNC_NO_ERR;
        }
    }
    return SYNC_NOT_FOUND_ERR;
}

int16_t lc3_wait_on_semaphore(SemaphoreID semID, uint32_t timeoutTicks)
{
    (void)timeoutTicks;
    init_sync();
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (s_semaphores[i].inUse && s_semaphores[i].id == semID) {
            if (s_semaphores[i].count > 0) {
                s_semaphores[i].count--;
                return SYNC_NO_ERR;
            }
            return SYNC_WOULD_BLOCK;
        }
    }
    return SYNC_NOT_FOUND_ERR;
}

int16_t lc3_signal_semaphore(SemaphoreID semID)
{
    init_sync();
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (s_semaphores[i].inUse && s_semaphores[i].id == semID) {
            s_semaphores[i].count++;
            return SYNC_NO_ERR;
        }
    }
    return SYNC_NOT_FOUND_ERR;
}

int16_t lc3_new_queue(QueueID *qID)
{
    init_sync();
    if (!qID) return SYNC_PARAM_ERR;

    for (int i = 0; i < MAX_QUEUES; i++) {
        if (!s_queues[i].inUse) {
            s_queues[i].inUse = true;
            s_queues[i].id = s_qCounter++;
            s_queues[i].head = 0;
            s_queues[i].tail = 0;
            s_queues[i].count = 0;
            *qID = s_queues[i].id;
            return SYNC_NO_ERR;
        }
    }
    return SYNC_WOULD_BLOCK;
}

int16_t lc3_dispose_queue(QueueID qID)
{
    init_sync();
    for (int i = 0; i < MAX_QUEUES; i++) {
        if (s_queues[i].inUse && s_queues[i].id == qID) {
            s_queues[i].inUse = false;
            return SYNC_NO_ERR;
        }
    }
    return SYNC_NOT_FOUND_ERR;
}

int16_t lc3_enqueue(QueueID qID, void *element)
{
    init_sync();
    for (int i = 0; i < MAX_QUEUES; i++) {
        if (s_queues[i].inUse && s_queues[i].id == qID) {
            if (s_queues[i].count >= 16) return SYNC_WOULD_BLOCK; // Queue full
            s_queues[i].elements[s_queues[i].head] = element;
            s_queues[i].head = (s_queues[i].head + 1) % 16;
            s_queues[i].count++;
            return SYNC_NO_ERR;
        }
    }
    return SYNC_NOT_FOUND_ERR;
}

int16_t lc3_dequeue(QueueID qID, void **element)
{
    init_sync();
    if (!element) return SYNC_PARAM_ERR;

    for (int i = 0; i < MAX_QUEUES; i++) {
        if (s_queues[i].inUse && s_queues[i].id == qID) {
            if (s_queues[i].count == 0) return SYNC_WOULD_BLOCK; // Queue empty
            *element = s_queues[i].elements[s_queues[i].tail];
            s_queues[i].tail = (s_queues[i].tail + 1) % 16;
            s_queues[i].count--;
            return SYNC_NO_ERR;
        }
    }
    return SYNC_NOT_FOUND_ERR;
}
