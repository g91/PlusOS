/* gfx.c - PlusGFX, the PlusOS rendering framework (see plusos/gfx.h). */
#include <plusos/plusos.h>
#include <plusos/syscall.h>
#include <plusos/gfx.h>
#include <plusos/fastmath.h>

extern void gfx__fill_bytes(uint8_t *p, uint32_t n, uint32_t pat32);
extern void gfx__copy_bytes(uint8_t *dst, const uint8_t *src, uint32_t n);
extern void gfx__xor_bytes(uint8_t *p, uint32_t n);
extern void gfx__mask_bytes(uint8_t *dst, const uint8_t *src, const uint8_t *mask, uint32_t n);

static GSurface screen_s, back_s;
static GSurface *tgt;
static int cl, ct, cr, cb;                  /* clip, in target coordinates */

static void init_once(void)
{
    if (screen_s.bits) return;
    int rb = plusos->screen_rowbytes ? plusos->screen_rowbytes : 64;
    int w  = plusos->screen_width    ? plusos->screen_width    : GFX_SCREEN_W;
    int h  = plusos->screen_height   ? plusos->screen_height   : GFX_SCREEN_H;
    gs_wrap(&screen_s, plusos->screen, rb, w, h);
    gs_wrap(&back_s, plusos->screen_back ? plusos->screen_back : plusos->screen,
            rb, w, h);
    if (!tgt) { tgt = &back_s; gfx_noclip(); }
}

/* ======================================================================== */
/* surfaces                                                                 */
/* ======================================================================== */
GSurface *gfx_screen(void)     { init_once(); return &screen_s; }
GSurface *gfx_backbuffer(void) { init_once(); return &back_s; }

void gs_wrap(GSurface *s, void *bits, int rowbytes, int w, int h)
{
    s->bits = bits; s->rowbytes = (int16_t)rowbytes;
    s->w = (int16_t)w; s->h = (int16_t)h; s->owned = 0;
}

int gs_init(GSurface *s, int w, int h)
{
    int rb = ((w + 15) >> 4) << 1;          /* whole words */
    uint8_t *b = malloc((size_t)rb * (size_t)(h > 0 ? h : 1));
    if (!b) { s->bits = 0; return -1; }
    memset(b, 0, (size_t)rb * (size_t)h);
    gs_wrap(s, b, rb, w, h);
    s->owned = 1;
    return 0;
}

void gs_free(GSurface *s)
{
    if (s->owned && s->bits) free(s->bits);
    s->bits = 0; s->owned = 0;
}

/* ======================================================================== */
/* target / clip                                                            */
/* ======================================================================== */
void gfx_target(GSurface *s) { init_once(); tgt = s ? s : &back_s; gfx_noclip(); }
GSurface *gfx_get_target(void) { init_once(); return tgt; }

void gfx_noclip(void) { init_once(); cl = 0; ct = 0; cr = tgt->w; cb = tgt->h; }

void gfx_clip(int l, int t, int r, int b)
{
    init_once();
    cl = l < 0 ? 0 : l;  ct = t < 0 ? 0 : t;
    cr = r > tgt->w ? tgt->w : r;  cb = b > tgt->h ? tgt->h : b;
}

void gfx_get_clip(GRect *r) { r->l = (int16_t)cl; r->t = (int16_t)ct; r->r = (int16_t)cr; r->b = (int16_t)cb; }
void gfx_set_clip(const GRect *r) { gfx_clip(r->l, r->t, r->r, r->b); }

/* ======================================================================== */
/* spans                                                                    */
/* ======================================================================== */
static const uint8_t lmask[8] = { 0xFF, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01 };
static const uint8_t rmask[8] = { 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF };

