/* desktop_windows.c - PlusOS Desktop Window Management and Built-in Applications.
 *
 * Implements window lifecycle, built-in system windows (About, Info, Settings,
 * Programs, Games), and synchronization with the kernel window server.
 */
#include "desktop.h"

Window *front(void) { return nwin ? z[nwin - 1] : 0; }
Window *find_kind(int kind) { for (int i = 0; i < nwin; i++) if (z[i]->kind == kind) return z[i]; return 0; }

void bring_front(Window *w)
{
    LOCK();
    int i; for (i = 0; i < nwin && z[i] != w; i++) {}
    if (i < nwin) {
        for (; i < nwin - 1; i++) z[i] = z[i + 1];
        z[nwin - 1] = w;
        cover_dirty = 1;
        tb_dirty = 1;
    }
    UNLOCK();
}

void send_back(Window *w)
{
    LOCK();
    int i; for (i = 0; i < nwin && z[i] != w; i++) {}
    if (i > 0 && i < nwin) {
        for (; i > 0; i--) z[i] = z[i - 1];
        z[0] = w;
        cover_dirty = 1;
        tb_dirty = 1;
    }
    UNLOCK();
}

void close_window(Window *w)
{
    LOCK();
    int i; for (i = 0; i < nwin && z[i] != w; i++) {}
    if (i < nwin) {
        for (; i < nwin - 1; i++) z[i] = z[i + 1];
        nwin--; w->used = 0;
        gs_free(&w->surf);
        cover_dirty = 1;
        tb_dirty = 1;
    }
    UNLOCK();
}

Window *new_window(int kind, const char *title, int x, int y, int w, int h)
{
    Window *old = kind == WK_APP ? 0 : find_kind(kind);       /* one of each built-in window */
    if (old) { bring_front(old); return 0; }
    if (nwin >= MAX_WIN) return 0;
    Window *win = 0;
    for (int i = 0; i < MAX_WIN; i++) if (!wins[i].used) { win = &wins[i]; break; }
    if (!win) return 0;
    memset(win, 0, sizeof *win);
    win->used = 1; win->kind = kind;
    win->x = snap16(x); win->y = y; win->w = (w + 15) & ~15; win->h = h;
    win->dirty = 1;
    scopy(win->title, title, sizeof win->title);
    LOCK();
    z[nwin++] = win;
    cover_dirty = 1;
    tb_dirty = 1;
    UNLOCK();
    return win;
}

/* Built-in Windows */
static void draw_about(Window *w, int l, int t, int r, int b)
{
    (void)w; (void)r; (void)b;
    char s[44];
    gfx_text(l + 12, t + 8, "PlusOS 2.0 - Multi-OS Desktop", 0);
    snprintf(s, sizeof s, "Theme: %s", plusos_theme_name(cur_theme));
    gfx_text(l + 12, t + 22, s, 0);
    gfx_text(l + 12, t + 38, "Switch OS look & feel anytime:", 0);
    gfx_text(l + 12, t + 50, "Mac 7.1 | Amiga | Win95 | Win98", 0);
    gfx_text(l + 12, t + 62, "Vista/7 Aero | Hacker Matrix", 0);
    gfx_text(l + 12, t + 78, "Use Theme menu, Settings, or", 0);
    gfx_text(l + 12, t + 88, "Cmd+1..6 / `theme` CLI tool.", 0);
}

static void draw_info(Window *w, int l, int t, int r, int b)
{
    (void)w; (void)r; (void)b;
    struct plusos_sysinfo si;
    plusos_syscall(SYS_sysinfo, &si);
    char s[48];
    unsigned long up = si.ticks / 60;
    gfx_text(l + 10, t + 8, si.machine, 0);
    snprintf(s, sizeof s, "CPU %s, RAM %luK", si.cpu, si.ram_bytes >> 10);  gfx_text(l + 10, t + 20, s, 0);
    snprintf(s, sizeof s, "Free memory %luK", si.free_bytes >> 10);         gfx_text(l + 10, t + 32, s, 0);
    snprintf(s, sizeof s, "Disk \"%s\"", si.volume);                        gfx_text(l + 10, t + 44, s, 0);
    snprintf(s, sizeof s, "Up %lu:%02lu:%02lu  %d processes", up / 3600, (up / 60) % 60, up % 60, si.nprocs);
    gfx_text(l + 10, t + 56, s, 0);
}

