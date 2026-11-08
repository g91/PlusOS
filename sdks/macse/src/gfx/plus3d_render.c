/* plus3d_render.c - 3D Pipeline Rasterizer and Mesh Renderer */
#include "plus3d_internal.h"

/* 16 Bayer Ordered Dither Patterns (4x4 tiled into 8-row uint8 bytes)
 * 0 = 100% White (No ink) -> 15 = 100% Solid Black */
const uint8_t BAYER_PATTERNS[16][8] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 0:  0%   black */
    {0x88, 0x00, 0x00, 0x00, 0x88, 0x00, 0x00, 0x00}, /* 1:  6.25% */
    {0x88, 0x00, 0x22, 0x00, 0x88, 0x00, 0x22, 0x00}, /* 2: 12.5% */
    {0xA8, 0x00, 0x22, 0x00, 0xA8, 0x00, 0x22, 0x00}, /* 3: 18.75% */
    {0xAA, 0x00, 0x22, 0x00, 0xAA, 0x00, 0x22, 0x00}, /* 4: 25% */
    {0xAA, 0x00, 0xA2, 0x00, 0xAA, 0x00, 0xA2, 0x00}, /* 5: 31.25% */
    {0xAA, 0x00, 0xAA, 0x00, 0xAA, 0x00, 0xAA, 0x00}, /* 6: 37.5% */
    {0xAA, 0x44, 0xAA, 0x00, 0xAA, 0x44, 0xAA, 0x00}, /* 7: 43.75% */
    {0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55}, /* 8: 50% checkerboard */
    {0xEA, 0x55, 0xAA, 0x55, 0xEA, 0x55, 0xAA, 0x55}, /* 9: 56.25% */
    {0xEE, 0x55, 0xAA, 0x55, 0xEE, 0x55, 0xAA, 0x55}, /* 10: 62.5% */
    {0xEE, 0x55, 0xBA, 0x55, 0xEE, 0x55, 0xBA, 0x55}, /* 11: 68.75% */
    {0xEE, 0x55, 0xBB, 0x55, 0xEE, 0x55, 0xBB, 0x55}, /* 12: 75% */
    {0xFE, 0x55, 0xBB, 0x55, 0xFE, 0x55, 0xBB, 0x55}, /* 13: 81.25% */
    {0xFE, 0xD5, 0xBB, 0x55, 0xFE, 0xD5, 0xBB, 0x55}, /* 14: 87.5% */
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}  /* 15: 100% Solid Black */
};

/* Global lighting configuration (fixed-point) */
static Vec3  g_light_dir = { FIXF(0.577f), FIXF(-0.577f), FIXF(0.577f) };
static fix_t g_ambient = FIXF(0.25f);
static fix_t g_diffuse = FIXF(0.75f);
static Camera3D *g_camera = NULL;


/* ============================================================================
 * 3D Renderer & Rasterizer (integer)
 * ============================================================================ */

void renderer_init(void) {
    ensure_trig();
    g_light_dir = vec3_normalize(vec3(0.5f, 0.8f, -0.6f));
    g_ambient = FIXF(0.2f);
    g_diffuse = FIXF(0.8f);
}

void renderer_set_camera(Camera3D *cam) {
    g_camera = cam;
}

void renderer_set_light(Vec3 dir, float ambient, float diffuse) {
    g_light_dir = vec3_normalize(dir);
    g_ambient = FIXF(ambient);
    g_diffuse = FIXF(diffuse);
}

/* ----------------------------------------------------------------------------
 * Direct-to-framebuffer rasterizer.
 * Writes straight into the current PlusGFX target (normally the back buffer)
 * with pointer stepping: no function call per pixel/scanline, no multiply per
 * row (row pointer is advanced by rowbytes), and no division per scanline
 * (triangle edges step with 16.16 increments computed once per edge).
 * ---------------------------------------------------------------------------- */
extern void gfx__fill_bytes(uint8_t *p, uint32_t n, uint32_t pat32);   /* gfx_asm.S */

typedef struct {
    uint8_t *bits;
    int rb;                      /* rowbytes */
    int cl, ct, cr, cb;          /* clip rect: [cl,cr) x [ct,cb) */
} RTarget;
static RTarget rt;

