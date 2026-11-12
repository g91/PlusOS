/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Color QuickDraw Offscreen GWorld Buffer Lock & Pixel Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_GWORLD_LOCK_H
#define LC3_GWORLD_LOCK_H

#include <stdint.h>
#include <stdbool.h>

#define GW_NO_ERR         0
#define GW_MEM_ERR      -108
#define GW_PARAM_ERR     -50

#define GW_PIXELS_LOCKED     0x0001
#define GW_PIXELS_PURGEABLE  0x0002
#define GW_PIXELS_PURGED     0x0004

typedef struct PixMapRecord {
    void *baseAddr;
    int16_t rowBytes;         /* Flagged with 0x8000 for Color PixMap */
    int16_t boundsTop;
    int16_t boundsLeft;
    int16_t boundsBottom;
    int16_t boundsRight;
    int16_t pmVersion;
    int16_t packType;
    int32_t packSize;
    int32_t hRes;
    int32_t vRes;
    int16_t pixelType;
    int16_t pixelSize;        /* 1, 2, 4, 8, 16, 32 */
    int16_t cmpCount;
    int16_t cmpSize;
    int32_t planeBytes;
    void *pmTable;
    int32_t pmReserved;
} PixMapRecord, *PixMapPtr, **PixMapHandle;

typedef struct GWorldRecord {
    void *port;
    PixMapHandle portPixMap;
    uint32_t gWorldFlags;
    uint32_t bufferState;
    void *realBuffer;
} GWorldRecord, *GWorldPtr;

/* GWorld Lock API */
bool lc3_gw_lock_pixels(PixMapHandle pm);
void lc3_gw_unlock_pixels(PixMapHandle pm);
void *lc3_gw_get_pix_base_addr(PixMapHandle pm);
int16_t lc3_gw_update_gworld(GWorldPtr *gw, int16_t depth, int16_t width, int16_t height);
void lc3_gw_set_pixels_state(PixMapHandle pm, uint32_t state);
uint32_t lc3_gw_get_pixels_state(PixMapHandle pm);

#endif /* LC3_GWORLD_LOCK_H */