/* op 0: set to pattern, 1: invert, 2: AND with pattern */
static void span(int l, int t, int r, int b, uint8_t p0, uint8_t p1, int op)
{
    init_once();
    if (l < cl) l = cl;
    if (t < ct) t = ct;
    if (r > cr) r = cr;
    if (b > cb) b = cb;
    if (l >= r || t >= b) return;
    int lb = l >> 3, rb = (r - 1) >> 3, rbytes = tgt->rowbytes;
    uint8_t lm = lmask[l & 7], rm = rmask[(r - 1) & 7];
    uint8_t *row = tgt->bits + ROWOFF(t, rbytes);
    /* Whole rows of a contiguous surface with one pattern (e.g. clearing the
     * screen): one asm fill over the block instead of a call per row. */
    if (op == 0 && p0 == p1 && l == 0 && r == tgt->w && (rbytes << 3) == tgt->w) {
        gfx__fill_bytes(row, (uint32_t)ROWOFF(b - t, rbytes),
                        ((uint32_t)p0 << 24) | ((uint32_t)p0 << 16) | ((uint32_t)p0 << 8) | p0);
        return;
    }
    int mid = rb - lb - 1;
    for (int y = t; y < b; y++, row += rbytes) {
        uint8_t pat = (y & 1) ? p1 : p0;
        if (lb == rb) {
            uint8_t m = lm & rm;
            if (op == 1) row[lb] ^= m;
            else if (op == 2) row[lb] &= (uint8_t)(pat | ~m);
            else row[lb] = (uint8_t)((row[lb] & ~m) | (pat & m));
            continue;
        }
        if (op == 1) {
            row[lb] ^= lm;
            if (mid > 0) gfx__xor_bytes(row + lb + 1, (uint32_t)mid);
            row[rb] ^= rm;
        } else if (op == 2) {
            row[lb] &= (uint8_t)(pat | ~lm);
            for (int i = lb + 1; i < rb; i++) row[i] &= pat;
            row[rb] &= (uint8_t)(pat | ~rm);
        } else {
            row[lb] = (uint8_t)((row[lb] & ~lm) | (pat & lm));
            if (mid > 8) {
                uint32_t p16 = (uint32_t)pat | ((uint32_t)pat << 8);   /* no 32-bit MUL */
                gfx__fill_bytes(row + lb + 1, (uint32_t)mid, p16 | (p16 << 16));
            } else {
                for (int i = lb + 1; i < rb; i++) row[i] = pat;
            }
            row[rb] = (uint8_t)((row[rb] & ~rm) | (pat & rm));
        }
    }
}

void gfx_fill(int l, int t, int r, int b, int black) { uint8_t v = black ? 0xFF : 0; span(l, t, r, b, v, v, 0); }
void gfx_pattern(int l, int t, int r, int b, uint8_t e, uint8_t o) { span(l, t, r, b, e, o, 0); }
void gfx_gray(int l, int t, int r, int b)   { span(l, t, r, b, 0xAA, 0x55, 0); }
void gfx_invert(int l, int t, int r, int b) { span(l, t, r, b, 0, 0, 1); }
void gfx_dim(int l, int t, int r, int b)    { span(l, t, r, b, 0x55, 0xAA, 2); }
void gfx_hline(int l, int r, int y, int black) { gfx_fill(l, y, r, y + 1, black); }

void gfx_vline(int x, int t, int b, int black)
{
    init_once();
    if (x < cl || x >= cr) return;
    if (t < ct) t = ct;
    if (b > cb) b = cb;
    uint8_t m = (uint8_t)(0x80 >> (x & 7));
    int rbytes = tgt->rowbytes;
    uint8_t *p = tgt->bits + ROWOFF(t, rbytes) + (x >> 3);
    if (black) for (int y = t; y < b; y++, p += rbytes) *p |= m;
    else       for (int y = t; y < b; y++, p += rbytes) *p &= (uint8_t)~m;
}

void gfx_frame(int l, int t, int r, int b)
{
    gfx_hline(l, r, t, 1); gfx_hline(l, r, b - 1, 1);
    gfx_vline(l, t, b, 1); gfx_vline(r - 1, t, b, 1);
}

void gfx_xor_frame(int l, int t, int r, int b)
{
    gfx_invert(l, t, r, t + 1);
    gfx_invert(l, b - 1, r, b);
    gfx_invert(l, t + 1, l + 1, b - 1);
    gfx_invert(r - 1, t + 1, r, b - 1);
}

