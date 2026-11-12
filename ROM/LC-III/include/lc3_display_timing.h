/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Multi-Resolution Video Timing Engine & Display Enabler
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_DISPLAY_TIMING_H
#define LC3_DISPLAY_TIMING_H

#include <stdint.h>
#include <stdbool.h>

#define DM_NO_ERR         0
#define DM_MODE_NOT_AVAIL -620
#define DM_PARAM_ERR      -50

typedef uint32_t VideoTimingID;

#define TIMING_12_INCH_RGB    0x01 // 512x384 @ 60 Hz
#define TIMING_13_INCH_RGB    0x02 // 640x480 @ 67 Hz
#define TIMING_PORTRAIT       0x03 // 640x870 @ 75 Hz
#define TIMING_16_INCH_RGB    0x04 // 832x624 @ 75 Hz
#define TIMING_VGA            0x05 // 640x480 @ 60 Hz

typedef struct VideoTimingRec {
    VideoTimingID timingID;
    int16_t width;
    int16_t height;
    int16_t refreshRate;
    int16_t maxDepth;
    uint32_t dotClockHz;
    uint8_t sonoraTimingCode;
} VideoTimingRec;

/* Video Timing API */
int16_t lc3_dm_get_video_timing_info(VideoTimingID timingID, VideoTimingRec *info);
int16_t lc3_dm_set_video_timing(VideoTimingID timingID);
int16_t lc3_dm_get_display_mode(VideoTimingID *currentTiming, int16_t *currentDepth);
int16_t lc3_dm_set_display_mode(VideoTimingID timingID, int16_t depth);
int16_t lc3_dm_check_display_mode(VideoTimingID timingID, int16_t depth);

#endif /* LC3_DISPLAY_TIMING_H */
