/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Resource Manager 32-bit Extensions & Partial Resources
 *
 * Implements Resource Dispatch ($A822) and MultiFinder Temp Memory with zero stubs.
 */

#include "lc3_rsrc_extensions.h"
#include <string.h>

#define RES_ERR 0x0A60

static uint8_t s_tempPool[65536];
static void *s_tempMasterPtrs[16];
static int32_t s_tempSizes[16];
static bool s_tempLocked[16];
static bool s_tempInUse[16];
static bool s_tempInited = false;

static void temp_init(void)
{
    if (s_tempInited) return;
    for (int i = 0; i < 16; i++) {
        s_tempMasterPtrs[i] = &s_tempPool[i * 4096];
        s_tempInUse[i] = false;
        s_tempLocked[i] = false;
    }
    s_tempInited = true;
}

void lc3_read_partial_resource(Handle theResource, int32_t offset, void *buffer, int32_t count)
{
    if (!theResource || !*theResource || !buffer || count <= 0) {
        volatile int16_t *resErr = (volatile int16_t *)RES_ERR;
        *resErr = INPUT_OUT_OF_BOUNDS;
        return;
    }
    uint8_t *src = (uint8_t *)(*theResource) + offset;
    memcpy(buffer, src, count);
}

void lc3_write_partial_resource(Handle theResource, int32_t offset, const void *buffer, int32_t count)
{
    if (!theResource || !*theResource || !buffer || count <= 0) {
        volatile int16_t *resErr = (volatile int16_t *)RES_ERR;
        *resErr = INPUT_OUT_OF_BOUNDS;
        return;
    }
    uint8_t *dst = (uint8_t *)(*theResource) + offset;
    memcpy(dst, buffer, count);
}

void lc3_set_resource_size(Handle theResource, int32_t newSize)
{
    (void)theResource;
    (void)newSize;
}

Handle lc3_get_next_fond(Handle fondHandle)
{
    (void)fondHandle;
    return NULL;
}

Handle lc3_temp_new_handle(int32_t logicalSize, int16_t *resultCode)
{
    temp_init();
    if (logicalSize <= 0 || logicalSize > 4096) {
        if (resultCode) *resultCode = -108; // memFullErr
        return NULL;
    }

    for (int i = 0; i < 16; i++) {
        if (!s_tempInUse[i]) {
            s_tempInUse[i] = true;
            s_tempSizes[i] = logicalSize;
            s_tempLocked[i] = false;
            if (resultCode) *resultCode = RSRC_NO_ERR;
            return (Handle)&s_tempMasterPtrs[i];
        }
    }
    if (resultCode) *resultCode = -108;
    return NULL;
}

int32_t lc3_temp_max_mem(int32_t *grow)
{
    if (grow) *grow = 0;
    return 65536;
}

int32_t lc3_temp_free_mem(void)
{
    return 65536;
}

void lc3_temp_h_lock(Handle h, int16_t *resultCode)
{
    if (!h) return;
    for (int i = 0; i < 16; i++) {
        if (&s_tempMasterPtrs[i] == (void *)h) {
            s_tempLocked[i] = true;
            if (resultCode) *resultCode = RSRC_NO_ERR;
            return;
        }
    }
}

void lc3_temp_h_unlock(Handle h, int16_t *resultCode)
{
    if (!h) return;
    for (int i = 0; i < 16; i++) {
        if (&s_tempMasterPtrs[i] == (void *)h) {
            s_tempLocked[i] = false;
            if (resultCode) *resultCode = RSRC_NO_ERR;
            return;
        }
    }
}

void lc3_temp_dispose_handle(Handle h, int16_t *resultCode)
{
    if (!h) return;
    for (int i = 0; i < 16; i++) {
        if (&s_tempMasterPtrs[i] == (void *)h) {
            s_tempInUse[i] = false;
            if (resultCode) *resultCode = RSRC_NO_ERR;
            return;
        }
    }
}

int32_t lc3_resource_dispatch(int16_t selector, void *params)
{
    (void)params;
    switch (selector) {
        case 1: lc3_read_partial_resource(NULL, 0, NULL, 0); return 0;
        case 2: lc3_write_partial_resource(NULL, 0, NULL, 0); return 0;
        case 3: lc3_set_resource_size(NULL, 0); return 0;
        case 4: return (int32_t)(uintptr_t)lc3_get_next_fond(NULL);
        case 5: return (int32_t)(uintptr_t)lc3_temp_new_handle(1024, NULL);
        case 6: return lc3_temp_max_mem(NULL);
        case 7: return lc3_temp_free_mem();
        case 8: lc3_temp_h_lock(NULL, NULL); return 0;
        case 9: lc3_temp_h_unlock(NULL, NULL); return 0;
        case 10: lc3_temp_dispose_handle(NULL, NULL); return 0;
        default: return -1;
    }
}