void gfx_pixel(int x, int y, int black)
{
    init_once();
    if (x < cl || x >= cr || y < ct || y >= cb) return;
    uint8_t *p = tgt->bits + ROWOFF(y, tgt->rowbytes) + (x >> 3);
    uint8_t m = (uint8_t)(0x80 >> (x & 7));
    if (black) *p |= m; else *p &= (uint8_t)~m;
}

int gfx_get_pixel(int x, int y)
{
    init_once();
    if (x < 0 || x >= tgt->w || y < 0 || y >= tgt->h) return 0;
    return (tgt->bits[ROWOFF(y, tgt->rowbytes) + (x >> 3)] >> (7 - (x & 7))) & 1;
}

void gfx_line(int x0, int y0, int x1, int y1, int black)
{
    if (y0 == y1) { if (x0 > x1) { int t = x0; x0 = x1; x1 = t; } gfx_hline(x0, x1 + 1, y0, black); return; }
    if (x0 == x1) { if (y0 > y1) { int t = y0; y0 = y1; y1 = t; } gfx_vline(x0, y0, y1 + 1, black); return; }
    init_once();
    /* trivial reject: both endpoints off the same side of the clip */
    if ((x0 < cl && x1 < cl) || (x0 >= cr && x1 >= cr) ||
        (y0 < ct && y1 < ct) || (y0 >= cb && y1 >= cb)) return;
    /* Bresenham with pointer + bit-mask stepping: no call or multiply per pixel */
    int dx = x1 - x0, dy = y1 - y0, sx = 1, sy = 1, rstep = tgt->rowbytes;
    if (dx < 0) { dx = -dx; sx = -1; }
    if (dy < 0) { dy = -dy; sy = -1; rstep = -rstep; }
    int inside = x0 >= cl && x0 < cr && x1 >= cl && x1 < cr &&
                 y0 >= ct && y0 < cb && y1 >= ct && y1 < cb;
    uint8_t *p = tgt->bits + ROWOFF(y0, tgt->rowbytes) + (x0 >> 3);
    uint8_t m = (uint8_t)(0x80 >> (x0 & 7));
    int x = x0, y = y0, n, err;
    if (dx >= dy) { n = dx; err = dx >> 1; } else { n = dy; err = dy >> 1; }
    for (int i = 0; i <= n; i++) {
        if (inside || (x >= cl && x < cr && y >= ct && y < cb)) {
            if (black) *p |= m; else *p &= (uint8_t)~m;
        }
        int stepx, stepy;
        if (dx >= dy) { stepx = 1; err -= dy; stepy = err < 0; if (stepy) err += dx; }
        else          { stepy = 1; err -= dx; stepx = err < 0; if (stepx) err += dy; }
        if (stepx) {
            if (sx > 0) { m >>= 1; if (!m) { m = 0x80; p++; } }
            else        { m = (uint8_t)(m << 1); if (!m) { m = 0x01; p--; } }
            x += sx;
        }
        if (stepy) { p += rstep; y += sy; }
    }
}

/* ======================================================================== */
/* patterns, dithering, circles, rounded rects, polygons & hexagons         */
/* ======================================================================== */
static const uint8_t bayer4x4[4][4] = {
    {  0,  8,  2, 10 },
    { 12,  4, 14,  6 },
    {  3, 11,  1,  9 },
    { 15,  7, 13,  5 }
};

static uint8_t dither_row_byte(int y, int shade)
{
    if (shade <= 0) return 0x00;
    if (shade >= 16) return 0xFF;
    const uint8_t *br = bayer4x4[y & 3];
    uint8_t nib = 0;
    if (shade > br[0]) nib |= 0x8;
    if (shade > br[1]) nib |= 0x4;
    if (shade > br[2]) nib |= 0x2;
    if (shade > br[3]) nib |= 0x1;
    return (uint8_t)((nib << 4) | nib);
}

