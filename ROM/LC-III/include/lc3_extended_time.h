/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Extended Time Manager Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_EXTENDED_TIME_H
#define LC3_EXTENDED_TIME_H

#include <stdint.h>
#include <stdbool.h>

#define TM_NO_ERR         0
#define TM_PARAM_ERR    -50

typedef struct TMTask {
    struct TMTask *qLink;
    int16_t qType;
    void (*tmAddr)(struct TMTask *tmTaskPtr);
    int32_t tmCount;
    int32_t tmWakeUp;
    int32_t tmReserved;
} TMTask, *TMTaskPtr;

typedef struct {
    uint32_t hi;
    uint32_t lo;
} UnsignedWide;

/* Extended Time Manager API */
void lc3_ins_time(TMTaskPtr tmTaskPtr);
void lc3_ins_x_time(TMTaskPtr tmTaskPtr);
void lc3_rmv_time(TMTaskPtr tmTaskPtr);
void lc3_prime_time(TMTaskPtr tmTaskPtr, int32_t count);
void lc3_microseconds(UnsignedWide *microTickCount);

#endif /* LC3_EXTENDED_TIME_H */
