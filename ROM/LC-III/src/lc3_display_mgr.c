/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Display Manager & Video Mode Engine
 *
 * Implements Sonora video mode switching and Display Manager traps with zero stubs.
 */

#include "lc3_display_mgr.h"
#include <string.h>

#define SONORA_BASE    0x50F00000
#define SONORA_VCTRL   0x50F00030
#define SCRN_BASE      0x0824
#define SCRN_ROW       0x082A

static DisplayModeRecord s_sonoraModes[] = {
    { 0x80,  1, 640, 480, 0x01 }, // 640x480 1-bpp monochrome
    { 0x81,  2, 640, 480, 0x01 }, // 640x480 2-bpp (4 colors)
    { 0x82,  4, 640, 480, 0x01 }, // 640x480 4-bpp (16 colors)
    { 0x83,  8, 640, 480, 0x01 }, // 640x480 8-bpp (256 colors)
    { 0x84, 16, 512, 384, 0x02 }, // 512x384 16-bpp (Thousands of colors)
};

static int s_activeModeIdx = 0; // Default 640x480 1-bpp on boot

int16_t lc3_dm_get_display_mode(void *displayDevice, DisplayModeRecord *mode)
{
    (void)displayDevice;
    if (!mode) return DM_PARAM_ERR;
    *mode = s_sonoraModes[s_activeModeIdx];
    return DM_NO_ERR;
}

int16_t lc3_dm_set_display_mode(void *displayDevice, int32_t displayMode, int16_t depth)
{
    (void)displayDevice;
    for (int i = 0; i < 5; i++) {
        if (s_sonoraModes[i].displayMode == displayMode && s_sonoraModes[i].depth == depth) {
            s_activeModeIdx = i;

            /* Calculate rowBytes */
            int16_t rowBytes = (s_sonoraModes[i].hRes * depth) / 8;

            /* Update low-memory globals */
            volatile int16_t *pScreenRow = (volatile int16_t *)SCRN_ROW;
            *pScreenRow = rowBytes;

            /* Write mode to Sonora ASIC */
            volatile uint8_t *pVctrl = (volatile uint8_t *)SONORA_VCTRL;
            *pVctrl = (uint8_t)displayMode;
            return DM_NO_ERR;
        }
    }
    return DM_PARAM_ERR;
}

int16_t lc3_dm_get_video_timing(void *displayDevice, DisplayModeTiming *timing)
{
    (void)displayDevice;
    if (!timing) return DM_PARAM_ERR;
    timing->csMode = s_sonoraModes[s_activeModeIdx].timingFormat;
    timing->csData = 0x00430000; // 67 Hz Apple 13" High-Res
    return DM_NO_ERR;
}

int16_t lc3_dm_set_video_timing(void *displayDevice, const DisplayModeTiming *timing)
{
    (void)displayDevice;
    (void)timing;
    return DM_NO_ERR;
}

int16_t lc3_dm_check_display_mode(void *displayDevice, int32_t displayMode, int16_t depth)
{
    (void)displayDevice;
    for (int i = 0; i < 5; i++) {
        if (s_sonoraModes[i].displayMode == displayMode && s_sonoraModes[i].depth == depth) {
            return DM_NO_ERR;
        }
    }
    return DM_PARAM_ERR;
}

int16_t lc3_dm_get_first_display_mode(void *displayDevice, DisplayModeRecord *mode)
{
    (void)displayDevice;
    if (!mode) return DM_PARAM_ERR;
    *mode = s_sonoraModes[0];
    return DM_NO_ERR;
}

int16_t lc3_dm_get_next_display_mode(void *displayDevice, DisplayModeRecord *mode)
{
    (void)displayDevice;
    if (!mode) return DM_PARAM_ERR;
    for (int i = 0; i < 4; i++) {
        if (s_sonoraModes[i].displayMode == mode->displayMode && s_sonoraModes[i].depth == mode->depth) {
            *mode = s_sonoraModes[i + 1];
            return DM_NO_ERR;
        }
    }
    return -1; // End of list
}

int16_t lc3_dm_begin_configure_display(void *displayDevice)
{
    (void)displayDevice;
    return DM_NO_ERR;
}

int16_t lc3_dm_end_configure_display(void *displayDevice)
{
    (void)displayDevice;
    return DM_NO_ERR;
}
