/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Component Manager Core
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_COMPONENT_MGR_H
#define LC3_COMPONENT_MGR_H

#include <stdint.h>
#include <stdbool.h>

#define CMP_NO_ERR           0
#define CMP_NOT_FOUND       -1
#define CMP_BAD_INST        -2

typedef uint32_t OSType;

typedef struct {
    OSType componentType;
    OSType componentSubType;
    OSType componentManufacturer;
    uint32_t componentFlags;
    uint32_t componentFlagsMask;
} ComponentDescription;

typedef void *Component;
typedef void *ComponentInstance;

/* Component Manager API */
Component lc3_register_component(const ComponentDescription *cd, void *routine, int16_t globalFlags, void *name, void *info, void *icon);
int16_t lc3_unregister_component(Component aComponent);
int32_t lc3_count_components(const ComponentDescription *looking);
Component lc3_find_next_component(Component aComponent, const ComponentDescription *looking);
int16_t lc3_get_component_info(Component aComponent, ComponentDescription *cd, void *name, void *info, void *icon);
ComponentInstance lc3_open_component(Component aComponent);
int16_t lc3_close_component(ComponentInstance aComponentInstance);
ComponentInstance lc3_open_default_component(OSType componentType, OSType componentSubType);
int32_t lc3_call_component_function(void *params, void *routine);

/* Component Dispatch ($A82A) */
int32_t lc3_component_dispatch(int16_t selector, void *params);

#endif /* LC3_COMPONENT_MGR_H */