void gfx_pattern8(int l, int t, int r, int b, const uint8_t pat[8])
{
    init_once();
    if (!pat) return;
    if (l < cl) l = cl;
    if (t < ct) t = ct;
    if (r > cr) r = cr;
    if (b > cb) b = cb;
    if (l >= r || t >= b) return;
    for (int y = t; y < b; y++) {
        uint8_t p = pat[y & 7];
        span(l, y, r, y + 1, p, p, 0);
    }
}

void gfx_dither_rect(int l, int t, int r, int b, int shade)
{
    if (shade <= 0)  { gfx_fill(l, t, r, b, 0); return; }
    if (shade >= 16) { gfx_fill(l, t, r, b, 1); return; }
    uint8_t pat[8];
    for (int i = 0; i < 8; i++) pat[i] = dither_row_byte(i, shade);
    gfx_pattern8(l, t, r, b, pat);
}

void gfx_circle(int cx, int cy, int r, int black)
{
    if (r <= 0) { if (r == 0) gfx_pixel(cx, cy, black); return; }
    int x = r, y = 0, err = 1 - r;
    while (x >= y) {
        gfx_pixel(cx + x, cy + y, black); gfx_pixel(cx - x, cy + y, black);
        gfx_pixel(cx + x, cy - y, black); gfx_pixel(cx - x, cy - y, black);
        gfx_pixel(cx + y, cy + x, black); gfx_pixel(cx - y, cy + x, black);
        gfx_pixel(cx + y, cy - x, black); gfx_pixel(cx - y, cy - x, black);
        y++;
        if (err < 0) err += 2 * y + 1;
        else { x--; err += 2 * (y - x) + 1; }
    }
}

void gfx_fill_circle(int cx, int cy, int r, int black)
{
    if (r <= 0) { if (r == 0) gfx_pixel(cx, cy, black); return; }
    int x = r, y = 0, err = 1 - r;
    while (x >= y) {
        gfx_hline(cx - x, cx + x + 1, cy + y, black);
        gfx_hline(cx - x, cx + x + 1, cy - y, black);
        gfx_hline(cx - y, cx + y + 1, cy + x, black);
        gfx_hline(cx - y, cx + y + 1, cy - x, black);
        y++;
        if (err < 0) err += 2 * y + 1;
        else { x--; err += 2 * (y - x) + 1; }
    }
}

void gfx_round_rect(int l, int t, int r, int b, int radius, int black)
{
    if (l >= r || t >= b) return;
    int w = r - l, h = b - t;
    if (radius * 2 > w) radius = w / 2;
    if (radius * 2 > h) radius = h / 2;
    if (radius <= 0) {
        gfx_hline(l, r, t, black); gfx_hline(l, r, b - 1, black);
        gfx_vline(l, t, b, black); gfx_vline(r - 1, t, b, black);
        return;
    }
    gfx_hline(l + radius, r - radius, t, black);
    gfx_hline(l + radius, r - radius, b - 1, black);
    gfx_vline(l, t + radius, b - radius, black);
    gfx_vline(r - 1, t + radius, b - radius, black);
    int cx0 = l + radius, cx1 = r - 1 - radius;
    int cy0 = t + radius, cy1 = b - 1 - radius;
    int x = radius, y = 0, err = 1 - radius;
    while (x >= y) {
        gfx_pixel(cx0 - x, cy0 - y, black); gfx_pixel(cx0 - y, cy0 - x, black);
        gfx_pixel(cx1 + x, cy0 - y, black); gfx_pixel(cx1 + y, cy0 - x, black);
        gfx_pixel(cx0 - x, cy1 + y, black); gfx_pixel(cx0 - y, cy1 + x, black);
        gfx_pixel(cx1 + x, cy1 + y, black); gfx_pixel(cx1 + y, cy1 + x, black);
        y++;
        if (err < 0) err += 2 * y + 1;
        else { x--; err += 2 * (y - x) + 1; }
    }
}

