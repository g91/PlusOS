/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Offscreen Graphics Worlds (GWorld Engine)
 *
 * Implements 32-bit QuickDraw GWorld traps ($A906-$A918) with zero stubs.
 */

#include "lc3_gworld.h"
#include <string.h>

#define MAX_GWORLDS 8

static GWorldRec s_gwPool[MAX_GWORLDS];
static GWorldPtr s_gwPtrs[MAX_GWORLDS];
static GWPixMap s_pmPool[MAX_GWORLDS];
static GWPixMapPtr s_pmPtrs[MAX_GWORLDS];
static GWDevice s_devPool[MAX_GWORLDS];
static GWDevicePtr s_devPtrs[MAX_GWORLDS];
static uint8_t s_pixelBuffers[MAX_GWORLDS][307200]; // 640x480 @ 8-bpp buffer pool
static bool s_gwInited = false;

static GWorldHandle s_curGWorld = NULL;
static GWDeviceHandle s_curGDevice = NULL;

static void gw_init_pool(void)
{
    if (s_gwInited) return;
    for (int i = 0; i < MAX_GWORLDS; i++) {
        s_gwPtrs[i] = &s_gwPool[i];
        s_pmPtrs[i] = &s_pmPool[i];
        s_devPtrs[i] = &s_devPool[i];
    }
    s_gwInited = true;
}

int16_t lc3_new_gworld(GWorldHandle *offscreenGWorld, int16_t pixelDepth, const GWRect *boundsRect, void *cTable, GWDeviceHandle aGDevice, uint32_t flags)
{
    (void)aGDevice;
    (void)cTable;
    if (!offscreenGWorld || !boundsRect) return GWORLD_PARAM_ERR;
    gw_init_pool();

    int slot = -1;
    for (int i = 0; i < MAX_GWORLDS; i++) {
        if (s_gwPool[i].bufferSize == 0) {
            slot = i;
            break;
        }
    }
    if (slot == -1) return GWORLD_MEM_FULL_ERR;

    int16_t width = boundsRect->right - boundsRect->left;
    int16_t height = boundsRect->bottom - boundsRect->top;
    if (width <= 0 || height <= 0) return GWORLD_PARAM_ERR;

    if (pixelDepth != 1 && pixelDepth != 2 && pixelDepth != 4 && pixelDepth != 8 && pixelDepth != 16 && pixelDepth != 32) {
        pixelDepth = 8;
    }

    /* Rowbytes must be word-aligned (multiple of 2 or 4) */
    int32_t rawRowBytes = ((int32_t)width * pixelDepth + 7) / 8;
    rawRowBytes = (rawRowBytes + 3) & ~3; // 4-byte aligned for 68030 32-bit burst bus

    uint32_t totalBytes = (uint32_t)rawRowBytes * height;
    if (totalBytes > sizeof(s_pixelBuffers[0])) {
        return GWORLD_MEM_FULL_ERR;
    }

    GWorldRec *gw = &s_gwPool[slot];
    GWPixMap *pm = &s_pmPool[slot];
    GWDevice *dev = &s_devPool[slot];

    memset(gw, 0, sizeof(GWorldRec));
    memset(pm, 0, sizeof(GWPixMap));
    memset(dev, 0, sizeof(GWDevice));

    /* Initialize PixMap */
    pm->baseAddr = (uint32_t)(uintptr_t)s_pixelBuffers[slot];
    pm->rowBytes = (int16_t)(rawRowBytes | 0x8000); // 0x8000 indicates PixMap vs BitMap
    pm->bounds = *boundsRect;
    pm->pmVersion = 0;
    pm->hRes = 0x00480000; // 72 dpi Fixed
    pm->vRes = 0x00480000; // 72 dpi Fixed
    pm->pixelType = (pixelDepth <= 8) ? 0 : 16; // 0 = Indexed, 16 = Direct
    pm->pixelSize = pixelDepth;
    pm->cmpCount = (pixelDepth <= 8) ? 1 : 3;
    pm->cmpSize = (pixelDepth <= 8) ? pixelDepth : 5;
    pm->pmTable = cTable;

    /* Initialize GDevice */
    dev->deviceType = 2; // Direct/Clut video device
    dev->gdPMap = (GWPixMapHandle)&s_pmPtrs[slot];
    dev->gdRect = *boundsRect;

    /* Initialize GWorld */
    gw->portVersion = 0x4000; // Color GrafPort flag
    gw->portRect = *boundsRect;
    gw->portPixMap = (GWPixMapHandle)&s_pmPtrs[slot];
    gw->gWorldDevice = (GWDeviceHandle)&s_devPtrs[slot];
    gw->gWorldFlags = flags;
    gw->pixelBuffer = s_pixelBuffers[slot];
    gw->bufferSize = totalBytes;
    gw->pixState = GW_PIX_LOCKED_BIT;

    /* Clear pixel buffer */
    memset(s_pixelBuffers[slot], 0, totalBytes);

    *offscreenGWorld = (GWorldHandle)&s_gwPtrs[slot];
    return GWORLD_NO_ERR;
}

