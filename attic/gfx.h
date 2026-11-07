/* gfx.h - PlusOS desktop drawing primitives and window system. */
#ifndef PLUSOS_GFX_H
#define PLUSOS_GFX_H
#include <stdint.h>

/* ---- direct framebuffer drawing (gfx_draw.c) ---- */
void gd_pixel(int x, int y, int black);
void gd_pixel_xor(int x, int y);
void gd_fill(int l, int t, int r, int b, int black);
void gd_fill_pat(int l, int t, int r, int b, uint8_t pat0, uint8_t pat1);
void gd_gray(int l, int t, int r, int b);
void gd_frame(int l, int t, int r, int b, int thick);
void gd_hline(int l, int r, int y, int black);
void gd_vline(int x, int t, int b, int black);
void gd_xor_frame(int l, int t, int r, int b);
void gd_char(int x, int y, int ch, int invert);
int  gd_text(int x, int y, const char *s, int invert);
int  gd_text_w(const char *s);

#define SCREEN_W 512
#define SCREEN_H 342
#define MENUBAR_H 20

/* ---- window system (window.c) ---- */
#define MAX_WINDOWS 8
#define TITLEBAR_H  18

typedef struct Window {
    int   x, y, w, h;               /* outer frame, including title bar */
    char  title[32];
    int   used;
    int   has_close;
    void (*draw_content)(struct Window *win); /* draws inside the content area */
    void (*on_key)(struct Window *win, int key);
    void (*on_click)(struct Window *win, int cx, int cy);  /* content coords */
    void *data;
} Window;

void    wm_init(void);
Window *wm_new(const char *title, int x, int y, int w, int h);
void    wm_close(Window *win);
void    wm_select(Window *win);
Window *wm_front(void);
void    wm_draw_all(void);
void    wm_content_rect(Window *win, int *l, int *t, int *r, int *b);
/* Run the desktop event loop until it is dismissed. */
void    desktop_main(void);
void    desktop_launch(void);           /* set up demo windows and run the desktop */

#endif
