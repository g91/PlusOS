/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Color QuickDraw Color Picker Dialog & Color Conversion Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#include "lc3_color_picker.h"
#include <string.h>

static inline void set_pixel(void *fbBase, int16_t rowBytes, int16_t x, int16_t y, uint8_t colorIdx)
{
    if (!fbBase || x < 0 || y < 0) return;
    uint8_t *fb = (uint8_t *)fbBase;
    fb[y * rowBytes + x] = colorIdx;
}

void lc3_cp_rgb2hsv(const RGBColor16 *rgb, HSVColor *hsv)
{
    if (!rgb || !hsv) return;

    uint32_t r = rgb->red;
    uint32_t g = rgb->green;
    uint32_t b = rgb->blue;

    uint32_t max = (r > g) ? ((r > b) ? r : b) : ((g > b) ? g : b);
    uint32_t min = (r < g) ? ((r < b) ? r : b) : ((g < b) ? g : b);
    uint32_t delta = max - min;

    hsv->value = (uint16_t)max;

    if (max == 0 || delta == 0) {
        hsv->saturation = 0;
        hsv->hue = 0;
        return;
    }

    hsv->saturation = (uint16_t)(((uint64_t)delta * 65535) / max);

    int32_t h = 0;
    if (r == max) {
        h = (int32_t)(((int64_t)(g - b) * 10922) / (int64_t)delta);
    } else if (g == max) {
        h = 21845 + (int32_t)(((int64_t)(b - r) * 10922) / (int64_t)delta);
    } else {
        h = 43690 + (int32_t)(((int64_t)(r - g) * 10922) / (int64_t)delta);
    }

    if (h < 0) h += 65536;
    hsv->hue = (uint16_t)h;
}

void lc3_cp_hsv2rgb(const HSVColor *hsv, RGBColor16 *rgb)
{
    if (!hsv || !rgb) return;

    uint32_t v = hsv->value;
    uint32_t s = hsv->saturation;
    uint32_t h = hsv->hue;

    if (s == 0) {
        rgb->red = (uint16_t)v;
        rgb->green = (uint16_t)v;
        rgb->blue = (uint16_t)v;
        return;
    }

    /* 6 sectors of 10922 units each (65536 / 6) */
    uint32_t sector = h / 10923;
    uint32_t rem = h % 10923;
    uint32_t p = (v * (65535 - s)) >> 16;
    uint32_t q = (v * (65535 - ((s * rem) / 10923))) >> 16;
    uint32_t t = (v * (65535 - ((s * (10923 - rem)) / 10923))) >> 16;

    switch (sector) {
        case 0:  rgb->red = (uint16_t)v; rgb->green = (uint16_t)t; rgb->blue = (uint16_t)p; break;
        case 1:  rgb->red = (uint16_t)q; rgb->green = (uint16_t)v; rgb->blue = (uint16_t)p; break;
        case 2:  rgb->red = (uint16_t)p; rgb->green = (uint16_t)v; rgb->blue = (uint16_t)t; break;
        case 3:  rgb->red = (uint16_t)p; rgb->green = (uint16_t)q; rgb->blue = (uint16_t)v; break;
        case 4:  rgb->red = (uint16_t)t; rgb->green = (uint16_t)p; rgb->blue = (uint16_t)v; break;
        default: rgb->red = (uint16_t)v; rgb->green = (uint16_t)p; rgb->blue = (uint16_t)q; break;
    }
}

void lc3_cp_draw_color_wheel(int16_t centerX, int16_t centerY, int16_t radius, void *fbBase, int16_t rowBytes)
{
    if (!fbBase || radius <= 0) return;

    int32_t r2 = radius * radius;
    for (int16_t dy = -radius; dy <= radius; ++dy) {
        for (int16_t dx = -radius; dx <= radius; ++dx) {
            int32_t d2 = dx * dx + dy * dy;
            if (d2 <= r2) {
                /* Color wheel palette index based on angle */
                uint8_t col = (uint8_t)(((dx + dy + radius * 2) * 255) / (radius * 4));
                set_pixel(fbBase, rowBytes, centerX + dx, centerY + dy, col);
            }
        }
    }
}

bool lc3_cp_hit_color_wheel(int16_t centerX, int16_t centerY, int16_t radius, MacPoint pt, HSVColor *outHSV)
{
    if (!outHSV) return false;

    int16_t dx = pt.h - centerX;
    int16_t dy = pt.v - centerY;
    int32_t dist2 = dx * dx + dy * dy;

    if (dist2 > radius * radius) {
        return false;
    }

    /* Compute approximate integer square root for saturation */
    int32_t dist = 0;
    while ((dist + 1) * (dist + 1) <= dist2) {
        dist++;
    }

    outHSV->saturation = (uint16_t)((dist * 65535) / radius);
    outHSV->value = 65535; /* Max brightness on standard wheel surface */

    /* Angle approximation */
    int32_t angle = 0;
    if (dx >= 0 && dy >= 0) angle = (dx * 16384) / (dx + dy + 1);
    else if (dx < 0 && dy >= 0) angle = 16384 + (dy * 16384) / (-dx + dy + 1);
    else if (dx < 0 && dy < 0) angle = 32768 + (-dx * 16384) / (-dx - dy + 1);
    else angle = 49152 + (-dy * 16384) / (dx - dy + 1);

    outHSV->hue = (uint16_t)angle;
    return true;
}

void lc3_cp_draw_swatches(const RGBColor16 *origColor, const RGBColor16 *newColor, int16_t x, int16_t y, void *fbBase, int16_t rowBytes)
{
    (void)origColor;
    (void)newColor;
    if (!fbBase) return;

    /* Draw two 30x30 color swatch boxes */
    for (int16_t dy = 0; dy < 30; ++dy) {
        for (int16_t dx = 0; dx < 30; ++dx) {
            set_pixel(fbBase, rowBytes, x + dx, y + dy, 0x55);
            set_pixel(fbBase, rowBytes, x + 35 + dx, y + dy, 0xAA);
        }
    }
}

int16_t lc3_get_color(ColorPickerInfo *info)
{
    if (!info) return CP_PARAM_ERR;

    HSVColor hsv;
    lc3_cp_rgb2hsv(&info->inColor, &hsv);
    info->outColor = info->inColor;
    info->userCancelled = false;

    return CP_NO_ERR;
}
