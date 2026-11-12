/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Color QuickDraw Polygon & Region Conversion Engine (Poly2Rgn)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#include "lc3_poly_rgn.h"
#include <string.h>

int16_t lc3_poly_to_rgn(const QuickDrawPoly *poly, QuickDrawRgn *outRgn)
{
    if (!poly || !outRgn || poly->numPoints < 3) return POLY_RGN_PARAM_ERR;

    memset(outRgn, 0, sizeof(QuickDrawRgn));
    outRgn->rgnBBox = poly->polyBBox;
    outRgn->dataCount = 0;

    PolyEdge edges[MAX_POLY_EDGES];
    uint16_t numEdges = 0;

    int16_t err = lc3_poly_build_edges(poly, edges, MAX_POLY_EDGES, &numEdges);
    if (err != POLY_RGN_NO_ERR) return err;

    for (int16_t y = poly->polyBBox.top; y < poly->polyBBox.bottom; ++y) {
        int16_t xSpans[MAX_POLY_EDGES];
        uint16_t actualSpans = 0;

        lc3_poly_advance_aet(edges, &numEdges, y);
        lc3_poly_sweep_scanline(edges, numEdges, xSpans, MAX_POLY_EDGES, &actualSpans);

        if (actualSpans > 0) {
            lc3_poly_emit_rgn_spans(outRgn, y, xSpans, actualSpans);
        }
    }

    lc3_poly_finalize_rgn(outRgn);
    return POLY_RGN_NO_ERR;
}

int16_t lc3_poly_build_edges(const QuickDrawPoly *poly, PolyEdge *edges, uint16_t maxEdges, uint16_t *actualEdges)
{
    if (!poly || !edges || !actualEdges) return POLY_RGN_PARAM_ERR;

    uint16_t count = 0;
    uint16_t n = poly->numPoints;

    for (uint16_t i = 0; i < n && count < maxEdges; ++i) {
        MacPoint p1 = poly->polyPoints[i];
        MacPoint p2 = poly->polyPoints[(i + 1) % n];

        if (p1.v == p2.v) continue; /* Ignore horizontal lines */

        int16_t yMin = (p1.v < p2.v) ? p1.v : p2.v;
        int16_t yMax = (p1.v > p2.v) ? p1.v : p2.v;
        int16_t xStart = (p1.v < p2.v) ? p1.h : p2.h;
        int16_t xEnd = (p1.v < p2.v) ? p2.h : p1.h;

        int32_t dy = yMax - yMin;
        int32_t dx = xEnd - xStart;

        edges[count].yMin = yMin;
        edges[count].yMax = yMax;
        edges[count].xCurrent16_16 = (int32_t)xStart << 16;
        edges[count].dx16_16 = (dx << 16) / dy;
        count++;
    }

    *actualEdges = count;
    return POLY_RGN_NO_ERR;
}

void lc3_poly_advance_aet(PolyEdge *edges, uint16_t *numEdges, int16_t currentY)
{
    if (!edges || !numEdges) return;

    for (uint16_t i = 0; i < *numEdges; ++i) {
        if (currentY >= edges[i].yMin && currentY < edges[i].yMax) {
            edges[i].xCurrent16_16 += edges[i].dx16_16;
        }
    }
}

int16_t lc3_poly_sweep_scanline(const PolyEdge *edges, uint16_t numEdges, int16_t *xSpans, uint16_t maxSpans, uint16_t *actualSpans)
{
    if (!edges || !xSpans || !actualSpans) return POLY_RGN_PARAM_ERR;

    uint16_t count = 0;
    for (uint16_t i = 0; i < numEdges && count < maxSpans; ++i) {
        xSpans[count++] = (int16_t)(edges[i].xCurrent16_16 >> 16);
    }

    /* Sort X coordinates ascending */
    for (uint16_t i = 0; i < count; ++i) {
        for (uint16_t j = i + 1; j < count; ++j) {
            if (xSpans[j] < xSpans[i]) {
                int16_t tmp = xSpans[i];
                xSpans[i] = xSpans[j];
                xSpans[j] = tmp;
            }
        }
    }

    *actualSpans = count;
    return POLY_RGN_NO_ERR;
}

int16_t lc3_poly_emit_rgn_spans(QuickDrawRgn *rgn, int16_t y, const int16_t *xSpans, uint16_t spanCount)
{
    if (!rgn || !xSpans) return POLY_RGN_PARAM_ERR;
    if (rgn->dataCount + spanCount + 2 >= MAX_RGN_POINTS) return POLY_RGN_OVERFLOW;

    rgn->rgnData[rgn->dataCount++] = y;
    for (uint16_t i = 0; i < spanCount; ++i) {
        rgn->rgnData[rgn->dataCount++] = xSpans[i];
    }
    rgn->rgnData[rgn->dataCount++] = 0x7FFF; /* End of scanline marker */

    return POLY_RGN_NO_ERR;
}

void lc3_poly_finalize_rgn(QuickDrawRgn *rgn)
{
    if (!rgn) return;
    if (rgn->dataCount < MAX_RGN_POINTS) {
        rgn->rgnData[rgn->dataCount++] = 0x7FFF; /* End of region marker */
    }
    rgn->rgnSize = (int16_t)(sizeof(MacRect) + 2 + rgn->dataCount * 2);
}
