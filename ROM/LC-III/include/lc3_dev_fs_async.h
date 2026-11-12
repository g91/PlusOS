/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Device Manager High-Level File System Asynchronous Dispatch
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_DEV_FS_ASYNC_H
#define LC3_DEV_FS_ASYNC_H

#include <stdint.h>
#include <stdbool.h>

#define ASYNC_NO_ERR         0
#define ASYNC_QUEUE_FULL  -3015
#define ASYNC_PARAM_ERR     -50

typedef struct AsyncIOQueueElement {
    void *qLink;
    int16_t qType;
    int16_t ioTrap;
    void (*ioCompletion)(void *paramBlock);
    int16_t ioResult;
    void *paramBlock;
    bool isPending;
} AsyncIOQueueElement;

/* Async Dispatch API */
int16_t lc3_async_queue_post(void *paramBlock, void (*completion)(void *pb), int16_t ioTrap);
void lc3_async_queue_process(void);
void lc3_async_completion_dispatch(AsyncIOQueueElement *elem);
void lc3_async_interrupt_hook(void);
bool lc3_check_pending_io(void);

#endif /* LC3_DEV_FS_ASYNC_H */
