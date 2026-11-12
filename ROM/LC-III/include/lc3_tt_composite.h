/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: TrueType 'glyf' Composite Glyph & Component Transformation Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_TT_COMPOSITE_H
#define LC3_TT_COMPOSITE_H

#include <stdint.h>
#include <stdbool.h>

#define TT_COMP_NO_ERR         0
#define TT_COMP_BAD_GLYPH     -1
#define TT_COMP_PARAM_ERR    -50

/* TrueType Composite Glyph Flags */
#define ARG_1_AND_2_ARE_WORDS    0x0001
#define ARGS_ARE_XY_VALUES       0x0002
#define ROUND_XY_TO_GRID         0x0004
#define WE_HAVE_A_SCALE          0x0008
#define MORE_COMPONENTS          0x0020
#define WE_HAVE_AN_X_AND_Y_SCALE 0x0040
#define WE_HAVE_A_TWO_BY_TWO     0x0080
#define WE_HAVE_INSTRUCTIONS     0x0100

typedef struct AffineMatrix2x2 {
    int32_t a;  /* 2.14 fixed point */
    int32_t b;
    int32_t c;
    int32_t d;
} AffineMatrix2x2;

typedef struct CompositeComponent {
    uint16_t flags;
    uint16_t glyphIndex;
    int16_t arg1;
    int16_t arg2;
    AffineMatrix2x2 transform;
} CompositeComponent;

typedef struct TTBoundingBox {
    int16_t xMin;
    int16_t yMin;
    int16_t xMax;
    int16_t yMax;
} TTBoundingBox;

/* TrueType Composite API */
bool lc3_tt_is_composite(int16_t numberOfContours);
int16_t lc3_tt_parse_composite_flags(const uint8_t **stream, CompositeComponent *comp);
void lc3_tt_apply_affine_transform(const AffineMatrix2x2 *matrix, int16_t *x, int16_t *y);
int16_t lc3_tt_assemble_composite(const uint8_t *glyfData, uint32_t glyfLen, CompositeComponent *components, uint16_t maxComp, uint16_t *actualComp);
void lc3_tt_compound_bbox(const TTBoundingBox *b1, const TTBoundingBox *b2, TTBoundingBox *outBox);
void lc3_tt_adjust_phantom_points(int16_t *pointsX, int16_t *pointsY, uint16_t pointCount, int16_t lsb, int16_t advanceWidth);

#endif /* LC3_TT_COMPOSITE_H */
