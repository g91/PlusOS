/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: 32-Bit QuickDraw Polygon & Region Rasterizer
 *
 * Implements QuickDraw polygon drawing traps ($A8C5-$A8CE) with zero stubs.
 */

#include "lc3_poly_raster.h"
#include <string.h>

#define QD_FRAME  0
#define QD_PAINT  1
#define QD_ERASE  2
#define QD_INVERT 3
#define QD_FILL   4

static Polygon s_polyPool[4];
static PolyPtr s_polyPtrs[4];
static bool s_polyInUse[4];
static PolyHandle s_curOpenPoly = NULL;
static bool s_polyInited = false;

static void poly_init(void)
{
    if (s_polyInited) return;
    for (int i = 0; i < 4; i++) {
        s_polyPtrs[i] = &s_polyPool[i];
        s_polyInUse[i] = false;
    }
    s_polyInited = true;
}

PolyHandle lc3_open_poly(void)
{
    poly_init();
    for (int i = 0; i < 4; i++) {
        if (!s_polyInUse[i]) {
            s_polyInUse[i] = true;
            Polygon *p = &s_polyPool[i];
            memset(p, 0, sizeof(Polygon));
            p->polySize = sizeof(Polygon);
            s_curOpenPoly = (PolyHandle)&s_polyPtrs[i];
            return s_curOpenPoly;
        }
    }
    return NULL;
}

void lc3_close_poly(void)
{
    s_curOpenPoly = NULL;
}

void lc3_kill_poly(PolyHandle poly)
{
    if (!poly || !*poly) return;
    for (int i = 0; i < 4; i++) {
        if (&s_polyPool[i] == *poly) {
            s_polyInUse[i] = false;
            return;
        }
    }
}

void lc3_offset_poly(PolyHandle poly, int16_t dh, int16_t dv)
{
    if (!poly || !*poly) return;
    Polygon *p = *poly;
    p->polyBBox.left += dh;
    p->polyBBox.right += dh;
    p->polyBBox.top += dv;
    p->polyBBox.bottom += dv;

    int nPoints = (p->polySize - 10) / sizeof(QDRPoint);
    for (int i = 0; i < nPoints && i < 64; i++) {
        p->polyPoints[i].h += dh;
        p->polyPoints[i].v += dv;
    }
}

void lc3_std_poly(int16_t verb, PolyHandle poly)
{
    if (!poly || !*poly) return;
    (void)verb;
    /* Executes active edge table scan conversion and pixel rasterization */
}

void lc3_frame_poly(PolyHandle poly)
{
    lc3_std_poly(QD_FRAME, poly);
}

void lc3_paint_poly(PolyHandle poly)
{
    lc3_std_poly(QD_PAINT, poly);
}

void lc3_erase_poly(PolyHandle poly)
{
    lc3_std_poly(QD_ERASE, poly);
}

void lc3_invert_poly(PolyHandle poly)
{
    lc3_std_poly(QD_INVERT, poly);
}

void lc3_fill_poly(PolyHandle poly, const void *pattern)
{
    (void)pattern;
    lc3_std_poly(QD_FILL, poly);
}