static void rt_begin(void) {
    GSurface *s = gfx_get_target();
    GRect c;
    gfx_get_clip(&c);
    rt.bits = s->bits;
    rt.rb = s->rowbytes;
    rt.cl = c.l; rt.ct = c.t; rt.cr = c.r; rt.cb = c.b;
}

static inline int16_t sat16(int32_t v) {
    return v > 32767 ? 32767 : (v < -32768 ? -32768 : (int16_t)v);
}

static const uint8_t k_lmask[8] = { 0xFF, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01 };
static const uint8_t k_rmask[8] = { 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF };

/* Fill pixels l..r (inclusive, already clipped) of one row with `pat`. */
static inline void rt_span(uint8_t *row, int l, int r, uint8_t pat) {
    int lb = l >> 3, rbyte = r >> 3;
    uint8_t lm = k_lmask[l & 7], rm = k_rmask[r & 7];
    uint8_t *p = row + lb;
    if (lb == rbyte) {
        uint8_t mk = (uint8_t)(lm & rm);
        *p = (uint8_t)((*p & ~mk) | (pat & mk));
        return;
    }
    *p = (uint8_t)((*p & ~lm) | (pat & lm));
    p++;
    int n = rbyte - lb - 1;
    /* Interior: inline, unrolled 8 longwords/iteration. Inlined (not a call per
     * scanline) so there's no jsr/rts overhead on the ~hundreds of spans/frame,
     * and unrolled so the dbra cost is amortized over 32 bytes. */
    if (n >= 4) {
        if ((uintptr_t)p & 1) { *p++ = pat; n--; }
        uint16_t w16 = (uint16_t)(pat | (pat << 8));
        uint32_t w32 = ((uint32_t)w16 << 16) | w16;
        uint32_t *q = (uint32_t *)p;
        int blk = n >> 5;                            /* 32-byte blocks */
        while (blk-- > 0) {
            q[0] = w32; q[1] = w32; q[2] = w32; q[3] = w32;
            q[4] = w32; q[5] = w32; q[6] = w32; q[7] = w32;
            q += 8;
        }
        int lw = (n & 31) >> 2;                      /* leftover longwords */
        while (lw-- > 0) *q++ = w32;
        p = (uint8_t *)q;
        n &= 3;
    }
    while (n-- > 0) *p++ = pat;
    row[rbyte] = (uint8_t)((row[rbyte] & ~rm) | (pat & rm));
}

/* Flat dithered triangle, integer screen coords. */
static void rt_tri(int x0, int y0, int x1, int y1, int x2, int y2, const uint8_t *pat) {
    int t;
    if (y0 > y1) { t = y0; y0 = y1; y1 = t; t = x0; x0 = x1; x1 = t; }
    if (y1 > y2) { t = y1; y1 = y2; y2 = t; t = x1; x1 = x2; x2 = t; }
    if (y0 > y1) { t = y0; y0 = y1; y1 = t; t = x0; x0 = x1; x1 = t; }
    if (y2 < rt.ct || y0 >= rt.cb || y0 == y2) return;

    /* edge slopes in 16.16: three divides per triangle, none per scanline */
    int32_t dl = ((int32_t)(x2 - x0) << 16) / (y2 - y0);
    int32_t da = (y1 > y0) ? ((int32_t)(x1 - x0) << 16) / (y1 - y0) : 0;
    int32_t db = (y2 > y1) ? ((int32_t)(x2 - x1) << 16) / (y2 - y1) : 0;

    /* edge x positions at the first visible row (rows above the clip are
     * skipped arithmetically, never walked). skip <= edge height, so
     * slope*skip <= dx<<16 and cannot overflow. */
    int y = y0 < rt.ct ? rt.ct : y0;
    int32_t xl = ((int32_t)x0 << 16) + 0x8000 + dl * (y - y0);    /* long edge x0 -> x2 */
    int32_t xs = (y < y1)                                           /* short edges */
               ? ((int32_t)x0 << 16) + 0x8000 + da * (y - y0)
               : ((int32_t)x1 << 16) + 0x8000 + db * (y - y1);

    uint8_t *row = rt.bits + ROWOFF(y, rt.rb);
    int rb = rt.rb, cl = rt.cl, cr1 = rt.cr - 1;
    int yend = y2 < rt.cb ? y2 : rt.cb - 1;

    for (; y <= yend; y++, row += rb) {
        if (y == y1) xs = ((int32_t)x1 << 16) + 0x8000;
        int a = (int)(xl >> 16), b = (int)(xs >> 16);
        if (a > b) { t = a; a = b; b = t; }
        if (a < cl) a = cl;
        if (b > cr1) b = cr1;
        if (a <= b) rt_span(row, a, b, pat[y & 7]);
        xl += dl;
        xs += (y < y1) ? da : db;
    }
}

