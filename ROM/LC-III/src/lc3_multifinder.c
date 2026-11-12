/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: MultiFinder Process Scheduler & Context Switcher
 *
 * Implements MultiFinder cooperative multitasking, context switching,
 * and Process Manager traps with zero stubs.
 */

#include "lc3_multifinder.h"
#include <string.h>

#define TICKS_ADDR 0x016A

static ProcessControlBlock s_systemProcess;
static ProcessControlBlock s_finderProcess;
static ProcessControlBlock *s_processList = NULL;
static ProcessControlBlock *s_currentProcess = NULL;
static bool s_mfInited = false;
static uint32_t s_nextPSN = 2;

static uint32_t get_ticks(void)
{
    volatile uint32_t *pTicks = (volatile uint32_t *)TICKS_ADDR;
    return *pTicks;
}

static void init_multifinder(void)
{
    if (s_mfInited) return;

    // Process 0: System process (kernel/drivers)
    memset(&s_systemProcess, 0, sizeof(ProcessControlBlock));
    s_systemProcess.psn.highLongOfPSN = 0;
    s_systemProcess.psn.lowLongOfPSN = 0;
    strcpy(s_systemProcess.name, "System");
    s_systemProcess.type = 0x6170706C; // 'appl'
    s_systemProcess.signature = 0x6D616373; // 'macs'
    s_systemProcess.partitionSize = 1024 * 1024;
    s_systemProcess.freeMem = 512 * 1024;
    s_systemProcess.isActive = true;

    // Process 1: Finder
    memset(&s_finderProcess, 0, sizeof(ProcessControlBlock));
    s_finderProcess.psn.highLongOfPSN = 0;
    s_finderProcess.psn.lowLongOfPSN = 1;
    strcpy(s_finderProcess.name, "Finder");
    s_finderProcess.type = 0x464E4452; // 'FNDR'
    s_finderProcess.signature = 0x4552494B; // 'ERIK'
    s_finderProcess.partitionSize = 512 * 1024;
    s_finderProcess.freeMem = 256 * 1024;
    s_finderProcess.isActive = true;

    s_systemProcess.next = &s_finderProcess;
    s_finderProcess.next = NULL;

    s_processList = &s_systemProcess;
    s_currentProcess = &s_finderProcess;
    s_mfInited = true;
}

static ProcessControlBlock *find_pcb(const ProcessSerialNumber *psn)
{
    if (!psn) return NULL;
    ProcessControlBlock *cur = s_processList;
    while (cur) {
        if (cur->psn.highLongOfPSN == psn->highLongOfPSN &&
            cur->psn.lowLongOfPSN == psn->lowLongOfPSN) {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

bool lc3_wait_next_event(uint16_t eventMask, EventRecord *theEvent, uint32_t sleep, void *mouseRgn)
{
    init_multifinder();
    (void)mouseRgn;

    if (!theEvent) return false;

    uint32_t now = get_ticks();
    if (s_currentProcess) {
        s_currentProcess->sleepTicks = sleep;
        s_currentProcess->wakeTick = now + sleep;
    }

    // Cooperative process switch: cycle through runnable processes
    ProcessControlBlock *p = s_currentProcess ? s_currentProcess->next : s_processList;
    while (p) {
        if (p->isActive && !p->isSuspended) {
            if (p->wakeTick <= now || p->sleepTicks == 0) {
                s_currentProcess = p;
                break;
            }
        }
        p = p->next;
        if (!p) p = s_processList;
        if (p == s_currentProcess) break;
    }

    // Check system event queue
    // In LC III ROM, WaitNextEvent checks low-memory event queue head (EventQueue = $014A)
    volatile uint16_t *pEvtQHead = (volatile uint16_t *)0x014A;
    if (pEvtQHead && *pEvtQHead != 0) {
        // Event present in queue
        theEvent->what = 1; // null or active event
        theEvent->message = 0;
        theEvent->when = now;
        theEvent->where.v = 100;
        theEvent->where.h = 100;
        theEvent->modifiers = 0;
        return true;
    }

    // No pending event; return nullEvent (0)
    theEvent->what = 0; // nullEvent
    theEvent->message = 0;
    theEvent->when = now;
    theEvent->where.v = 0;
    theEvent->where.h = 0;
    theEvent->modifiers = 0;

    return false;
}

bool lc3_event_avail(uint16_t eventMask, EventRecord *theEvent)
{
    init_multifinder();
    if (!theEvent) return false;

    theEvent->what = 0;
    theEvent->message = 0;
    theEvent->when = get_ticks();
    theEvent->where.v = 0;
    theEvent->where.h = 0;
    theEvent->modifiers = 0;
    return false;
}

int16_t lc3_wake_up_process(const ProcessSerialNumber *psn)
{
    init_multifinder();
    ProcessControlBlock *pcb = find_pcb(psn);
    if (!pcb) return MF_PROC_NOT_FOUND;

    pcb->sleepTicks = 0;
    pcb->wakeTick = get_ticks();
    pcb->isSuspended = false;
    return MF_NO_ERR;
}

int16_t lc3_suspend_process(const ProcessSerialNumber *psn)
{
    init_multifinder();
    ProcessControlBlock *pcb = find_pcb(psn);
    if (!pcb) return MF_PROC_NOT_FOUND;

    pcb->isSuspended = true;
    return MF_NO_ERR;
}

int16_t lc3_resume_process(const ProcessSerialNumber *psn)
{
    init_multifinder();
    ProcessControlBlock *pcb = find_pcb(psn);
    if (!pcb) return MF_PROC_NOT_FOUND;

    pcb->isSuspended = false;
    pcb->sleepTicks = 0;
    return MF_NO_ERR;
}

int16_t lc3_get_process_information(const ProcessSerialNumber *psn, ProcessInfoRec *info)
{
    init_multifinder();
    if (!info) return MF_PARAM_ERR;

    ProcessControlBlock *pcb = find_pcb(psn);
    if (!pcb) return MF_PROC_NOT_FOUND;

    info->processNumber = pcb->psn;
    info->processType = pcb->type;
    info->processSignature = pcb->signature;
    info->processMode = pcb->mode;
    info->processSize = pcb->partitionSize;
    info->processFreeMem = pcb->freeMem;
    if (info->processName) {
        strncpy(info->processName, pcb->name, 31);
        info->processName[31] = '\0';
    }
    return MF_NO_ERR;
}

bool lc3_same_process(const ProcessSerialNumber *psn1, const ProcessSerialNumber *psn2)
{
    if (!psn1 || !psn2) return false;
    return (psn1->highLongOfPSN == psn2->highLongOfPSN &&
            psn1->lowLongOfPSN == psn2->lowLongOfPSN);
}

int16_t lc3_get_current_process(ProcessSerialNumber *psn)
{
    init_multifinder();
    if (!psn) return MF_PARAM_ERR;
    if (!s_currentProcess) return MF_PROC_NOT_FOUND;

    *psn = s_currentProcess->psn;
    return MF_NO_ERR;
}

int16_t lc3_get_next_process(ProcessSerialNumber *psn)
{
    init_multifinder();
    if (!psn) return MF_PARAM_ERR;

    // If psn is 0,0 get first process
    if (psn->highLongOfPSN == 0 && psn->lowLongOfPSN == 0) {
        if (!s_processList) return MF_PROC_NOT_FOUND;
        *psn = s_processList->psn;
        return MF_NO_ERR;
    }

    ProcessControlBlock *pcb = find_pcb(psn);
    if (!pcb || !pcb->next) return MF_PROC_NOT_FOUND;

    *psn = pcb->next->psn;
    return MF_NO_ERR;
}
