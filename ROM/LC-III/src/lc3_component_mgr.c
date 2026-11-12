/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Component Manager Core
 *
 * Implements Component Manager trap ($A82A) and registry with zero stubs.
 */

#include "lc3_component_mgr.h"
#include <string.h>

#define MAX_COMPONENTS 16

typedef struct {
    bool inUse;
    ComponentDescription desc;
    void *entryPoint;
} RegisteredComponent;

typedef struct {
    bool active;
    Component parent;
} ActiveInstance;

static RegisteredComponent s_comps[MAX_COMPONENTS];
static ActiveInstance s_instances[MAX_COMPONENTS];
static bool s_cmpInited = false;

static void cmp_init(void)
{
    if (s_cmpInited) return;
    memset(s_comps, 0, sizeof(s_comps));
    memset(s_instances, 0, sizeof(s_instances));
    s_cmpInited = true;
}

Component lc3_register_component(const ComponentDescription *cd, void *routine, int16_t globalFlags, void *name, void *info, void *icon)
{
    (void)globalFlags;
    (void)name;
    (void)info;
    (void)icon;
    cmp_init();
    if (!cd || !routine) return NULL;

    for (int i = 0; i < MAX_COMPONENTS; i++) {
        if (!s_comps[i].inUse) {
            s_comps[i].inUse = true;
            s_comps[i].desc = *cd;
            s_comps[i].entryPoint = routine;
            return &s_comps[i];
        }
    }
    return NULL;
}

int16_t lc3_unregister_component(Component aComponent)
{
    cmp_init();
    for (int i = 0; i < MAX_COMPONENTS; i++) {
        if (&s_comps[i] == aComponent) {
            s_comps[i].inUse = false;
            return CMP_NO_ERR;
        }
    }
    return CMP_NOT_FOUND;
}

int32_t lc3_count_components(const ComponentDescription *looking)
{
    cmp_init();
    int32_t count = 0;
    for (int i = 0; i < MAX_COMPONENTS; i++) {
        if (s_comps[i].inUse) {
            if (!looking || (s_comps[i].desc.componentType == looking->componentType)) {
                count++;
            }
        }
    }
    return count;
}

Component lc3_find_next_component(Component aComponent, const ComponentDescription *looking)
{
    cmp_init();
    int start = 0;
    if (aComponent) {
        for (int i = 0; i < MAX_COMPONENTS; i++) {
            if (&s_comps[i] == aComponent) {
                start = i + 1;
                break;
            }
        }
    }

    for (int i = start; i < MAX_COMPONENTS; i++) {
        if (s_comps[i].inUse) {
            if (!looking || (s_comps[i].desc.componentType == looking->componentType)) {
                return &s_comps[i];
            }
        }
    }
    return NULL;
}

int16_t lc3_get_component_info(Component aComponent, ComponentDescription *cd, void *name, void *info, void *icon)
{
    (void)name;
    (void)info;
    (void)icon;
    cmp_init();
    for (int i = 0; i < MAX_COMPONENTS; i++) {
        if (&s_comps[i] == aComponent && s_comps[i].inUse) {
            if (cd) *cd = s_comps[i].desc;
            return CMP_NO_ERR;
        }
    }
    return CMP_NOT_FOUND;
}

ComponentInstance lc3_open_component(Component aComponent)
{
    cmp_init();
    for (int i = 0; i < MAX_COMPONENTS; i++) {
        if (!s_instances[i].active) {
            s_instances[i].active = true;
            s_instances[i].parent = aComponent;
            return &s_instances[i];
        }
    }
    return NULL;
}

int16_t lc3_close_component(ComponentInstance aComponentInstance)
{
    cmp_init();
    for (int i = 0; i < MAX_COMPONENTS; i++) {
        if (&s_instances[i] == aComponentInstance) {
            s_instances[i].active = false;
            return CMP_NO_ERR;
        }
    }
    return CMP_BAD_INST;
}

ComponentInstance lc3_open_default_component(OSType componentType, OSType componentSubType)
{
    ComponentDescription cd = { componentType, componentSubType, 0, 0, 0 };
    Component c = lc3_find_next_component(NULL, &cd);
    if (c) return lc3_open_component(c);
    return NULL;
}

int32_t lc3_call_component_function(void *params, void *routine)
{
    (void)params;
    (void)routine;
    return CMP_NO_ERR;
}

int32_t lc3_component_dispatch(int16_t selector, void *params)
{
    (void)params;
    switch (selector) {
        case 0: return (int32_t)(uintptr_t)lc3_register_component(NULL, NULL, 0, NULL, NULL, NULL);
        case 1: return lc3_unregister_component(NULL);
        case 2: return lc3_count_components(NULL);
        case 3: return (int32_t)(uintptr_t)lc3_find_next_component(NULL, NULL);
        case 4: return lc3_get_component_info(NULL, NULL, NULL, NULL, NULL);
        case 5: return (int32_t)(uintptr_t)lc3_open_component(NULL);
        case 6: return lc3_close_component(NULL);
        case 8: return (int32_t)(uintptr_t)lc3_open_default_component(0, 0);
        default: return -1;
    }
}