void gfx_fill_round_rect(int l, int t, int r, int b, int radius, int black)
{
    if (l >= r || t >= b) return;
    int w = r - l, h = b - t;
    if (radius * 2 > w) radius = w / 2;
    if (radius * 2 > h) radius = h / 2;
    if (radius <= 0) { gfx_fill(l, t, r, b, black); return; }
    gfx_fill(l, t + radius, r, b - radius, black);
    int cx0 = l + radius, cx1 = r - 1 - radius;
    int cy0 = t + radius, cy1 = b - 1 - radius;
    int x = radius, y = 0, err = 1 - radius;
    while (x >= y) {
        gfx_hline(cx0 - x, cx1 + x + 1, cy0 - y, black);
        gfx_hline(cx0 - y, cx1 + y + 1, cy0 - x, black);
        gfx_hline(cx0 - x, cx1 + x + 1, cy1 + y, black);
        gfx_hline(cx0 - y, cx1 + y + 1, cy1 + x, black);
        y++;
        if (err < 0) err += 2 * y + 1;
        else { x--; err += 2 * (y - x) + 1; }
    }
}

void gfx_poly_frame(const int *pts, int npts, int black)
{
    if (!pts || npts < 2) return;
    for (int i = 0; i < npts; i++) {
        int j = (i + 1) % npts;
        gfx_line(pts[i * 2], pts[i * 2 + 1], pts[j * 2], pts[j * 2 + 1], black);
    }
}

/* Scanline polygon rasterizer (even-odd rule, QuickDraw polygon algorithm) */
void gfx_poly_dither(const int *pts, int npts, int shade)
{
    init_once();
    if (!pts || npts < 3) return;
    int min_y = pts[1], max_y = pts[1];
    for (int i = 1; i < npts; i++) {
        int y = pts[i * 2 + 1];
        if (y < min_y) min_y = y;
        if (y > max_y) max_y = y;
    }
    if (min_y < ct) min_y = ct;
    if (max_y >= cb) max_y = cb - 1;
    int xints[32];
    for (int y = min_y; y <= max_y; y++) {
        int nx = 0;
        for (int i = 0, j = npts - 1; i < npts; j = i++) {
            int x0 = pts[j * 2], y0 = pts[j * 2 + 1];
            int x1 = pts[i * 2], y1 = pts[i * 2 + 1];
            if ((y0 <= y && y1 > y) || (y1 <= y && y0 > y)) {
                if (nx < 32) {
                    /* |y-y0| <= |dy| so the quotient fits 16 bits: MULS.W + DIVS.W */
                    int dy = y1 - y0;
                    xints[nx++] = x0 + divs16(muls16((int16_t)(y - y0), (int16_t)(x1 - x0)), (int16_t)dy);
                }
            }
        }
        for (int i = 0; i < nx - 1; i++) {
            for (int k = i + 1; k < nx; k++) {
                if (xints[i] > xints[k]) {
                    int tmp = xints[i]; xints[i] = xints[k]; xints[k] = tmp;
                }
            }
        }
        uint8_t pat = dither_row_byte(y, shade);
        for (int i = 0; i + 1 < nx; i += 2) {
            int xl = xints[i], xr = xints[i + 1] + 1;
            if (xl < xr) span(xl, y, xr, y + 1, pat, pat, 0);
        }
    }
}

void gfx_poly_fill(const int *pts, int npts, int black)
{
    gfx_poly_dither(pts, npts, black ? 16 : 0);
}

/* Quarter-wave sine table (0..90 deg in 1024 fixed-point units) */
static const int16_t sin_q90[91] = {
       0,   18,   36,   54,   71,   89,  107,  125,  143,  160,
     178,  195,  213,  230,  248,  265,  282,  299,  316,  333,
     350,  367,  384,  400,  416,  433,  449,  465,  481,  496,
     512,  527,  543,  558,  573,  587,  602,  616,  630,  644,
     658,  672,  685,  698,  711,  724,  737,  749,  761,  773,
     784,  796,  807,  818,  828,  839,  849,  859,  868,  878,
     887,  896,  904,  912,  920,  928,  935,  943,  949,  956,
     962,  968,  974,  979,  984,  989,  994,  998, 1002, 1005,
    1008, 1011, 1014, 1016, 1018, 1020, 1022, 1023, 1023, 1024, 1024
};

