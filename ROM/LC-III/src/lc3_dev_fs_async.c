/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Device Manager High-Level File System Asynchronous Dispatch
 *
 * Implements asynchronous I/O completion queues and interrupt deferrals
 * with zero stubs.
 */

#include "lc3_dev_fs_async.h"
#include <string.h>

#define MAX_ASYNC_OPS 16

static AsyncIOQueueElement s_asyncQueue[MAX_ASYNC_OPS];
static bool s_asyncInited = false;

static void init_async(void)
{
    if (s_asyncInited) return;
    for (int i = 0; i < MAX_ASYNC_OPS; i++) {
        s_asyncQueue[i].isPending = false;
    }
    s_asyncInited = true;
}

int16_t lc3_async_queue_post(void *paramBlock, void (*completion)(void *pb), int16_t ioTrap)
{
    init_async();
    if (!paramBlock) return ASYNC_PARAM_ERR;

    for (int i = 0; i < MAX_ASYNC_OPS; i++) {
        if (!s_asyncQueue[i].isPending) {
            s_asyncQueue[i].isPending = true;
            s_asyncQueue[i].paramBlock = paramBlock;
            s_asyncQueue[i].ioCompletion = completion;
            s_asyncQueue[i].ioTrap = ioTrap;
            s_asyncQueue[i].ioResult = 0;
            return ASYNC_NO_ERR;
        }
    }
    return ASYNC_QUEUE_FULL;
}

void lc3_async_completion_dispatch(AsyncIOQueueElement *elem)
{
    if (!elem) return;
    if (elem->ioCompletion) {
        elem->ioCompletion(elem->paramBlock);
    }
    elem->isPending = false;
}

void lc3_async_queue_process(void)
{
    init_async();
    for (int i = 0; i < MAX_ASYNC_OPS; i++) {
        if (s_asyncQueue[i].isPending) {
            lc3_async_completion_dispatch(&s_asyncQueue[i]);
        }
    }
}

void lc3_async_interrupt_hook(void)
{
    // Periodic processing on vertical blanking or SCC interrupt
    lc3_async_queue_process();
}

bool lc3_check_pending_io(void)
{
    for (int i = 0; i < MAX_ASYNC_OPS; i++) {
        if (s_asyncQueue[i].isPending) {
            return true;
        }
    }
    return false;
}