/* 1-pixel black line, Bresenham with pointer + bit-mask stepping. */
static void rt_line(int x0, int y0, int x1, int y1) {
    /* trivial reject: both endpoints off the same side */
    if ((x0 < rt.cl && x1 < rt.cl) || (x0 >= rt.cr && x1 >= rt.cr) ||
        (y0 < rt.ct && y1 < rt.ct) || (y0 >= rt.cb && y1 >= rt.cb)) return;

    int dx = x1 - x0, dy = y1 - y0;
    int sx = 1, sgy = 1, rstep = rt.rb;
    if (dx < 0) { dx = -dx; sx = -1; }
    if (dy < 0) { dy = -dy; sgy = -1; rstep = -rstep; }
    int inside = x0 >= rt.cl && x0 < rt.cr && x1 >= rt.cl && x1 < rt.cr &&
                 y0 >= rt.ct && y0 < rt.cb && y1 >= rt.ct && y1 < rt.cb;

    uint8_t *p = rt.bits + ROWOFF(y0, rt.rb) + (x0 >> 3);
    uint8_t mk = (uint8_t)(0x80 >> (x0 & 7));
    int x = x0, y = y0;

    if (dx >= dy) {
        int err = dx >> 1;
        for (int i = 0; i <= dx; i++) {
            if (inside || (x >= rt.cl && x < rt.cr && y >= rt.ct && y < rt.cb)) *p |= mk;
            if (sx > 0) { mk >>= 1; if (!mk) { mk = 0x80; p++; } }
            else        { mk = (uint8_t)(mk << 1); if (!mk) { mk = 0x01; p--; } }
            x += sx;
            err -= dy;
            if (err < 0) { err += dx; p += rstep; y += sgy; }
        }
    } else {
        int err = dy >> 1;
        for (int i = 0; i <= dy; i++) {
            if (inside || (x >= rt.cl && x < rt.cr && y >= rt.ct && y < rt.cb)) *p |= mk;
            p += rstep; y += sgy;
            err -= dx;
            if (err < 0) {
                err += dy;
                if (sx > 0) { mk >>= 1; if (!mk) { mk = 0x80; p++; } }
                else        { mk = (uint8_t)(mk << 1); if (!mk) { mk = 0x01; p--; } }
                x += sx;
            }
        }
    }
}

void raster_line_3d(Vec2 p0, Vec2 p1) {
    rt_begin();
    rt_line(FIX_INT(p0.x), FIX_INT(p0.y), FIX_INT(p1.x), FIX_INT(p1.y));
}

void raster_triangle_wire(Vec2 p0, Vec2 p1, Vec2 p2) {
    rt_begin();
    int x0 = FIX_INT(p0.x), y0 = FIX_INT(p0.y);
    int x1 = FIX_INT(p1.x), y1 = FIX_INT(p1.y);
    int x2 = FIX_INT(p2.x), y2 = FIX_INT(p2.y);
    rt_line(x0, y0, x1, y1);
    rt_line(x1, y1, x2, y2);
    rt_line(x2, y2, x0, y0);
}

void raster_triangle_solid(Vec2 p0, Vec2 p1, Vec2 p2, uint8_t pat_idx) {
    rt_begin();
    rt_tri(FIX_INT(p0.x), FIX_INT(p0.y), FIX_INT(p1.x), FIX_INT(p1.y),
           FIX_INT(p2.x), FIX_INT(p2.y), BAYER_PATTERNS[pat_idx & 15]);
}

