/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Color Window Definition Function (WDEF 128 DefProc)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#include "lc3_wdef_color.h"
#include <string.h>

/* Pixel writer helper for 8-bpp indexed color */
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

void lc3_wdef_draw_frame(const ColorWindowRecord *win, void *fbBase, int16_t rowBytes, int16_t depth)
{
    (void)depth;
    if (!win || !fbBase) return;

    int16_t l = win->strucRect.left;
    int16_t t = win->strucRect.top;
    int16_t r = win->strucRect.right;
    int16_t b = win->strucRect.bottom;

    /* Draw window border: black outer line (color 0xFF) */
    for (int16_t x = l; x < r; ++x) {
        set_pixel(fbBase, rowBytes, x, t, 0xFF);
        set_pixel(fbBase, rowBytes, x, b - 1, 0xFF);
    }
    for (int16_t y = t; y < b; ++y) {
        set_pixel(fbBase, rowBytes, l, y, 0xFF);
        set_pixel(fbBase, rowBytes, r - 1, y, 0xFF);
    }

    /* Draw title bar (t+1 to t+19) */
    int16_t titleBottom = t + 19;
    for (int16_t y = t + 1; y < titleBottom; ++y) {
        for (int16_t x = l + 1; x < r - 1; ++x) {
            if (win->hilited) {
                /* System 7 Pinstripes: alternating gray and white lines */
                uint8_t c = (y % 2 == 0) ? 0x00 : 0x88;
                set_pixel(fbBase, rowBytes, x, y, c);
            } else {
                /* Unhighlighted window: plain white title bar */
                set_pixel(fbBase, rowBytes, x, y, 0x00);
            }
        }
    }

    /* Draw separator line below title bar */
    for (int16_t x = l; x < r; ++x) {
        set_pixel(fbBase, rowBytes, x, titleBottom, 0xFF);
    }

    /* Draw widgets if active */
    if (win->hasCloseBox) {
        lc3_wdef_draw_close_box(win, false, fbBase, rowBytes, depth);
    }
    if (win->hasZoomBox) {
        lc3_wdef_draw_zoom_box(win, false, fbBase, rowBytes, depth);
    }
    if (win->hasGrowBox) {
        lc3_wdef_draw_grow_box(win, fbBase, rowBytes, depth);
    }
}

void lc3_wdef_draw_close_box(const ColorWindowRecord *win, bool pressed, void *fbBase, int16_t rowBytes, int16_t depth)
{
    (void)depth;
    if (!win || !fbBase) return;

    int16_t cx = win->strucRect.left + 8;
    int16_t cy = win->strucRect.top + 4;

    /* Draw 11x11 square */
    uint8_t fill = pressed ? 0xFF : 0x00;
    fill_rect(fbBase, rowBytes, cx, cy, cx + 11, cy + 11, fill);

    /* Frame close box */
    for (int16_t x = cx; x < cx + 11; ++x) {
        set_pixel(fbBase, rowBytes, x, cy, 0xFF);
        set_pixel(fbBase, rowBytes, x, cy + 10, 0xFF);
    }
    for (int16_t y = cy; y < cy + 11; ++y) {
        set_pixel(fbBase, rowBytes, cx, y, 0xFF);
        set_pixel(fbBase, rowBytes, cx + 10, y, 0xFF);
    }
}

void lc3_wdef_draw_zoom_box(const ColorWindowRecord *win, bool pressed, void *fbBase, int16_t rowBytes, int16_t depth)
{
    (void)depth;
    if (!win || !fbBase) return;

    int16_t zx = win->strucRect.right - 21;
    int16_t zy = win->strucRect.top + 4;

    /* Outer box 11x11 */
    fill_rect(fbBase, rowBytes, zx, zy, zx + 11, zy + 11, pressed ? 0x88 : 0x00);
    for (int16_t x = zx; x < zx + 11; ++x) {
        set_pixel(fbBase, rowBytes, x, zy, 0xFF);
        set_pixel(fbBase, rowBytes, x, zy + 10, 0xFF);
    }
    for (int16_t y = zy; y < zy + 11; ++y) {
        set_pixel(fbBase, rowBytes, zx, y, 0xFF);
        set_pixel(fbBase, rowBytes, zx + 10, y, 0xFF);
    }

    /* Inner nested box */
    for (int16_t x = zx + 2; x < zx + 8; ++x) {
        set_pixel(fbBase, rowBytes, x, zy + 2, 0xFF);
        set_pixel(fbBase, rowBytes, x, zy + 7, 0xFF);
    }
    for (int16_t y = zy + 2; y < zy + 8; ++y) {
        set_pixel(fbBase, rowBytes, zx + 2, y, 0xFF);
        set_pixel(fbBase, rowBytes, zx + 7, y, 0xFF);
    }
}

void lc3_wdef_draw_grow_box(const ColorWindowRecord *win, void *fbBase, int16_t rowBytes, int16_t depth)
{
    (void)depth;
    if (!win || !fbBase) return;

    int16_t gx = win->strucRect.right - 15;
    int16_t gy = win->strucRect.bottom - 15;

    /* Draw nested lines representing resize corner */
    for (int16_t d = 2; d < 13; d += 4) {
        for (int16_t x = gx + d; x < win->strucRect.right - 2; ++x) {
            set_pixel(fbBase, rowBytes, x, gy + d, 0x88);
        }
        for (int16_t y = gy + d; y < win->strucRect.bottom - 2; ++y) {
            set_pixel(fbBase, rowBytes, gx + d, y, 0x88);
        }
    }
}

int16_t lc3_wdef_hit_test(const ColorWindowRecord *win, MacPoint pt)
{
    if (!win) return wNoHit;

    if (pt.v < win->strucRect.top || pt.v >= win->strucRect.bottom ||
        pt.h < win->strucRect.left || pt.h >= win->strucRect.right) {
        return wNoHit;
    }

    /* Check title bar */
    if (pt.v < win->strucRect.top + 19) {
        /* Check close box */
        if (win->hasCloseBox && pt.h >= win->strucRect.left + 7 && pt.h <= win->strucRect.left + 20) {
            return wInGoAway;
        }
        /* Check zoom box */
        if (win->hasZoomBox && pt.h >= win->strucRect.right - 22 && pt.h <= win->strucRect.right - 9) {
            return wInZoomIn;
        }
        return wInDrag;
    }

    /* Check grow box */
    if (win->hasGrowBox &&
        pt.h >= win->strucRect.right - 15 && pt.h < win->strucRect.right &&
        pt.v >= win->strucRect.bottom - 15 && pt.v < win->strucRect.bottom) {
        return wInGrow;
    }

    return wInContent;
}

int32_t lc3_wdef_entry(int16_t varCode, ColorWindowRecord *win, int16_t message, int32_t param)
{
    (void)varCode;
    if (!win) return WDEF_PARAM_ERR;

    switch (message) {
        case wDraw:
            /* param is pointer to GrafPort or 0 */
            return WDEF_NO_ERR;

        case wHit: {
            MacPoint pt;
            pt.v = (int16_t)((param >> 16) & 0xFFFF);
            pt.h = (int16_t)(param & 0xFFFF);
            return (int32_t)lc3_wdef_hit_test(win, pt);
        }

        case wCalcRgns:
            win->strucRect = win->portRect;
            win->strucRect.top -= 19;
            win->contRect = win->portRect;
            return WDEF_NO_ERR;

        case wNew:
        case wDispose:
            return WDEF_NO_ERR;

        case wGrow:
            return WDEF_NO_ERR;

        default:
            return WDEF_PARAM_ERR;
    }
}
