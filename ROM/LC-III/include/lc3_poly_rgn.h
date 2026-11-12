/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Color QuickDraw Polygon & Region Conversion Engine (Poly2Rgn)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_POLY_RGN_H
#define LC3_POLY_RGN_H

#include <stdint.h>
#include <stdbool.h>

#define POLY_RGN_NO_ERR         0
#define POLY_RGN_OVERFLOW      -1
#define POLY_RGN_PARAM_ERR    -50

#define MAX_POLY_EDGES        64
#define MAX_RGN_POINTS       256

typedef struct MacPoint {
    int16_t v;
    int16_t h;
} MacPoint;

typedef struct MacRect {
    int16_t top;
    int16_t left;
    int16_t bottom;
    int16_t right;
} MacRect;

typedef struct PolyEdge {
    int16_t yMin;
    int16_t yMax;
    int32_t xCurrent16_16;
    int32_t dx16_16;
} PolyEdge;

typedef struct QuickDrawPoly {
    int16_t polySize;
    MacRect polyBBox;
    MacPoint polyPoints[MAX_POLY_EDGES];
    uint16_t numPoints;
} QuickDrawPoly;

typedef struct QuickDrawRgn {
    int16_t rgnSize;
    MacRect rgnBBox;
    int16_t rgnData[MAX_RGN_POINTS];
    uint16_t dataCount;
} QuickDrawRgn;

/* Poly to Region API */
int16_t lc3_poly_to_rgn(const QuickDrawPoly *poly, QuickDrawRgn *outRgn);
int16_t lc3_poly_build_edges(const QuickDrawPoly *poly, PolyEdge *edges, uint16_t maxEdges, uint16_t *actualEdges);
void lc3_poly_advance_aet(PolyEdge *edges, uint16_t *numEdges, int16_t currentY);
int16_t lc3_poly_sweep_scanline(const PolyEdge *edges, uint16_t numEdges, int16_t *xSpans, uint16_t maxSpans, uint16_t *actualSpans);
int16_t lc3_poly_emit_rgn_spans(QuickDrawRgn *rgn, int16_t y, const int16_t *xSpans, uint16_t spanCount);
void lc3_poly_finalize_rgn(QuickDrawRgn *rgn);

#endif /* LC3_POLY_RGN_H */