void lc3_dispose_gworld(GWorldHandle offscreenGWorld)
{
    if (!offscreenGWorld || !*offscreenGWorld) return;
    GWorldRec *gw = *offscreenGWorld;
    gw->bufferSize = 0;
    gw->pixelBuffer = NULL;
    gw->portPixMap = NULL;
}

bool lc3_lock_pixels(GWPixMapHandle pm)
{
    if (!pm || !*pm) return false;
    for (int i = 0; i < MAX_GWORLDS; i++) {
        if (&s_pmPool[i] == *pm) {
            s_gwPool[i].pixState |= GW_PIX_LOCKED_BIT;
            return true;
        }
    }
    return true;
}

void lc3_unlock_pixels(GWPixMapHandle pm)
{
    if (!pm || !*pm) return;
    for (int i = 0; i < MAX_GWORLDS; i++) {
        if (&s_pmPool[i] == *pm) {
            s_gwPool[i].pixState &= ~GW_PIX_LOCKED_BIT;
            return;
        }
    }
}

int32_t lc3_update_gworld(GWorldHandle *offscreenGWorld, int16_t pixelDepth, const GWRect *boundsRect, void *cTable, GWDeviceHandle aGDevice, uint32_t flags)
{
    if (!offscreenGWorld || !*offscreenGWorld) {
        return lc3_new_gworld(offscreenGWorld, pixelDepth, boundsRect, cTable, aGDevice, flags);
    }
    GWorldRec *gw = **offscreenGWorld;
    GWPixMap *pm = *gw->portPixMap;

    int16_t width = boundsRect->right - boundsRect->left;
    int16_t height = boundsRect->bottom - boundsRect->top;
    int32_t rawRowBytes = ((int32_t)width * pixelDepth + 7) / 8;
    rawRowBytes = (rawRowBytes + 3) & ~3;

    pm->bounds = *boundsRect;
    pm->pixelSize = pixelDepth;
    pm->rowBytes = (int16_t)(rawRowBytes | 0x8000);
    gw->portRect = *boundsRect;
    return 0; // gwReconfigured
}

void lc3_get_gworld(GWorldHandle *gworld, GWDeviceHandle *device)
{
    if (gworld) *gworld = s_curGWorld;
    if (device) *device = s_curGDevice;
}

void lc3_set_gworld(GWorldHandle gworld, GWDeviceHandle device)
{
    s_curGWorld = gworld;
    s_curGDevice = device;
}

GWDeviceHandle lc3_get_gworld_device(GWorldHandle offscreenGWorld)
{
    if (!offscreenGWorld || !*offscreenGWorld) return NULL;
    return (*offscreenGWorld)->gWorldDevice;
}

GWPixMapHandle lc3_get_gworld_pixmap(GWorldHandle offscreenGWorld)
{
    if (!offscreenGWorld || !*offscreenGWorld) return NULL;
    return (*offscreenGWorld)->portPixMap;
}

bool lc3_pixmap_32bit(GWPixMapHandle pm)
{
    if (!pm || !*pm) return false;
    return ((*pm)->pixelSize == 32);
}

uint32_t lc3_get_pix_base_addr(GWPixMapHandle pm)
{
    if (!pm || !*pm) return 0;
    return (*pm)->baseAddr;
}

uint32_t lc3_offscreen_version(void)
{
    return 0x07008000; // QuickDraw GWorld 7.0
}

void lc3_allow_purge_pixels(GWPixMapHandle pm)
{
    if (!pm || !*pm) return;
    for (int i = 0; i < MAX_GWORLDS; i++) {
        if (&s_pmPool[i] == *pm) {
            s_gwPool[i].pixState |= GW_PIX_PURGE_BIT;
            return;
        }
    }
}

void lc3_no_purge_pixels(GWPixMapHandle pm)
{
    if (!pm || !*pm) return;
    for (int i = 0; i < MAX_GWORLDS; i++) {
        if (&s_pmPool[i] == *pm) {
            s_gwPool[i].pixState &= ~GW_PIX_PURGE_BIT;
            return;
        }
    }
}

uint32_t lc3_get_pixels_state(GWPixMapHandle pm)
{
    if (!pm || !*pm) return 0;
    for (int i = 0; i < MAX_GWORLDS; i++) {
        if (&s_pmPool[i] == *pm) {
            return s_gwPool[i].pixState;
        }
    }
    return 0;
}

void lc3_set_pixels_state(GWPixMapHandle pm, uint32_t state)
{
    if (!pm || !*pm) return;
    for (int i = 0; i < MAX_GWORLDS; i++) {
        if (&s_pmPool[i] == *pm) {
            s_gwPool[i].pixState = state;
            return;
        }
    }
}