static int isin_deg(int deg)
{
    deg %= 360;
    if (deg < 0) deg += 360;
    if (deg <= 90)  return  sin_q90[deg];
    if (deg <= 180) return  sin_q90[180 - deg];
    if (deg <= 270) return -sin_q90[deg - 180];
    return -sin_q90[360 - deg];
}

static int icos_deg(int deg) { return isin_deg(deg + 90); }

void gfx_ngon_pts(int cx, int cy, int radius, int sides, int angle_deg, int *pts_out)
{
    if (!pts_out || sides < 3) return;
    if (sides > 24) sides = 24;
    for (int i = 0; i < sides; i++) {
        int a = angle_deg + (i * 360) / sides;
        pts_out[i * 2]     = cx + (radius * icos_deg(a)) / 1024;
        pts_out[i * 2 + 1] = cy + (radius * isin_deg(a)) / 1024;
    }
}

void gfx_ngon_frame(int cx, int cy, int radius, int sides, int angle_deg, int black)
{
    if (sides < 3) return;
    if (sides > 24) sides = 24;
    int pts[48];
    gfx_ngon_pts(cx, cy, radius, sides, angle_deg, pts);
    gfx_poly_frame(pts, sides, black);
}

void gfx_ngon_fill(int cx, int cy, int radius, int sides, int angle_deg, int shade)
{
    if (sides < 3) return;
    if (sides > 24) sides = 24;
    int pts[48];
    gfx_ngon_pts(cx, cy, radius, sides, angle_deg, pts);
    gfx_poly_dither(pts, sides, shade);
}

void gfx_hexagon(int cx, int cy, int radius, int angle_deg, int shade, int border)
{
    int pts[12];
    gfx_ngon_pts(cx, cy, radius, 6, angle_deg, pts);
    if (shade >= 0) gfx_poly_dither(pts, 6, shade);
    if (border >= 0) gfx_poly_frame(pts, 6, border);
}

/* ======================================================================== */
/* text                                                                     */
/* ======================================================================== */
void gfx_char(int x, int y, int ch, int inverse)
{
    init_once();
    if (x >= cr || y >= cb || x + 8 <= cl || y + 8 <= ct) return;
    const uint8_t *g = plusos->font8x8 + ((ch >= 0x20 && ch < 0x7F) ? ch - 0x20 : 0) * 8;
    uint8_t iv = inverse ? 0xFF : 0;
    int rbytes = tgt->rowbytes;
    if (x >= cl && x + 8 <= cr && y >= ct && y + 8 <= cb) {
        int sh = x & 7;
        uint8_t *p = tgt->bits + ROWOFF(y, rbytes) + (x >> 3);
        if (!sh) {
            for (int r = 0; r < 8; r++, p += rbytes) *p = g[r] ^ iv;
        } else {
            uint8_t m0 = (uint8_t)(0xFF >> sh), m1 = (uint8_t)~m0;
            int two = (x >> 3) + 1 < rbytes;
            int sh2 = 8 - sh;
            for (int r = 0; r < 8; r++, p += rbytes) {
                uint8_t bits = g[r] ^ iv;
                p[0] = (uint8_t)((p[0] & ~m0) | (bits >> sh));
                if (two) p[1] = (uint8_t)((p[1] & ~m1) | (uint8_t)(bits << sh2));
            }
        }
        return;
    }
    for (int r = 0; r < 8; r++) {
        uint8_t bits = g[r] ^ iv;
        for (int c = 0; c < 8; c++) gfx_pixel(x + c, y + r, (bits >> (7 - c)) & 1);
    }
}

int gfx_text(int x, int y, const char *s, int inverse)
{
    while (*s) { gfx_char(x, y, (uint8_t)*s++, inverse); x += 8; }
    return x;
}

int gfx_text_width(const char *s) { return (int)strlen(s) * 8; }

