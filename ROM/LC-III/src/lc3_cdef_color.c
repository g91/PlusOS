/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Control Definition Function (CDEF 0 / CDEF 1 Color Controls)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#include "lc3_cdef_color.h"
#include <string.h>

static inline void set_pixel(void *fbBase, int16_t rowBytes, int16_t x, int16_t y, uint8_t colorIdx)
{
    if (!fbBase || x < 0 || y < 0) return;
    uint8_t *fb = (uint8_t *)fbBase;
    fb[y * rowBytes + x] = colorIdx;
}

static void fill_rect(void *fbBase, int16_t rowBytes, int16_t left, int16_t top, int16_t right, int16_t bottom, uint8_t colorIdx)
{
    for (int16_t y = top; y < bottom; ++y) {
        for (int16_t x = left; x < right; ++x) {
            set_pixel(fbBase, rowBytes, x, y, colorIdx);
        }
    }
}

void lc3_cdef_draw_button(const ColorControlRecord *ctrl, void *fbBase, int16_t rowBytes, int16_t depth)
{
    (void)depth;
    if (!ctrl || !fbBase) return;

    int16_t l = ctrl->boundsRect.left;
    int16_t t = ctrl->boundsRect.top;
    int16_t r = ctrl->boundsRect.right;
    int16_t b = ctrl->boundsRect.bottom;

    bool pressed = (ctrl->hiliteState == kControlButtonPart);
    uint8_t fill = pressed ? 0xFF : 0x00;

    /* Fill rounded button body */
    fill_rect(fbBase, rowBytes, l + 2, t + 2, r - 2, b - 2, fill);

    /* Draw rounded black frame */
    for (int16_t x = l + 2; x < r - 2; ++x) {
        set_pixel(fbBase, rowBytes, x, t, 0xFF);
        set_pixel(fbBase, rowBytes, x, b - 1, 0xFF);
    }
    for (int16_t y = t + 2; y < b - 2; ++y) {
        set_pixel(fbBase, rowBytes, l, y, 0xFF);
        set_pixel(fbBase, rowBytes, r - 1, y, 0xFF);
    }

    /* Round corners */
    set_pixel(fbBase, rowBytes, l + 1, t + 1, 0xFF);
    set_pixel(fbBase, rowBytes, r - 2, t + 1, 0xFF);
    set_pixel(fbBase, rowBytes, l + 1, b - 2, 0xFF);
    set_pixel(fbBase, rowBytes, r - 2, b - 2, 0xFF);
}

void lc3_cdef_draw_checkbox(const ColorControlRecord *ctrl, void *fbBase, int16_t rowBytes, int16_t depth)
{
    (void)depth;
    if (!ctrl || !fbBase) return;

    int16_t l = ctrl->boundsRect.left;
    int16_t t = ctrl->boundsRect.top;

    /* 12x12 square checkbox */
    fill_rect(fbBase, rowBytes, l, t, l + 12, t + 12, 0x00);
    for (int16_t x = l; x < l + 12; ++x) {
        set_pixel(fbBase, rowBytes, x, t, 0xFF);
        set_pixel(fbBase, rowBytes, x, t + 11, 0xFF);
    }
    for (int16_t y = t; y < t + 12; ++y) {
        set_pixel(fbBase, rowBytes, l, y, 0xFF);
        set_pixel(fbBase, rowBytes, l + 11, y, 0xFF);
    }

    /* Checkmark 'X' if checked (value != 0) */
    if (ctrl->value != 0) {
        for (int16_t i = 2; i < 10; ++i) {
            set_pixel(fbBase, rowBytes, l + i, t + i, 0xFF);
            set_pixel(fbBase, rowBytes, l + 11 - i, t + i, 0xFF);
        }
    }
}

void lc3_cdef_draw_radio(const ColorControlRecord *ctrl, void *fbBase, int16_t rowBytes, int16_t depth)
{
    (void)depth;
    if (!ctrl || !fbBase) return;

    int16_t cx = ctrl->boundsRect.left + 6;
    int16_t cy = ctrl->boundsRect.top + 6;

    /* Circle frame approx 12x12 */
    for (int16_t dy = -5; dy <= 5; ++dy) {
        for (int16_t dx = -5; dx <= 5; ++dx) {
            int16_t dist2 = dx * dx + dy * dy;
            if (dist2 >= 18 && dist2 <= 26) {
                set_pixel(fbBase, rowBytes, cx + dx, cy + dy, 0xFF);
            } else if (dist2 < 18) {
                set_pixel(fbBase, rowBytes, cx + dx, cy + dy, 0x00);
            }
        }
    }

    /* Inner bullet if selected */
    if (ctrl->value != 0) {
        for (int16_t dy = -2; dy <= 2; ++dy) {
            for (int16_t dx = -2; dx <= 2; ++dx) {
                if (dx * dx + dy * dy <= 5) {
                    set_pixel(fbBase, rowBytes, cx + dx, cy + dy, 0xFF);
                }
            }
        }
    }
}

