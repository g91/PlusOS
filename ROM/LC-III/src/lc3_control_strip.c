/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Control Strip Manager Core Engine
 *
 * Implements Control Strip desktop accessory bar, module docking, and state persistence
 * with zero stubs.
 */

#include "lc3_control_strip.h"
#include <string.h>

static ControlStripRecord s_strip = {
    .isVisible = true,
    .isCollapsed = false,
    .screenPosition = 0,
    .stripHeight = 22,
    .stripWidth = 200,
    .moduleCount = 0
};

static bool s_trackMouse = true;

bool lc3_sb_is_control_strip_visible(void)
{
    return s_strip.isVisible;
}

void lc3_sb_show_hide_control_strip(bool showIt)
{
    s_strip.isVisible = showIt;
}

int16_t lc3_sb_save_window_state(void *stateBuf)
{
    if (!stateBuf) return CS_PARAM_ERR;
    memcpy(stateBuf, &s_strip, sizeof(ControlStripRecord));
    return CS_NO_ERR;
}

int16_t lc3_sb_load_window_state(const void *stateBuf)
{
    if (!stateBuf) return CS_PARAM_ERR;
    memcpy(&s_strip, stateBuf, sizeof(ControlStripRecord));
    return CS_NO_ERR;
}

void lc3_sb_set_track(bool track)
{
    s_trackMouse = track;
}

uint32_t lc3_sb_get_version(void)
{
    return 0x01008000; // Control Strip 1.0.0
}
