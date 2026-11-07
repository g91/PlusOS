/* window.c - a small window manager and desktop event loop for PlusOS,
 * drawn with the gfx_draw primitives. Windows have a title bar, a close box,
 * and are draggable. The ROM supplies the arrow cursor. */
#include "kernel.h"
#include "proc.h"
#include "gfx.h"
#include <plusrom/QuickDraw.h>

static Window wins[MAX_WINDOWS];
static Window *zorder[MAX_WINDOWS];     /* [0]=back ... [n-1]=front */
static int    nwin;
static int    desktop_running;

/* ---- mouse (low memory) ---- */
static int mouse_x(void) { return (int16_t)LM_WORD(0x832); }
static int mouse_y(void) { return (int16_t)LM_WORD(0x830); }
static int mouse_down(void) { return (LM_BYTE(0x172) & 0x80) == 0; }   /* MBState bit7=0 down */

void wm_content_rect(Window *w, int *l, int *t, int *r, int *b)
{
    *l = w->x + 1;
    *t = w->y + TITLEBAR_H;
    *r = w->x + w->w - 1;
    *b = w->y + w->h - 1;
}

void wm_init(void) { memset(wins, 0, sizeof wins); nwin = 0; }

Window *wm_new(const char *title, int x, int y, int w, int h)
{
    if (nwin >= MAX_WINDOWS) return 0;
    Window *win = 0;
    for (int i = 0; i < MAX_WINDOWS; i++) if (!wins[i].used) { win = &wins[i]; break; }
    if (!win) return 0;
    memset(win, 0, sizeof *win);
    win->used = 1;
    win->has_close = 1;
    win->x = x; win->y = y; win->w = w; win->h = h;
    strlcpy(win->title, title, sizeof win->title);
    zorder[nwin++] = win;               /* new window on top */
    return win;
}

void wm_select(Window *win)
{
    int i;
    for (i = 0; i < nwin; i++) if (zorder[i] == win) break;
    if (i >= nwin) return;
    for (; i < nwin - 1; i++) zorder[i] = zorder[i + 1];
    zorder[nwin - 1] = win;             /* move to front */
}

Window *wm_front(void) { return nwin ? zorder[nwin - 1] : 0; }

void wm_close(Window *win)
{
    int i;
    for (i = 0; i < nwin; i++) if (zorder[i] == win) break;
    if (i >= nwin) return;
    for (; i < nwin - 1; i++) zorder[i] = zorder[i + 1];
    nwin--;
    win->used = 0;
}

static void draw_window(Window *w, int active)
{
    /* drop shadow */
    gd_fill(w->x + 2, w->y + w->h, w->x + w->w + 2, w->y + w->h + 2, 1);
    gd_fill(w->x + w->w, w->y + 2, w->x + w->w + 2, w->y + w->h, 1);
    /* frame + white body */
    gd_fill(w->x, w->y, w->x + w->w, w->y + w->h, 0);
    gd_frame(w->x, w->y, w->x + w->w, w->y + w->h, 1);
    /* title bar */
    int tb = w->y + TITLEBAR_H - 1;
    gd_hline(w->x, w->x + w->w, tb, 1);
    if (active)                                   /* six racing stripes */
        for (int yy = w->y + 2; yy < tb - 1; yy += 2)
            gd_hline(w->x + 1, w->x + w->w - 1, yy, 1);
    /* close box */
    if (w->has_close) {
        gd_fill(w->x + 6, w->y + 4, w->x + 16, w->y + 14, 0);
        gd_frame(w->x + 6, w->y + 4, w->x + 16, w->y + 14, 1);
    }
    /* title, centred, on a white gap over the stripes */
    int tw = gd_text_w(w->title);
    int tx = w->x + (w->w - tw) / 2;
    if (tx < w->x + 20) tx = w->x + 20;
    gd_fill(tx - 4, w->y + 2, tx + tw + 4, tb - 1, 0);
    gd_text(tx, w->y + 4, w->title, 0);
    if (w->draw_content) w->draw_content(w);
}

void wm_draw_all(void)
{
    HideCursor();
    gd_gray(0, MENUBAR_H, SCREEN_W, SCREEN_H);            /* desktop */
    /* menu bar */
    gd_fill(0, 0, SCREEN_W, MENUBAR_H, 0);
    gd_hline(0, SCREEN_W, MENUBAR_H - 1, 1);
    gd_text(8, 6, "\x14", 0);                             /* placeholder Apple glyph */
    gd_text(22, 6, "PlusOS", 0);
    gd_text(90, 6, "File", 0);
    gd_text(140, 6, "Windows", 0);
    char clk[16];
    uint32_t t = tick_count() / 60;
    snprintf(clk, sizeof clk, "%lu:%02lu", (unsigned long)(t / 60), (unsigned long)(t % 60));
    gd_text(SCREEN_W - gd_text_w(clk) - 8, 6, clk, 0);
    for (int i = 0; i < nwin; i++) draw_window(zorder[i], i == nwin - 1);
    ShowCursor();
}

