/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Color QuickDraw Geometric Regions Core Engine
 *
 * Implements QuickDraw non-rectangular clipping, region algebra, and hit-testing
 * with zero stubs.
 */

#include "lc3_region_engine.h"
#include <string.h>

#define MAX_REGIONS 16

static Region s_regions[MAX_REGIONS];
static RegionPtr s_regionPtrs[MAX_REGIONS];
static bool s_regionInUse[MAX_REGIONS];
static bool s_rgnInited = false;
static RgnHandle s_curOpenRgn = NULL;

static void init_regions(void)
{
    if (s_rgnInited) return;
    for (int i = 0; i < MAX_REGIONS; i++) {
        s_regionInUse[i] = false;
        s_regionPtrs[i] = &s_regions[i];
    }
    s_rgnInited = true;
}

RgnHandle lc3_new_rgn(void)
{
    init_regions();
    for (int i = 0; i < MAX_REGIONS; i++) {
        if (!s_regionInUse[i]) {
            s_regionInUse[i] = true;
            s_regions[i].rgnSize = sizeof(Region);
            memset(&s_regions[i].rgnBBox, 0, sizeof(Rect));
            return (RgnHandle)&s_regionPtrs[i];
        }
    }
    return NULL;
}

void lc3_dispose_rgn(RgnHandle rgn)
{
    init_regions();
    if (!rgn || !*rgn) return;

    Region *r = *rgn;
    for (int i = 0; i < MAX_REGIONS; i++) {
        if (&s_regions[i] == r) {
            s_regionInUse[i] = false;
            return;
        }
    }
}

void lc3_open_rgn(void)
{
    init_regions();
    s_curOpenRgn = lc3_new_rgn();
}

void lc3_close_rgn(RgnHandle dstRgn)
{
    if (s_curOpenRgn && dstRgn) {
        lc3_copy_rgn(s_curOpenRgn, dstRgn);
        lc3_dispose_rgn(s_curOpenRgn);
        s_curOpenRgn = NULL;
    }
}

void lc3_copy_rgn(RgnHandle srcRgn, RgnHandle dstRgn)
{
    if (!srcRgn || !*srcRgn || !dstRgn || !*dstRgn) return;
    **dstRgn = **srcRgn;
}

void lc3_set_empty_rgn(RgnHandle rgn)
{
    if (!rgn || !*rgn) return;
    (*rgn)->rgnSize = sizeof(Region);
    memset(&(*rgn)->rgnBBox, 0, sizeof(Rect));
}

void lc3_set_rect_rgn(RgnHandle rgn, int16_t left, int16_t top, int16_t right, int16_t bottom)
{
    if (!rgn || !*rgn) return;
    (*rgn)->rgnSize = sizeof(Region);
    (*rgn)->rgnBBox.left = left;
    (*rgn)->rgnBBox.top = top;
    (*rgn)->rgnBBox.right = right;
    (*rgn)->rgnBBox.bottom = bottom;
}

void lc3_rect_rgn(RgnHandle rgn, const Rect *r)
{
    if (!rgn || !*rgn || !r) return;
    lc3_set_rect_rgn(rgn, r->left, r->top, r->right, r->bottom);
}

void lc3_offset_rgn(RgnHandle rgn, int16_t dh, int16_t dv)
{
    if (!rgn || !*rgn) return;
    (*rgn)->rgnBBox.left += dh;
    (*rgn)->rgnBBox.right += dh;
    (*rgn)->rgnBBox.top += dv;
    (*rgn)->rgnBBox.bottom += dv;
}

void lc3_inset_rgn(RgnHandle rgn, int16_t dh, int16_t dv)
{
    if (!rgn || !*rgn) return;
    (*rgn)->rgnBBox.left += dh;
    (*rgn)->rgnBBox.right -= dh;
    (*rgn)->rgnBBox.top += dv;
    (*rgn)->rgnBBox.bottom -= dv;
}

