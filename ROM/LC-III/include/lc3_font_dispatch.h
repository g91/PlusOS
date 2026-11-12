/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: TrueType FontDispatch ($A854) Trap & Outline Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_FONT_DISPATCH_H
#define LC3_FONT_DISPATCH_H

#include <stdint.h>
#include <stdbool.h>

#define FD_NO_ERR         0
#define FD_NOT_FOUND     -1
#define FD_PARAM_ERR    -50

typedef void *FontContextHandle;

typedef struct OutlineMetricsRecord {
    int32_t ascent;
    int32_t descent;
    int32_t widMax;
    int32_t leading;
    int32_t unused;
} OutlineMetricsRecord;

/* FontDispatch API */
int16_t lc3_fd_new_font_context(int16_t fontID, int16_t size, int16_t face, FontContextHandle *hContext);
int16_t lc3_fd_dispose_font_context(FontContextHandle hContext);
int16_t lc3_fd_get_outline(FontContextHandle hContext, uint16_t charCode, void *outlineBuffer, uint32_t *bufSize);
int16_t lc3_fd_get_bitmap(FontContextHandle hContext, uint16_t charCode, void *bitmapBuffer, int16_t rowBytes);
int16_t lc3_fd_get_real_font_name(int16_t fontID, char *nameOut);
int16_t lc3_fd_outline_metrics(FontContextHandle hContext, OutlineMetricsRecord *metrics);
int16_t lc3_font_dispatch(int16_t selector, void *params);

#endif /* LC3_FONT_DISPATCH_H */
