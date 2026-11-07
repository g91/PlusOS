/* gfx.h - PlusGFX, the PlusOS rendering framework.
 *
 * A small, fast 2-D graphics layer for the 512 x 342 1-bit screen, shared by
 * every program (link with libplusgfx.a; the PlusOS app build does this).
 *
 *   Surfaces  - GSurface is a 1-bit bitmap (1 = black, leftmost pixel in the
 *               high bit). Draw into any surface: the screen, the off-screen
 *               back buffer, or bitmaps you create (gs_init) to cache things
 *               like windows and icons.
 *   Target    - drawing calls go to the current target surface, limited to its
 *               clip rectangle. Rectangles are half-open: [l,r) x [t,b).
 *   Blits     - copy rectangles between surfaces (copy/or/xor/bic, or through
 *               a mask). Byte-aligned blits run as straight block copies.
 *   Present   - compose a frame in the back buffer, then present it: the video
 *               driver copies it to the screen in one pass, so nothing flickers.
 *
 * Fills and copies run in hand-written 68000 assembly; text is drawn a byte at
 * a time.
 */
#ifndef PLUSOS_GFX_H
#define PLUSOS_GFX_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GFX_SCREEN_W 512
#define GFX_SCREEN_H 342

typedef struct GSurface {
    uint8_t *bits;          /* 1 bpp, 1 = black, MSB = leftmost pixel */
    int16_t  rowbytes;      /* bytes per row (even) */
    int16_t  w, h;          /* size in pixels */
    int16_t  owned;         /* bits were allocated by gs_init */
} GSurface;

typedef struct { int16_t l, t, r, b; } GRect;

/* ---- surfaces ---- */
GSurface *gfx_screen(void);                 /* the live screen */
GSurface *gfx_backbuffer(void);             /* off-screen frame for compositing */
int   gs_init(GSurface *s, int w, int h);   /* allocate a white surface; 0 = ok */
void  gs_wrap(GSurface *s, void *bits, int rowbytes, int w, int h);
void  gs_free(GSurface *s);

/* ---- target and clipping ---- */
void      gfx_target(GSurface *s);          /* also resets the clip to the whole surface */
GSurface *gfx_get_target(void);
void      gfx_clip(int l, int t, int r, int b);     /* intersected with the surface */
void      gfx_noclip(void);
void      gfx_get_clip(GRect *r);
void      gfx_set_clip(const GRect *r);

/* ---- primitives ---- */
void gfx_fill(int l, int t, int r, int b, int black);
void gfx_pattern(int l, int t, int r, int b, uint8_t even_rows, uint8_t odd_rows);
void gfx_gray(int l, int t, int r, int b);          /* 50% desktop gray */
void gfx_invert(int l, int t, int r, int b);
void gfx_dim(int l, int t, int r, int b);           /* knock out half the ink: "disabled" */
void gfx_hline(int l, int r, int y, int black);
void gfx_vline(int x, int t, int b, int black);
void gfx_frame(int l, int t, int r, int b);
void gfx_xor_frame(int l, int t, int r, int b);
void gfx_pixel(int x, int y, int black);
int  gfx_get_pixel(int x, int y);
void gfx_line(int x0, int y0, int x1, int y1, int black);
void gfx_char(int x, int y, int ch, int inverse);   /* 8 x 8 system font */
int  gfx_text(int x, int y, const char *s, int inverse);   /* returns the end x */
int  gfx_text_width(const char *s);

/* ---- patterns, dithering, circles, polygons & hexagons ---- */
void gfx_pattern8(int l, int t, int r, int b, const uint8_t pat[8]);
void gfx_dither_rect(int l, int t, int r, int b, int shade);    /* shade 0 (white) .. 16 (black) */
void gfx_circle(int cx, int cy, int r, int black);
void gfx_fill_circle(int cx, int cy, int r, int black);
void gfx_round_rect(int l, int t, int r, int b, int radius, int black);
void gfx_fill_round_rect(int l, int t, int r, int b, int radius, int black);
void gfx_poly_frame(const int *pts, int npts, int black);       /* pts = [x0,y0, x1,y1, ...] */
void gfx_poly_fill(const int *pts, int npts, int black);        /* scanline polygon fill */
void gfx_poly_dither(const int *pts, int npts, int shade);      /* shade 0..16 Bayer dither */
void gfx_ngon_pts(int cx, int cy, int radius, int sides, int angle_deg, int *pts_out);
void gfx_ngon_frame(int cx, int cy, int radius, int sides, int angle_deg, int black);
void gfx_ngon_fill(int cx, int cy, int radius, int sides, int angle_deg, int shade);
void gfx_hexagon(int cx, int cy, int radius, int angle_deg, int shade, int border);

/* ---- block transfer ---- */
enum { GFX_COPY = 0, GFX_OR = 1, GFX_XOR = 2, GFX_BIC = 3 };
void gfx_blit(const GSurface *src, int sx, int sy, int w, int h, int dx, int dy, int mode);
/* dst = (dst & ~mask) | (src & mask): draws a shaped image (icons, cursors) */
void gfx_blit_masked(const GSurface *src, const GSurface *mask,
                     int sx, int sy, int w, int h, int dx, int dy);

/* ---- showing frames ---- */
void gfx_present(void);                     /* whole back buffer -> screen */
void gfx_present_rows(int top, int bottom); /* rows [top, bottom) only */

/* ---- windows on the desktop ------------------------------------------------
 * A program running under the desktop opens a window, draws into w.surf with
 * the calls above (gfx_target(&w.surf)), then gwin_update() to show it. The
 * desktop moves and stacks windows; your program keeps running meanwhile.
 *
 *   GWindow w;
 *   gwin_open(&w, "Hello", 200, 100);
 *   gfx_target(&w.surf); gfx_text(10, 10, "Hi!", 0); gwin_update(&w);
 *   GEvent e; while (gwin_event(&w, &e, 1) > 0 && e.type != GEV_CLOSE) { ... }
 */
typedef struct { int id; GSurface surf; } GWindow;
typedef struct { int type, x, y, key; } GEvent;        /* x, y relative to the window */
enum { GEV_NONE = 0, GEV_MOUSEDOWN = 1, GEV_MOUSEUP = 2, GEV_KEY = 3, GEV_CLOSE = 4 };
int  gwin_open(GWindow *w, const char *title, int width, int height);   /* 0 = ok */
void gwin_update(GWindow *w);                       /* the drawing is done: show it */
int  gwin_event(GWindow *w, GEvent *ev, int wait);  /* 1 = event, 0 = none, -1 = closed */
void gwin_title(GWindow *w, const char *title);
void gwin_close(GWindow *w);

#ifdef __cplusplus
}
#endif
#endif
