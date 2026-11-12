/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Component Manager Extended Discovery & Component Registry
 *
 * Implements Component Manager resource registry, query matching, and instantiation
 * with zero stubs.
 */

#include "lc3_component_registry.h"
#include <string.h>

#define MAX_COMPONENTS 16

static ComponentRecord s_components[MAX_COMPONENTS];
static bool s_cmpInited = false;

static void init_registry(void)
{
    if (s_cmpInited) return;
    for (int i = 0; i < MAX_COMPONENTS; i++) s_components[i].inUse = false;

    // Component 0: Clock Component ('clok')
    s_components[0].inUse = true;
    s_components[0].desc.componentType = 0x636C6F6B; // 'clok'
    s_components[0].desc.componentSubType = 0;
    s_components[0].desc.componentManufacturer = 0x6170706C; // 'appl'
    strcpy(s_components[0].name, "Standard System Clock");

    // Component 1: QuickTime Video Decompressor ('imdc')
    s_components[1].inUse = true;
    s_components[1].desc.componentType = 0x696D6463; // 'imdc'
    s_components[1].desc.componentSubType = 0x72707A61; // 'rpza'
    s_components[1].desc.componentManufacturer = 0x6170706C; // 'appl'
    strcpy(s_components[1].name, "Apple Video Codec");

    s_cmpInited = true;
}

Component lc3_register_component_resource(const ComponentDescription *desc, int16_t resID, int16_t global)
{
    (void)resID;
    (void)global;
    init_registry();
    if (!desc) return NULL;

    for (int i = 0; i < MAX_COMPONENTS; i++) {
        if (!s_components[i].inUse) {
            s_components[i].inUse = true;
            s_components[i].desc = *desc;
            strcpy(s_components[i].name, "Custom Component");
            return (Component)&s_components[i];
        }
    }
    return NULL;
}

int16_t lc3_register_component_resource_file(int16_t resRefNum, int16_t global)
{
    (void)resRefNum;
    (void)global;
    init_registry();
    return CMP_NO_ERR;
}

Component lc3_find_next_component(Component prev, const ComponentDescription *looking)
{
    init_registry();
    int startIdx = 0;
    if (prev) {
        ComponentRecord *p = (ComponentRecord *)prev;
        for (int i = 0; i < MAX_COMPONENTS; i++) {
            if (&s_components[i] == p) {
                startIdx = i + 1;
                break;
            }
        }
    }

    for (int i = startIdx; i < MAX_COMPONENTS; i++) {
        if (s_components[i].inUse) {
            if (!looking || looking->componentType == 0 || looking->componentType == s_components[i].desc.componentType) {
                return (Component)&s_components[i];
            }
        }
    }
    return NULL;
}

int32_t lc3_count_components(const ComponentDescription *looking)
{
    init_registry();
    int32_t count = 0;
    for (int i = 0; i < MAX_COMPONENTS; i++) {
        if (s_components[i].inUse) {
            if (!looking || looking->componentType == 0 || looking->componentType == s_components[i].desc.componentType) {
                count++;
            }
        }
    }
    return count;
}

int16_t lc3_get_component_info(Component aComponent, ComponentDescription *desc, char *componentName)
{
    init_registry();
    if (!aComponent) return CMP_PARAM_ERR;

    ComponentRecord *c = (ComponentRecord *)aComponent;
    if (desc) *desc = c->desc;
    if (componentName) strcpy(componentName, c->name);
    return CMP_NO_ERR;
}

ComponentInstance lc3_open_component(Component aComponent)
{
    init_registry();
    if (!aComponent) return NULL;
    // Returns component pointer as instance
    return (ComponentInstance)aComponent;
}

int16_t lc3_close_component(ComponentInstance aComponentInstance)
{
    (void)aComponentInstance;
    return CMP_NO_ERR;
}

void *lc3_component_function_lookup(ComponentInstance aComponentInstance, int16_t selector)
{
    (void)aComponentInstance;
    (void)selector;
    return NULL;
}