static void radio(int x, int y, int on, const char *label)
{
    gfx_frame(x, y, x + 10, y + 10);
    gfx_fill(x + 1, y + 1, x + 9, y + 9, 0);
    if (on) gfx_fill(x + 3, y + 3, x + 7, y + 7, 1);
    gfx_text(x + 14, y + 1, label, 0);
}

static int boot_pref(void) { return (int)plusos_syscall(SYS_getcfg, (void *)PLUSOS_CFG_BOOT); }

static void draw_settings(Window *w, int l, int t, int r, int b)
{
    (void)w; (void)r; (void)b;
    int pref = boot_pref();
    gfx_text(l + 10, t + 6, "OS Theme (or Cmd+1..6):", 0);
    radio(l + 14,  t + 20, cur_theme == PLUSOS_THEME_MAC,    "Mac OS 7.1");
    radio(l + 136, t + 20, cur_theme == PLUSOS_THEME_AMIGA,  "Amiga WB");
    radio(l + 14,  t + 36, cur_theme == PLUSOS_THEME_WIN95,  "Windows 95");
    radio(l + 136, t + 36, cur_theme == PLUSOS_THEME_WIN98,  "Windows 98");
    radio(l + 14,  t + 52, cur_theme == PLUSOS_THEME_AERO,   "Vista / 7");
    radio(l + 136, t + 52, cur_theme == PLUSOS_THEME_HACKER, "Hacker CRT");

    gfx_hline(l + 8, l + 248, t + 68, 1);
    gfx_text(l + 10, t + 74, "Boot into:", 0);
    radio(l + 14,  t + 88, pref == PLUSOS_BOOT_SHELL,   "Shell");
    radio(l + 136, t + 88, pref == PLUSOS_BOOT_DESKTOP, "Desktop");

    gfx_text(l + 10, t + 106, "Wallpaper:", 0);
    radio(l + 14,  t + 120, !matrix_on, "Theme Pattern");
    radio(l + 136, t + 120, matrix_on,  "Matrix Rain");
}

static void click_settings(Window *w, int cx, int cy, int dbl)
{
    (void)dbl;
    if (cx < 10) return;
    int col = cx >= 132 ? 1 : 0;
    if (cy >= 18 && cy < 32)      { set_theme(col ? PLUSOS_THEME_AMIGA  : PLUSOS_THEME_MAC); }
    else if (cy >= 34 && cy < 48) { set_theme(col ? PLUSOS_THEME_WIN98  : PLUSOS_THEME_WIN95); }
    else if (cy >= 50 && cy < 64) { set_theme(col ? PLUSOS_THEME_HACKER : PLUSOS_THEME_AERO); }
    else if (cy >= 86 && cy < 100) {
        struct plusos_cfg_args c = { PLUSOS_CFG_BOOT, col ? PLUSOS_BOOT_DESKTOP : PLUSOS_BOOT_SHELL };
        plusos_syscall(SYS_setcfg, &c);
        refresh_win(w);
    } else if (cy >= 118 && cy < 134) {
        if (!col && matrix_on)  { w->dirty = 1; set_wallpaper(0); }
        if (col  && !matrix_on) { w->dirty = 1; set_wallpaper(1); }
    }
}

#define PW_COLS  4
#define PW_CELLW 68
#define PW_CELLH 13
static char prog_names[80][32];
static int  nprog, prog_sel = -1;
static long bin_dir;

void scan_programs(void)
{
    if (!bin_dir) bin_dir = dir_id("bin");
    nprog = bin_dir ? list_files(bin_dir, prog_names, 80) : 0;
    if (prog_sel >= nprog) prog_sel = -1;
}

