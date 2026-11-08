/* desktop.h - Shared declarations and state for PlusOS Desktop modules.
 */
#ifndef DESKTOP_H
#define DESKTOP_H

#include <plusos/plusos.h>
#include <plusos/syscall.h>
#include <plusos/gfx.h>
#include <plusos/thread.h>
#include <plusos/theme.h>
#include <plusrom/QuickDraw.h>
#include <plusrom/OSUtil.h>
#include <plusrom/FileMgr.h>

#define SCREEN_W   512
#define SCREEN_H   342
#define ROWB       64
#define FB_BYTES   (ROWB * SCREEN_H)
#define MENUBAR_H  20
#define TASKBAR_H  22
#define TITLEBAR_H 18
#define ICON_W     64
#define ICON_H     46
#define MAX_WIN    8
#define MAX_ICONS  40

#define DL         96
#define DR         416
#define ROOT_DIR   2L

#define APP_PADX   8
#define APP_TOP    (TITLEBAR_H + 2)
#define APP_PADB   6

/* Icons */
enum { K_PROGRAMS, K_GAMES, K_SHELL, K_HEX, K_TASKS, K_FILES, K_SETTINGS, K_TRASH, K_FILE };

typedef struct {
    int  kind, x, y;
    char id[28], label[32], path[64];
    int  program;
    GSurface img, mask;
    int  rendered;              /* -1: not rendered, else the selection state drawn */
} Icon;

/* Windows */
enum { WK_ABOUT, WK_INFO, WK_SETTINGS, WK_PROGRAMS, WK_GAMES, WK_APP };

typedef struct Window Window;
struct Window {
    int  x, y, w, h, kind, used;
    char title[32];
    void (*draw)(Window *w, int l, int t, int r, int b);
    void (*click)(Window *w, int cx, int cy, int dbl);
    int  live;
    GSurface surf;
    int  dirty, drawn_active;
    int  app_id, app_term, app_dead;
    unsigned long app_seq;
    uint8_t *app_bits; int app_rb, app_w, app_h;
};

/* Menus */
#define NMENUS 6
typedef struct { const char *title; const char *items[8]; int n; } Menu;

/* File targets */
typedef struct { char path[64]; char name[32]; int desktop; int prot; } Target;

/* Shared global state */
extern GSurface *BACK, *FRONT;
extern GSurface wall_s;
extern GSurface mb_s;
extern GSurface tb_s;
extern int cur_theme;
extern int tb_dirty;
extern int mb_dirty;
extern int cover_dirty;
extern gmutex_t scr;

extern Icon icons[MAX_ICONS];
extern int  nicons;
extern int  icon_sel;
extern int  focus_desktop;

extern Window  wins[MAX_WIN];
extern Window *z[MAX_WIN];
extern int     nwin;

extern Menu menus[NMENUS];
extern int menu_x[NMENUS], menu_w[NMENUS];
extern int  matrix_on;
extern char clip_path[64], clip_name[32];
extern char bench_str[32];
extern uint32_t bench_until;
extern int mb_minute;
extern INTEGER vref;
extern int quit_requested;
extern int game_sel;
extern uint32_t last_click_tick;
extern int last_click_icon;

#define LOCK()   gmutex_lock(&scr)
#define UNLOCK() gmutex_unlock(&scr)

/* Inline helpers */
static inline int mouse_x(void)    { return *(volatile int16_t *)0x832; }
static inline int mouse_y(void)    { return *(volatile int16_t *)0x830; }
static inline int mouse_down(void) { return (*(volatile uint8_t *)0x172 & 0x80) == 0; }
static inline int poll_key(void)   { return (int)plusos_syscall(SYS_pollkey, 0); }
static inline void nap(int t)      { plusos_syscall(SYS_sleep, (void *)(intptr_t)t); }
static inline uint32_t now(void)   { return plusos->ticks(); }
static inline int cmd_down(void)   { return (plusos->key_mods() & 0x0100) != 0; }
static inline int snap16(int x)    { return (x + 8) & ~15; }

static inline int in_rect(int x, int y, int l, int t, int r, int b) { return x >= l && x < r && y >= t && y < b; }
static inline int overlap(int l1, int t1, int r1, int b1, int l2, int t2, int r2, int b2)
{ return l1 < r2 && l2 < r1 && t1 < b2 && t2 < b1; }
static inline int mini(int a, int b) { return a < b ? a : b; }
static inline int maxi(int a, int b) { return a > b ? a : b; }

