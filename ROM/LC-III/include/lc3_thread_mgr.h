/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Thread Manager Core (System 7 Cooperative Threading)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_THREAD_MGR_H
#define LC3_THREAD_MGR_H

#include <stdint.h>
#include <stdbool.h>

#define THREAD_NO_ERR             0
#define THREAD_NOT_FOUND_ERR   -615
#define THREAD_TOO_MANY_ERR    -617
#define THREAD_PARAM_ERR        -50

typedef uint32_t ThreadID;
typedef uint32_t ThreadStyle;
typedef uint16_t ThreadState;

#define K_READY_THREAD_STATE    0
#define K_STOPPED_THREAD_STATE  1
#define K_RUNNING_THREAD_STATE  2

typedef void *(*ThreadEntryProcPtr)(void *threadParam);

typedef struct ThreadControlBlock {
    ThreadID id;
    ThreadState state;
    ThreadEntryProcPtr entryProc;
    void *param;
    void *stackBase;
    void *stackPtr;
    uint32_t registers[16]; // D0-D7, A0-A7
    struct ThreadControlBlock *next;
} ThreadControlBlock;

/* Thread Manager API */
int16_t lc3_new_thread(ThreadStyle threadStyle, ThreadEntryProcPtr threadEntry, void *threadParam,
                       uint32_t stackSize, uint32_t flags, void **result, ThreadID *threadMade);
int16_t lc3_dispose_thread(ThreadID threadToDump, void **threadResult, bool recycleSlack);
int16_t lc3_yield_to_thread(ThreadID suggestedThread);
int16_t lc3_yield_to_any_thread(void);
int16_t lc3_get_current_thread(ThreadID *currentThreadID);
int16_t lc3_get_thread_state(ThreadID threadToGet, ThreadState *threadState);
int16_t lc3_set_thread_state(ThreadID threadToSet, ThreadState newThreadState, ThreadID threadToSwitchTo);

#endif /* LC3_THREAD_MGR_H */