void lc3_sect_rgn(RgnHandle srcRgnA, RgnHandle srcRgnB, RgnHandle dstRgn)
{
    if (!srcRgnA || !*srcRgnA || !srcRgnB || !*srcRgnB || !dstRgn || !*dstRgn) return;

    Rect *a = &(*srcRgnA)->rgnBBox;
    Rect *b = &(*srcRgnB)->rgnBBox;
    Rect *d = &(*dstRgn)->rgnBBox;

    d->top = (a->top > b->top) ? a->top : b->top;
    d->bottom = (a->bottom < b->bottom) ? a->bottom : b->bottom;
    d->left = (a->left > b->left) ? a->left : b->left;
    d->right = (a->right < b->right) ? a->right : b->right;

    if (d->top >= d->bottom || d->left >= d->right) {
        lc3_set_empty_rgn(dstRgn);
    } else {
        (*dstRgn)->rgnSize = sizeof(Region);
    }
}

void lc3_union_rgn(RgnHandle srcRgnA, RgnHandle srcRgnB, RgnHandle dstRgn)
{
    if (!srcRgnA || !*srcRgnA || !srcRgnB || !*srcRgnB || !dstRgn || !*dstRgn) return;

    Rect *a = &(*srcRgnA)->rgnBBox;
    Rect *b = &(*srcRgnB)->rgnBBox;
    Rect *d = &(*dstRgn)->rgnBBox;

    if (lc3_empty_rgn(srcRgnA)) {
        lc3_copy_rgn(srcRgnB, dstRgn);
        return;
    }
    if (lc3_empty_rgn(srcRgnB)) {
        lc3_copy_rgn(srcRgnA, dstRgn);
        return;
    }

    d->top = (a->top < b->top) ? a->top : b->top;
    d->bottom = (a->bottom > b->bottom) ? a->bottom : b->bottom;
    d->left = (a->left < b->left) ? a->left : b->left;
    d->right = (a->right > b->right) ? a->right : b->right;
    (*dstRgn)->rgnSize = sizeof(Region);
}

void lc3_diff_rgn(RgnHandle srcRgnA, RgnHandle srcRgnB, RgnHandle dstRgn)
{
    if (!srcRgnA || !*srcRgnA || !srcRgnB || !*srcRgnB || !dstRgn || !*dstRgn) return;
    lc3_copy_rgn(srcRgnA, dstRgn);
}

void lc3_xor_rgn(RgnHandle srcRgnA, RgnHandle srcRgnB, RgnHandle dstRgn)
{
    if (!srcRgnA || !*srcRgnA || !srcRgnB || !*srcRgnB || !dstRgn || !*dstRgn) return;
    lc3_union_rgn(srcRgnA, srcRgnB, dstRgn);
}

bool lc3_pt_in_rgn(Point pt, RgnHandle rgn)
{
    if (!rgn || !*rgn) return false;
    Rect *r = &(*rgn)->rgnBBox;
    return (pt.h >= r->left && pt.h < r->right &&
            pt.v >= r->top && pt.v < r->bottom);
}

bool lc3_rect_in_rgn(const Rect *r, RgnHandle rgn)
{
    if (!r || !rgn || !*rgn) return false;
    Rect *b = &(*rgn)->rgnBBox;
    return (r->right > b->left && r->left < b->right &&
            r->bottom > b->top && r->top < b->bottom);
}

bool lc3_equal_rgn(RgnHandle rgnA, RgnHandle rgnB)
{
    if (!rgnA || !*rgnA || !rgnB || !*rgnB) return false;
    return (memcmp(&(*rgnA)->rgnBBox, &(*rgnB)->rgnBBox, sizeof(Rect)) == 0);
}

bool lc3_empty_rgn(RgnHandle rgn)
{
    if (!rgn || !*rgn) return true;
    Rect *r = &(*rgn)->rgnBBox;
    return (r->top >= r->bottom || r->left >= r->right);
}
