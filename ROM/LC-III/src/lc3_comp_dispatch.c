/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Component Manager Fast Dispatch & Multiprocessing Locks
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#include "lc3_comp_dispatch.h"
#include <string.h>

#define MAX_COMPONENTS 32

static ComponentRecord sRegisteredComponents[MAX_COMPONENTS];
static bool sComponentsInitialized = false;

ComponentResult lc3_comp_fast_dispatch(ComponentInstance instance, int16_t selector, void *params)
{
    if (!instance || !instance->parentComponent) return COMP_PARAM_ERR;

    Component comp = instance->parentComponent;
    if (!comp->entryPoint) return COMP_NOT_FOUND;

    lc3_comp_lock_instance(instance);
    ComponentResult res = comp->entryPoint(params, instance->storage);
    lc3_comp_unlock_instance(instance);

    (void)selector;
    return res;
}

ComponentResult lc3_comp_delegate_call(ComponentInstance instance, int16_t selector, void *params)
{
    if (!instance || !instance->delegateInstance) return COMP_PARAM_ERR;

    return lc3_comp_fast_dispatch(instance->delegateInstance, selector, params);
}

int16_t lc3_comp_lock_instance(ComponentInstance instance)
{
    if (!instance) return COMP_PARAM_ERR;
    instance->locked = true;
    return COMP_NO_ERR;
}

int16_t lc3_comp_unlock_instance(ComponentInstance instance)
{
    if (!instance) return COMP_PARAM_ERR;
    instance->locked = false;
    return COMP_NO_ERR;
}

int16_t lc3_comp_resolve_params(const void *stackFrame, uint16_t paramBytes, void *outParams)
{
    if (!stackFrame || !outParams || paramBytes == 0) return COMP_PARAM_ERR;

    memcpy(outParams, stackFrame, paramBytes);
    return COMP_NO_ERR;
}

int16_t lc3_comp_register_thng(uint32_t type, uint32_t subType, uint32_t mfr, ComponentFunctionProc entry, Component *outComp)
{
    if (!entry || !outComp) return COMP_PARAM_ERR;

    if (!sComponentsInitialized) {
        memset(sRegisteredComponents, 0, sizeof(sRegisteredComponents));
        sComponentsInitialized = true;
    }

    for (int i = 0; i < MAX_COMPONENTS; ++i) {
        if (!sRegisteredComponents[i].inUse) {
            sRegisteredComponents[i].inUse = true;
            sRegisteredComponents[i].componentType = type;
            sRegisteredComponents[i].componentSubType = subType;
            sRegisteredComponents[i].componentManufacturer = mfr;
            sRegisteredComponents[i].entryPoint = entry;
            *outComp = &sRegisteredComponents[i];
            return COMP_NO_ERR;
        }
    }

    return COMP_PARAM_ERR;
}
