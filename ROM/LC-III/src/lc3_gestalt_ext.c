/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Gestalt Universal Table & Extensibility Engine
 *
 * Implements dynamic Gestalt selector registration, replacement, and table walking
 * with zero stubs.
 */

#include "lc3_gestalt_ext.h"
#include <string.h>

#define MAX_EXT_GESTALT 32

static GestaltEntry s_gestaltTable[MAX_EXT_GESTALT];
static bool s_gstInited = false;

static void init_gestalt_ext(void)
{
    if (s_gstInited) return;
    for (int i = 0; i < MAX_EXT_GESTALT; i++) s_gestaltTable[i].inUse = false;
    s_gstInited = true;
}

int16_t lc3_new_gestalt(OSType selector, SelectorFunctionProcPtr gestaltFunction)
{
    init_gestalt_ext();
    if (!gestaltFunction) return GST_PARAM_ERR;

    // Check for duplicate selector
    for (int i = 0; i < MAX_EXT_GESTALT; i++) {
        if (s_gestaltTable[i].inUse && s_gestaltTable[i].selector == selector) {
            return GST_DUP_ERR;
        }
    }

    for (int i = 0; i < MAX_EXT_GESTALT; i++) {
        if (!s_gestaltTable[i].inUse) {
            s_gestaltTable[i].inUse = true;
            s_gestaltTable[i].selector = selector;
            s_gestaltTable[i].func = gestaltFunction;
            s_gestaltTable[i].isFunction = true;
            return GST_NO_ERR;
        }
    }
    return GST_PARAM_ERR;
}

int16_t lc3_replace_gestalt(OSType selector, SelectorFunctionProcPtr gestaltFunction, SelectorFunctionProcPtr *oldGestaltFunction)
{
    init_gestalt_ext();
    if (!gestaltFunction) return GST_PARAM_ERR;

    for (int i = 0; i < MAX_EXT_GESTALT; i++) {
        if (s_gestaltTable[i].inUse && s_gestaltTable[i].selector == selector) {
            if (oldGestaltFunction) *oldGestaltFunction = s_gestaltTable[i].func;
            s_gestaltTable[i].func = gestaltFunction;
            return GST_NO_ERR;
        }
    }
    return GST_UNDEF_ERR;
}

int16_t lc3_delete_gestalt(OSType selector)
{
    init_gestalt_ext();
    for (int i = 0; i < MAX_EXT_GESTALT; i++) {
        if (s_gestaltTable[i].inUse && s_gestaltTable[i].selector == selector) {
            s_gestaltTable[i].inUse = false;
            return GST_NO_ERR;
        }
    }
    return GST_UNDEF_ERR;
}

int16_t lc3_gestalt_query_ext(OSType selector, uint32_t *response)
{
    init_gestalt_ext();
    if (!response) return GST_PARAM_ERR;

    for (int i = 0; i < MAX_EXT_GESTALT; i++) {
        if (s_gestaltTable[i].inUse && s_gestaltTable[i].selector == selector) {
            if (s_gestaltTable[i].isFunction && s_gestaltTable[i].func) {
                return s_gestaltTable[i].func(selector, response);
            }
            *response = s_gestaltTable[i].staticValue;
            return GST_NO_ERR;
        }
    }
    return GST_UNDEF_ERR;
}

int16_t lc3_gestalt_enumerate(uint32_t index, OSType *selector, uint32_t *response)
{
    init_gestalt_ext();
    if (!selector || !response) return GST_PARAM_ERR;

    uint32_t cur = 0;
    for (int i = 0; i < MAX_EXT_GESTALT; i++) {
        if (s_gestaltTable[i].inUse) {
            if (cur == index) {
                *selector = s_gestaltTable[i].selector;
                if (s_gestaltTable[i].isFunction && s_gestaltTable[i].func) {
                    s_gestaltTable[i].func(*selector, response);
                } else {
                    *response = s_gestaltTable[i].staticValue;
                }
                return GST_NO_ERR;
            }
            cur++;
        }
    }
    return GST_UNDEF_ERR;
}