static void draw_programs(Window *w, int l, int t, int r, int b)
{
    (void)w; (void)r; (void)b;
    for (int i = 0; i < nprog; i++) {
        int x = l + 4 + (i % PW_COLS) * PW_CELLW, y = t + 4 + (i / PW_COLS) * PW_CELLH;
        char s[10]; scopy(s, prog_names[i], 9);
        int sel = i == prog_sel && !focus_desktop;
        if (sel) gfx_fill(x - 2, y - 2, x + PW_CELLW - 4, y + 10, 1);
        gfx_text(x, y, s, sel);
    }
}

static void click_programs(Window *w, int cx, int cy, int dbl)
{
    int col = (cx - 2) / PW_CELLW, row = (cy - 2) / PW_CELLH;
    int i = row * PW_COLS + col;
    if (col < 0 || col >= PW_COLS || row < 0 || i >= nprog) i = -1;
    int old = prog_sel;
    prog_sel = i;
    if (old != i || icon_sel >= 0) {
        int oi = icon_sel; icon_sel = -1;
        refresh_win(w);
        if (oi >= 0) refresh_icon(oi);
    }
    if (dbl && i >= 0) { char *av[] = { prog_names[i], 0 }; start_prog(1, av); }
}

void open_about(void)
{
    Window *w = new_window(WK_ABOUT, "About PlusOS", 40, 44, 272, 118);
    if (w) w->draw = draw_about;
}
void open_info(void)
{
    Window *w = new_window(WK_INFO, "System Info", 72, 176, 296, 92);
    if (w) { w->draw = draw_info; w->live = 1; }
}
void open_settings(void)
{
    Window *w = new_window(WK_SETTINGS, "Theme & Settings", 136, 52, 256, 160);
    if (w) { w->draw = draw_settings; w->click = click_settings; }
}
void open_programs(void)
{
    scan_programs();
    int rows = (nprog + PW_COLS - 1) / PW_COLS;
    if (rows < 1) rows = 1;
    Window *w = new_window(WK_PROGRAMS, "Programs (bin)", 96, 44, PW_COLS * PW_CELLW + 8, TITLEBAR_H + rows * PW_CELLH + 10);
    if (w) { w->draw = draw_programs; w->click = click_programs; }
}

/* Games folder view */
#define GW_COLS   4
#define GW_CELLW  74
#define GW_CELLH  54
static char game_names[80][32];
static int  ngame;
int game_sel = -1;
static long games_dir;

void scan_games(void)
{
    if (!games_dir) games_dir = dir_id("Games");
    ngame = games_dir ? list_files(games_dir, game_names, 80) : 0;
    if (game_sel >= ngame) game_sel = -1;
}

static void draw_games(Window *w, int l, int t, int r, int b)
{
    (void)w; (void)r; (void)b;
    if (ngame == 0) {
        gfx_text(l + 12, t + 16, games_dir ? "This folder is empty." : "No Games folder on this disk.", 0);
        return;
    }
    int on_top = front() && front()->kind == WK_GAMES && !focus_desktop;
    for (int i = 0; i < ngame; i++) {
        int col = i % GW_COLS, row = i / GW_COLS;
        int cx = l + 6 + col * GW_CELLW, cy = t + 6 + row * GW_CELLH;
        int ax = cx + GW_CELLW / 2 - 16;
        Icon tmp; memset(&tmp, 0, sizeof tmp); tmp.kind = K_FILE; tmp.program = 1;
        icon_art(&tmp, ax, cy, 0);
        char lab[12]; scopy(lab, game_names[i], 9);
        int lw = gfx_text_width(lab), lx = cx + (GW_CELLW - lw) / 2;
        int sel = i == game_sel && on_top;
        if (sel) gfx_invert(ax + 4, cy + 2, ax + 28, cy + 26);
        gfx_fill(lx - 2, cy + 34, lx + lw + 2, cy + 44, sel);
        gfx_text(lx, cy + 35, lab, sel);
    }
}

