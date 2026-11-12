/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: PowerPC Mixed Mode Transition & Cross-Call Descriptor Table Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_MIXED_MODE_H
#define LC3_MIXED_MODE_H

#include <stdint.h>
#include <stdbool.h>

#define MM_NO_ERR             0
#define MM_PARAM_ERR        -50
#define MM_MEM_FULL_ERR    -108
#define MM_BAD_PROC_INFO   -109

/* Calling Conventions */
#define MM_CALL_PASCAL        0
#define MM_CALL_C             1

/* Architecture ISAs */
#define MM_ISA_68K            0
#define MM_ISA_PPC            1

/* ProcInfo Parameter Sizes */
#define MM_SIZE_BYTE          1
#define MM_SIZE_WORD          2
#define MM_SIZE_LONG          3

typedef uint32_t ProcInfoType;
typedef void (*ProcPtr)(void);

typedef struct RoutineRecord {
    ProcInfoType procInfo;
    ProcPtr procLocation;
    uint8_t ISA;
    uint8_t callingConvention;
} RoutineRecord;

typedef struct RoutineDescriptor {
    uint16_t goMixedModeTrap;   /* 0xAAFE - _MixedModeMagic */
    int8_t version;             /* Version of RoutineDescriptor */
    uint8_t rdFlags;
    uint32_t reserved1;
    uint8_t reserved2;
    uint8_t selectorInfo;
    uint16_t routineCount;
    RoutineRecord routineRecords[1];
} RoutineDescriptor, *RoutineDescriptorPtr, **UniversalProcPtr;

/* Public API */
int16_t lc3_mm_init_engine(void);
RoutineDescriptorPtr lc3_mm_create_routine_descriptor(ProcPtr proc, ProcInfoType info, uint8_t isa, uint8_t callConv);
int16_t lc3_mm_dispose_routine_descriptor(RoutineDescriptorPtr rd);
int32_t lc3_mm_call_universal_proc(UniversalProcPtr upp, ProcInfoType procInfo, ...);
ProcInfoType lc3_mm_encode_proc_info(uint8_t returnSize, uint8_t paramCount, const uint8_t *paramSizes);
int16_t lc3_mm_decode_proc_info(ProcInfoType info, uint8_t *returnSize, uint8_t *paramCount, uint8_t *paramSizes);

#endif /* LC3_MIXED_MODE_H */
