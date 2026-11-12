/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Component Manager Fast Dispatch & Multiprocessing Locks
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_COMP_DISPATCH_H
#define LC3_COMP_DISPATCH_H

#include <stdint.h>
#include <stdbool.h>

#define COMP_NO_ERR         0
#define COMP_NOT_FOUND    -43
#define COMP_PARAM_ERR    -50

typedef int32_t ComponentResult;
typedef struct ComponentRecord *Component;
typedef struct ComponentInstanceRecord *ComponentInstance;

typedef ComponentResult (*ComponentFunctionProc)(void *params, void *storage);

typedef struct ComponentInstanceRecord {
    Component parentComponent;
    void *storage;
    bool locked;
    ComponentInstance delegateInstance;
} ComponentInstanceRecord;

typedef struct ComponentRecord {
    uint32_t componentType;
    uint32_t componentSubType;
    uint32_t componentManufacturer;
    ComponentFunctionProc entryPoint;
    bool inUse;
} ComponentRecord;

/* Component Dispatch API */
ComponentResult lc3_comp_fast_dispatch(ComponentInstance instance, int16_t selector, void *params);
ComponentResult lc3_comp_delegate_call(ComponentInstance instance, int16_t selector, void *params);
int16_t lc3_comp_lock_instance(ComponentInstance instance);
int16_t lc3_comp_unlock_instance(ComponentInstance instance);
int16_t lc3_comp_resolve_params(const void *stackFrame, uint16_t paramBytes, void *outParams);
int16_t lc3_comp_register_thng(uint32_t type, uint32_t subType, uint32_t mfr, ComponentFunctionProc entry, Component *outComp);

#endif /* LC3_COMP_DISPATCH_H */
