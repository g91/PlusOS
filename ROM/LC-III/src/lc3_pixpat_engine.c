/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Color QuickDraw Pixel Pattern & Dither Pattern Engine
 *
 * Implements PixPat creation, RGB dither pattern generation, and fast tiled
 * rectangle / oval filling with zero stubs.
 */

#include "lc3_pixpat_engine.h"

#define MAX_STATIC_PIXPATS 16
static PixPatRecord s_staticPixPats[MAX_STATIC_PIXPATS];
static PixPatPtr s_staticPixPatPtrs[MAX_STATIC_PIXPATS];
static bool s_pixPatUsed[MAX_STATIC_PIXPATS];

/* 1476: lc3_pixpat_new */
PixPatHandle lc3_pixpat_new(void)
{
    int freeIdx = -1;
    for (int i = 0; i < MAX_STATIC_PIXPATS; ++i) {
        if (!s_pixPatUsed[i]) {
            freeIdx = i;
            break;
        }
    }

    if (freeIdx == -1) return 0;

    s_pixPatUsed[freeIdx] = true;
    PixPatRecord *rec = &s_staticPixPats[freeIdx];
    rec->patType = PAT_TYPE_BW;
    rec->patMap = 0;
    rec->patData = 0;
    rec->patXData = 0;
    rec->patXValid = 0;
    rec->patXMap = 0;

    /* Default 50% gray pattern */
    for (int i = 0; i < 8; ++i) {
        rec->pat1Data[i] = (i % 2 == 0) ? 0xAA : 0x55;
    }

    for (int i = 0; i < 64; ++i) {
        rec->patternData8x8[i] = 0x00;
    }

    s_staticPixPatPtrs[freeIdx] = rec;
    return &s_staticPixPatPtrs[freeIdx];
}

/* 1477: lc3_pixpat_copy */
int16_t lc3_pixpat_copy(PixPatHandle src, PixPatHandle dst)
{
    if (!src || !*src || !dst || !*dst) return PIXPAT_PARAM_ERR;

    PixPatRecord *s = *src;
    PixPatRecord *d = *dst;

    d->patType = s->patType;
    for (int i = 0; i < 8; ++i) {
        d->pat1Data[i] = s->pat1Data[i];
    }
    for (int i = 0; i < 64; ++i) {
        d->patternData8x8[i] = s->patternData8x8[i];
    }

    return PIXPAT_NO_ERR;
}

/* 1478: lc3_pixpat_make_rgb_pat */
int16_t lc3_pixpat_make_rgb_pat(PixPatHandle pp, const RGBColor *myColor)
{
    if (!pp || !*pp || !myColor) return PIXPAT_PARAM_ERR;

    PixPatRecord *p = *pp;
    p->patType = PAT_TYPE_RGB_DITHER;

    /* 8x8 Bayer matrix */
    static const uint8_t bayer8[8][8] = {
        {  0, 32,  8, 40,  2, 34, 10, 42 },
        { 48, 16, 56, 24, 50, 18, 58, 26 },
        { 12, 44,  4, 36, 14, 46,  6, 38 },
        { 60, 28, 52, 20, 62, 30, 54, 22 },
        {  3, 35, 11, 43,  1, 33,  9, 41 },
        { 51, 19, 59, 27, 49, 17, 57, 25 },
        { 15, 47,  7, 39, 13, 45,  5, 37 },
        { 63, 31, 55, 23, 61, 29, 53, 21 }
    };

    /* Quantize to 6x6x6 color cube (216 colors) in 8-bpp */
    uint8_t rLevel = (uint8_t)((myColor->red * 5) / 65535);
    uint8_t gLevel = (uint8_t)((myColor->green * 5) / 65535);
    uint8_t bLevel = (uint8_t)((myColor->blue * 5) / 65535);

    uint8_t baseIndex = (uint8_t)(rLevel * 36 + gLevel * 6 + bLevel);

    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            uint8_t threshold = bayer8[y][x];
            uint8_t add = (myColor->green >> 10) > threshold ? 1 : 0;
            p->patternData8x8[y * 8 + x] = (uint8_t)(baseIndex + add);
        }
    }

    return PIXPAT_NO_ERR;
}

/* 1479: lc3_pixpat_fill_rect */
int16_t lc3_pixpat_fill_rect(const MacRect *r, PixPatHandle pp, uint8_t *fb, int16_t rowBytes, int16_t depth)
{
    (void)depth;
    if (!r || !pp || !*pp || !fb) return PIXPAT_PARAM_ERR;

    PixPatRecord *p = *pp;

    for (int16_t y = r->top; y < r->bottom; ++y) {
        uint8_t *row = fb + (y * rowBytes);
        int patY = y & 7;
        for (int16_t x = r->left; x < r->right; ++x) {
            int patX = x & 7;
            row[x] = p->patternData8x8[patY * 8 + patX];
        }
    }

    return PIXPAT_NO_ERR;
}

/* 1480: lc3_pixpat_fill_oval */
int16_t lc3_pixpat_fill_oval(const MacRect *r, PixPatHandle pp, uint8_t *fb, int16_t rowBytes, int16_t depth)
{
    (void)depth;
    if (!r || !pp || !*pp || !fb) return PIXPAT_PARAM_ERR;

    PixPatRecord *p = *pp;

    int32_t a = (r->right - r->left) / 2;
    int32_t b = (r->bottom - r->top) / 2;
    if (a <= 0 || b <= 0) return PIXPAT_NO_ERR;

    int32_t cx = r->left + a;
    int32_t cy = r->top + b;

    int64_t a2 = (int64_t)a * a;
    int64_t b2 = (int64_t)b * b;

    for (int16_t y = r->top; y < r->bottom; ++y) {
        uint8_t *row = fb + (y * rowBytes);
        int32_t dy = y - cy;
        int64_t dy2_b2 = (int64_t)dy * dy * a2;
        int patY = y & 7;

        for (int16_t x = r->left; x < r->right; ++x) {
            int32_t dx = x - cx;
            if (((int64_t)dx * dx * b2 + dy2_b2) <= (a2 * b2)) {
                int patX = x & 7;
                row[x] = p->patternData8x8[patY * 8 + patX];
            }
        }
    }

    return PIXPAT_NO_ERR;
}

/* 1481: lc3_pixpat_dispose */
int16_t lc3_pixpat_dispose(PixPatHandle pp)
{
    if (!pp || !*pp) return PIXPAT_PARAM_ERR;

    for (int i = 0; i < MAX_STATIC_PIXPATS; ++i) {
        if (&s_staticPixPatPtrs[i] == pp) {
            s_pixPatUsed[i] = false;
            return PIXPAT_NO_ERR;
        }
    }

    return PIXPAT_PARAM_ERR;
}