/* ======================================================================== */
/* blits                                                                    */
/* ======================================================================== */
/* 8 source bits starting at bit position `bit` of a row (0 outside the row) */
static inline uint8_t src8(const uint8_t *row, int bit, int rbytes)
{
    int i = bit >> 3, sh = bit & 7;               /* >> floors for negatives with gcc */
    uint8_t a = (i >= 0 && i < rbytes) ? row[i] : 0;
    if (!sh) return a;
    uint8_t b = (i + 1 >= 0 && i + 1 < rbytes) ? row[i + 1] : 0;
    return (uint8_t)((a << sh) | (b >> (8 - sh)));
}

static inline void put(uint8_t *d, uint8_t s, uint8_t m, int mode)
{
    switch (mode) {
    case GFX_OR:  *d |= (uint8_t)(s & m); break;
    case GFX_XOR: *d ^= (uint8_t)(s & m); break;
    case GFX_BIC: *d &= (uint8_t)~(s & m); break;
    default:      *d = (uint8_t)((*d & ~m) | (s & m)); break;
    }
}

/* clip a destination rectangle; returns 0 if nothing is left */
static int clip_blit(const GSurface *src, int *sx, int *sy, int *w, int *h, int *dx, int *dy)
{
    if (*sx < 0) { *dx -= *sx; *w += *sx; *sx = 0; }
    if (*sy < 0) { *dy -= *sy; *h += *sy; *sy = 0; }
    if (*sx + *w > src->w) *w = src->w - *sx;
    if (*sy + *h > src->h) *h = src->h - *sy;
    if (*dx < cl) { int d = cl - *dx; *sx += d; *w -= d; *dx = cl; }
    if (*dy < ct) { int d = ct - *dy; *sy += d; *h -= d; *dy = ct; }
    if (*dx + *w > cr) *w = cr - *dx;
    if (*dy + *h > cb) *h = cb - *dy;
    return *w > 0 && *h > 0;
}

void gfx_blit(const GSurface *src, int sx, int sy, int w, int h, int dx, int dy, int mode)
{
    init_once();
    if (!src || !src->bits || !clip_blit(src, &sx, &sy, &w, &h, &dx, &dy)) return;
    int srb = src->rowbytes, drb = tgt->rowbytes;
    const uint8_t *srow = src->bits + ROWOFF(sy, srb);
    uint8_t *drow = tgt->bits + ROWOFF(dy, drb);
    /* whole rows of identical layout: one contiguous copy */
    if (mode == GFX_COPY && sx == 0 && dx == 0 && w == src->w && w == tgt->w && srb == drb && !(w & 7)) {
        gfx__copy_bytes(drow, srow, (uint32_t)ROWOFF(h, srb));
        return;
    }
    /* byte-aligned straight copy: the common case for cached windows */
    if (mode == GFX_COPY && !(dx & 7) && !(sx & 7) && !(w & 7)) {
        int n = w >> 3;
        for (int y = 0; y < h; y++, srow += srb, drow += drb)
            gfx__copy_bytes(drow + (dx >> 3), srow + (sx >> 3), (uint32_t)n);
        return;
    }
    int lb = dx >> 3, rb = (dx + w - 1) >> 3;
    uint8_t lm = lmask[dx & 7], rm = rmask[(dx + w - 1) & 7];
    for (int y = 0; y < h; y++, srow += srb, drow += drb) {
        for (int db = lb; db <= rb; db++) {
            uint8_t m = 0xFF;
            if (db == lb) m &= lm;
            if (db == rb) m &= rm;
            put(drow + db, src8(srow, sx + (db * 8 - dx), srb), m, mode);
        }
    }
}

