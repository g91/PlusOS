/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Offscreen Graphics Worlds (GWorld Engine)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_GWORLD_H
#define LC3_GWORLD_H

#include <stdint.h>
#include <stdbool.h>

#define GWORLD_NO_ERR          0
#define GWORLD_PARAM_ERR     -50
#define GWORLD_MEM_FULL_ERR -108

#define GW_PIX_PURGE_BIT      (1 << 0)
#define GW_PIX_LOCKED_BIT     (1 << 1)
#define GW_PIX_TEMP_BIT       (1 << 2)

#define KEEP_LOCAL            (1 << 0)
#define PIX_PURGE             (1 << 1)
#define NO_NEW_DEVICE         (1 << 2)
#define USE_TEMP_MEM          (1 << 3)
#define KEEP_BITS             (1 << 4)

typedef struct {
    int16_t top;
    int16_t left;
    int16_t bottom;
    int16_t right;
} GWRect;

typedef struct {
    uint32_t baseAddr;
    int16_t rowBytes;
    GWRect bounds;
    int16_t pmVersion;
    int16_t packType;
    int32_t packSize;
    int32_t hRes;
    int32_t vRes;
    int16_t pixelType;
    int16_t pixelSize;
    int16_t cmpCount;
    int16_t cmpSize;
    int32_t planeBytes;
    void *pmTable;
    int32_t pmReserved;
} GWPixMap, *GWPixMapPtr, **GWPixMapHandle;

typedef struct {
    int16_t deviceType;
    int32_t gDeviceFlags;
    GWPixMapHandle gdPMap;
    GWRect gdRect;
    int16_t gdRefNum;
    int16_t gdID;
} GWDevice, *GWDevicePtr, **GWDeviceHandle;

typedef struct {
    int16_t portVersion;
    GWRect portRect;
    GWPixMapHandle portPixMap;
    void *portVisRgn;
    void *portClipRgn;
    GWDeviceHandle gWorldDevice;
    uint32_t gWorldFlags;
    uint8_t *pixelBuffer;
    uint32_t bufferSize;
    uint32_t pixState;
} GWorldRec, *GWorldPtr, **GWorldHandle;

/* GWorld Core API */
int16_t lc3_new_gworld(GWorldHandle *offscreenGWorld, int16_t pixelDepth, const GWRect *boundsRect, void *cTable, GWDeviceHandle aGDevice, uint32_t flags);
void lc3_dispose_gworld(GWorldHandle offscreenGWorld);
bool lc3_lock_pixels(GWPixMapHandle pm);
void lc3_unlock_pixels(GWPixMapHandle pm);
int32_t lc3_update_gworld(GWorldHandle *offscreenGWorld, int16_t pixelDepth, const GWRect *boundsRect, void *cTable, GWDeviceHandle aGDevice, uint32_t flags);
void lc3_get_gworld(GWorldHandle *gworld, GWDeviceHandle *device);
void lc3_set_gworld(GWorldHandle gworld, GWDeviceHandle device);
GWDeviceHandle lc3_get_gworld_device(GWorldHandle offscreenGWorld);
GWPixMapHandle lc3_get_gworld_pixmap(GWorldHandle offscreenGWorld);
bool lc3_pixmap_32bit(GWPixMapHandle pm);
uint32_t lc3_get_pix_base_addr(GWPixMapHandle pm);
uint32_t lc3_offscreen_version(void);
void lc3_allow_purge_pixels(GWPixMapHandle pm);
void lc3_no_purge_pixels(GWPixMapHandle pm);
uint32_t lc3_get_pixels_state(GWPixMapHandle pm);
void lc3_set_pixels_state(GWPixMapHandle pm, uint32_t state);

#endif /* LC3_GWORLD_H */
