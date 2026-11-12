/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: PowerPC Mixed Mode Transition & Cross-Call Descriptor Table Engine
 *
 * Implements RoutineDescriptor creation, parameter encoding, stack marshaling,
 * and universal proc calling with zero stubs.
 */

#include "lc3_mixed_mode.h"
#include <stdarg.h>

#define MAX_STATIC_RDS 32

static RoutineDescriptor s_staticRDs[MAX_STATIC_RDS];
static bool s_rdUsed[MAX_STATIC_RDS];
static bool s_mmInitialized = false;

/* 1434: lc3_mm_init_engine */
int16_t lc3_mm_init_engine(void)
{
    for (int i = 0; i < MAX_STATIC_RDS; ++i) {
        s_rdUsed[i] = false;
    }
    s_mmInitialized = true;
    return MM_NO_ERR;
}

/* 1435: lc3_mm_create_routine_descriptor */
RoutineDescriptorPtr lc3_mm_create_routine_descriptor(ProcPtr proc, ProcInfoType info, uint8_t isa, uint8_t callConv)
{
    if (!proc) return 0;
    if (!s_mmInitialized) lc3_mm_init_engine();

    int freeIdx = -1;
    for (int i = 0; i < MAX_STATIC_RDS; ++i) {
        if (!s_rdUsed[i]) {
            freeIdx = i;
            break;
        }
    }

    if (freeIdx == -1) return 0;

    RoutineDescriptor *rd = &s_staticRDs[freeIdx];
    s_rdUsed[freeIdx] = true;

    rd->goMixedModeTrap = 0xAAFE; /* _MixedModeMagic opcode */
    rd->version = 7;             /* System 7.1 / System 7 Pro */
    rd->rdFlags = 0;
    rd->reserved1 = 0;
    rd->reserved2 = 0;
    rd->selectorInfo = 0;
    rd->routineCount = 1;

    rd->routineRecords[0].procInfo = info;
    rd->routineRecords[0].procLocation = proc;
    rd->routineRecords[0].ISA = isa;
    rd->routineRecords[0].callingConvention = callConv;

    return rd;
}

/* 1436: lc3_mm_dispose_routine_descriptor */
int16_t lc3_mm_dispose_routine_descriptor(RoutineDescriptorPtr rd)
{
    if (!rd) return MM_PARAM_ERR;

    for (int i = 0; i < MAX_STATIC_RDS; ++i) {
        if (&s_staticRDs[i] == rd) {
            s_rdUsed[i] = false;
            return MM_NO_ERR;
        }
    }

    return MM_PARAM_ERR;
}

/* 1437: lc3_mm_call_universal_proc */
int32_t lc3_mm_call_universal_proc(UniversalProcPtr upp, ProcInfoType procInfo, ...)
{
    if (!upp) return 0;

    RoutineDescriptorPtr rd = *upp;
    if (!rd || rd->goMixedModeTrap != 0xAAFE) {
        /* Fallback: if not an RD pointer, invoke as standard 680x0 proc pointer */
        typedef int32_t (*RawFunc)(void);
        RawFunc fn = (RawFunc)upp;
        return fn();
    }

    /* Target procedure is in 680x0 code */
    ProcPtr target = rd->routineRecords[0].procLocation;
    if (!target) return 0;

    typedef int32_t (*TargetFunc)(uint32_t, uint32_t, uint32_t, uint32_t);
    TargetFunc fn = (TargetFunc)target;

    va_list args;
    va_start(args, procInfo);

    uint8_t retSize = 0, paramCount = 0;
    uint8_t paramSizes[16];
    lc3_mm_decode_proc_info(procInfo, &retSize, &paramCount, paramSizes);

    uint32_t p0 = (paramCount > 0) ? va_arg(args, uint32_t) : 0;
    uint32_t p1 = (paramCount > 1) ? va_arg(args, uint32_t) : 0;
    uint32_t p2 = (paramCount > 2) ? va_arg(args, uint32_t) : 0;
    uint32_t p3 = (paramCount > 3) ? va_arg(args, uint32_t) : 0;

    va_end(args);

    return fn(p0, p1, p2, p3);
}

/* 1438: lc3_mm_encode_proc_info */
ProcInfoType lc3_mm_encode_proc_info(uint8_t returnSize, uint8_t paramCount, const uint8_t *paramSizes)
{
    if (paramCount > 12) paramCount = 12;

    /* Bits 0-3: return size (0=void, 1=byte, 2=word, 3=long) */
    ProcInfoType info = (returnSize & 0x0F);

    /* Bits 4-5 reserved */
    /* Bits 6 onwards: 2 bits per parameter size */
    for (uint8_t i = 0; i < paramCount; ++i) {
        uint8_t sz = paramSizes ? (paramSizes[i] & 0x03) : MM_SIZE_LONG;
        info |= ((ProcInfoType)sz << (6 + (i * 2)));
    }

    return info;
}

/* 1439: lc3_mm_decode_proc_info */
int16_t lc3_mm_decode_proc_info(ProcInfoType info, uint8_t *returnSize, uint8_t *paramCount, uint8_t *paramSizes)
{
    if (!returnSize || !paramCount || !paramSizes) {
        return MM_PARAM_ERR;
    }

    *returnSize = (uint8_t)(info & 0x0F);

    uint8_t count = 0;
    for (uint8_t i = 0; i < 12; ++i) {
        uint8_t sz = (uint8_t)((info >> (6 + (i * 2))) & 0x03);
        if (sz != 0) {
            paramSizes[i] = sz;
            count = i + 1;
        } else {
            paramSizes[i] = 0;
        }
    }

    *paramCount = count;
    return MM_NO_ERR;
}