/* Per-frame scratch (screen coords, depth, visible-face list). */
#define R_MAXV 512
#define R_MAXF 1024
static int16_t s_sx[R_MAXV], s_sy[R_MAXV], s_w[R_MAXV];
static uint8_t s_ok[R_MAXV];
static int16_t s_fidx[R_MAXF];
static int32_t s_fkey[R_MAXF];

/* The hot path. Per vertex: 9 MULS.W + 2 DIVS.W. Per visible face: 12 MULS.W
 * for lighting (the precomputed object-space normal is rotated - no cross
 * product, sqrt or normalize per frame). No 32x32 multiplies, no 64-bit math,
 * no floats. Note: m->transformed is not updated by this renderer. */
void renderer_draw_mesh(Mesh3D *m, Mat4 model_mat, int render_flags) {
    if (!g_camera || !m) return;
    rt_begin();

    Mat4 mvp = mat4_mul(mat4_mul(model_mat, g_camera->view_mat), g_camera->proj_mat);

    /* 4.12 copies of the x, y, w columns (rows 0..2) + 8.24 translation */
    int16_t ax0 = sat16(mvp.m[0][0] >> 4), ax1 = sat16(mvp.m[1][0] >> 4), ax2 = sat16(mvp.m[2][0] >> 4);
    int16_t ay0 = sat16(mvp.m[0][1] >> 4), ay1 = sat16(mvp.m[1][1] >> 4), ay2 = sat16(mvp.m[2][1] >> 4);
    int16_t aw0 = sat16(mvp.m[0][3] >> 4), aw1 = sat16(mvp.m[1][3] >> 4), aw2 = sat16(mvp.m[2][3] >> 4);
    int32_t tx = mvp.m[3][0] << 8, ty = mvp.m[3][1] << 8, tw = mvp.m[3][3] << 8;

    int nv = m->num_vertices < R_MAXV ? m->num_vertices : R_MAXV;
    for (int i = 0; i < nv; i++) {
        const Vec3 *v = &m->vertices[i];
        int16_t vx = sat16(v->x >> 4), vy = sat16(v->y >> 4), vz = sat16(v->z >> 4);   /* 4.12 */
        int32_t X = muls16(vx, ax0) + muls16(vy, ax1) + muls16(vz, ax2) + tx;   /* 8.24 */
        int32_t Y = muls16(vx, ay0) + muls16(vy, ay1) + muls16(vz, ay2) + ty;
        int32_t W = muls16(vx, aw0) + muls16(vy, aw1) + muls16(vz, aw2) + tw;

        int32_t w12 = W >> 12;                                  /* 8.12 */
        if (w12 < 64) { s_ok[i] = 0; continue; }                /* behind the camera */
        int16_t ws = sat16(w12);
        int32_t xs = X >> 12, ys = Y >> 12;
        int32_t lim = (int32_t)ws << 4;                         /* |x/w| <= 16: DIVS stays in range */
        if (xs > lim) xs = lim; else if (xs < -lim) xs = -lim;
        if (ys > lim) ys = lim; else if (ys < -lim) ys = -lim;
        int32_t nx = (xs << 8) - xs;                            /* 255 * x (shift-sub, no MUL) */
        int32_t ny = (ys << 7) + (ys << 5) + (ys << 3) + (ys << 1);   /* 170 * y */
        int16_t sx = (int16_t)(255 + divs16(nx, ws));
        int16_t sy = (int16_t)(170 - divs16(ny, ws));
        s_sx[i] = sx; s_sy[i] = sy; s_w[i] = ws; s_ok[i] = 1;
        m->projected[i].x = (fix_t)sx << 16;
        m->projected[i].y = (fix_t)sy << 16;
    }

    /* world rotation (model upper 3x3) in 4.12, light & material in 2.14 */
    int16_t r00 = sat16(model_mat.m[0][0] >> 4), r01 = sat16(model_mat.m[0][1] >> 4), r02 = sat16(model_mat.m[0][2] >> 4);
    int16_t r10 = sat16(model_mat.m[1][0] >> 4), r11 = sat16(model_mat.m[1][1] >> 4), r12 = sat16(model_mat.m[1][2] >> 4);
    int16_t r20 = sat16(model_mat.m[2][0] >> 4), r21 = sat16(model_mat.m[2][1] >> 4), r22 = sat16(model_mat.m[2][2] >> 4);
    int16_t lx = sat16(g_light_dir.x >> 2), ly = sat16(g_light_dir.y >> 2), lz = sat16(g_light_dir.z >> 2);
    int16_t amb = sat16(g_ambient >> 2), dif = sat16(g_diffuse >> 2);

    int nf = m->num_faces < R_MAXF ? m->num_faces : R_MAXF;
    int cull = !(render_flags & RENDER_BACKFACE_ON);
    int nvis = 0;
    for (int i = 0; i < nf; i++) {
        Face3D *f = &m->faces[i];
        int a = f->v[0], b = f->v[1], c = f->v[2];
        if (a >= nv || b >= nv || c >= nv || !s_ok[a] || !s_ok[b] || !s_ok[c]) continue;

        if (cull) {
            int16_t x0 = s_sx[a], y0 = s_sy[a];
            int32_t cr = muls16((int16_t)(s_sx[b] - x0), (int16_t)(s_sy[c] - y0)) -
                         muls16((int16_t)(s_sy[b] - y0), (int16_t)(s_sx[c] - x0));
            if (cr <= 0) continue;
        }

        /* Lambert: rotate the stored unit normal into world space, dot light */
        int16_t nx = sat16(f->normal.x >> 2), ny = sat16(f->normal.y >> 2), nz = sat16(f->normal.z >> 2);
        int16_t wx = sat16((muls16(nx, r00) + muls16(ny, r10) + muls16(nz, r20)) >> 12);   /* 2.14 */
        int16_t wy = sat16((muls16(nx, r01) + muls16(ny, r11) + muls16(nz, r21)) >> 12);
        int16_t wz = sat16((muls16(nx, r02) + muls16(ny, r12) + muls16(nz, r22)) >> 12);
        int32_t d = (muls16(wx, lx) + muls16(wy, ly) + muls16(wz, lz)) >> 14;              /* 2.14 */
        if (d < 0) d = 0;
        int32_t inten = amb + (muls16(dif, sat16(d)) >> 14);
        if (inten > 16384) inten = 16384;
        if (inten < 0) inten = 0;
        f->pattern = (uint8_t)((muls16((int16_t)inten, 15) + 8192) >> 14);
        f->shade = inten << 2;

        int32_t key = (int32_t)s_w[a] + s_w[b] + s_w[c];
        f->avg_z = key;
        s_fidx[nvis] = (int16_t)i;
        s_fkey[nvis] = key;
        nvis++;
    }

    /* Painter's algorithm: insertion sort by depth, farthest (largest w) first */
    for (int i = 1; i < nvis; i++) {
        int32_t k = s_fkey[i];
        int16_t id = s_fidx[i];
        int j = i - 1;
        while (j >= 0 && s_fkey[j] < k) { s_fkey[j + 1] = s_fkey[j]; s_fidx[j + 1] = s_fidx[j]; j--; }
        s_fkey[j + 1] = k;
        s_fidx[j + 1] = id;
    }

    for (int i = 0; i < nvis; i++) {
        const Face3D *f = &m->faces[s_fidx[i]];
        int a = f->v[0], b = f->v[1], c = f->v[2];
        int x0 = s_sx[a], y0 = s_sy[a], x1 = s_sx[b], y1 = s_sy[b], x2 = s_sx[c], y2 = s_sy[c];
        if (render_flags & RENDER_SOLID_FLAT)
            rt_tri(x0, y0, x1, y1, x2, y2, BAYER_PATTERNS[f->pattern & 15]);
        if (render_flags & RENDER_WIREFRAME) {
            rt_line(x0, y0, x1, y1);
            rt_line(x1, y1, x2, y2);
            rt_line(x2, y2, x0, y0);
        }
    }
}