/* Drag the front window by its title bar using an XOR outline. */
static void drag_window(Window *w, int grabx, int graby)
{
    HideCursor();
    int ox = w->x, oy = w->y;
    int nx = ox, ny = oy;
    gd_xor_frame(nx, ny, nx + w->w, ny + w->h);
    while (mouse_down()) {
        int mx = mouse_x() - grabx, my = mouse_y() - graby;
        if (my < MENUBAR_H) my = MENUBAR_H;
        if (mx != nx || my != ny) {
            gd_xor_frame(nx, ny, nx + w->w, ny + w->h);   /* erase */
            nx = mx; ny = my;
            gd_xor_frame(nx, ny, nx + w->w, ny + w->h);   /* redraw */
        }
    }
    gd_xor_frame(nx, ny, nx + w->w, ny + w->h);           /* erase final outline */
    w->x = nx; w->y = ny;
    ShowCursor();
    wm_draw_all();
}

static int in_rect(int px, int py, int l, int t, int r, int b)
{ return px >= l && px < r && py >= t && py < b; }

static void handle_click(int mx, int my)
{
    /* front-to-back hit test */
    for (int i = nwin - 1; i >= 0; i--) {
        Window *w = zorder[i];
        if (!in_rect(mx, my, w->x, w->y, w->x + w->w, w->y + w->h + 2)) continue;
        if (w != wm_front()) { wm_select(w); wm_draw_all(); return; }
        if (w->has_close && in_rect(mx, my, w->x + 6, w->y + 4, w->x + 16, w->y + 14)) {
            wm_close(w); wm_draw_all(); return;
        }
        if (my < w->y + TITLEBAR_H) { drag_window(w, mx - w->x, my - w->y); return; }
        if (w->on_click) {
            int cl, ct, cr, cb; wm_content_rect(w, &cl, &ct, &cr, &cb);
            w->on_click(w, mx - cl, my - ct);
        }
        return;
    }
}

static void draw_welcome(Window *w)
{
    int l, t, r, b; wm_content_rect(w, &l, &t, &r, &b);
    gd_text(l + 8, t + 8,  "Welcome to PlusOS " PLUSOS_VERSION, 0);
    gd_text(l + 8, t + 26, "A windowed desktop drawn", 0);
    gd_text(l + 8, t + 36, "directly with QuickDraw.", 0);
    gd_text(l + 8, t + 56, "Drag a window by its title.", 0);
    gd_text(l + 8, t + 66, "Close box is top-left.", 0);
    gd_text(l + 8, t + 86, "Press Esc for the shell.", 0);
}

static void draw_sysinfo(Window *w)
{
    int l, t, r, b; wm_content_rect(w, &l, &t, &r, &b);
    char line[48];
    snprintf(line, sizeof line, "Machine: %s", plusrom_machine_name());
    gd_text(l + 8, t + 8, line, 0);
    snprintf(line, sizeof line, "CPU: %s", cpu_name());
    gd_text(l + 8, t + 22, line, 0);
    snprintf(line, sizeof line, "RAM: %luK", (unsigned long)(ram_bytes() >> 10));
    gd_text(l + 8, t + 36, line, 0);
    snprintf(line, sizeof line, "Uptime: %lus", (unsigned long)(tick_count() / 60));
    gd_text(l + 8, t + 50, line, 0);
    snprintf(line, sizeof line, "Processes: %d", proc_count());
    gd_text(l + 8, t + 64, line, 0);
}

void desktop_launch(void)
{
    wm_init();
    Window *a = wm_new("Welcome", 36, 40, 288, 150);
    if (a) a->draw_content = draw_welcome;
    Window *b = wm_new("System", 150, 150, 224, 110);
    if (b) b->draw_content = draw_sysinfo;
    desktop_main();
}

void desktop_main(void)
{
    InitCursor();
    desktop_running = 1;
    wm_draw_all();
    int was_down = 0;
    for (;;) {
        int down = mouse_down();
        if (down && !was_down) handle_click(mouse_x(), mouse_y());
        was_down = down;

        int k = kbd_poll();
        if (k) {
            if (k == KEY_ESC) break;                      /* Esc leaves the desktop */
            Window *f = wm_front();
            if (f && f->on_key) f->on_key(f, k);
        }
        if (!nwin) break;                                 /* all windows closed */
        proc_sleep_ticks(1);
        /* keep the menu-bar clock ticking */
        static uint32_t last;
        if (tick_count() - last >= 60) { last = tick_count(); wm_draw_all(); }
    }
    desktop_running = 0;
    HideCursor();
}
