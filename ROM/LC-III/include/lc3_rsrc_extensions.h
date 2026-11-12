/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Resource Manager 32-bit Extensions & Partial Resources
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_RSRC_EXTENSIONS_H
#define LC3_RSRC_EXTENSIONS_H

#include <stdint.h>
#include <stdbool.h>

#define RSRC_NO_ERR           0
#define RSRC_NOT_FOUND     -192
#define RES_ATTR_ERR       -198
#define INPUT_OUT_OF_BOUNDS -199

typedef uint32_t ResType;
typedef void **Handle;

/* Partial Resource API */
void lc3_read_partial_resource(Handle theResource, int32_t offset, void *buffer, int32_t count);
void lc3_write_partial_resource(Handle theResource, int32_t offset, const void *buffer, int32_t count);
void lc3_set_resource_size(Handle theResource, int32_t newSize);
Handle lc3_get_next_fond(Handle fondHandle);

/* Temporary Memory Allocation (Operating in MultiFinder Temporary Zone) */
Handle lc3_temp_new_handle(int32_t logicalSize, int16_t *resultCode);
int32_t lc3_temp_max_mem(int32_t *grow);
int32_t lc3_temp_free_mem(void);
void lc3_temp_h_lock(Handle h, int16_t *resultCode);
void lc3_temp_h_unlock(Handle h, int16_t *resultCode);
void lc3_temp_dispose_handle(Handle h, int16_t *resultCode);

/* Resource Dispatch ($A822) */
int32_t lc3_resource_dispatch(int16_t selector, void *params);

#endif /* LC3_RSRC_EXTENSIONS_H */
