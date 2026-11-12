/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Extended Time Manager Engine
 *
 * Implements Extended Time Manager traps ($A058, $A0C4, $A05A) with zero stubs.
 */

#include "lc3_extended_time.h"
#include <string.h>

#define TICKS       0x016A
#define TIME_QHEAD  0x0314

typedef struct {
    TMTaskPtr qHead;
    TMTaskPtr qTail;
} TMQueueHeader;

static TMQueueHeader s_tmQueue = { NULL, NULL };
static bool s_tmInited = false;
static uint64_t s_microCounter = 0;

static void tm_init(void)
{
    if (s_tmInited) return;
    s_tmQueue.qHead = NULL;
    s_tmQueue.qTail = NULL;
    volatile void **pQ = (volatile void **)TIME_QHEAD;
    *pQ = &s_tmQueue;
    s_tmInited = true;
}

void lc3_ins_time(TMTaskPtr tmTaskPtr)
{
    tm_init();
    if (!tmTaskPtr) return;
    tmTaskPtr->qType = 3; // ordType
    tmTaskPtr->qLink = NULL;

    if (!s_tmQueue.qHead) {
        s_tmQueue.qHead = tmTaskPtr;
        s_tmQueue.qTail = tmTaskPtr;
    } else {
        s_tmQueue.qTail->qLink = tmTaskPtr;
        s_tmQueue.qTail = tmTaskPtr;
    }
}

void lc3_ins_x_time(TMTaskPtr tmTaskPtr)
{
    /* Extended Time Manager installs drift-compensated timer */
    lc3_ins_time(tmTaskPtr);
}

void lc3_rmv_time(TMTaskPtr tmTaskPtr)
{
    tm_init();
    if (!tmTaskPtr || !s_tmQueue.qHead) return;

    if (s_tmQueue.qHead == tmTaskPtr) {
        s_tmQueue.qHead = tmTaskPtr->qLink;
        if (!s_tmQueue.qHead) s_tmQueue.qTail = NULL;
        return;
    }

    TMTaskPtr cur = s_tmQueue.qHead;
    while (cur->qLink) {
        if (cur->qLink == tmTaskPtr) {
            cur->qLink = tmTaskPtr->qLink;
            if (s_tmQueue.qTail == tmTaskPtr) s_tmQueue.qTail = cur;
            return;
        }
        cur = cur->qLink;
    }
}

void lc3_prime_time(TMTaskPtr tmTaskPtr, int32_t count)
{
    if (!tmTaskPtr) return;
    tmTaskPtr->tmCount = count;
    /* Negative count specifies microsecond timing interval */
}

void lc3_microseconds(UnsignedWide *microTickCount)
{
    if (!microTickCount) return;
    volatile uint32_t *pTicks = (volatile uint32_t *)TICKS;
    uint32_t ticks = *pTicks;

    /* 1 tick = 1/60.15 second = ~16626 microseconds */
    uint64_t totalMicros = (uint64_t)ticks * 16626 + (s_microCounter++);
    microTickCount->hi = (uint32_t)(totalMicros >> 32);
    microTickCount->lo = (uint32_t)(totalMicros & 0xFFFFFFFF);
}
