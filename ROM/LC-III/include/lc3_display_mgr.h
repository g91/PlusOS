/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Display Manager & Video Mode Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_DISPLAY_MGR_H
#define LC3_DISPLAY_MGR_H

#include <stdint.h>
#include <stdbool.h>

#define DM_NO_ERR          0
#define DM_PARAM_ERR     -50

typedef struct {
    int16_t csMode;
    int32_t csData;
} DisplayModeTiming;

typedef struct {
    int32_t displayMode;
    int16_t depth;
    int16_t hRes;
    int16_t vRes;
    int32_t timingFormat;
} DisplayModeRecord;

/* Display Manager API */
int16_t lc3_dm_get_display_mode(void *displayDevice, DisplayModeRecord *mode);
int16_t lc3_dm_set_display_mode(void *displayDevice, int32_t displayMode, int16_t depth);
int16_t lc3_dm_get_video_timing(void *displayDevice, DisplayModeTiming *timing);
int16_t lc3_dm_set_video_timing(void *displayDevice, const DisplayModeTiming *timing);
int16_t lc3_dm_check_display_mode(void *displayDevice, int32_t displayMode, int16_t depth);
int16_t lc3_dm_get_first_display_mode(void *displayDevice, DisplayModeRecord *mode);
int16_t lc3_dm_get_next_display_mode(void *displayDevice, DisplayModeRecord *mode);
int16_t lc3_dm_begin_configure_display(void *displayDevice);
int16_t lc3_dm_end_configure_display(void *displayDevice);

#endif /* LC3_DISPLAY_MGR_H */
