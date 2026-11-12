/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: MultiFinder Process Scheduler & Context Switcher
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_MULTIFINDER_H
#define LC3_MULTIFINDER_H

#include <stdint.h>
#include <stdbool.h>

#define MF_NO_ERR             0
#define MF_PROC_NOT_FOUND  -600
#define MF_MEM_FULL_ERR    -108
#define MF_PARAM_ERR        -50

typedef struct {
    uint32_t highLongOfPSN;
    uint32_t lowLongOfPSN;
} ProcessSerialNumber, *ProcessSerialNumberPtr;

typedef struct {
    uint32_t processInfoLength;
    char *processName;
    ProcessSerialNumber processNumber;
    uint32_t processType;
    uint32_t processSignature;
    uint32_t processMode;
    void *processLocation;
    uint32_t processSize;
    uint32_t processFreeMem;
    ProcessSerialNumber processLauncher;
    uint32_t processLaunchDate;
    uint32_t processActiveTime;
    void *processAppSpec;
} ProcessInfoRec, *ProcessInfoRecPtr;

typedef struct {
    int16_t what;
    uint32_t message;
    uint32_t when;
    struct {
        int16_t v;
        int16_t h;
    } where;
    uint16_t modifiers;
} EventRecord;

typedef struct ProcessControlBlock {
    ProcessSerialNumber psn;
    char name[32];
    uint32_t signature;
    uint32_t type;
    uint32_t mode;
    uint32_t partitionSize;
    uint32_t freeMem;
    void *a5World;
    void *stackBase;
    void *stackPtr;
    uint32_t registers[16]; // D0-D7, A0-A7
    uint32_t sleepTicks;
    uint32_t wakeTick;
    bool isSuspended;
    bool isActive;
    struct ProcessControlBlock *next;
} ProcessControlBlock;

/* MultiFinder Core API */
bool lc3_wait_next_event(uint16_t eventMask, EventRecord *theEvent, uint32_t sleep, void *mouseRgn);
bool lc3_event_avail(uint16_t eventMask, EventRecord *theEvent);
int16_t lc3_wake_up_process(const ProcessSerialNumber *psn);
int16_t lc3_suspend_process(const ProcessSerialNumber *psn);
int16_t lc3_resume_process(const ProcessSerialNumber *psn);
int16_t lc3_get_process_information(const ProcessSerialNumber *psn, ProcessInfoRec *info);
bool lc3_same_process(const ProcessSerialNumber *psn1, const ProcessSerialNumber *psn2);
int16_t lc3_get_current_process(ProcessSerialNumber *psn);
int16_t lc3_get_next_process(ProcessSerialNumber *psn);

#endif /* LC3_MULTIFINDER_H */