static void click_games(Window *w, int cx, int cy, int dbl)
{
    int col = (cx - 6) / GW_CELLW, row = (cy - 6) / GW_CELLH;
    int i = row * GW_COLS + col;
    if (col < 0 || col >= GW_COLS || row < 0 || i >= ngame) i = -1;
    int old = game_sel;
    game_sel = i;
    if (old != i || icon_sel >= 0) {
        int oi = icon_sel; icon_sel = -1;
        refresh_win(w);
        if (oi >= 0) refresh_icon(oi);
    }
    if (dbl && i >= 0) {
        char gp[80]; snprintf(gp, sizeof gp, ":Games:%s", game_names[i]);
        char *av[] = { gp, 0 };
        start_prog(1, av);
    }
}

void open_games(void)
{
    scan_games();
    int rows = (ngame + GW_COLS - 1) / GW_COLS;
    if (rows < 1) rows = 1;
    Window *w = new_window(WK_GAMES, "Games", 88, 54, GW_COLS * GW_CELLW + 14, TITLEBAR_H + rows * GW_CELLH + 14);
    if (w) { w->draw = draw_games; w->click = click_games; }
}

/* Running programs */
void launch(int argc, char **argv, int pause)
{
    render_suspend();
    HideCursor();
    con_clear();
    int pid = plusos->run(argv[0], argc, argv);
    if (pid >= 0) {
        plusos_syscall(SYS_wait, (void *)(intptr_t)pid);
        if (pause) {
            printf("\n[finished - press a key to return to the desktop]");
            plusos->key_get();
        }
    }
    InitCursor();
    render_resume();
    redraw();
    if (pid < 0) {
        char s[48];
        snprintf(s, sizeof s, "Can't open \"%s\".", argv[0]);
        message(s, pid == -1001 ? "It isn't on the disk." : "The program could not start.");
    }
}

static const char *leaf(const char *p) { const char *l = p; for (; *p; p++) if (*p == ':') l = p + 1; return l; }

static int is_gui_app(const char *name)
{
    const char *n = leaf(name);
    return ieq(n, "taskman") || ieq(n, "files") ||
           ieq(n, "calc") || ieq(n, "notes") || ieq(n, "puzzle");
}

static int is_fullscreen_app(const char *name)
{
    static const char *fs[] = { "qdemo", "asteroids", "breakout", "cube3d", "dungeon",
        "flappy", "invaders", "lunar", "mines", "pacman", "racer", "snake", "solarsys",
        "starflight", "tetris", "lander", "maze", "pong", "life",
        "chip8", "emu6502", "basic", "sound", "macrun", 0 };
    const char *n = leaf(name);
    for (int i = 0; fs[i]; i++) if (ieq(n, fs[i])) return 1;
    return 0;
}

void start_prog(int argc, char **argv)
{
    const char *n = leaf(argv[0]);
    if (ieq(n, "desktop")) return;
    if (is_fullscreen_app(n)) { launch(argc, argv, 1); return; }
    struct plusos_spawn_args s = { argv[0], argc, argv };
    int pid = is_gui_app(argv[0]) ? (int)plusos_syscall(SYS_spawn, &s)
                                  : (int)plusos_syscall(SYS_termspawn, &s);
    if (pid < 0) {
        char m[48];
        snprintf(m, sizeof m, "Can't open \"%s\".", leaf(argv[0]));
        message(m, pid == -1001 ? "It isn't on the disk." : "The program could not start.");
    }
}

void hex_edit(const char *path)
{
    char *av[] = { "hexedit", (char *)path, 0 };
    start_prog(path ? 2 : 1, av);
}

void post_ev(int id, int type, int x, int y, int key)
{
    struct plusos_event e = { type, x, y, key };
    struct plusos_win_args a; memset(&a, 0, sizeof a);
    a.op = PLUSOS_WIN_POST; a.id = id; a.ev = &e;
    plusos_syscall(SYS_win, &a);
}

void destroy_app_window(int id)
{
    struct plusos_win_args a; memset(&a, 0, sizeof a);
    a.op = PLUSOS_WIN_DESTROY; a.id = id;
    plusos_syscall(SYS_win, &a);
}

#define MAX_KW 12
static struct plusos_wininfo kwl[MAX_KW];
static unsigned long ws_last;
static int cascade;