static inline int ieq(const char *a, const char *b)
{
    for (;; a++, b++) {
        int x = *a, y = *b;
        if (x >= 'A' && x <= 'Z') x += 32;
        if (y >= 'A' && y <= 'Z') y += 32;
        if (x != y) return 0;
        if (!x) return 1;
    }
}
static inline void scopy(char *d, const char *s, int n)
{
    int i = 0; for (; s[i] && i < n - 1; i++) d[i] = s[i]; d[i] = 0;
}

static inline int rget(const char *k, char *v, int n)
{
    struct plusos_reg_args a; memset(&a, 0, sizeof a);
    a.key = k; a.vbuf = v; a.vsize = n;
    return (int)plusos_syscall(SYS_regget, &a);
}
static inline void rset(const char *k, const char *v)
{
    struct plusos_reg_args a; memset(&a, 0, sizeof a);
    a.key = k; a.value = v;
    plusos_syscall(SYS_regset, &a);
}

static inline int theme_has_taskbar(void)
{
    return cur_theme == PLUSOS_THEME_WIN95 || cur_theme == PLUSOS_THEME_WIN98 ||
           cur_theme == PLUSOS_THEME_AERO  || cur_theme == PLUSOS_THEME_HACKER;
}
static inline int desk_bottom(void)
{
    return theme_has_taskbar() ? (SCREEN_H - TASKBAR_H) : SCREEN_H;
}

/* Function prototypes across modules */

/* desktop.c (Compositor & Main Engine) */
void compose(int l, int t, int r, int b);
void show(int t, int b);
void refresh(int l, int t, int r, int b);
void redraw(void);
void refresh_icon(int i);
void refresh_win(Window *w);
void swcursor_init(void);
void present_with_cursor(int t, int b);
void drag_live(int *px, int *py, int w, int h, int gx, int gy, int ymin);
void drag_window(Window *w, int gx, int gy);
void track_menus(void);
void do_menu(int m, int item);
int  item_enabled(int m, int i);
int  item_checked(int m, int i);
void click(int mx, int my);

/* desktop_theme.c (Multi-OS Themes, Decorations & Dialogs) */
void set_theme(int t);
void draw_win_btn(int l, int t, int r, int b, int kind);
void render_window(Window *w, int active);
void layout_menus(void);
void render_menubar(void);
void draw_wallpaper_rect(int l, int t, int r, int b);
void button(int l, int t, int r, int b, const char *label, int dflt);
void dlg_frame(int t, int b);
int  wait_input(int *mx, int *my);
void message(const char *l1, const char *l2);
int  confirm(const char *l1, const char *l2, const char *ok);
int  prompt(const char *title, char *buf, int size);

/* desktop_taskbar.c (Bottom Taskbar & Start Menu) */
void render_taskbar(void);
void track_start_menu(void);
void click_taskbar(int mx, int my);

/* desktop_windows.c (Window Lifecycle, Built-in Windows & App Sync) */
Window *front(void);
Window *find_kind(int kind);
void    bring_front(Window *w);
void    send_back(Window *w);
void    close_window(Window *w);
Window *new_window(int kind, const char *title, int x, int y, int w, int h);
void    open_about(void);
void    open_info(void);
void    open_settings(void);
void    open_programs(void);
void    open_games(void);
void    scan_programs(void);
void    scan_games(void);
void    start_prog(int argc, char **argv);
void    launch(int argc, char **argv, int pause);
void    hex_edit(const char *path);
void    post_ev(int id, int type, int x, int y, int key);
void    destroy_app_window(int id);
void    sync_apps(void);
void    start_system(int id, const char *name);

/* desktop_icons.c (Desktop & System Icons, Files, File Actions) */
void c2p(unsigned char *p, const char *c);
long dir_id(const char *name);
int  list_files(long dir, char names[][32], int max);
int  file_exists(const char *path);
int  is_program(const char *path);
int  copy_file(const char *src, const char *dst);
int  delete_file(const char *path);
int  rename_file(const char *from, const char *to);
void build_icons(void);
void free_icon(Icon *ic);
void icon_art(const Icon *ic, int bx, int by, int shape);
void render_icon(Icon *ic, int sel);
int  icon_at(int mx, int my);
void press_icon(int i, int mx, int my);
void open_icon(int i);
void cmd_open(void);
void cmd_copy(void);
void cmd_paste(void);
void cmd_duplicate(void);
void cmd_delete(void);
void cmd_rename(void);
void clean_up(void);
int  get_target(Target *t);
void copy_to_desktop(const char *src, const char *name);

/* desktop_matrix.c (Matrix Rain Wallpaper & Suspension) */
void set_overlay(int l, int t, int r, int b);
void clear_overlay(void);
void render_suspend(void);
void render_resume(void);
void m_init(void);
void m_cover_update(void);
void set_wallpaper(int matrix);
void renderer_thread(void *arg);

#endif /* DESKTOP_H */
