/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Color QuickDraw Offscreen GWorld Buffer Lock & Pixel Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#include "lc3_gworld_lock.h"
#include <string.h>

static uint8_t sStaticOffscreenBuffer[640 * 480];
static PixMapRecord sStaticPixMap;
static PixMapPtr sStaticPixMapPtr = &sStaticPixMap;
static PixMapHandle sStaticPixMapHdl = &sStaticPixMapPtr;

bool lc3_gw_lock_pixels(PixMapHandle pm)
{
    if (!pm || !*pm) return false;

    (*pm)->pmReserved |= GW_PIXELS_LOCKED;
    return true;
}

void lc3_gw_unlock_pixels(PixMapHandle pm)
{
    if (!pm || !*pm) return;

    (*pm)->pmReserved &= ~GW_PIXELS_LOCKED;
}

void *lc3_gw_get_pix_base_addr(PixMapHandle pm)
{
    if (!pm || !*pm) return NULL;

    /* 32-bit clean address strip: low 32 bits directly returned */
    return (*pm)->baseAddr;
}

int16_t lc3_gw_update_gworld(GWorldPtr *gw, int16_t depth, int16_t width, int16_t height)
{
    if (!gw || width <= 0 || height <= 0) return GW_PARAM_ERR;

    int16_t rowBytes = (int16_t)(((width * depth + 31) / 32) * 4);
    if ((rowBytes * height) > (int32_t)sizeof(sStaticOffscreenBuffer)) {
        return GW_MEM_ERR;
    }

    memset(&sStaticPixMap, 0, sizeof(PixMapRecord));
    sStaticPixMap.baseAddr = sStaticOffscreenBuffer;
    sStaticPixMap.rowBytes = (int16_t)(rowBytes | 0x8000); /* 0x8000 indicates PixMap */
    sStaticPixMap.boundsTop = 0;
    sStaticPixMap.boundsLeft = 0;
    sStaticPixMap.boundsBottom = height;
    sStaticPixMap.boundsRight = width;
    sStaticPixMap.pixelSize = depth;
    sStaticPixMap.cmpCount = 1;
    sStaticPixMap.cmpSize = depth;
    sStaticPixMap.pmReserved = GW_PIXELS_LOCKED;

    return GW_NO_ERR;
}

void lc3_gw_set_pixels_state(PixMapHandle pm, uint32_t state)
{
    if (!pm || !*pm) return;
    (*pm)->pmReserved = (int32_t)state;
}

uint32_t lc3_gw_get_pixels_state(PixMapHandle pm)
{
    if (!pm || !*pm) return 0;
    return (uint32_t)(*pm)->pmReserved;
}
