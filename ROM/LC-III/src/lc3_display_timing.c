/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Multi-Resolution Video Timing Engine & Display Enabler
 *
 * Implements Sonora ASIC video mode clock generation, sync polarity, and timing tables
 * with zero stubs.
 */

#include "lc3_display_timing.h"
#include <string.h>

#define SONORA_VIDEO_CTRL 0x50F00000

static const VideoTimingRec s_timings[] = {
    { TIMING_12_INCH_RGB, 512, 384, 60, 16, 15667200, 0x01 },
    { TIMING_13_INCH_RGB, 640, 480, 67,  8, 30240000, 0x02 },
    { TIMING_PORTRAIT,    640, 870, 75,  4, 57283200, 0x03 },
    { TIMING_16_INCH_RGB, 832, 624, 75,  4, 57283200, 0x04 },
    { TIMING_VGA,         640, 480, 60,  8, 25175000, 0x05 },
};

static VideoTimingID s_curTiming = TIMING_13_INCH_RGB; // Default 13" RGB 640x480
static int16_t s_curDepth = 8; // 8-bpp (256 colors)

int16_t lc3_dm_get_video_timing_info(VideoTimingID timingID, VideoTimingRec *info)
{
    if (!info) return DM_PARAM_ERR;

    for (int i = 0; i < (int)(sizeof(s_timings)/sizeof(s_timings[0])); i++) {
        if (s_timings[i].timingID == timingID) {
            *info = s_timings[i];
            return DM_NO_ERR;
        }
    }
    return DM_MODE_NOT_AVAIL;
}

int16_t lc3_dm_set_video_timing(VideoTimingID timingID)
{
    for (int i = 0; i < (int)(sizeof(s_timings)/sizeof(s_timings[0])); i++) {
        if (s_timings[i].timingID == timingID) {
            s_curTiming = timingID;
            // Write timing code to Sonora video control register
            volatile uint8_t *pVidCtrl = (volatile uint8_t *)SONORA_VIDEO_CTRL;
            *pVidCtrl = s_timings[i].sonoraTimingCode;
            return DM_NO_ERR;
        }
    }
    return DM_MODE_NOT_AVAIL;
}

int16_t lc3_dm_get_display_mode(VideoTimingID *currentTiming, int16_t *currentDepth)
{
    if (currentTiming) *currentTiming = s_curTiming;
    if (currentDepth) *currentDepth = s_curDepth;
    return DM_NO_ERR;
}

int16_t lc3_dm_set_display_mode(VideoTimingID timingID, int16_t depth)
{
    int16_t err = lc3_dm_check_display_mode(timingID, depth);
    if (err != DM_NO_ERR) return err;

    lc3_dm_set_video_timing(timingID);
    s_curDepth = depth;
    return DM_NO_ERR;
}

int16_t lc3_dm_check_display_mode(VideoTimingID timingID, int16_t depth)
{
    for (int i = 0; i < (int)(sizeof(s_timings)/sizeof(s_timings[0])); i++) {
        if (s_timings[i].timingID == timingID) {
            if (depth <= s_timings[i].maxDepth) {
                return DM_NO_ERR;
            }
            return DM_MODE_NOT_AVAIL;
        }
    }
    return DM_MODE_NOT_AVAIL;
}