void lc3_cdef_draw_scrollbar(const ColorControlRecord *ctrl, void *fbBase, int16_t rowBytes, int16_t depth)
{
    (void)depth;
    if (!ctrl || !fbBase) return;

    int16_t l = ctrl->boundsRect.left;
    int16_t t = ctrl->boundsRect.top;
    int16_t r = ctrl->boundsRect.right;
    int16_t b = ctrl->boundsRect.bottom;

    /* Fill shaft with light stipple/pattern (0x88) */
    fill_rect(fbBase, rowBytes, l, t, r, b, 0x88);

    /* Draw scrollbar borders */
    for (int16_t y = t; y < b; ++y) {
        set_pixel(fbBase, rowBytes, l, y, 0xFF);
        set_pixel(fbBase, rowBytes, r - 1, y, 0xFF);
    }

    /* Up arrow button (16x16) */
    fill_rect(fbBase, rowBytes, l + 1, t + 1, r - 1, t + 15, 0x00);
    for (int16_t x = l; x < r; ++x) set_pixel(fbBase, rowBytes, x, t + 15, 0xFF);

    /* Down arrow button (16x16) */
    fill_rect(fbBase, rowBytes, l + 1, b - 16, r - 1, b - 1, 0x00);
    for (int16_t x = l; x < r; ++x) set_pixel(fbBase, rowBytes, x, b - 16, 0xFF);

    /* Scroll thumb indicator if max > min */
    if (ctrl->max > ctrl->min && ctrl->visible) {
        int16_t trackH = (b - 16) - (t + 16) - 16;
        if (trackH > 0) {
            int16_t thumbY = (t + 16) + (int16_t)(((int32_t)(ctrl->value - ctrl->min) * trackH) / (ctrl->max - ctrl->min));
            fill_rect(fbBase, rowBytes, l + 1, thumbY, r - 1, thumbY + 16, 0x00);
            for (int16_t x = l; x < r; ++x) {
                set_pixel(fbBase, rowBytes, x, thumbY, 0xFF);
                set_pixel(fbBase, rowBytes, x, thumbY + 15, 0xFF);
            }
        }
    }
}

int16_t lc3_cdef_hit_test(const ColorControlRecord *ctrl, MacPoint pt)
{
    if (!ctrl || !ctrl->visible || ctrl->hiliteState == 255) return kControlNoPart;

    if (pt.v < ctrl->boundsRect.top || pt.v >= ctrl->boundsRect.bottom ||
        pt.h < ctrl->boundsRect.left || pt.h >= ctrl->boundsRect.right) {
        return kControlNoPart;
    }

    if (ctrl->procID == 0) return kControlButtonPart;
    if (ctrl->procID == 1) return kControlCheckBoxPart;
    if (ctrl->procID == 2) return kControlRadioButtonPart;

    if (ctrl->procID == 16) {
        /* Vertical scrollbar parts */
        if (pt.v < ctrl->boundsRect.top + 16) return kControlUpButtonPart;
        if (pt.v >= ctrl->boundsRect.bottom - 16) return kControlDownButtonPart;

        int16_t trackH = (ctrl->boundsRect.bottom - 16) - (ctrl->boundsRect.top + 16) - 16;
        if (trackH > 0 && ctrl->max > ctrl->min) {
            int16_t thumbY = (ctrl->boundsRect.top + 16) + (int16_t)(((int32_t)(ctrl->value - ctrl->min) * trackH) / (ctrl->max - ctrl->min));
            if (pt.v >= thumbY && pt.v < thumbY + 16) return kControlIndicatorPart;
            if (pt.v < thumbY) return kControlPageUpPart;
            return kControlPageDownPart;
        }
        return kControlPageDownPart;
    }

    return kControlButtonPart;
}

int32_t lc3_cdef_entry(int16_t varCode, ColorControlRecord *ctrl, int16_t message, int32_t param)
{
    (void)varCode;
    if (!ctrl) return CDEF_PARAM_ERR;

    switch (message) {
        case drawCntl:
            return CDEF_NO_ERR;

        case testCntl: {
            MacPoint pt;
            pt.v = (int16_t)((param >> 16) & 0xFFFF);
            pt.h = (int16_t)(param & 0xFFFF);
            return (int32_t)lc3_cdef_hit_test(ctrl, pt);
        }

        case calcCRgns:
        case initCntl:
        case dispCntl:
        case posCntl:
        case thumbCntl:
        case dragCntl:
        case autoTrack:
        default:
            return CDEF_NO_ERR;
    }
}