static Window *find_app(int id)
{
    for (int i = 0; i < nwin; i++)
        if (z[i]->kind == WK_APP && z[i]->app_id == id) return z[i];
    return 0;
}

void sync_apps(void)
{
    struct plusos_win_args a; memset(&a, 0, sizeof a);
    a.op = PLUSOS_WIN_LIST; a.list = kwl; a.max = MAX_KW;
    int n = (int)plusos_syscall(SYS_win, &a);
    if (n < 0 || a.seq == ws_last) return;
    ws_last = a.seq;
    for (int i = nwin - 1; i >= 0; i--) {                  /* gone? */
        Window *w = z[i];
        if (w->kind != WK_APP) continue;
        int found = 0;
        for (int j = 0; j < n; j++) if (kwl[j].id == w->app_id) found = 1;
        if (found) continue;
        int l = w->x, t = w->y, r = w->x + w->w + 2, b = w->y + w->h + 2;
        close_window(w);
        refresh(l, t, r, b);
        if (theme_has_taskbar()) refresh(0, SCREEN_H - TASKBAR_H, SCREEN_W, SCREEN_H);
        Window *nf = front();
        if (nf) refresh_win(nf);
        i = nwin;                                           /* the list changed: rescan */
    }
    for (int j = 0; j < n; j++) {
        struct plusos_wininfo *k = &kwl[j];
        Window *w = find_app(k->id);
        if (!w) {                                           /* new */
            int ww = k->w + 2 * APP_PADX, hh = k->h + APP_TOP + APP_PADB;
            int x = k->x >= 0 ? k->x : 16 + (cascade % 6) * 16;
            int y = k->y >= 0 ? k->y : 28 + (cascade % 6) * 18;
            cascade++;
            if (x + ww > SCREEN_W) x = SCREEN_W - ww;
            if (x < 0) x = 0;
            if (y + hh > desk_bottom()) y = desk_bottom() - hh;
            if (y < MENUBAR_H) y = MENUBAR_H;
            Window *old = front();
            w = new_window(WK_APP, k->title, x, y, ww, hh);
            if (!w) continue;
            w->app_id = k->id; w->app_term = k->term; w->app_dead = k->dead; w->app_seq = k->seq;
            w->app_bits = k->bits; w->app_rb = k->rowbytes; w->app_w = k->w; w->app_h = k->h;
            focus_desktop = 0;
            if (old) refresh(old->x, old->y, old->x + old->w + 2, old->y + old->h + 2);
            refresh_win(w);
            if (theme_has_taskbar()) refresh(0, SCREEN_H - TASKBAR_H, SCREEN_W, SCREEN_H);
            continue;
        }
        int retitle = strcmp(w->title, k->title) != 0;
        if (retitle) { scopy(w->title, k->title, sizeof w->title); tb_dirty = 1; }
        w->app_dead = k->dead;
        if (retitle) {
            w->app_seq = k->seq;
            refresh_win(w);
            if (theme_has_taskbar()) refresh(0, SCREEN_H - TASKBAR_H, SCREEN_W, SCREEN_H);
        } else if (k->seq != w->app_seq) {
            w->app_seq = k->seq;
            refresh(w->x + APP_PADX, w->y + APP_TOP, w->x + APP_PADX + w->app_w, w->y + APP_TOP + w->app_h);
        }
    }
}

void start_system(int id, const char *name)
{
    render_suspend();
    char s[40];
    snprintf(s, sizeof s, "Starting %s...", name);
    dlg_frame(130, 170);
    gfx_text(DL + 16, 146, s, 0);
    show(130, 174); UNLOCK();
    long rc = plusos_syscall(SYS_chainboot, (void *)(intptr_t)id);
    clear_overlay(); refresh(DL, 130, DR + 4, 174); render_resume();
    if (rc == PLUSOS_EBOOT_NOTSE) message("Switching Systems needs a", "Macintosh SE.");
    else if (rc == PLUSOS_EBOOT_NODISK) { snprintf(s, sizeof s, "No disk at SCSI ID %d.", id); message(s, 0); }
    else message("Could not set the startup disk.", 0);
}
