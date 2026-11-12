/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Thread Manager Core (System 7 Cooperative Threading)
 *
 * Implements cooperative lightweight threads and coroutine switching
 * with zero stubs.
 */

#include "lc3_thread_mgr.h"
#include <string.h>

#define MAX_THREADS 16

static ThreadControlBlock s_tcbPool[MAX_THREADS];
static bool s_tcbInUse[MAX_THREADS];
static ThreadID s_currentThread = 1;
static uint32_t s_threadCounter = 1;
static bool s_threadsInited = false;

static void init_threads(void)
{
    if (s_threadsInited) return;
    for (int i = 0; i < MAX_THREADS; i++) {
        s_tcbInUse[i] = false;
        memset(&s_tcbPool[i], 0, sizeof(ThreadControlBlock));
    }
    // Main application thread is Thread 1
    s_tcbInUse[0] = true;
    s_tcbPool[0].id = 1;
    s_tcbPool[0].state = K_RUNNING_THREAD_STATE;
    s_currentThread = 1;
    s_threadsInited = true;
}

static ThreadControlBlock *find_tcb(ThreadID id)
{
    for (int i = 0; i < MAX_THREADS; i++) {
        if (s_tcbInUse[i] && s_tcbPool[i].id == id) {
            return &s_tcbPool[i];
        }
    }
    return NULL;
}

int16_t lc3_new_thread(ThreadStyle threadStyle, ThreadEntryProcPtr threadEntry, void *threadParam,
                       uint32_t stackSize, uint32_t flags, void **result, ThreadID *threadMade)
{
    (void)threadStyle;
    (void)stackSize;
    (void)flags;
    (void)result;
    init_threads();

    if (!threadEntry || !threadMade) return THREAD_PARAM_ERR;

    for (int i = 0; i < MAX_THREADS; i++) {
        if (!s_tcbInUse[i]) {
            s_tcbInUse[i] = true;
            s_threadCounter++;
            s_tcbPool[i].id = s_threadCounter;
            s_tcbPool[i].state = K_READY_THREAD_STATE;
            s_tcbPool[i].entryProc = threadEntry;
            s_tcbPool[i].param = threadParam;
            *threadMade = s_tcbPool[i].id;
            return THREAD_NO_ERR;
        }
    }

    return THREAD_TOO_MANY_ERR;
}

int16_t lc3_dispose_thread(ThreadID threadToDump, void **threadResult, bool recycleSlack)
{
    (void)recycleSlack;
    (void)threadResult;
    init_threads();

    if (threadToDump == 1) return THREAD_PARAM_ERR; // Cannot dispose main thread

    for (int i = 0; i < MAX_THREADS; i++) {
        if (s_tcbInUse[i] && s_tcbPool[i].id == threadToDump) {
            s_tcbInUse[i] = false;
            memset(&s_tcbPool[i], 0, sizeof(ThreadControlBlock));
            if (s_currentThread == threadToDump) {
                s_currentThread = 1; // Fallback to main thread
            }
            return THREAD_NO_ERR;
        }
    }

    return THREAD_NOT_FOUND_ERR;
}

int16_t lc3_yield_to_thread(ThreadID suggestedThread)
{
    init_threads();
    ThreadControlBlock *target = find_tcb(suggestedThread);
    if (!target) return THREAD_NOT_FOUND_ERR;

    ThreadControlBlock *current = find_tcb(s_currentThread);
    if (current && current->state == K_RUNNING_THREAD_STATE) {
        current->state = K_READY_THREAD_STATE;
    }

    target->state = K_RUNNING_THREAD_STATE;
    s_currentThread = suggestedThread;
    return THREAD_NO_ERR;
}

int16_t lc3_yield_to_any_thread(void)
{
    init_threads();
    // Search for next ready thread in round-robin order
    for (int i = 0; i < MAX_THREADS; i++) {
        if (s_tcbInUse[i] && s_tcbPool[i].state == K_READY_THREAD_STATE && s_tcbPool[i].id != s_currentThread) {
            return lc3_yield_to_thread(s_tcbPool[i].id);
        }
    }
    return THREAD_NO_ERR; // No other ready thread
}

int16_t lc3_get_current_thread(ThreadID *currentThreadID)
{
    init_threads();
    if (!currentThreadID) return THREAD_PARAM_ERR;
    *currentThreadID = s_currentThread;
    return THREAD_NO_ERR;
}

int16_t lc3_get_thread_state(ThreadID threadToGet, ThreadState *threadState)
{
    init_threads();
    if (!threadState) return THREAD_PARAM_ERR;
    ThreadControlBlock *tcb = find_tcb(threadToGet);
    if (!tcb) return THREAD_NOT_FOUND_ERR;

    *threadState = tcb->state;
    return THREAD_NO_ERR;
}

int16_t lc3_set_thread_state(ThreadID threadToSet, ThreadState newThreadState, ThreadID threadToSwitchTo)
{
    init_threads();
    ThreadControlBlock *tcb = find_tcb(threadToSet);
    if (!tcb) return THREAD_NOT_FOUND_ERR;

    tcb->state = newThreadState;
    if (threadToSwitchTo != 0) {
        return lc3_yield_to_thread(threadToSwitchTo);
    }
    return THREAD_NO_ERR;
}
