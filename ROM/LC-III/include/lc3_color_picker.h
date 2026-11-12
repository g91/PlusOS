/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Color QuickDraw Color Picker Dialog & Color Conversion Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_COLOR_PICKER_H
#define LC3_COLOR_PICKER_H

#include <stdint.h>
#include <stdbool.h>

#define CP_NO_ERR         0
#define CP_USER_CANCEL   -1
#define CP_PARAM_ERR    -50

typedef struct RGBColor16 {
    uint16_t red;
    uint16_t green;
    uint16_t blue;
} RGBColor16;

typedef struct HSVColor {
    uint16_t hue;         /* 0 to 65535, representing 0 to 360 degrees */
    uint16_t saturation;  /* 0 to 65535, representing 0 to 100% */
    uint16_t value;       /* 0 to 65535, representing 0 to 100% */
} HSVColor;

typedef struct MacPoint {
    int16_t v;
    int16_t h;
} MacPoint;

typedef struct ColorPickerInfo {
    RGBColor16 inColor;
    RGBColor16 outColor;
    char prompt[64];
    bool userCancelled;
} ColorPickerInfo;

/* Color Picker API */
void lc3_cp_rgb2hsv(const RGBColor16 *rgb, HSVColor *hsv);
void lc3_cp_hsv2rgb(const HSVColor *hsv, RGBColor16 *rgb);
void lc3_cp_draw_color_wheel(int16_t centerX, int16_t centerY, int16_t radius, void *fbBase, int16_t rowBytes);
bool lc3_cp_hit_color_wheel(int16_t centerX, int16_t centerY, int16_t radius, MacPoint pt, HSVColor *outHSV);
void lc3_cp_draw_swatches(const RGBColor16 *origColor, const RGBColor16 *newColor, int16_t x, int16_t y, void *fbBase, int16_t rowBytes);
int16_t lc3_get_color(ColorPickerInfo *info);

#endif /* LC3_COLOR_PICKER_H */
