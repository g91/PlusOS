/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Color QuickDraw Custom Pen Modes & BitBlt Arithmetic Modes
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_CQD_PEN_MODES_H
#define LC3_CQD_PEN_MODES_H

#include <stdint.h>
#include <stdbool.h>

#define PEN_NO_ERR         0
#define PEN_PARAM_ERR    -50

/* Transfer Modes */
#define srcCopy             0
#define srcOr               1
#define srcXor              2
#define srcBic              3
#define notSrcCopy          4
#define notSrcOr            5
#define notSrcXor           6
#define notSrcBic           7
#define blend              32
#define addPin             33
#define addOver            34
#define subPin             35
#define transparent        36
#define adMin              37
#define adMax              38
#define subOver            39
#define hilite             50
#define ditherCopy         64

typedef struct RGBColor {
    uint16_t red;
    uint16_t green;
    uint16_t blue;
} RGBColor;

typedef struct Pattern8 {
    uint8_t pat[8];
} Pattern8;

/* QuickDraw Transfer Modes API */
void lc3_cqd_apply_arithmetic_mode(uint16_t mode, const RGBColor *src, RGBColor *dst, const RGBColor *opColor);
void lc3_cqd_apply_hilite_mode(RGBColor *dst, const RGBColor *hiliteRGB);
uint8_t lc3_cqd_apply_boolean_mode(uint16_t mode, uint8_t src, uint8_t dst);
bool lc3_cqd_apply_transparent_mode(const RGBColor *src, const RGBColor *keyColor);
void lc3_cqd_pattern_mask_line(const Pattern8 *pat, int16_t y, uint8_t *dstLine, int16_t width);
uint16_t lc3_cqd_resolve_effective_mode(uint16_t baseMode, bool dither, bool hilited);

#endif /* LC3_CQD_PEN_MODES_H */
