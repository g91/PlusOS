/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Component Manager Extended Discovery & Component Registry
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_COMPONENT_REGISTRY_H
#define LC3_COMPONENT_REGISTRY_H

#include <stdint.h>
#include <stdbool.h>

#define CMP_NO_ERR         0
#define CMP_NOT_FOUND   -3005
#define CMP_PARAM_ERR     -50

typedef void *Component;
typedef void *ComponentInstance;

typedef struct ComponentDescription {
    uint32_t componentType;
    uint32_t componentSubType;
    uint32_t componentManufacturer;
    uint32_t componentFlags;
    uint32_t componentFlagsMask;
} ComponentDescription;

typedef struct ComponentRecord {
    ComponentDescription desc;
    char name[64];
    void *entryPoint;
    bool inUse;
} ComponentRecord;

/* Component Registry API */
Component lc3_register_component_resource(const ComponentDescription *desc, int16_t resID, int16_t global);
int16_t lc3_register_component_resource_file(int16_t resRefNum, int16_t global);
Component lc3_find_next_component(Component prev, const ComponentDescription *looking);
int32_t lc3_count_components(const ComponentDescription *looking);
int16_t lc3_get_component_info(Component aComponent, ComponentDescription *desc, char *componentName);
ComponentInstance lc3_open_component(Component aComponent);
int16_t lc3_close_component(ComponentInstance aComponentInstance);
void *lc3_component_function_lookup(ComponentInstance aComponentInstance, int16_t selector);

#endif /* LC3_COMPONENT_REGISTRY_H */
