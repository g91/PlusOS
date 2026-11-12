/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Gestalt Universal Table & Extensibility Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_GESTALT_EXT_H
#define LC3_GESTALT_EXT_H

#include <stdint.h>
#include <stdbool.h>

#define GST_NO_ERR         0
#define GST_DUP_ERR     -5551
#define GST_UNDEF_ERR   -5550
#define GST_PARAM_ERR     -50

typedef uint32_t OSType;
typedef int16_t (*SelectorFunctionProcPtr)(OSType selector, uint32_t *response);

typedef struct GestaltEntry {
    OSType selector;
    SelectorFunctionProcPtr func;
    uint32_t staticValue;
    bool isFunction;
    bool inUse;
} GestaltEntry;

/* Gestalt Extensibility API */
int16_t lc3_new_gestalt(OSType selector, SelectorFunctionProcPtr gestaltFunction);
int16_t lc3_replace_gestalt(OSType selector, SelectorFunctionProcPtr gestaltFunction, SelectorFunctionProcPtr *oldGestaltFunction);
int16_t lc3_delete_gestalt(OSType selector);
int16_t lc3_gestalt_query_ext(OSType selector, uint32_t *response);
int16_t lc3_gestalt_enumerate(uint32_t index, OSType *selector, uint32_t *response);

#endif /* LC3_GESTALT_EXT_H */
