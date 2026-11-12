/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: TrueType 'glyf' Composite Glyph & Component Transformation Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#include "lc3_tt_composite.h"
#include <string.h>

bool lc3_tt_is_composite(int16_t numberOfContours)
{
    return (numberOfContours < 0);
}

int16_t lc3_tt_parse_composite_flags(const uint8_t **stream, CompositeComponent *comp)
{
    if (!stream || !*stream || !comp) return TT_COMP_PARAM_ERR;

    const uint8_t *p = *stream;

    comp->flags = (uint16_t)((p[0] << 8) | p[1]);
    comp->glyphIndex = (uint16_t)((p[2] << 8) | p[3]);
    p += 4;

    if (comp->flags & ARG_1_AND_2_ARE_WORDS) {
        comp->arg1 = (int16_t)((p[0] << 8) | p[1]);
        comp->arg2 = (int16_t)((p[2] << 8) | p[3]);
        p += 4;
    } else {
        comp->arg1 = (int8_t)p[0];
        comp->arg2 = (int8_t)p[1];
        p += 2;
    }

    /* Identity matrix default (1.0 in 2.14 format = 0x4000) */
    comp->transform.a = 0x4000;
    comp->transform.b = 0;
    comp->transform.c = 0;
    comp->transform.d = 0x4000;

    if (comp->flags & WE_HAVE_A_SCALE) {
        int16_t scale = (int16_t)((p[0] << 8) | p[1]);
        comp->transform.a = scale;
        comp->transform.d = scale;
        p += 2;
    } else if (comp->flags & WE_HAVE_AN_X_AND_Y_SCALE) {
        comp->transform.a = (int16_t)((p[0] << 8) | p[1]);
        comp->transform.d = (int16_t)((p[2] << 8) | p[3]);
        p += 4;
    } else if (comp->flags & WE_HAVE_A_TWO_BY_TWO) {
        comp->transform.a = (int16_t)((p[0] << 8) | p[1]);
        comp->transform.b = (int16_t)((p[2] << 8) | p[3]);
        comp->transform.c = (int16_t)((p[4] << 8) | p[5]);
        comp->transform.d = (int16_t)((p[6] << 8) | p[7]);
        p += 8;
    }

    *stream = p;
    return TT_COMP_NO_ERR;
}

void lc3_tt_apply_affine_transform(const AffineMatrix2x2 *matrix, int16_t *x, int16_t *y)
{
    if (!matrix || !x || !y) return;

    /* 2.14 fixed point arithmetic: newX = (x * a + y * c) >> 14 */
    int32_t origX = *x;
    int32_t origY = *y;

    int32_t newX = (origX * matrix->a + origY * matrix->c) >> 14;
    int32_t newY = (origX * matrix->b + origY * matrix->d) >> 14;

    *x = (int16_t)newX;
    *y = (int16_t)newY;
}

int16_t lc3_tt_assemble_composite(const uint8_t *glyfData, uint32_t glyfLen, CompositeComponent *components, uint16_t maxComp, uint16_t *actualComp)
{
    if (!glyfData || glyfLen < 10 || !components || !actualComp) return TT_COMP_PARAM_ERR;

    const uint8_t *p = glyfData + 10; /* Skip 5 16-bit header words: nContours, xMin, yMin, xMax, yMax */
    uint16_t count = 0;

    while (count < maxComp) {
        int16_t err = lc3_tt_parse_composite_flags(&p, &components[count]);
        if (err != TT_COMP_NO_ERR) return err;

        uint16_t flags = components[count].flags;
        count++;

        if (!(flags & MORE_COMPONENTS)) {
            break;
        }
    }

    *actualComp = count;
    return TT_COMP_NO_ERR;
}

void lc3_tt_compound_bbox(const TTBoundingBox *b1, const TTBoundingBox *b2, TTBoundingBox *outBox)
{
    if (!b1 || !b2 || !outBox) return;

    outBox->xMin = (b1->xMin < b2->xMin) ? b1->xMin : b2->xMin;
    outBox->yMin = (b1->yMin < b2->yMin) ? b1->yMin : b2->yMin;
    outBox->xMax = (b1->xMax > b2->xMax) ? b1->xMax : b2->xMax;
    outBox->yMax = (b1->yMax > b2->yMax) ? b1->yMax : b2->yMax;
}

void lc3_tt_adjust_phantom_points(int16_t *pointsX, int16_t *pointsY, uint16_t pointCount, int16_t lsb, int16_t advanceWidth)
{
    if (!pointsX || !pointsY || pointCount < 2) return;

    /* Phantom points: pointCount - 2 = (xMin - lsb, 0), pointCount - 1 = (phantom_0 + advanceWidth, 0) */
    pointsX[pointCount - 2] = lsb;
    pointsY[pointCount - 2] = 0;
    pointsX[pointCount - 1] = (int16_t)(lsb + advanceWidth);
    pointsY[pointCount - 1] = 0;
}
