/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Color QuickDraw Custom Pen Modes & BitBlt Arithmetic Modes
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#include "lc3_cqd_pen_modes.h"
#include <string.h>

void lc3_cqd_apply_arithmetic_mode(uint16_t mode, const RGBColor *src, RGBColor *dst, const RGBColor *opColor)
{
    if (!src || !dst) return;

    uint32_t r, g, b;

    switch (mode) {
        case blend: {
            uint32_t weight = opColor ? opColor->red : 0x8000;
            r = ((uint32_t)src->red * weight + (uint32_t)dst->red * (65535 - weight)) >> 16;
            g = ((uint32_t)src->green * weight + (uint32_t)dst->green * (65535 - weight)) >> 16;
            b = ((uint32_t)src->blue * weight + (uint32_t)dst->blue * (65535 - weight)) >> 16;
            break;
        }
        case addPin:
            r = (uint32_t)src->red + (uint32_t)dst->red; if (r > 65535) r = 65535;
            g = (uint32_t)src->green + (uint32_t)dst->green; if (g > 65535) g = 65535;
            b = (uint32_t)src->blue + (uint32_t)dst->blue; if (b > 65535) b = 65535;
            break;
        case subPin:
            r = (dst->red > src->red) ? (dst->red - src->red) : 0;
            g = (dst->green > src->green) ? (dst->green - src->green) : 0;
            b = (dst->blue > src->blue) ? (dst->blue - src->blue) : 0;
            break;
        case adMax:
            r = (src->red > dst->red) ? src->red : dst->red;
            g = (src->green > dst->green) ? src->green : dst->green;
            b = (src->blue > dst->blue) ? src->blue : dst->blue;
            break;
        case adMin:
            r = (src->red < dst->red) ? src->red : dst->red;
            g = (src->green < dst->green) ? src->green : dst->green;
            b = (src->blue < dst->blue) ? src->blue : dst->blue;
            break;
        default:
            r = src->red;
            g = src->green;
            b = src->blue;
            break;
    }

    dst->red = (uint16_t)r;
    dst->green = (uint16_t)g;
    dst->blue = (uint16_t)b;
}

void lc3_cqd_apply_hilite_mode(RGBColor *dst, const RGBColor *hiliteRGB)
{
    if (!dst || !hiliteRGB) return;

    /* Invert colors or replace with HiliteRGB */
    if (dst->red == 0 && dst->green == 0 && dst->blue == 0) {
        *dst = *hiliteRGB;
    } else {
        dst->red = 65535 - dst->red;
        dst->green = 65535 - dst->green;
        dst->blue = 65535 - dst->blue;
    }
}

uint8_t lc3_cqd_apply_boolean_mode(uint16_t mode, uint8_t src, uint8_t dst)
{
    switch (mode & 0x07) {
        case srcCopy:    return src;
        case srcOr:      return (uint8_t)(src | dst);
        case srcXor:     return (uint8_t)(src ^ dst);
        case srcBic:     return (uint8_t)(dst & ~src);
        case notSrcCopy: return (uint8_t)(~src);
        case notSrcOr:   return (uint8_t)(~src | dst);
        case notSrcXor:  return (uint8_t)(~src ^ dst);
        case notSrcBic:  return (uint8_t)(dst & src);
        default:         return src;
    }
}

bool lc3_cqd_apply_transparent_mode(const RGBColor *src, const RGBColor *keyColor)
{
    if (!src || !keyColor) return false;

    return (src->red == keyColor->red &&
            src->green == keyColor->green &&
            src->blue == keyColor->blue);
}

void lc3_cqd_pattern_mask_line(const Pattern8 *pat, int16_t y, uint8_t *dstLine, int16_t width)
{
    if (!pat || !dstLine || width <= 0) return;

    uint8_t patByte = pat->pat[y & 7];
    for (int16_t x = 0; x < width; ++x) {
        bool bit = (patByte & (1 << (7 - (x & 7)))) != 0;
        if (!bit) {
            dstLine[x] = 0x00; /* Stipple hole */
        }
    }
}

uint16_t lc3_cqd_resolve_effective_mode(uint16_t baseMode, bool dither, bool hilited)
{
    uint16_t mode = baseMode;
    if (dither) mode |= ditherCopy;
    if (hilited) mode |= hilite;
    return mode;
}