void gfx_blit_masked(const GSurface *src, const GSurface *mask,
                     int sx, int sy, int w, int h, int dx, int dy)
{
    init_once();
    if (!src || !mask || !src->bits || !mask->bits || !clip_blit(src, &sx, &sy, &w, &h, &dx, &dy)) return;
    int srb = src->rowbytes, mrb = mask->rowbytes, drb = tgt->rowbytes;
    const uint8_t *srow = src->bits + ROWOFF(sy, srb), *mrow = mask->bits + ROWOFF(sy, mrb);
    uint8_t *drow = tgt->bits + ROWOFF(dy, drb);
    if (!(dx & 7) && !(sx & 7) && !(w & 7)) {          /* byte-aligned: assembly loop */
        uint32_t n = (uint32_t)(w >> 3);
        for (int y = 0; y < h; y++, srow += srb, mrow += mrb, drow += drb)
            gfx__mask_bytes(drow + (dx >> 3), srow + (sx >> 3), mrow + (sx >> 3), n);
        return;
    }
    int lb = dx >> 3, rb = (dx + w - 1) >> 3;
    uint8_t lm = lmask[dx & 7], rm = rmask[(dx + w - 1) & 7];
    int aligned = !((sx - dx) & 7);
    for (int y = 0; y < h; y++, srow += srb, mrow += mrb, drow += drb) {
        for (int db = lb; db <= rb; db++) {
            uint8_t m = 0xFF;
            if (db == lb) m &= lm;
            if (db == rb) m &= rm;
            int bit = sx + (db * 8 - dx);
            uint8_t s, k;
            if (aligned && bit >= 0 && (bit >> 3) < srb) { s = srow[bit >> 3]; k = mrow[bit >> 3]; }
            else { s = src8(srow, bit, srb); k = src8(mrow, bit, mrb); }
            k &= m;
            drow[db] = (uint8_t)((drow[db] & ~k) | (s & k));
        }
    }
}

/* ======================================================================== */
/* present                                                                  */
/* ======================================================================== */
void gfx_present(void)
{
    init_once();
    /* Safeguard: if an application targeted gfx_screen() directly and still
     * calls gfx_present(), sync screen_s -> back_s and switch target to back_s
     * so present() never overwrites the screen with a blank backbuffer. */
    if (tgt == &screen_s && back_s.bits && back_s.bits != screen_s.bits) {
        gfx__copy_bytes(back_s.bits, screen_s.bits, 64 * GFX_SCREEN_H);
        tgt = &back_s;
        return;
    }
    plusos->present();
}
void gfx_present_rows(int top, int bottom) { plusos->present_rect(top, bottom); }

/* ======================================================================== */
/* windows (the kernel window server; the desktop composites them)          */
/* ======================================================================== */
static long win_call(struct plusos_win_args *a) { return plusos_syscall(SYS_win, a); }

int gwin_open(GWindow *w, const char *title, int width, int height)
{
    struct plusos_win_args a;
    memset(&a, 0, sizeof a);
    a.op = PLUSOS_WIN_CREATE; a.title = title; a.w = width; a.h = height; a.x = -1; a.y = -1;
    int id = (int)win_call(&a);
    if (id <= 0) { w->id = 0; w->surf.bits = 0; return -1; }
    w->id = id;
    gs_wrap(&w->surf, a.bits, a.rowbytes, a.w, a.h);
    return 0;
}

void gwin_update(GWindow *w)
{
    struct plusos_win_args a; memset(&a, 0, sizeof a);
    a.op = PLUSOS_WIN_UPDATE; a.id = w->id;
    win_call(&a);
}

int gwin_event(GWindow *w, GEvent *ev, int wait)
{
    struct plusos_win_args a; memset(&a, 0, sizeof a);
    a.op = PLUSOS_WIN_EVENT; a.id = w->id; a.ev = (struct plusos_event *)ev; a.wait = wait;
    return (int)win_call(&a);
}

void gwin_title(GWindow *w, const char *title)
{
    struct plusos_win_args a; memset(&a, 0, sizeof a);
    a.op = PLUSOS_WIN_TITLE; a.id = w->id; a.title = title;
    win_call(&a);
}

void gwin_close(GWindow *w)
{
    struct plusos_win_args a; memset(&a, 0, sizeof a);
    a.op = PLUSOS_WIN_DESTROY; a.id = w->id;
    win_call(&a);
    w->id = 0; w->surf.bits = 0;
}
