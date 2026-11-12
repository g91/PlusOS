/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Control Strip Manager Core Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_CONTROL_STRIP_H
#define LC3_CONTROL_STRIP_H

#include <stdint.h>
#include <stdbool.h>

#define CS_NO_ERR         0
#define CS_PARAM_ERR    -50

typedef struct ControlStripRecord {
    bool isVisible;
    bool isCollapsed;
    int16_t screenPosition; // 0 = bottom-left, 1 = bottom-right
    int16_t stripHeight;
    int16_t stripWidth;
    uint32_t moduleCount;
} ControlStripRecord;

/* Control Strip API */
bool lc3_sb_is_control_strip_visible(void);
void lc3_sb_show_hide_control_strip(bool showIt);
int16_t lc3_sb_save_window_state(void *stateBuf);
int16_t lc3_sb_load_window_state(const void *stateBuf);
void lc3_sb_set_track(bool track);
uint32_t lc3_sb_get_version(void);

#endif /* LC3_CONTROL_STRIP_H */
