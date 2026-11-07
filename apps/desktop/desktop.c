/* desktop.c - the PlusOS desktop, a ring-2 (userland) application, built on
 * the PlusGFX rendering framework (plusos/gfx.h).
 *
 *  - A file-system desktop: file icons are the files in the volume's
 *    "Desktop" folder, plus system icons. Every icon can be dragged; positions
 *    are saved in the system registry.
 *  - Edit menu: Copy, Paste, Duplicate, Rename, Delete (or drag to the Trash).
 *  - Pull-down menus, windows that move live under the mouse, a Programs
 *    window listing bin, Settings, a menu-bar clock, and an animated Matrix
 *    wallpaper.
 *
 * Rendering: every window, icon and the menu bar has its own cached surface
 * (a backing store), redrawn only when its contents change. The compositor
 * rebuilds just the rectangle that changed in the back buffer - wallpaper,
 * then icons, then windows, then the menu bar, all byte-aligned block copies -
 * and flips just those rows to the screen.
 */
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

static GSurface *BACK, *FRONT;
static GSurface wall_s;                 /* Matrix wallpaper */
static GSurface mb_s;                   /* cached menu bar */
static GSurface tb_s;                   /* cached bottom taskbar (Win95/98/Aero/Hacker) */
static int cur_theme = PLUSOS_THEME_MAC;
static int tb_dirty = 1;

static int theme_has_taskbar(void)
{
    return cur_theme == PLUSOS_THEME_WIN95 || cur_theme == PLUSOS_THEME_WIN98 ||
           cur_theme == PLUSOS_THEME_AERO  || cur_theme == PLUSOS_THEME_HACKER;
}
static int desk_bottom(void)
{
    return theme_has_taskbar() ? (SCREEN_H - TASKBAR_H) : SCREEN_H;
}

/* Two threads draw: the main thread (input, windows, icons, menus) and the
 * renderer thread (the animated wallpaper, which never stops). They share the
 * screen through this lock; it is recursive, so nested drawing calls are fine. */
static gmutex_t scr = GMUTEX_INIT;
#define LOCK()   gmutex_lock(&scr)
#define UNLOCK() gmutex_unlock(&scr)

/* Something drawn on top of the scene for a while (an open menu, a dialog):
 * the wallpaper animation keeps out of it. */
static volatile int ov_on, ov_l, ov_t, ov_r, ov_b;
static int cover_dirty = 1;
static void set_overlay(int l, int t, int r, int b) { LOCK(); ov_l = l; ov_t = t; ov_r = r; ov_b = b; ov_on = 1; cover_dirty = 1; UNLOCK(); }
static void clear_overlay(void) { LOCK(); ov_on = 0; cover_dirty = 1; UNLOCK(); }

/* Renderer park/resume handshake. When a full-screen program runs or a modal
 * dialog is up, the renderer must not touch the screen at all (or its animation
 * leaks through). render_suspend() asks it to stop and waits until it confirms
 * it is parked; render_resume() lets it run again. Nested suspends are counted. */
static volatile int render_pause_req, render_parked;
static int render_depth;
static void nap(int t);
static void render_suspend(void)
{
    if (render_depth++ > 0) return;
    render_pause_req = 1;
    for (int i = 0; i < 200 && !render_parked; i++) nap(1);   /* wait, bounded */
}
static void render_resume(void) { if (--render_depth <= 0) { render_depth = 0; render_pause_req = 0; } }

/* ======================================================================== */
/* helpers                                                                  */
/* ======================================================================== */
static int mouse_x(void)    { return *(volatile int16_t *)0x832; }
static int mouse_y(void)    { return *(volatile int16_t *)0x830; }
static int mouse_down(void) { return (*(volatile uint8_t *)0x172 & 0x80) == 0; }
static int poll_key(void)   { return (int)plusos_syscall(SYS_pollkey, 0); }
static void nap(int t)      { plusos_syscall(SYS_sleep, (void *)(intptr_t)t); }
static uint32_t now(void)   { return plusos->ticks(); }
static int cmd_down(void)   { return (plusos->key_mods() & 0x0100) != 0; }
static int snap16(int x)    { return (x + 8) & ~15; }   /* keeps copies longword-aligned */

static int in_rect(int x, int y, int l, int t, int r, int b) { return x >= l && x < r && y >= t && y < b; }
static int overlap(int l1, int t1, int r1, int b1, int l2, int t2, int r2, int b2)
{ return l1 < r2 && l2 < r1 && t1 < b2 && t2 < b1; }
static int mini(int a, int b) { return a < b ? a : b; }
static int maxi(int a, int b) { return a > b ? a : b; }

static int ieq(const char *a, const char *b)
{
    for (;; a++, b++) {
        int x = *a, y = *b;
        if (x >= 'A' && x <= 'Z') x += 32;
        if (y >= 'A' && y <= 'Z') y += 32;
        if (x != y) return 0;
        if (!x) return 1;
    }
}
static void scopy(char *d, const char *s, int n) { int i = 0; for (; s[i] && i < n - 1; i++) d[i] = s[i]; d[i] = 0; }

static int rget(const char *k, char *v, int n)
{
    struct plusos_reg_args a; memset(&a, 0, sizeof a);
    a.key = k; a.vbuf = v; a.vsize = n;
    return (int)plusos_syscall(SYS_regget, &a);
}
static void rset(const char *k, const char *v)
{
    struct plusos_reg_args a; memset(&a, 0, sizeof a);
    a.key = k; a.value = v;
    plusos_syscall(SYS_regset, &a);
}

/* ======================================================================== */
/* files                                                                    */
/* ======================================================================== */
#define ROOT_DIR 2L
static INTEGER vref;

static void c2p(unsigned char *p, const char *c)
{
    int n = 0;
    while (c[n] && n < 254) { p[n + 1] = (unsigned char)c[n]; n++; }
    p[0] = (unsigned char)n;
}

static long dir_id(const char *name)
{
    for (int idx = 1; idx < 200; idx++) {
        CInfoPBRec pb; unsigned char nm[64]; char c[64];
        memset(&pb, 0, sizeof pb);
        pb.dirInfo.ioNamePtr = nm;
        pb.dirInfo.ioVRefNum = vref;
        pb.dirInfo.ioDrDirID = ROOT_DIR;
        pb.dirInfo.ioFDirIndex = idx;
        if (PBGetCatInfoSync(&pb) != 0) break;
        int n = nm[0]; for (int i = 0; i < n; i++) c[i] = (char)nm[i + 1]; c[n] = 0;
        if ((pb.dirInfo.ioFlAttrib & 0x10) && ieq(c, name)) return pb.dirInfo.ioDrDirID;
    }
    return 0;
}

static int list_files(long dir, char names[][32], int max)
{
    int n = 0;
    for (int idx = 1; idx < 300 && n < max; idx++) {
        CInfoPBRec pb; unsigned char nm[64];
        memset(&pb, 0, sizeof pb);
        pb.hFileInfo.ioNamePtr = nm;
        pb.hFileInfo.ioVRefNum = vref;
        pb.hFileInfo.ioDirID = dir;
        pb.hFileInfo.ioFDirIndex = idx;
        if (PBGetCatInfoSync(&pb) != 0) break;
        if (pb.hFileInfo.ioFlAttrib & 0x10) continue;
        int k = nm[0] > 31 ? 31 : nm[0];
        for (int i = 0; i < k; i++) names[n][i] = (char)nm[i + 1];
        names[n][k] = 0;
        n++;
    }
    return n;
}

static int file_exists(const char *path)
{
    unsigned char p[256]; FInfo fi;
    c2p(p, path);
    return GetFInfo(p, vref, &fi) == 0;
}

static int is_program(const char *path)
{
    unsigned char p[256]; INTEGER ref; char h[8];
    c2p(p, path);
    if (FSOpen(p, vref, &ref) != 0) return 0;
    LONGINT n = 8;
    int ok = FSRead(ref, &n, h) == 0 && n == 8 && h[4] == 'P' && h[5] == 'L' && h[6] == 'O' && h[7] == 'S';
    FSClose(ref);
    return ok;
}

static int copy_file(const char *src, const char *dst)
{
    unsigned char ps[256], pd[256];
    c2p(ps, src); c2p(pd, dst);
    FInfo fi;
    OSType creator = 0x504C4F53, type = 0x504C4F53;
    if (GetFInfo(ps, vref, &fi) == 0) { creator = fi.fdCreator; type = fi.fdType; }
    INTEGER in, out;
    if (FSOpen(ps, vref, &in) != 0) return -1;
    if (Create(pd, vref, creator, type) != 0 || FSOpen(pd, vref, &out) != 0) { FSClose(in); return -1; }
    char buf[512];
    int rc = 0;
    for (;;) {
        LONGINT n = sizeof buf;
        OSErr e = FSRead(in, &n, buf);
        if (n > 0) { LONGINT w = n; if (FSWrite(out, &w, buf) != 0 || w != n) { rc = -1; break; } }
        if (e != 0) break;
    }
    FSClose(in); FSClose(out);
    FlushVol(0, vref);
    return rc;
}

static int delete_file(const char *path)
{
    unsigned char p[256]; c2p(p, path);
    int rc = FSDelete(p, vref);
    FlushVol(0, vref);
    return rc;
}

static int rename_file(const char *from, const char *to)
{
    unsigned char a[256], b[256]; c2p(a, from); c2p(b, to);
    int rc = Rename(a, vref, b);
    FlushVol(0, vref);
    return rc;
}

/* ======================================================================== */
/* icons (each cached as an image + mask surface)                           */
/* ======================================================================== */
enum { K_PROGRAMS, K_GAMES, K_SHELL, K_HEX, K_TASKS, K_FILES, K_SETTINGS, K_TRASH, K_FILE };

typedef struct {
    int  kind, x, y;
    char id[28], label[32], path[64];
    int  program;
    GSurface img, mask;
    int  rendered;              /* -1: not rendered, else the selection state drawn */
} Icon;

static Icon icons[MAX_ICONS];
static int  nicons;
static int  icon_sel = -1;
static int  focus_desktop = 1;

static const struct { int kind; const char *id, *label; int x, y; } sys_icons[] = {
    { K_PROGRAMS, "programs", "Programs",   SCREEN_W - 80, 28 },
    { K_GAMES,    "games",    "Games",      24, 28 },
    { K_SHELL,    "shell",    "Shell",      SCREEN_W - 80, 76 },
    { K_FILES,    "files",    "Files",      SCREEN_W - 80, 124 },
    { K_HEX,      "hexedit",  "Hex Edit", SCREEN_W - 80, 170 },
    { K_TASKS,    "tasks",    "Tasks",      SCREEN_W - 80, 216 },
    { K_SETTINGS, "settings", "Settings",   SCREEN_W - 80, 262 },
    { K_TRASH,    "trash",    "Trash",      SCREEN_W - 80, SCREEN_H - ICON_H - 4 },
};
#define NSYS ((int)(sizeof sys_icons / sizeof sys_icons[0]))

static void icon_key(const Icon *ic, char *k) { snprintf(k, 40, "icon.%s", ic->id); }

static void save_icon_pos(const Icon *ic)
{
    if (ic->kind != K_FILE) return;         /* only desktop files remember their place */
    char k[40], v[16];
    icon_key(ic, k);
    snprintf(v, sizeof v, "%d,%d", ic->x, ic->y);
    rset(k, v);
}

static int load_icon_pos(Icon *ic)
{
    char k[40], v[24];
    icon_key(ic, k);
    if (rget(k, v, sizeof v) <= 0) return 0;
    int x = atoi(v), y = 0;
    for (char *p = v; *p; p++) if (*p == ',') { y = atoi(p + 1); break; }
    if (x < 0 || x > SCREEN_W - ICON_W || y < MENUBAR_H || y > SCREEN_H - ICON_H) return 0;
    ic->x = snap16(x); ic->y = y;
    return 1;
}

static int spot_free(int x, int y, int skip)
{
    for (int i = 0; i < nicons; i++)
        if (i != skip && overlap(x, y, x + ICON_W, y + ICON_H, icons[i].x, icons[i].y,
                                 icons[i].x + ICON_W, icons[i].y + ICON_H)) return 0;
    return 1;
}

static void place_free(Icon *ic, int self)
{
    for (int y = 200; y + ICON_H <= SCREEN_H; y += 52)
        for (int x = 16; x + ICON_W <= SCREEN_W - 150; x += 80)
            if (spot_free(x, y, self)) { ic->x = x; ic->y = y; return; }
    ic->x = 16; ic->y = 200;
}

static long desk_dir;

static void free_icon(Icon *ic) { gs_free(&ic->img); gs_free(&ic->mask); }

static void build_icons(void)
{
    /* read the disk first, without holding the screen lock ... */
    static char names[MAX_ICONS][32];
    static uint8_t progs[MAX_ICONS];
    if (!desk_dir) desk_dir = dir_id("Desktop");
    int nfiles = desk_dir ? list_files(desk_dir, names, MAX_ICONS - NSYS) : 0;
    for (int i = 0; i < nfiles; i++) {
        char path[64];
        snprintf(path, sizeof path, ":Desktop:%s", names[i]);
        progs[i] = (uint8_t)is_program(path);
    }
    /* ... then rebuild the list in one locked step */
    LOCK();
    char keep[32] = "";
    if (icon_sel >= 0) scopy(keep, icons[icon_sel].id, sizeof keep);
    for (int i = 0; i < nicons; i++) free_icon(&icons[i]);
    nicons = 0;
    for (int i = 0; i < NSYS; i++) {
        Icon *ic = &icons[nicons++];
        memset(ic, 0, sizeof *ic);
        ic->kind = sys_icons[i].kind;
        scopy(ic->id, sys_icons[i].id, sizeof ic->id);
        scopy(ic->label, sys_icons[i].label, sizeof ic->label);
        ic->x = sys_icons[i].x; ic->y = sys_icons[i].y;   /* system icons: fixed column */
        if (ic->kind == K_TRASH) ic->y = desk_bottom() - ICON_H - 4;
        ic->rendered = -1;
    }
    for (int i = 0; i < nfiles; i++) {
        Icon *ic = &icons[nicons];
        memset(ic, 0, sizeof *ic);
        ic->kind = K_FILE;
        ic->rendered = -1;
        scopy(ic->label, names[i], sizeof ic->label);
        snprintf(ic->id, sizeof ic->id, "f/%s", names[i]);
        snprintf(ic->path, sizeof ic->path, ":Desktop:%s", names[i]);
        ic->program = progs[i];
        nicons++;
        if (!load_icon_pos(ic)) place_free(ic, nicons - 1);
    }
    icon_sel = -1;
    for (int i = 0; i < nicons; i++) if (keep[0] && !strcmp(icons[i].id, keep)) icon_sel = i;
    cover_dirty = 1;
    UNLOCK();
}

/* draw an icon's picture into the current target at (bx,by); `shape` draws
 * the solid silhouette instead (for the mask) */
static void icon_art(const Icon *ic, int bx, int by, int shape)
{
    switch (ic->kind) {
    case K_GAMES:                                        /* folder (with a game pip) */
    case K_PROGRAMS:                                     /* folder / Amiga drawer */
        if (cur_theme == PLUSOS_THEME_AMIGA) {           /* Amiga Workbench 3D Drawer */
            if (shape) { gfx_fill(bx + 1, by + 3, bx + 31, by + 26, 1); break; }
            gfx_fill(bx + 1, by + 3, bx + 31, by + 26, 0);
            gfx_frame(bx + 1, by + 3, bx + 31, by + 26);
            gfx_frame(bx + 4, by + 8, bx + 28, by + 23);
            gfx_hline(bx + 4, bx + 28, by + 6, 1);
            gfx_fill(bx + 12, by + 13, bx + 20, by + 17, 1); /* Amiga drawer handle */
            gfx_fill(bx + 14, by + 14, bx + 18, by + 16, 0);
            break;
        }
        if (shape) { gfx_fill(bx + 2, by + 4, bx + 14, by + 9, 1); gfx_fill(bx + 2, by + 8, bx + 30, by + 26, 1); break; }
        gfx_fill(bx + 2, by + 4, bx + 14, by + 9, 0); gfx_frame(bx + 2, by + 4, bx + 14, by + 9);
        gfx_fill(bx + 2, by + 8, bx + 30, by + 26, 0); gfx_frame(bx + 2, by + 8, bx + 30, by + 26);
        if (cur_theme == PLUSOS_THEME_WIN95 || cur_theme == PLUSOS_THEME_WIN98 || cur_theme == PLUSOS_THEME_AERO)
            gfx_dither_rect(bx + 3, by + 10, bx + 29, by + 25, 4);
        gfx_hline(bx + 3, bx + 29, by + 11, 1);
        if (ic->kind == K_GAMES) {                       /* a little joystick on the folder */
            gfx_vline(bx + 16, by + 16, by + 22, 1);
            gfx_fill(bx + 14, by + 21, bx + 19, by + 24, 1);
            gfx_fill(bx + 15, by + 14, bx + 18, by + 17, 1);
        }
        break;
    case K_TRASH:                                        /* can / Recycle Bin */
        if (shape) { gfx_fill(bx + 13, by + 1, bx + 19, by + 3, 1); gfx_fill(bx + 5, by + 3, bx + 27, by + 6, 1);
                     gfx_fill(bx + 7, by + 6, bx + 25, by + 27, 1); break; }
        gfx_fill(bx + 7, by + 6, bx + 25, by + 27, 0); gfx_frame(bx + 7, by + 6, bx + 25, by + 27);
        gfx_fill(bx + 5, by + 3, bx + 27, by + 6, 0); gfx_frame(bx + 5, by + 3, bx + 27, by + 6);
        gfx_fill(bx + 13, by + 1, bx + 19, by + 3, 0); gfx_frame(bx + 13, by + 1, bx + 19, by + 3);
        if (cur_theme == PLUSOS_THEME_WIN95 || cur_theme == PLUSOS_THEME_WIN98 || cur_theme == PLUSOS_THEME_AERO) {
            /* Recycle Bin arrows */
            gfx_dither_rect(bx + 8, by + 7, bx + 24, by + 26, 3);
            gfx_frame(bx + 12, by + 11, bx + 20, by + 21);
        } else {
            for (int x = bx + 11; x < bx + 23; x += 4) gfx_vline(x, by + 9, by + 24, 1);
        }
        break;
    case K_FILE:
        if (ic->program) {
            if (shape) { gfx_fill(bx + 4, by + 2, bx + 28, by + 26, 1); break; }
            gfx_fill(bx + 4, by + 2, bx + 28, by + 26, 0);
            gfx_frame(bx + 4, by + 2, bx + 28, by + 26);
            gfx_frame(bx + 6, by + 4, bx + 26, by + 24);
            gfx_fill(bx + 6, by + 4, bx + 26, by + 8, 1);
            gfx_text(bx + 8, by + 12, "PX", 0);
        } else {
            if (shape) { gfx_fill(bx + 7, by + 1, bx + 25, by + 27, 1); break; }
            gfx_fill(bx + 7, by + 1, bx + 25, by + 27, 0);
            gfx_frame(bx + 7, by + 1, bx + 25, by + 27);
            gfx_fill(bx + 20, by + 1, bx + 25, by + 6, 0);
            gfx_line(bx + 20, by + 1, bx + 24, by + 5, 1);
            gfx_hline(bx + 20, bx + 25, by + 6, 1); gfx_vline(bx + 20, by + 1, by + 7, 1);
            for (int y = by + 10; y < by + 24; y += 3) gfx_hline(bx + 10, bx + 22, y, 1);
        }
        break;
    case K_FILES:                                        /* hard disk / My Computer */
        if (cur_theme == PLUSOS_THEME_WIN95 || cur_theme == PLUSOS_THEME_WIN98 || cur_theme == PLUSOS_THEME_AERO) {
            if (shape) { gfx_fill(bx + 4, by + 2, bx + 28, by + 26, 1); break; }
            gfx_fill(bx + 5, by + 2, bx + 27, by + 19, 0); gfx_frame(bx + 5, by + 2, bx + 27, by + 19);
            gfx_frame(bx + 7, by + 4, bx + 25, by + 16);
            gfx_fill(bx + 3, by + 20, bx + 29, by + 26, 0); gfx_frame(bx + 3, by + 20, bx + 29, by + 26);
            gfx_hline(bx + 18, bx + 26, by + 23, 1);
            break;
        }
        if (shape) { gfx_fill(bx + 2, by + 6, bx + 30, by + 24, 1); break; }
        gfx_fill(bx + 2, by + 6, bx + 30, by + 24, 0);
        gfx_frame(bx + 2, by + 6, bx + 30, by + 24);
        gfx_fill(bx + 20, by + 10, bx + 26, by + 13, 1);     /* activity light */
        gfx_hline(bx + 5, bx + 16, by + 20, 1);
        break;
    default: {
        if (shape) { gfx_fill(bx, by, bx + 32, by + 28, 1); break; }
        const char *g = ic->kind == K_SHELL ? ">_" : ic->kind == K_HEX ? "0F"
                      : ic->kind == K_TASKS ? "Tk" : "[]";
        gfx_fill(bx, by, bx + 32, by + 28, 0);
        gfx_frame(bx, by, bx + 32, by + 28);
        gfx_fill(bx + 1, by + 1, bx + 31, by + 6, 1);
        gfx_text(bx + 16 - gfx_text_width(g) / 2, by + 12, g, 0);
        break;
    }
    }
}

static void render_icon(Icon *ic, int sel)
{
    if (!ic->img.bits && (gs_init(&ic->img, ICON_W, ICON_H) || gs_init(&ic->mask, ICON_W, ICON_H))) return;
    char lab[12]; scopy(lab, ic->label, 9);            /* 8 characters fit under an icon */
    int lw = gfx_text_width(lab), lx = ICON_W / 2 - lw / 2;
    gfx_target(&ic->img);
    gfx_fill(0, 0, ICON_W, ICON_H, 0);
    if (sel) {                     /* selected: inverted, with a white rim so it shows on any wallpaper */
        gfx_fill(15, 1, 49, 31, 0);
        gfx_fill(16, 2, 48, 30, 0);
        icon_art(ic, 16, 2, 0);
        gfx_invert(16, 2, 48, 30);
        gfx_fill(lx - 3, 33, lx + lw + 3, 45, 0);
    } else {
        icon_art(ic, 16, 2, 0);
    }
    gfx_fill(lx - 2, 34, lx + lw + 2, 44, sel);
    gfx_text(lx, 35, lab, sel);
    gfx_target(&ic->mask);
    gfx_fill(0, 0, ICON_W, ICON_H, 0);
    if (sel) { gfx_fill(15, 1, 49, 31, 1); gfx_fill(lx - 3, 33, lx + lw + 3, 45, 1); }
    else { icon_art(ic, 16, 2, 1); gfx_fill(lx - 2, 34, lx + lw + 2, 44, 1); }
    gfx_target(BACK);
    ic->rendered = sel;
}

/* ======================================================================== */
/* windows (each cached in its own surface)                                 */
/* ======================================================================== */
enum { WK_ABOUT, WK_INFO, WK_SETTINGS, WK_PROGRAMS, WK_GAMES, WK_APP };
typedef struct Window Window;
struct Window {
    int  x, y, w, h, kind, used;
    char title[32];
    void (*draw)(Window *w, int l, int t, int r, int b);   /* window-local coordinates */
    void (*click)(Window *w, int cx, int cy, int dbl);
    int  live;
    GSurface surf;
    int  dirty, drawn_active;
    /* WK_APP: a window owned by another program (kernel window server) */
    int  app_id, app_term, app_dead;
    unsigned long app_seq;
    uint8_t *app_bits; int app_rb, app_w, app_h;
};
#define APP_PADX 8                          /* program content sits inside the frame here */
#define APP_TOP  (TITLEBAR_H + 2)
#define APP_PADB 6
static Window  wins[MAX_WIN];
static Window *z[MAX_WIN];
static int     nwin;

static Window *front(void) { return nwin ? z[nwin - 1] : 0; }
static Window *find_kind(int kind) { for (int i = 0; i < nwin; i++) if (z[i]->kind == kind) return z[i]; return 0; }

static void bring_front(Window *w)
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

static void send_back(Window *w)
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

static void close_window(Window *w)
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

/* Draw a 3D Win95/Win98 caption button at (l, t, r, b) */
static void draw_win_btn(int l, int t, int r, int b, int kind)
{
    gfx_fill(l, t, r, b, 0);
    gfx_hline(l, r - 1, t, 0);
    gfx_vline(l, t, b - 1, 0);
    gfx_hline(l, r, b - 1, 1);
    gfx_vline(r - 1, t, b, 1);
    gfx_frame(l, t, r, b);
    int cx = (l + r) / 2, cy = (t + b) / 2;
    if (kind == 0) { /* minimize [_] */
        gfx_hline(cx - 2, cx + 3, b - 3, 1);
        gfx_hline(cx - 2, cx + 3, b - 4, 1);
    } else if (kind == 1) { /* maximize [□] */
        gfx_frame(cx - 3, cy - 3, cx + 3, cy + 3);
        gfx_hline(cx - 3, cx + 3, cy - 2, 1);
    } else { /* close [X] */
        gfx_line(cx - 2, cy - 2, cx + 2, cy + 2, 1);
        gfx_line(cx - 2, cy + 2, cx + 2, cy - 2, 1);
    }
}

static void render_window(Window *w, int active)
{
    if (!w->surf.bits && gs_init(&w->surf, w->w, w->h)) return;
    gfx_target(&w->surf);
    gfx_noclip();
    gfx_fill(0, 0, w->w, w->h, 0);
    int tb = TITLEBAR_H - 1;
    int tw = gfx_text_width(w->title);

    switch (cur_theme) {
    case PLUSOS_THEME_MAC:
    default: {
        gfx_frame(0, 0, w->w, w->h);
        gfx_hline(0, w->w, tb, 1);
        if (active) {
            for (int y = 3; y < tb - 2; y += 2) gfx_hline(2, w->w - 2, y, 1);
            gfx_fill(7, 4, 18, 14, 0);
            gfx_frame(7, 4, 18, 14);
            /* System 7 right zoom box */
            gfx_fill(w->w - 18, 4, w->w - 7, 14, 0);
            gfx_frame(w->w - 18, 4, w->w - 7, 14);
            gfx_frame(w->w - 18, 4, w->w - 11, 10);
        }
        int tx = (w->w - tw) / 2;
        if (tx < 24) tx = 24;
        gfx_fill(tx - 5, 2, tx + tw + 5, tb - 1, 0);
        gfx_text(tx, 5, w->title, 0);
        break;
    }
    case PLUSOS_THEME_AMIGA: {
        /* Amiga Workbench 1.3/2.0: double-bordered window, left [|] close gadget,
         * twin thick drag bars, right front/back depth gadgets, bottom-right sizing corner */
        gfx_frame(0, 0, w->w, w->h);
        gfx_frame(1, 1, w->w - 1, w->h - 1);
        gfx_hline(0, w->w, tb, 1);
        gfx_hline(0, w->w, tb - 1, 1);
        /* Close gadget [|] on left */
        gfx_vline(19, 1, tb, 1);
        gfx_frame(4, 3, 16, tb - 2);
        gfx_fill(9, 5, 11, tb - 4, 1);
        /* Title text left-aligned after close gadget */
        int tx = 23;
        gfx_text(tx, 5, w->title, 0);
        /* Twin Amiga drag bars between title and right depth gadgets */
        int drag_l = tx + tw + 6;
        int drag_r = w->w - 38;
        if (drag_l < drag_r) {
            if (active) {
                gfx_fill(drag_l, 4, drag_r, 8, 1);
                gfx_fill(drag_l, 10, drag_r, 14, 1);
            } else {
                gfx_dither_rect(drag_l, 5, drag_r, 8, 8);
                gfx_dither_rect(drag_l, 10, drag_r, 13, 8);
            }
        }
        /* Right depth gadgets (Front / Back overlapping boxes) */
        gfx_vline(w->w - 35, 1, tb, 1);
        gfx_vline(w->w - 18, 1, tb, 1);
        /* Back depth gadget */
        gfx_frame(w->w - 32, 3, w->w - 24, 10);
        gfx_fill(w->w - 29, 6, w->w - 20, 14, 1);
        gfx_fill(w->w - 28, 7, w->w - 21, 13, 0);
        /* Front/Close gadget on far right */
        gfx_fill(w->w - 15, 3, w->w - 7, 11, 1);
        gfx_frame(w->w - 12, 6, w->w - 4, 14);
        /* Amiga bottom-right sizing gadget corner */
        gfx_frame(w->w - 12, w->h - 10, w->w - 1, w->h - 1);
        break;
    }
    case PLUSOS_THEME_WIN95: {
        /* Windows 95: 3D bevel frame, solid dark active title bar, left system icon, right [_][□][X] */
        gfx_frame(0, 0, w->w, w->h);
        gfx_hline(1, w->w - 2, 1, 0);
        gfx_vline(1, 1, w->h - 2, 0);
        gfx_hline(1, w->w - 1, w->h - 2, 1);
        gfx_vline(w->w - 2, 1, w->h - 1, 1);
        gfx_frame(2, 2, w->w - 2, w->h - 2);
        if (active) gfx_fill(3, 3, w->w - 3, tb, 1);
        else        gfx_dither_rect(3, 3, w->w - 3, tb, 8);
        gfx_hline(2, w->w - 2, tb, 1);
        /* Left system menu icon [-] */
        gfx_fill(5, 5, 15, 14, 0);
        gfx_frame(5, 5, 15, 14);
        gfx_hline(7, 13, 9, 1);
        /* Title text */
        if (!active) gfx_fill(18, 4, mini(22 + tw, w->w - 48), tb - 1, 1);
        gfx_text(19, 5, w->title, 1);
        /* Right 3D caption buttons [_][□][X] */
        draw_win_btn(w->w - 42, 4, w->w - 32, 14, 0);
        draw_win_btn(w->w - 30, 4, w->w - 20, 14, 1);
        draw_win_btn(w->w - 17, 4, w->w - 5,  14, 2);
        break;
    }
    case PLUSOS_THEME_WIN98: {
        /* Windows 98: 3D frame + smooth 8-step Bayer dither gradient titlebar (shade 16 -> 4) */
        gfx_frame(0, 0, w->w, w->h);
        gfx_frame(2, 2, w->w - 2, w->h - 2);
        int gl = 3, gr = w->w - 3, gw = gr - gl;
        for (int s = 0; s < 8; s++) {
            int x0 = gl + (gw * s) / 8;
            int x1 = gl + (gw * (s + 1)) / 8;
            int shade = active ? (16 - (s * 12) / 7) : (10 - s);
            gfx_dither_rect(x0, 3, x1, tb, shade);
        }
        gfx_hline(2, w->w - 2, tb, 1);
        /* System icon */
        gfx_fill(5, 5, 15, 14, 0);
        gfx_frame(5, 5, 15, 14);
        gfx_frame(7, 7, 13, 12);
        /* Solid dark backdrop right under title text for crisp legibility on gradient */
        int tbox_r = mini(21 + tw, w->w - 46);
        if (tbox_r > 18) gfx_fill(17, 4, tbox_r, tb - 1, 1);
        gfx_text(19, 5, w->title, 1);
        draw_win_btn(w->w - 42, 4, w->w - 32, 14, 0);
        draw_win_btn(w->w - 30, 4, w->w - 20, 14, 1);
        draw_win_btn(w->w - 17, 4, w->w - 5,  14, 2);
        break;
    }
    case PLUSOS_THEME_AERO: {
        /* Windows Vista / 7 Aero Glass: rounded outer frame, frosted dithered glass titlebar
         * with diagonal specular reflection streaks, rounded title pill, and glass pill buttons */
        gfx_round_rect(0, 0, w->w, w->h, 6, 1);
        gfx_round_rect(1, 1, w->w - 1, w->h - 1, 5, 1);
        gfx_dither_rect(2, 2, w->w - 2, tb, active ? 6 : 3);
        /* Diagonal Aero glass reflection streaks */
        for (int gx = 16; gx < w->w - 40; gx += 44) {
            gfx_line(gx, 2, gx - 10, tb - 1, 0);
            gfx_line(gx + 1, 2, gx - 9, tb - 1, 0);
            gfx_line(gx + 5, 2, gx - 5, tb - 1, 0);
        }
        gfx_hline(2, w->w - 2, tb, 1);
        /* Left Aero orb/close icon */
        gfx_fill_circle(11, 9, 4, 0);
        gfx_circle(11, 9, 4, 1);
        /* Rounded white glass pill behind title */
        int tx = 20;
        gfx_fill_round_rect(tx - 3, 3, mini(tx + tw + 4, w->w - 48), tb - 1, 4, 0);
        gfx_text(tx, 5, w->title, 0);
        /* Right Aero glass pill button group [ - | □ | X ] */
        int pb_l = w->w - 44, pb_r = w->w - 5;
        gfx_fill(pb_l, 2, pb_r, 13, 0);
        gfx_round_rect(pb_l, 2, pb_r, 13, 3, 1);
        gfx_vline(pb_l + 12, 2, 13, 1);
        gfx_vline(pb_l + 24, 2, 13, 1);
        gfx_hline(pb_l + 4, pb_l + 9, 9, 1);
        gfx_frame(pb_l + 15, 5, pb_l + 21, 10);
        if (active) gfx_dither_rect(pb_l + 25, 3, pb_r - 1, 12, 4);
        gfx_line(pb_l + 29, 5, pb_l + 34, 10, 1);
        gfx_line(pb_l + 29, 10, pb_l + 34, 5, 1);
        break;
    }
    case PLUSOS_THEME_HACKER: {
        /* Hacker Mode: cyberpunk bracketed frame, scanline header, [X] kill switch,
         * and inverted dark-mode window interior */
        gfx_fill(0, 0, w->w, w->h, 0);
        gfx_frame(0, 0, w->w, w->h);
        gfx_frame(2, 2, w->w - 2, w->h - 2);
        /* Tech corner bracket ticks */
        gfx_fill(0, 0, 6, 3, 1); gfx_fill(0, 0, 3, 6, 1);
        gfx_fill(w->w - 6, 0, w->w, 3, 1); gfx_fill(w->w - 3, 0, w->w, 6, 1);
        gfx_fill(0, w->h - 3, 6, w->h, 1); gfx_fill(0, w->h - 6, 3, w->h, 1);
        gfx_fill(w->w - 6, w->h - 3, w->w, w->h, 1); gfx_fill(w->w - 3, w->h - 6, w->w, w->h, 1);
        /* Header scanlines */
        gfx_fill(2, 2, w->w - 2, tb, 1);
        if (!active) {
            for (int y = 3; y < tb; y += 2) gfx_hline(3, w->w - 3, y, 0);
        }
        gfx_hline(2, w->w - 2, tb, 0);
        gfx_hline(2, w->w - 2, tb + 1, 1);
        /* Left & right [X] kill switches */
        gfx_text(5, 5, "[X]", 1);
        char htitle[40];
        snprintf(htitle, sizeof htitle, ":: %s ::", w->title);
        int htw = gfx_text_width(htitle);
        int htx = (w->w - htw) / 2;
        if (htx < 30) htx = 30;
        gfx_fill(htx - 2, 4, htx + htw + 2, tb - 1, 1);
        gfx_text(htx, 5, htitle, 1);
        gfx_text(w->w - 22, 5, "[X]", 1);
        break;
    }
    }

    gfx_clip(1, TITLEBAR_H, w->w - 1, w->h - 1);
    if (w->draw) {
        w->draw(w, 1, TITLEBAR_H, w->w - 1, w->h - 1);
        /* In Hacker Mode, invert built-in window client area for true CRT dark mode! */
        if (cur_theme == PLUSOS_THEME_HACKER) {
            gfx_invert(2, TITLEBAR_H + 1, w->w - 2, w->h - 2);
        }
    }
    gfx_target(BACK);
    w->dirty = 0;
    w->drawn_active = active;
}

static Window *new_window(int kind, const char *title, int x, int y, int w, int h)
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

/* ======================================================================== */
/* menus & bottom taskbar                                                   */
/* ======================================================================== */
typedef struct { const char *title; const char *items[8]; int n; } Menu;
static const Menu menus[] = {
    { "PlusOS",  { "About PlusOS", "System Info", "Calculator", "Note Pad", "Puzzle", "Task Manager", "Settings..." }, 7 },
    { "File",    { "Open", "Open in Hex Editor", "Programs", "-", "Quit Desktop" }, 5 },
    { "Edit",    { "Copy", "Paste", "Duplicate", "Rename...", "-", "Delete" }, 6 },
    { "View",    { "Clean Up Icons", "-", "Theme Wallpaper", "Matrix Wallpaper" }, 4 },
    { "Theme",   { "Mac OS 7.1", "Commodore Amiga", "Windows 95", "Windows 98", "Windows Vista/7", "Hacker Mode" }, 6 },
    { "Special", { "Start System 7.5.5", "Start System 7.1", "Start System 6.0.8", "-",
                   "Restart", "Shut Down" }, 6 },
};
#define NMENUS 6
static int menu_x[NMENUS], menu_w[NMENUS];
static int  matrix_on;
static char clip_path[64], clip_name[32];
static char bench_str[32];
static uint32_t bench_until;
static int mb_minute = -1, mb_dirty = 1;

static void layout_menus(void)
{
    int x = 10;
    for (int i = 0; i < NMENUS; i++) { menu_w[i] = gfx_text_width(menus[i].title) + 14; menu_x[i] = x; x += menu_w[i]; }
}

static void render_menubar(void)
{
    if (!mb_s.bits && gs_init(&mb_s, SCREEN_W, MENUBAR_H)) return;
    gfx_target(&mb_s);
    gfx_noclip();

    int inv_bar = (cur_theme == PLUSOS_THEME_AMIGA || cur_theme == PLUSOS_THEME_HACKER);
    gfx_fill(0, 0, SCREEN_W, MENUBAR_H, inv_bar ? 1 : 0);
    if (cur_theme == PLUSOS_THEME_AERO) {
        gfx_dither_rect(0, 0, SCREEN_W, MENUBAR_H - 1, 3);
        gfx_hline(0, SCREEN_W, 1, 0);
    }
    gfx_hline(0, SCREEN_W, MENUBAR_H - 1, 1);
    if (cur_theme == PLUSOS_THEME_AMIGA || cur_theme == PLUSOS_THEME_HACKER)
        gfx_hline(0, SCREEN_W, MENUBAR_H - 2, 0);

    /* Classic Mac rounded top screen corners */
    if (cur_theme == PLUSOS_THEME_MAC) {
        gfx_fill(0, 0, 5, 1, 1); gfx_fill(0, 1, 3, 2, 1); gfx_fill(0, 2, 2, 3, 1); gfx_fill(0, 3, 1, 5, 1);
        gfx_fill(SCREEN_W - 5, 0, SCREEN_W, 1, 1); gfx_fill(SCREEN_W - 3, 1, SCREEN_W, 2, 1);
        gfx_fill(SCREEN_W - 2, 2, SCREEN_W, 3, 1); gfx_fill(SCREEN_W - 1, 3, SCREEN_W, 5, 1);
    }

    for (int i = 0; i < NMENUS; i++) {
        if (cur_theme == PLUSOS_THEME_AERO)
            gfx_fill_round_rect(menu_x[i] + 3, 3, menu_x[i] + menu_w[i] - 3, MENUBAR_H - 3, 3, 0);
        gfx_text(menu_x[i] + 7, 6, menus[i].title, inv_bar);
    }

    /* Theme badge in menubar */
    if (bench_str[0]) {
        gfx_text(310, 6, bench_str, inv_bar);
    } else if (cur_theme == PLUSOS_THEME_AMIGA) {
        gfx_text(318, 6, "Workbench", 1);
    } else if (cur_theme == PLUSOS_THEME_HACKER) {
        gfx_text(318, 6, "[ROOT@PLUSOS]", 1);
    }

    ULONGINT secs; ReadDateTime(&secs);
    DateTimeRec d; SecondsToDate(secs, &d);
    char s[8];
    int h = d.hour % 12;
    snprintf(s, sizeof s, "%2d:%02d%c", h ? h : 12, d.minute, d.hour < 12 ? 'a' : 'p');
    int clk_x = SCREEN_W - gfx_text_width(s) - (cur_theme == PLUSOS_THEME_AMIGA ? 28 : 10);
    if (cur_theme == PLUSOS_THEME_AERO)
        gfx_fill(clk_x - 3, 3, clk_x + gfx_text_width(s) + 3, MENUBAR_H - 3, 0);
    gfx_text(clk_x, 6, s, inv_bar);

    /* Amiga Workbench screen depth gadget on far right of menu bar */
    if (cur_theme == PLUSOS_THEME_AMIGA) {
        gfx_vline(SCREEN_W - 22, 0, MENUBAR_H - 1, 0);
        gfx_frame(SCREEN_W - 18, 3, SCREEN_W - 10, 11);
        gfx_fill(SCREEN_W - 14, 7, SCREEN_W - 5, 15, 0);
    }

    mb_minute = d.minute;
    gfx_target(BACK);
    mb_dirty = 0;
}

static void render_taskbar(void)
{
    if (!tb_s.bits && gs_init(&tb_s, SCREEN_W, TASKBAR_H)) return;
    gfx_target(&tb_s);
    gfx_noclip();

    int start_r = 56;
    if (cur_theme == PLUSOS_THEME_HACKER) {
        /* Cyberpunk HUD status / exec bar */
        gfx_fill(0, 0, SCREEN_W, TASKBAR_H, 1);
        gfx_hline(0, SCREEN_W, 0, 0);
        gfx_hline(0, SCREEN_W, 1, 1);
        gfx_hline(0, SCREEN_W, 2, 0);
        gfx_frame(3, 4, 58, TASKBAR_H - 2);
        gfx_text(7, 8, "> EXEC", 1);
        start_r = 62;
    } else if (cur_theme == PLUSOS_THEME_AERO) {
        /* Windows Vista/7 Aero Superbar with circular Start Orb */
        gfx_fill(0, 0, SCREEN_W, TASKBAR_H, 0);
        gfx_dither_rect(0, 2, SCREEN_W, TASKBAR_H, 5);
        gfx_hline(0, SCREEN_W, 0, 1);
        gfx_hline(0, SCREEN_W, 1, 0);
        /* Round Aero Pearl Start Orb */
        gfx_fill_circle(15, 11, 9, 1);
        gfx_circle(15, 11, 10, 1);
        gfx_fill_circle(15, 11, 7, 0);
        /* 4-pane Windows flag inside Orb */
        gfx_fill(11, 7, 15, 11, 1);  gfx_fill(16, 7, 20, 11, 1);
        gfx_fill(11, 12, 15, 16, 1); gfx_fill(16, 12, 20, 16, 1);
        start_r = 32;
    } else {
        /* Windows 95 / 98 3D Taskbar with [Start] button */
        gfx_fill(0, 0, SCREEN_W, TASKBAR_H, 0);
        gfx_hline(0, SCREEN_W, 0, 1);
        gfx_hline(0, SCREEN_W, 1, 0);
        /* 3D raised Start button */
        gfx_frame(2, 3, 56, TASKBAR_H - 2);
        gfx_hline(2, 55, TASKBAR_H - 3, 1);
        gfx_vline(55, 3, TASKBAR_H - 2, 1);
        /* Mini 4-pane Windows logo */
        gfx_fill(6, 7, 10, 11, 1);  gfx_fill(11, 7, 15, 11, 1);
        gfx_fill(6, 12, 10, 16, 1); gfx_fill(11, 12, 15, 16, 1);
        gfx_text(18, 8, "Start", 0);
        start_r = 60;
        if (cur_theme == PLUSOS_THEME_WIN98) {
            /* Win98 Quick Launch divider + icons */
            gfx_vline(60, 4, TASKBAR_H - 3, 1);
            gfx_vline(62, 5, TASKBAR_H - 4, 1);
            gfx_text(66, 8, ">_", 0);
            gfx_vline(86, 4, TASKBAR_H - 3, 1);
            start_r = 90;
        }
    }

    /* Sunken clock tray on the right */
    ULONGINT secs; ReadDateTime(&secs);
    DateTimeRec d; SecondsToDate(secs, &d);
    char clk[10];
    int h = d.hour % 12;
    snprintf(clk, sizeof clk, "%2d:%02d%c", h ? h : 12, d.minute, d.hour < 12 ? 'a' : 'p');
    int tray_w = gfx_text_width(clk) + 14;
    int tray_l = SCREEN_W - tray_w - 3;
    if (cur_theme == PLUSOS_THEME_HACKER) {
        gfx_frame(tray_l, 4, SCREEN_W - 3, TASKBAR_H - 2);
        gfx_text(tray_l + 6, 8, clk, 1);
    } else {
        gfx_fill(tray_l, 4, SCREEN_W - 3, TASKBAR_H - 2, 0);
        gfx_frame(tray_l, 4, SCREEN_W - 3, TASKBAR_H - 2);
        gfx_text(tray_l + 6, 8, clk, 0);
    }

    /* Taskbar buttons for open windows */
    if (nwin > 0) {
        int avail = tray_l - start_r - 4;
        int bw = avail / nwin;
        if (bw > 96) bw = 96;
        if (bw >= 28) {
            for (int i = 0; i < nwin; i++) {
                Window *w = z[i];
                int act = (i == nwin - 1);
                int bl = start_r + i * bw;
                int br = bl + bw - 3;
                char bt[12];
                int maxc = (bw - 10) / 6;
                if (maxc < 2) maxc = 2;
                if (maxc > 10) maxc = 10;
                scopy(bt, w->title, maxc + 1);
                if (cur_theme == PLUSOS_THEME_HACKER) {
                    if (act) {
                        gfx_fill(bl, 4, br, TASKBAR_H - 2, 0);
                        gfx_text(bl + 4, 8, bt, 0);
                    } else {
                        gfx_frame(bl, 4, br, TASKBAR_H - 2);
                        gfx_text(bl + 4, 8, bt, 1);
                    }
                } else if (cur_theme == PLUSOS_THEME_AERO) {
                    gfx_fill(bl, 3, br, TASKBAR_H - 2, act ? 1 : 0);
                    gfx_round_rect(bl, 3, br, TASKBAR_H - 2, 3, 1);
                    gfx_text(bl + 5, 8, bt, act ? 1 : 0);
                } else {
                    if (act) {
                        gfx_dither_rect(bl, 3, br, TASKBAR_H - 2, 3);
                        gfx_frame(bl, 3, br, TASKBAR_H - 2);
                        gfx_hline(bl + 1, br - 1, 4, 1);
                        gfx_vline(bl + 1, 4, TASKBAR_H - 3, 1);
                    } else {
                        gfx_fill(bl, 3, br, TASKBAR_H - 2, 0);
                        gfx_frame(bl, 3, br, TASKBAR_H - 2);
                        gfx_hline(bl, br - 1, TASKBAR_H - 3, 1);
                        gfx_vline(br - 1, 3, TASKBAR_H - 2, 1);
                    }
                    gfx_text(bl + 5, 8, bt, 0);
                }
            }
        }
    }

    gfx_target(BACK);
    tb_dirty = 0;
}

/* ======================================================================== */
/* compositor                                                               */
/* ======================================================================== */
static void draw_wallpaper_rect(int l, int t, int r, int b)
{
    static const uint8_t pat_amiga[8]  = { 0x88, 0x22, 0x88, 0x22, 0x88, 0x22, 0x88, 0x22 };
    static const uint8_t pat_win95[8]  = { 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55 };
    static const uint8_t pat_win98[8]  = { 0xCC, 0x66, 0x33, 0x99, 0xCC, 0x66, 0x33, 0x99 };
    static const uint8_t pat_aero[8]   = { 0x82, 0x44, 0x28, 0x10, 0x28, 0x44, 0x82, 0x01 };
    static const uint8_t pat_hacker[8] = { 0xFF, 0xDF, 0xFF, 0xF7, 0xFF, 0xDF, 0xFF, 0x7F };

    if (matrix_on && wall_s.bits) {
        gfx_blit(&wall_s, l, t, r - l, b - t, l, t, GFX_COPY);
        return;
    }
    switch (cur_theme) {
    case PLUSOS_THEME_MAC:
    default:            gfx_gray(l, t, r, b); break;
    case PLUSOS_THEME_AMIGA:  gfx_pattern8(l, t, r, b, pat_amiga);  break;
    case PLUSOS_THEME_WIN95:  gfx_pattern8(l, t, r, b, pat_win95);  break;
    case PLUSOS_THEME_WIN98:  gfx_pattern8(l, t, r, b, pat_win98);  break;
    case PLUSOS_THEME_AERO:   gfx_pattern8(l, t, r, b, pat_aero);   break;
    case PLUSOS_THEME_HACKER: gfx_pattern8(l, t, r, b, pat_hacker); break;
    }
}

static void compose(int l, int t, int r, int b)
{
    l &= ~7; r = (r + 7) & ~7;
    if (l < 0) l = 0;
    if (t < 0) t = 0;
    if (r > SCREEN_W) r = SCREEN_W;
    if (b > SCREEN_H) b = SCREEN_H;
    if (l >= r || t >= b) return;
    LOCK();
    gfx_target(BACK);
    gfx_clip(l, t, r, b);
    int bt = t < MENUBAR_H ? MENUBAR_H : t;
    int bb = b > desk_bottom() ? desk_bottom() : b;
    if (bt < bb) {
        draw_wallpaper_rect(l, bt, r, bb);
    }
    for (int i = 0; i < nicons; i++) {
        Icon *ic = &icons[i];
        if (!overlap(l, t, r, b, ic->x, ic->y, ic->x + ICON_W, ic->y + ICON_H)) continue;
        int sel = i == icon_sel;
        if (ic->rendered != sel) render_icon(ic, sel);
        gfx_clip(l, t, r, b);
        gfx_blit_masked(&ic->img, &ic->mask, 0, 0, ICON_W, ICON_H, ic->x, ic->y);
    }
    for (int i = 0; i < nwin; i++) {
        Window *w = z[i];
        if (!overlap(l, t, r, b, w->x, w->y, w->x + w->w + 2, w->y + w->h + 2)) continue;
        int active = i == nwin - 1;
        if (w->dirty || w->drawn_active != active || !w->surf.bits) render_window(w, active);
        gfx_clip(l, t, r, b);
        gfx_fill(w->x + 2, w->y + w->h, w->x + w->w + 2, w->y + w->h + 2, 1);   /* shadow */
        gfx_fill(w->x + w->w, w->y + 2, w->x + w->w + 2, w->y + w->h, 1);
        gfx_blit(&w->surf, 0, 0, w->w, w->h, w->x, w->y, GFX_COPY);
        if (w->kind == WK_APP && w->app_bits) {      /* the program's own drawing */
            GSurface s;
            gs_wrap(&s, w->app_bits, w->app_rb, w->app_w, w->app_h);
            gfx_clip(l, t, r, b);
            gfx_blit(&s, 0, 0, w->app_w, w->app_h, w->x + APP_PADX, w->y + APP_TOP, GFX_COPY);
            /* In Hacker Mode, terminal windows get dark-mode inverted interiors */
            if (cur_theme == PLUSOS_THEME_HACKER && w->app_term) {
                int il = maxi(l, w->x + APP_PADX), it = maxi(t, w->y + APP_TOP);
                int ir = mini(r, w->x + APP_PADX + w->app_w), ib = mini(b, w->y + APP_TOP + w->app_h);
                if (il < ir && it < ib) gfx_invert(il, it, ir, ib);
            }
        }
    }
    if (theme_has_taskbar() && b > SCREEN_H - TASKBAR_H) {
        if (tb_dirty) render_taskbar();
        gfx_clip(l, t, r, b);
        gfx_blit(&tb_s, 0, 0, SCREEN_W, TASKBAR_H, 0, SCREEN_H - TASKBAR_H, GFX_COPY);
    }
    if (t < MENUBAR_H) {
        if (mb_dirty) render_menubar();
        gfx_clip(l, t, r, b);
        gfx_blit(&mb_s, 0, 0, SCREEN_W, MENUBAR_H, 0, 0, GFX_COPY);
    }
    gfx_noclip();
    cover_dirty = 1;
    UNLOCK();
}

static void show(int t, int b)
{
    if (t < 0) t = 0;
    LOCK();
    int my = mouse_y();
    int hide = my + 17 >= t && my - 2 < b;
    if (hide) HideCursor();
    gfx_present_rows(t, b);
    if (hide) ShowCursor();
    UNLOCK();
}

static void refresh(int l, int t, int r, int b) { LOCK(); compose(l, t, r, b); show(t, b); UNLOCK(); }
static void redraw(void) { refresh(0, 0, SCREEN_W, SCREEN_H); }
static void refresh_icon(int i) { if (i >= 0 && i < nicons) refresh(icons[i].x, icons[i].y, icons[i].x + ICON_W, icons[i].y + ICON_H); }
static void refresh_win(Window *w) { w->dirty = 1; refresh(w->x, w->y, w->x + w->w + 2, w->y + w->h + 2); }

/* ---- software cursor (used while something moves under the mouse) ---- */
static GSurface cur_data, cur_mask;
static uint8_t  cur_save[16][3];
static int      cur_x = -100, cur_y = -100;

static void swcursor_init(void)
{
    QDGlobals *q = (QDGlobals *)plusos->qd;
    gs_wrap(&cur_data, q->arrow.data, 2, 16, 16);
    gs_wrap(&cur_mask, q->arrow.mask, 2, 16, 16);
}

/* Present rows [t,b) with the arrow drawn at the mouse, keeping BACK clean. */
static void present_with_cursor(int t, int b)
{
    QDGlobals *q = (QDGlobals *)plusos->qd;
    LOCK();
    int oy = cur_y;
    cur_x = mouse_x() - q->arrow.hotSpot.h;
    cur_y = mouse_y() - q->arrow.hotSpot.v;
    int x0 = cur_x >> 3;
    for (int r = 0; r < 16; r++) {                  /* save what the arrow covers */
        int y = cur_y + r;
        for (int c = 0; c < 3; c++)
            cur_save[r][c] = (y >= 0 && y < SCREEN_H && x0 + c >= 0 && x0 + c < ROWB)
                             ? BACK->bits[y * ROWB + x0 + c] : 0;
    }
    gfx_target(BACK); gfx_noclip();
    gfx_blit_masked(&cur_data, &cur_mask, 0, 0, 16, 16, cur_x, cur_y);
    int pt = mini(t, mini(oy, cur_y)), pb = maxi(b, maxi(oy, cur_y) + 16);
    if (oy == -100) { pt = mini(t, cur_y); pb = maxi(b, cur_y + 16); }
    gfx_present_rows(pt < 0 ? 0 : pt, pb > SCREEN_H ? SCREEN_H : pb);
    for (int r = 0; r < 16; r++) {                  /* and put it back */
        int y = cur_y + r;
        for (int c = 0; c < 3; c++)
            if (y >= 0 && y < SCREEN_H && x0 + c >= 0 && x0 + c < ROWB) BACK->bits[y * ROWB + x0 + c] = cur_save[r][c];
    }
    UNLOCK();
}

/* ======================================================================== */
/* Matrix wallpaper                                                         */
/* ======================================================================== */
#define MCOLS 64
#define MROWS 40
static int     m_head[MCOLS], m_len[MCOLS];
static uint8_t m_rate[MCOLS], m_cnt[MCOLS];
static uint8_t m_cover[MROWS][MCOLS];
static uint8_t m_pend[MROWS][MCOLS];
static uint32_t rnd_state = 0x1234567;
static const char m_chars[] = "0123456789ABCDEFXZ$#@%&*+=<>?:;!";

static unsigned rnd(void) { rnd_state = rnd_state * 1103515245u + 12345u; return (rnd_state >> 16) & 0x7FFF; }

static void m_reset(int c, int fresh)
{
    m_head[c] = fresh ? -(int)(rnd() % MROWS) : -(int)(rnd() % 24);
    m_len[c] = 6 + (int)(rnd() % 20);
    m_rate[c] = (uint8_t)(1 + rnd() % 3);
    m_cnt[c] = 0;
}

static void m_init(void)
{
    if (!wall_s.bits && gs_init(&wall_s, SCREEN_W, SCREEN_H)) { matrix_on = 0; return; }
    memset(wall_s.bits, 0xFF, FB_BYTES);
    memset(m_pend, 0, sizeof m_pend);
    rnd_state ^= now();
    for (int c = 0; c < MCOLS; c++) m_reset(c, 1);
}

static void m_cover_update(void)
{
    memset(m_cover, 0, sizeof m_cover);
    for (int i = 0; i < nicons; i++) {
        int c0 = icons[i].x >> 3, c1 = (icons[i].x + ICON_W - 1) >> 3;
        int r0 = (icons[i].y - MENUBAR_H) >> 3, r1 = (icons[i].y + ICON_H - 1 - MENUBAR_H) >> 3;
        for (int r = maxi(r0, 0); r <= r1 && r < MROWS; r++)
            for (int c = maxi(c0, 0); c <= c1 && c < MCOLS; c++) m_cover[r][c] = 1;
    }
    for (int i = 0; i < nwin; i++) {
        Window *w = z[i];
        int c0 = w->x >> 3, c1 = (w->x + w->w + 1) >> 3;
        int r0 = (w->y - MENUBAR_H) >> 3, r1 = (w->y + w->h + 1 - MENUBAR_H) >> 3;
        for (int r = maxi(r0, 0); r <= r1 && r < MROWS; r++)
            for (int c = maxi(c0, 0); c <= c1 && c < MCOLS; c++) m_cover[r][c] = 1;
    }
    if (theme_has_taskbar()) {
        int r0 = (SCREEN_H - TASKBAR_H - MENUBAR_H) >> 3;
        for (int r = maxi(r0, 0); r < MROWS; r++)
            for (int c = 0; c < MCOLS; c++) m_cover[r][c] = 1;
    }
    if (ov_on) {                                     /* an open menu or dialog */
        int c0 = ov_l >> 3, c1 = (ov_r - 1) >> 3;
        int r0 = (ov_t - MENUBAR_H) >> 3, r1 = (ov_b - 1 - MENUBAR_H) >> 3;
        for (int r = maxi(r0, 0); r <= r1 && r < MROWS; r++)
            for (int c = maxi(c0, 0); c <= c1 && c < MCOLS; c++) m_cover[r][c] = 1;
    }
    cover_dirty = 0;
}

static void m_cell(int c, int r, int style)
{
    uint8_t *p = wall_s.bits + (MENUBAR_H + r * 8) * ROWB + c;
    if (!style) { for (int i = 0; i < 8; i++, p += ROWB) *p = 0xFF; return; }
    int ch = m_chars[rnd() % (sizeof m_chars - 1)];
    const uint8_t *g = plusos->font8x8 + (ch - 0x20) * 8;
    for (int i = 0; i < 8; i++, p += ROWB) *p = style == 2 ? g[i] : (uint8_t)~g[i];
}

static int m_mx0, m_mx1, m_my0, m_my1;

static void m_push(int c, int r)
{
    if (m_cover[r][c]) return;
    if (c >= m_mx0 && c <= m_mx1 && r >= m_my0 && r <= m_my1) { m_pend[r][c] = 1; return; }
    int off = (MENUBAR_H + r * 8) * ROWB + c;
    for (int i = 0; i < 8; i++, off += ROWB) BACK->bits[off] = FRONT->bits[off] = wall_s.bits[off];
    m_pend[r][c] = 0;
}

/* One animation step. The lock is taken per column (not for the whole frame),
 * so a drag or a window update can slip in between columns and never stalls. */
static void m_step(void)
{
    LOCK();
    if (cover_dirty) m_cover_update();
    int mx = mouse_x(), my = mouse_y();
    m_mx0 = (mx - 6) >> 3; m_mx1 = (mx + 20) >> 3;
    m_my0 = (my - 6 - MENUBAR_H) >> 3; m_my1 = (my + 20 - MENUBAR_H) >> 3;
    for (int r = 0; r < MROWS; r++)
        for (int c = 0; c < MCOLS; c++)
            if (m_pend[r][c]) m_push(c, r);
    UNLOCK();
    for (int c = 0; c < MCOLS; c++) {
        if (++m_cnt[c] < m_rate[c]) continue;
        m_cnt[c] = 0;
        LOCK();
        int h = m_head[c];
        if (h >= 0 && h < MROWS) { m_cell(c, h, 1); m_push(c, h); }
        h = ++m_head[c];
        if (h >= 0 && h < MROWS) { m_cell(c, h, 2); m_push(c, h); }
        int tail = h - m_len[c];
        if (tail >= 0 && tail < MROWS) { m_cell(c, tail, 0); m_push(c, tail); }
        if (h > 2 && (rnd() & 7) == 0) {
            int r = h - 1 - (int)(rnd() % (m_len[c] > 1 ? m_len[c] - 1 : 1));
            if (r >= 0 && r < MROWS && r > tail) { m_cell(c, r, 1); m_push(c, r); }
        }
        if (tail >= MROWS) m_reset(c, 0);
        UNLOCK();
    }
}

static void set_wallpaper(int matrix)
{
    render_suspend();                        /* renderer must not touch wall_s while we change it */
    LOCK();
    if (matrix) { m_init(); matrix_on = wall_s.bits != 0; }
    else { matrix_on = 0; gs_free(&wall_s); }   /* free the 21K buffer when not in use */
    UNLOCK();
    render_resume();
    rset("desktop.wallpaper", matrix_on ? "matrix" : "gray");
    redraw();
}

static void set_theme(int t)
{
    if (t < 0 || t >= PLUSOS_THEME_COUNT) t = PLUSOS_THEME_MAC;
    render_suspend();
    LOCK();
    int prev = cur_theme;
    cur_theme = t;
    plusos_theme_set(t);
    /* Automatically enable Matrix digital rain when entering Hacker Mode,
     * and return to the theme pattern when leaving Hacker Mode. */
    if (t == PLUSOS_THEME_HACKER && !matrix_on) {
        m_init();
        matrix_on = wall_s.bits != 0;
        rset("desktop.wallpaper", "matrix");
    } else if (prev == PLUSOS_THEME_HACKER && t != PLUSOS_THEME_HACKER && matrix_on) {
        matrix_on = 0;
        gs_free(&wall_s);
        rset("desktop.wallpaper", "gray");
    }
    for (int i = 0; i < nicons; i++) {
        icons[i].rendered = -1;
        if (icons[i].kind == K_TRASH)
            icons[i].y = desk_bottom() - ICON_H - 4;
    }
    for (int i = 0; i < MAX_WIN; i++) {
        if (wins[i].used) wins[i].dirty = 1;
    }
    mb_dirty = 1;
    tb_dirty = 1;
    cover_dirty = 1;
    UNLOCK();
    render_resume();
    redraw();
}

/* The renderer thread: steps the wallpaper animation ~30 times a second, all
 * the time - while windows and icons are dragged, menus are open or dialogs
 * wait - because it is scheduled on its own. */
static void renderer_thread(void *arg)
{
    (void)arg;
    for (;;) {
        if (render_pause_req) { render_parked = 1; gthread_sleep(2); continue; }
        render_parked = 0;
        if (matrix_on && wall_s.bits) m_step();      /* locks per column */
        gthread_sleep(2);
    }
}

/* ======================================================================== */
/* dialogs (drawn straight into the back buffer, repaired afterwards)       */
/* ======================================================================== */
#define DL 96
#define DR 416

static void button(int l, int t, int r, int b, const char *label, int dflt)
{
    gfx_fill(l, t, r, b, 0);
    gfx_frame(l, t, r, b);
    if (dflt) gfx_frame(l - 2, t - 2, r + 2, b + 2);
    gfx_text(l + (r - l - gfx_text_width(label)) / 2, t + (b - t - 8) / 2, label, 0);
}

static void dlg_frame(int t, int b)
{
    set_overlay(DL, t, DR + 4, b + 4);
    LOCK();
    gfx_target(BACK); gfx_noclip();
    gfx_fill(DL + 3, t + 3, DR + 3, b + 3, 1);
    gfx_fill(DL, t, DR, b, 0);
    gfx_frame(DL, t, DR, b);
    gfx_frame(DL + 3, t + 3, DR - 3, b - 3);
}

static int wait_input(int *mx, int *my)
{
    while (mouse_down()) nap(1);
    for (;;) {
        int k = poll_key();
        if (k) return k;
        if (mouse_down()) {
            *mx = mouse_x(); *my = mouse_y();
            while (mouse_down()) nap(1);
            return -1;
        }
        nap(1);
    }
}

static void message(const char *l1, const char *l2)
{
    render_suspend();                       /* freeze the animation under the modal dialog */
    int t = 130, b = 196;
    dlg_frame(t, b);
    gfx_text(DL + 16, t + 16, l1, 0);
    if (l2) gfx_text(DL + 16, t + 30, l2, 0);
    button(DR - 80, b - 26, DR - 16, b - 10, "OK", 1);
    show(t, b + 4); UNLOCK();
    int mx = 0, my = 0;
    for (;;) {
        int k = wait_input(&mx, &my);
        if (k > 0 || in_rect(mx, my, DR - 80, b - 26, DR - 16, b - 10)) break;
    }
    clear_overlay(); refresh(DL, t, DR + 4, b + 4); render_resume();
}

static int confirm(const char *l1, const char *l2, const char *ok)
{
    render_suspend();
    int t = 130, b = 200;
    dlg_frame(t, b);
    gfx_text(DL + 16, t + 16, l1, 0);
    if (l2) gfx_text(DL + 16, t + 30, l2, 0);
    button(DR - 180, b - 26, DR - 100, b - 10, "Cancel", 0);
    button(DR - 84, b - 26, DR - 16, b - 10, ok, 1);
    show(t, b + 4); UNLOCK();
    int mx = 0, my = 0, res = -1;
    while (res < 0) {
        int k = wait_input(&mx, &my);
        if (k == KEY_RETURN || k == KEY_ENTER) res = 1;
        else if (k == KEY_ESC) res = 0;
        else if (k == -1) {
            if (in_rect(mx, my, DR - 84, b - 26, DR - 16, b - 10)) res = 1;
            else if (in_rect(mx, my, DR - 180, b - 26, DR - 100, b - 10)) res = 0;
        }
    }
    clear_overlay(); refresh(DL, t, DR + 4, b + 4); render_resume();
    return res;
}

static int prompt(const char *title, char *buf, int size)
{
    render_suspend();
    int t = 130, b = 206;
    int n = (int)strlen(buf);
    int mx = 0, my = 0, res = -1;
    while (res < 0) {
        dlg_frame(t, b);
        gfx_text(DL + 16, t + 14, title, 0);
        gfx_frame(DL + 16, t + 30, DR - 16, t + 46);
        char vis[40]; int off = n > 35 ? n - 35 : 0;
        scopy(vis, buf + off, sizeof vis);
        int x = gfx_text(DL + 20, t + 34, vis, 0);
        gfx_fill(x, t + 33, x + 2, t + 43, 1);
        button(DR - 180, b - 24, DR - 100, b - 8, "Cancel", 0);
        button(DR - 84, b - 24, DR - 16, b - 8, "OK", 1);
        show(t, b + 4); UNLOCK();
        int k = wait_input(&mx, &my);
        if (k == KEY_RETURN || k == KEY_ENTER) res = 1;
        else if (k == KEY_ESC) res = 0;
        else if (k == KEY_BACKSPACE) { if (n) buf[--n] = 0; }
        else if (k >= 0x20 && k < 0x7F && k != ':' && n < size - 1) { buf[n++] = (char)k; buf[n] = 0; }
        else if (k == -1) {
            if (in_rect(mx, my, DR - 84, b - 24, DR - 16, b - 8)) res = 1;
            else if (in_rect(mx, my, DR - 180, b - 24, DR - 100, b - 8)) res = 0;
        }
    }
    clear_overlay(); refresh(DL, t, DR + 4, b + 4); render_resume();
    return res && buf[0];
}

/* ======================================================================== */
/* window contents (window-local coordinates)                               */
/* ======================================================================== */
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

static void scan_programs(void)
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

static void launch(int argc, char **argv, int pause);
static void start_prog(int argc, char **argv);

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

static void open_about(void)
{
    Window *w = new_window(WK_ABOUT, "About PlusOS", 40, 44, 272, 118);
    if (w) w->draw = draw_about;
}
static void open_info(void)
{
    Window *w = new_window(WK_INFO, "System Info", 72, 176, 296, 92);
    if (w) { w->draw = draw_info; w->live = 1; }
}
static void open_settings(void)
{
    Window *w = new_window(WK_SETTINGS, "Theme & Settings", 136, 52, 256, 160);
    if (w) { w->draw = draw_settings; w->click = click_settings; }
}
static void open_programs(void)
{
    scan_programs();
    int rows = (nprog + PW_COLS - 1) / PW_COLS;
    if (rows < 1) rows = 1;
    Window *w = new_window(WK_PROGRAMS, "Programs (bin)", 96, 44, PW_COLS * PW_CELLW + 8, TITLEBAR_H + rows * PW_CELLH + 10);
    if (w) { w->draw = draw_programs; w->click = click_programs; }
}

/* ---- the Games folder: an icon view of the :Games: directory ---- */
#define GW_COLS   4
#define GW_CELLW  74
#define GW_CELLH  54
static char game_names[80][32];
static int  ngame, game_sel = -1;
static long games_dir;

static void scan_games(void)
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
        int ax = cx + GW_CELLW / 2 - 16;                 /* icon_art centers art near bx+16 */
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

static void open_games(void)
{
    scan_games();
    int rows = (ngame + GW_COLS - 1) / GW_COLS;
    if (rows < 1) rows = 1;
    Window *w = new_window(WK_GAMES, "Games", 88, 54, GW_COLS * GW_CELLW + 14, TITLEBAR_H + rows * GW_CELLH + 14);
    if (w) { w->draw = draw_games; w->click = click_games; }
}

/* ======================================================================== */
/* running programs                                                         */
/* ======================================================================== */
static void launch(int argc, char **argv, int pause)
{
    render_suspend();                        /* stop the renderer: the program owns the screen */
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

static int interactive(const char *name)
{
    return ieq(name, "shell") || ieq(name, "taskman") || ieq(name, "hexedit") ||
           ieq(name, "qdemo") || ieq(name, "desktop");
}

/* ---- programs in windows ---------------------------------------------------
 * Programs started from the desktop run alongside it: a text program's console
 * becomes a terminal window (opened when it first prints), a graphical one
 * opens its own windows. The desktop never waits for them. Only the QuickDraw
 * demo, which draws on the real screen, still gets the screen to itself. */
static const char *leaf(const char *p) { const char *l = p; for (; *p; p++) if (*p == ':') l = p + 1; return l; }

/* GUI programs draw their own windows (plain spawn); console programs get a
 * terminal window; qdemo/desktop take the whole screen. */
static int is_gui_app(const char *name)
{
    const char *n = leaf(name);
    return ieq(n, "taskman") || ieq(n, "files") ||
           ieq(n, "calc") || ieq(n, "notes") || ieq(n, "puzzle");
}

/* Full-screen programs draw straight to the screen (games, the QuickDraw demo),
 * so they get it to themselves and the Matrix renderer is parked while they run
 * - otherwise the animation leaks through them. */
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

static void start_prog(int argc, char **argv)
{
    const char *n = leaf(argv[0]);
    if (ieq(n, "desktop")) return;                 /* never re-launch the desktop over itself */
    if (is_fullscreen_app(n)) { launch(argc, argv, 1); return; }   /* screen to itself, Matrix off */
    struct plusos_spawn_args s = { argv[0], argc, argv };
    int pid = is_gui_app(argv[0]) ? (int)plusos_syscall(SYS_spawn, &s)       /* opens its own GUI window */
                                  : (int)plusos_syscall(SYS_termspawn, &s);  /* terminal window */
    if (pid < 0) {
        char m[48];
        snprintf(m, sizeof m, "Can't open \"%s\".", leaf(argv[0]));
        message(m, pid == -1001 ? "It isn't on the disk." : "The program could not start.");
    }
}

static void hex_edit(const char *path)
{
    char *av[] = { "hexedit", (char *)path, 0 };
    start_prog(path ? 2 : 1, av);
}

static void post_ev(int id, int type, int x, int y, int key)
{
    struct plusos_event e = { type, x, y, key };
    struct plusos_win_args a; memset(&a, 0, sizeof a);
    a.op = PLUSOS_WIN_POST; a.id = id; a.ev = &e;
    plusos_syscall(SYS_win, &a);
}

static void destroy_app_window(int id)
{
    struct plusos_win_args a; memset(&a, 0, sizeof a);
    a.op = PLUSOS_WIN_DESTROY; a.id = id;
    plusos_syscall(SYS_win, &a);
}

#define MAX_KW 12
static struct plusos_wininfo kwl[MAX_KW];
static unsigned long ws_last;
static int cascade;

static Window *find_app(int id) { for (int i = 0; i < nwin; i++) if (z[i]->kind == WK_APP && z[i]->app_id == id) return z[i]; return 0; }

/* Bring the desktop's picture of program windows up to date with the window
 * server: new windows appear, closed ones vanish, updated ones are redrawn. */
static void sync_apps(void)
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

static void start_system(int id, const char *name)
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

/* ======================================================================== */
/* file commands                                                            */
/* ======================================================================== */
typedef struct { char path[64]; char name[32]; int desktop; int prot; } Target;

static int is_protected(const char *name) { return ieq(name, "shell") || ieq(name, "desktop"); }

static int get_target(Target *t)
{
    Window *f = front();
    if (!focus_desktop && f && f->kind == WK_PROGRAMS && prog_sel >= 0) {
        snprintf(t->path, sizeof t->path, ":bin:%s", prog_names[prog_sel]);
        scopy(t->name, prog_names[prog_sel], sizeof t->name);
        t->desktop = 0; t->prot = is_protected(t->name);
        return 1;
    }
    if (icon_sel >= 0 && icons[icon_sel].kind == K_FILE) {
        scopy(t->path, icons[icon_sel].path, sizeof t->path);
        scopy(t->name, icons[icon_sel].label, sizeof t->name);
        t->desktop = 1; t->prot = 0;
        return 1;
    }
    return 0;
}

static void unique_desktop_name(const char *base, char *out, int size)
{
    char path[80];
    snprintf(out, size, "%s", base);
    snprintf(path, sizeof path, ":Desktop:%s", out);
    for (int n = 1; file_exists(path) && n < 50; n++) {
        char b[24]; scopy(b, base, 20);
        if (n == 1) snprintf(out, size, "%s copy", b);
        else snprintf(out, size, "%s copy %d", b, n);
        snprintf(path, sizeof path, ":Desktop:%s", out);
    }
}

static void select_icon_named(const char *name)
{
    char id[32]; snprintf(id, sizeof id, "f/%s", name);
    for (int i = 0; i < nicons; i++) if (!strcmp(icons[i].id, id)) icon_sel = i;
    focus_desktop = 1;
}

static void copy_to_desktop(const char *src, const char *name)
{
    if (!desk_dir) { message("There is no Desktop folder", "on this disk."); return; }
    char nm[32], dst[80];
    unique_desktop_name(name, nm, sizeof nm);
    snprintf(dst, sizeof dst, ":Desktop:%s", nm);
    if (copy_file(src, dst) != 0) { message("Could not copy the file.", "The disk may be full."); return; }
    build_icons();
    select_icon_named(nm);
    redraw();
}

static void cmd_copy(void)
{
    Target t;
    if (!get_target(&t)) return;
    scopy(clip_path, t.path, sizeof clip_path);
    scopy(clip_name, t.name, sizeof clip_name);
}
static void cmd_paste(void) { if (clip_path[0]) copy_to_desktop(clip_path, clip_name); }
static void cmd_duplicate(void) { Target t; if (get_target(&t)) copy_to_desktop(t.path, t.name); }

static void cmd_delete(void)
{
    Target t;
    if (!get_target(&t)) return;
    if (t.prot) { message("That program is part of PlusOS", "and can't be deleted."); return; }
    char l1[48];
    snprintf(l1, sizeof l1, "Delete \"%s\"?", t.name);
    if (!confirm(l1, "It will be erased from the disk.", "Delete")) return;
    if (delete_file(t.path) != 0) { message("Could not delete the file.", 0); return; }
    if (t.desktop) {
        char k[48]; snprintf(k, sizeof k, "icon.f/%s", t.name); rset(k, "");
        int l = icons[icon_sel].x, tp = icons[icon_sel].y;
        icon_sel = -1;
        build_icons();
        refresh(l, tp, l + ICON_W, tp + ICON_H);
    } else {
        prog_sel = -1;
        scan_programs();
        Window *w = find_kind(WK_PROGRAMS);
        if (w) refresh_win(w);
    }
    if (!strcmp(clip_path, t.path)) clip_path[0] = 0;
}

static void cmd_rename(void)
{
    Target t;
    if (!get_target(&t)) return;
    if (t.prot) { message("That program is part of PlusOS", "and can't be renamed."); return; }
    char nm[32]; scopy(nm, t.name, sizeof nm);
    if (!prompt("New name:", nm, 28) || !strcmp(nm, t.name)) return;
    char to[80];
    snprintf(to, sizeof to, "%s%s", t.desktop ? ":Desktop:" : ":bin:", nm);
    if (file_exists(to)) { message("There is already a file", "with that name."); return; }
    if (rename_file(t.path, to) != 0) { message("Could not rename the file.", 0); return; }
    if (t.desktop) {
        char k0[48], k1[48], v[24];
        snprintf(k0, sizeof k0, "icon.f/%s", t.name);
        snprintf(k1, sizeof k1, "icon.f/%s", nm);
        if (rget(k0, v, sizeof v) > 0) { rset(k1, v); rset(k0, ""); }
        build_icons();
        select_icon_named(nm);
        redraw();
    } else {
        scan_programs();
        Window *w = find_kind(WK_PROGRAMS);
        if (w) refresh_win(w);
    }
}

static void open_icon(int i)
{
    Icon *ic = &icons[i];
    switch (ic->kind) {
    case K_PROGRAMS: open_programs(); redraw(); break;
    case K_GAMES:    open_games(); redraw(); break;
    case K_SHELL:    { char *av[] = { "shell", 0 };   start_prog(1, av); } break;
    case K_HEX:      hex_edit(0); break;
    case K_TASKS:    { char *av[] = { "taskman", 0 }; start_prog(1, av); } break;
    case K_FILES:    { char *av[] = { "files", 0 };   start_prog(1, av); } break;
    case K_SETTINGS: open_settings(); redraw(); break;
    case K_TRASH:    message("Drag a file onto the Trash", "to delete it."); break;
    case K_FILE:
        if (ic->program) { char *av[] = { ic->path, 0 }; start_prog(1, av); }
        else hex_edit(ic->path);
        break;
    }
}

static void cmd_open(void)
{
    Window *f = front();
    if (!focus_desktop && f && f->kind == WK_PROGRAMS && prog_sel >= 0) {
        char *av[] = { prog_names[prog_sel], 0 };
        start_prog(1, av);
    } else if (icon_sel >= 0) open_icon(icon_sel);
}

static void clean_up(void)
{
    for (int i = 0; i < nicons; i++) {
        if (icons[i].kind != K_FILE) {
            for (int s = 0; s < NSYS; s++)
                if (sys_icons[s].kind == icons[i].kind) {
                    icons[i].x = sys_icons[s].x;
                    icons[i].y = sys_icons[s].kind == K_TRASH ? (desk_bottom() - ICON_H - 4) : sys_icons[s].y;
                }
        } else icons[i].x = -1000;
    }
    for (int i = 0; i < nicons; i++) if (icons[i].kind == K_FILE) place_free(&icons[i], i);
    for (int i = 0; i < nicons; i++) save_icon_pos(&icons[i]);
    redraw();
}

/* ======================================================================== */
/* menus & Start menu                                                       */
/* ======================================================================== */
static int quit_requested;

static int item_enabled(int m, int i)
{
    Target t;
    if (m == 1 && (i == 0 || i == 1)) {
        int have = get_target(&t);
        return i == 0 ? (have || icon_sel >= 0) : have;
    }
    if (m == 2) {
        int have = get_target(&t);
        if (i == 1) return clip_path[0] != 0;
        if (i == 3 || i == 5) return have && !t.prot;
        return have;
    }
    return 1;
}

static int item_checked(int m, int i)
{
    if (m == 3) return (i == 2 && !matrix_on) || (i == 3 && matrix_on);
    if (m == 4) return i == cur_theme;
    return 0;
}

static void do_menu(int m, int item)
{
    switch (m) {
    case 0:
        if (item == 0) { open_about(); redraw(); }
        else if (item == 1) { open_info(); redraw(); }
        else if (item == 2) { char *av[] = { "calc", 0 }; start_prog(1, av); }
        else if (item == 3) { char *av[] = { "notes", 0 }; start_prog(1, av); }
        else if (item == 4) { char *av[] = { "puzzle", 0 }; start_prog(1, av); }
        else if (item == 5) { char *av[] = { "taskman", 0 }; start_prog(1, av); }
        else if (item == 6) { open_settings(); redraw(); }
        break;
    case 1:
        if (item == 0) cmd_open();
        else if (item == 1) { Target t; if (get_target(&t)) hex_edit(t.path); }
        else if (item == 2) { open_programs(); redraw(); }
        else if (item == 4) quit_requested = 1;
        break;
    case 2:
        if (item == 0) cmd_copy();
        else if (item == 1) cmd_paste();
        else if (item == 2) cmd_duplicate();
        else if (item == 3) cmd_rename();
        else if (item == 5) cmd_delete();
        break;
    case 3:
        if (item == 0) clean_up();
        else if (item == 2) set_wallpaper(0);
        else if (item == 3) set_wallpaper(1);
        break;
    case 4:
        if (item >= 0 && item < PLUSOS_THEME_COUNT) set_theme(item);
        break;
    case 5:
        if (item == 0) start_system(3, "System 7.5.5");
        else if (item == 1) start_system(2, "System 7.1");
        else if (item == 2) start_system(1, "System 6.0.8");
        else if (item == 4) plusos->reboot();
        else if (item == 5) plusos->shutdown();
        break;
    }
}

/* Pull-down menus are drawn in the back buffer and flipped, so they appear
 * in one piece. */
static void track_menus(void)
{
    int open = -1, hi = -1;
    int dl = 0, dt = MENUBAR_H, dr = 0, db = 0;
    for (;;) {
        int mx = mouse_x(), my = mouse_y();
        if (my < MENUBAR_H) {
            int m = -1;
            for (int i = 0; i < NMENUS; i++) if (mx >= menu_x[i] && mx < menu_x[i] + menu_w[i]) m = i;
            if (m != open && m >= 0) {
                LOCK();
                if (open >= 0) compose(0, 0, dr + 4, db + 4);
                int odr = dr, odb = db;
                open = m; hi = -1;
                const Menu *mn = &menus[m];
                int wmax = 0;
                for (int i = 0; i < mn->n; i++) if (gfx_text_width(mn->items[i]) > wmax) wmax = gfx_text_width(mn->items[i]);
                dl = menu_x[m]; dr = dl + wmax + 34; db = dt + mn->n * 16 + 4;
                gfx_target(BACK); gfx_noclip();
                gfx_invert(menu_x[m], 1, menu_x[m] + menu_w[m], MENUBAR_H - 1);
                gfx_fill(dl + 2, dt + 2, dr + 2, db + 2, 1);
                gfx_fill(dl, dt, dr, db, 0);
                gfx_frame(dl, dt, dr, db);
                for (int i = 0; i < mn->n; i++) {
                    int y = dt + 2 + i * 16;
                    if (mn->items[i][0] == '-') { gfx_pattern(dl + 1, y + 8, dr - 1, y + 9, 0xAA, 0xAA); continue; }
                    if (item_checked(m, i)) gfx_text(dl + 4, y + 4, "*", 0);
                    gfx_text(dl + 16, y + 4, mn->items[i], 0);
                    if (!item_enabled(m, i)) gfx_dim(dl + 1, y + 2, dr - 1, y + 14);
                }
                ov_l = dl; ov_t = dt; ov_r = dr + 3; ov_b = db + 3; ov_on = 1; cover_dirty = 1;
                show(0, maxi(db, odb) + 4);
                UNLOCK();
                (void)odr;
            }
        }
        int nh = -1;
        if (open >= 0 && in_rect(mx, my, dl + 1, dt + 2, dr - 1, db - 2)) {
            nh = (my - dt - 2) / 16;
            if (nh >= menus[open].n || menus[open].items[nh][0] == '-' || !item_enabled(open, nh)) nh = -1;
        }
        if (nh != hi) {
            LOCK();
            gfx_target(BACK); gfx_noclip();
            if (hi >= 0) gfx_invert(dl + 1, dt + 2 + hi * 16, dr - 1, dt + 2 + hi * 16 + 16);
            if (nh >= 0) gfx_invert(dl + 1, dt + 2 + nh * 16, dr - 1, dt + 2 + nh * 16 + 16);
            int a = hi >= 0 ? hi : nh, c = nh >= 0 ? nh : hi;
            show(dt + 2 + mini(a, c) * 16, dt + 2 + maxi(a, c) * 16 + 16);
            UNLOCK();
            hi = nh;
        }
        if (!mouse_down()) break;
        nap(1);
    }
    if (hi >= 0) {
        for (int k = 0; k < 2; k++) {
            LOCK();
            gfx_target(BACK); gfx_noclip();
            gfx_invert(dl + 1, dt + 2 + hi * 16, dr - 1, dt + 2 + hi * 16 + 16);
            show(dt + 2 + hi * 16, dt + 2 + hi * 16 + 16);
            UNLOCK();
            nap(3);
        }
    }
    clear_overlay();
    if (open >= 0) refresh(0, 0, dr + 4, db + 4);
    if (open >= 0 && hi >= 0) do_menu(open, hi);
}

/* Interactive Start Menu popup for Windows 95, Windows 98, Vista/7 Aero, and Hacker Mode */
static void track_start_menu(void)
{
    static const char *sitems[] = {
        "Programs",
        "Games",
        "Command Shell",
        "File Manager",
        "Calculator",
        "Task Manager",
        "Theme Settings...",
        "Next OS Theme",
        "Shut Down..."
    };
    const int nitems = 9;
    int dl = 2, dr = 164;
    int db = SCREEN_H - TASKBAR_H - 1;
    int dt = db - (nitems * 16 + 6);

    set_overlay(dl, dt, dr + 4, SCREEN_H);
    LOCK();
    gfx_target(BACK); gfx_noclip();
    /* Invert Start button on taskbar while menu is up */
    gfx_invert(2, SCREEN_H - TASKBAR_H + 2, 56, SCREEN_H - 2);
    /* Drop shadow + menu box */
    gfx_fill(dl + 2, dt + 2, dr + 2, db + 2, 1);
    gfx_fill(dl, dt, dr, db, 0);
    gfx_frame(dl, dt, dr, db);
    gfx_frame(dl + 1, dt + 1, dr - 1, db - 1);
    /* Classic vertical OS banner strip on left of Start Menu */
    gfx_fill(dl + 2, dt + 2, dl + 22, db - 2, 1);
    const char *vtag = cur_theme == PLUSOS_THEME_WIN95  ? "WIN95"
                     : cur_theme == PLUSOS_THEME_WIN98  ? "WIN98"
                     : cur_theme == PLUSOS_THEME_AERO   ? "AERO7"
                     :                                    "ROOT!";
    for (int k = 0; vtag[k]; k++) {
        char ch[2] = { vtag[k], 0 };
        gfx_text(dl + 8, db - 18 - (4 - k) * 18, ch, 1);
    }
    for (int i = 0; i < nitems; i++) {
        int y = dt + 3 + i * 16;
        if (i == 6 || i == 8) gfx_hline(dl + 24, dr - 2, y - 1, 1);
        gfx_text(dl + 28, y + 4, sitems[i], 0);
    }
    if (cur_theme == PLUSOS_THEME_HACKER) {
        gfx_invert(dl + 23, dt + 2, dr - 2, db - 2);
    }
    show(dt, SCREEN_H);
    UNLOCK();

    while (mouse_down()) nap(1);    /* allow click-to-open OR drag-to-select */
    int hi = -1, chosen = -1;
    for (;;) {
        int mx = mouse_x(), my = mouse_y();
        int nh = -1;
        if (in_rect(mx, my, dl + 23, dt + 3, dr - 2, db - 3)) {
            nh = (my - dt - 3) / 16;
            if (nh < 0 || nh >= nitems) nh = -1;
        }
        if (nh != hi) {
            LOCK();
            gfx_target(BACK); gfx_noclip();
            if (hi >= 0) gfx_invert(dl + 23, dt + 3 + hi * 16, dr - 2, dt + 3 + hi * 16 + 15);
            if (nh >= 0) gfx_invert(dl + 23, dt + 3 + nh * 16, dr - 2, dt + 3 + nh * 16 + 15);
            show(dt, db);
            UNLOCK();
            hi = nh;
        }
        if (mouse_down()) {
            chosen = hi;
            while (mouse_down()) nap(1);
            break;
        }
        int k = poll_key();
        if (k == KEY_ESC) break;
        nap(1);
    }

    clear_overlay();
    tb_dirty = 1;
    refresh(0, dt, dr + 6, SCREEN_H);

    switch (chosen) {
    case 0: open_programs(); redraw(); break;
    case 1: open_games(); redraw(); break;
    case 2: { char *av[] = { "shell", 0 };   start_prog(1, av); } break;
    case 3: { char *av[] = { "files", 0 };   start_prog(1, av); } break;
    case 4: { char *av[] = { "calc", 0 };    start_prog(1, av); } break;
    case 5: { char *av[] = { "taskman", 0 }; start_prog(1, av); } break;
    case 6: open_settings(); redraw(); break;
    case 7: set_theme((cur_theme + 1) % PLUSOS_THEME_COUNT); break;
    case 8: plusos->shutdown(); break;
    }
}

static void click_taskbar(int mx, int my)
{
    (void)my;
    int start_r = (cur_theme == PLUSOS_THEME_AERO) ? 32 : 58;
    if (mx < start_r) {
        track_start_menu();
        return;
    }
    if (cur_theme == PLUSOS_THEME_WIN98 && mx >= 60 && mx < 88) {
        char *av[] = { "shell", 0 };
        start_prog(1, av);
        return;
    }
    int btn_l = (cur_theme == PLUSOS_THEME_WIN98) ? 90 : start_r;
    int tray_l = SCREEN_W - 60;
    if (nwin > 0 && mx >= btn_l && mx < tray_l) {
        int avail = tray_l - btn_l - 4;
        int bw = avail / nwin;
        if (bw > 96) bw = 96;
        if (bw >= 28) {
            int idx = (mx - btn_l) / bw;
            if (idx >= 0 && idx < nwin) {
                Window *w = z[idx];
                Window *old = front();
                bring_front(w);
                focus_desktop = 0;
                if (old && old != w) refresh(old->x, old->y, old->x + old->w + 2, old->y + old->h + 2);
                refresh_win(w);
                refresh(0, SCREEN_H - TASKBAR_H, SCREEN_W, SCREEN_H);
            }
        }
    }
}

/* ======================================================================== */
/* live dragging                                                            */
/* ======================================================================== */
/* Move a rectangle (window or icon) with the mouse. Each step recomposes
 * only the area it left and entered and flips those rows, with the arrow
 * drawn in software so nothing flickers. */
static void drag_live(int *px, int *py, int w, int h, int gx, int gy, int ymin)
{
    HideCursor();
    cur_x = cur_y = -100;
    present_with_cursor(*py, *py + h);
    while (mouse_down()) {
        int nx = snap16(mouse_x() - gx), ny = mouse_y() - gy;
        if (nx < -w + 32) nx = (-w + 32) & ~15;
        if (nx > SCREEN_W - 32) nx = SCREEN_W - 32;
        if (ny < ymin) ny = ymin;
        if (ny > desk_bottom() - 16) ny = desk_bottom() - 16;
        int moved = nx != *px || ny != *py;
        int cx = mouse_x(), cy = mouse_y();
        if (moved) {
            LOCK();                                  /* move + repaint as one step */
            int ox = *px, oy = *py;
            *px = nx; *py = ny;
            int l = mini(ox, nx), t = mini(oy, ny), r = maxi(ox, nx) + w, b = maxi(oy, ny) + h;
            compose(l, t, r, b);
            present_with_cursor(t, b);
            UNLOCK();
        } else if (cx - cur_x != 1 || cy - cur_y != 1) {
            present_with_cursor(cy, cy);             /* just move the arrow */
        }
        nap(1);
    }
    LOCK();
    compose(cur_x, cur_y, cur_x + 24, cur_y + 16); /* erase the soft arrow */
    gfx_present_rows(maxi(cur_y, 0), mini(cur_y + 16, SCREEN_H));
    UNLOCK();
    ShowCursor();
}

static void drag_window(Window *w, int gx, int gy)
{
    int x = w->x, y = w->y;
    /* the window itself is what moves: point the window at x/y each step */
    int *px = &w->x, *py = &w->y;
    (void)x; (void)y;
    drag_live(px, py, w->w + 2, w->h + 2, gx, gy, MENUBAR_H);
}

static uint32_t last_click_tick;
static int      last_click_icon = -1;

static int icon_at(int mx, int my)
{
    for (int i = nicons - 1; i >= 0; i--)
        if (in_rect(mx, my, icons[i].x, icons[i].y, icons[i].x + ICON_W, icons[i].y + ICON_H)) return i;
    return -1;
}

static void press_icon(int i, int mx, int my)
{
    int old = icon_sel;
    int was_desktop = focus_desktop;
    focus_desktop = 1;
    if (old != i) {
        icon_sel = i;
        if (old >= 0) refresh_icon(old);
        refresh_icon(i);
    }
    if (!was_desktop) {
        Window *p = find_kind(WK_PROGRAMS); if (p) refresh_win(p);
        Window *g = find_kind(WK_GAMES);    if (g) refresh_win(g);
    }
    int sx = mx, sy = my, dragging = 0;
    while (mouse_down()) {
        int dx = mouse_x() - sx, dy = mouse_y() - sy;
        if (dx > 3 || dx < -3 || dy > 3 || dy < -3) { dragging = 1; break; }
        nap(1);
    }
    if (dragging) {
        Icon *ic = &icons[i];
        int ox = ic->x, oy = ic->y;
        /* draw the icon on top while it moves: put it last in paint order */
        LOCK();
        Icon tmp = *ic;
        for (int k = i; k < nicons - 1; k++) icons[k] = icons[k + 1];
        icons[nicons - 1] = tmp;
        icon_sel = nicons - 1;
        ic = &icons[nicons - 1];
        UNLOCK();
        drag_live(&ic->x, &ic->y, ICON_W, ICON_H, sx - ox, sy - oy, MENUBAR_H);
        if (ic->x < 0) ic->x = 0;
        if (ic->x > SCREEN_W - ICON_W) ic->x = SCREEN_W - ICON_W;
        if (ic->y > desk_bottom() - ICON_H) ic->y = desk_bottom() - ICON_H;
        int tr = -1;
        for (int k = 0; k < nicons; k++) if (icons[k].kind == K_TRASH) tr = k;
        if (tr >= 0 && tr != icon_sel && ic->kind == K_FILE &&
            in_rect(ic->x + ICON_W / 2, ic->y + ICON_H / 2, icons[tr].x, icons[tr].y,
                    icons[tr].x + ICON_W, icons[tr].y + ICON_H)) {
            int nx = ic->x, ny = ic->y;
            ic->x = ox; ic->y = oy;                   /* back where it was, then ask */
            refresh(nx, ny, nx + ICON_W, ny + ICON_H);
            refresh_icon(icon_sel);
            cmd_delete();
            return;
        }
        save_icon_pos(ic);
        last_click_icon = -1;
        return;
    }
    uint32_t t = now();
    int dbl = i == last_click_icon && t - last_click_tick < 30;
    last_click_icon = i; last_click_tick = t;
    if (dbl) { last_click_icon = -1; open_icon(i); }
}

static void click(int mx, int my)
{
    if (my < MENUBAR_H) {
        /* Amiga Workbench screen depth gadget on far right of menu bar */
        if (cur_theme == PLUSOS_THEME_AMIGA && mx >= SCREEN_W - 24) {
            Window *f = front();
            if (f && nwin > 1) { send_back(f); redraw(); }
            return;
        }
        track_menus();
        return;
    }
    if (theme_has_taskbar() && my >= SCREEN_H - TASKBAR_H) {
        click_taskbar(mx, my);
        return;
    }
    for (int i = nwin - 1; i >= 0; i--) {
        Window *w = z[i];
        if (!in_rect(mx, my, w->x, w->y, w->x + w->w, w->y + w->h)) continue;
        if (w != front()) {
            Window *old = front();
            bring_front(w);
            focus_desktop = 0;
            if (old) refresh(old->x, old->y, old->x + old->w + 2, old->y + old->h + 2);
            refresh(w->x, w->y, w->x + w->w + 2, w->y + w->h + 2);
            if (theme_has_taskbar()) refresh(0, SCREEN_H - TASKBAR_H, SCREEN_W, SCREEN_H);
            if (my < w->y + TITLEBAR_H) drag_window(w, mx - w->x, my - w->y);
            return;
        }
        /* Amiga Workbench right depth gadget sends window to the back */
        if (cur_theme == PLUSOS_THEME_AMIGA &&
            in_rect(mx, my, w->x + w->w - 35, w->y + 1, w->x + w->w - 18, w->y + TITLEBAR_H)) {
            if (nwin > 1) { send_back(w); redraw(); }
            return;
        }
        /* Close button: left box (all themes) OR right [X] button (Win95/98/Aero/Hacker/Amiga) */
        int hit_left_close = in_rect(mx, my, w->x + 4, w->y + 3, w->x + 20, w->y + 15);
        int hit_right_close = (cur_theme != PLUSOS_THEME_MAC) &&
                              in_rect(mx, my, w->x + w->w - 22, w->y + 2, w->x + w->w - 3, w->y + 15);
        if (hit_left_close || hit_right_close) {
            if (w->kind == WK_APP) {          /* the program's window: ask it / end it */
                if (w->app_dead || w->app_term) destroy_app_window(w->app_id);
                else post_ev(w->app_id, PLUSOS_EV_CLOSE, 0, 0, 0);
                return;                       /* sync_apps removes it once it's gone */
            }
            int l = w->x, t = w->y, r = w->x + w->w + 2, b = w->y + w->h + 2;
            if (w->kind == WK_PROGRAMS) { prog_sel = -1; focus_desktop = 1; }
            if (w->kind == WK_GAMES) { game_sel = -1; focus_desktop = 1; }
            close_window(w);
            refresh(l, t, r, b);
            if (theme_has_taskbar()) refresh(0, SCREEN_H - TASKBAR_H, SCREEN_W, SCREEN_H);
            Window *nf = front();
            if (nf) refresh(nf->x, nf->y, nf->x + nf->w + 2, nf->y + nf->h + 2);
            return;
        }
        if (my < w->y + TITLEBAR_H) { drag_window(w, mx - w->x, my - w->y); return; }
        if (w->kind == WK_APP) {              /* clicks go to the program that owns it */
            focus_desktop = 0;
            int cx = mx - w->x - APP_PADX, cy = my - w->y - APP_TOP;
            if (cx >= 0 && cy >= 0 && cx < w->app_w && cy < w->app_h) {
                post_ev(w->app_id, PLUSOS_EV_MOUSEDOWN, cx, cy, 0);
                while (mouse_down()) nap(1);
                post_ev(w->app_id, PLUSOS_EV_MOUSEUP, mouse_x() - w->x - APP_PADX, mouse_y() - w->y - APP_TOP, 0);
            }
            return;
        }
        if (w->click) {
            uint32_t tk = now();
            int dbl = tk - last_click_tick < 30 && last_click_icon == -2;
            last_click_tick = tk; last_click_icon = -2;
            focus_desktop = 0;
            w->click(w, mx - w->x - 1, my - w->y - TITLEBAR_H, dbl);
            if (dbl) last_click_icon = -1;
        }
        return;
    }
    int i = icon_at(mx, my);
    if (i >= 0) { press_icon(i, mx, my); return; }
    focus_desktop = 1;
    if (icon_sel >= 0) { int old = icon_sel; icon_sel = -1; refresh_icon(old); }
}

/* ======================================================================== */
int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    FRONT = gfx_screen();
    BACK = gfx_backbuffer();
    vref = (INTEGER)plusos->boot_vrefnum;
    gfx_target(BACK);
    swcursor_init();
    rset("sys.desktop", "1");                /* tells programs a desktop is compositing */
    cur_theme = plusos_theme_get();
    layout_menus();
    build_icons();
    char wp[16];
    if ((rget("desktop.wallpaper", wp, sizeof wp) > 0 && !strcmp(wp, "matrix")) ||
        cur_theme == PLUSOS_THEME_HACKER) {
        matrix_on = 1;
        m_init();
    }
    open_about();
    InitCursor();
    redraw();
    gthread_create_ex(renderer_thread, 0, 4096, 1);   /* runs until the desktop quits */

    int was_down = 0;
    uint32_t last_sec = now(), last_m = now();
    while (!quit_requested) {
        int down = mouse_down();
        if (down && !was_down) click(mouse_x(), mouse_y());
        was_down = mouse_down();

        sync_apps();                                 /* program windows: new / gone / updated */
        {   struct plusos_reap r;                    /* tidy up programs that have ended */
            do { r.pid = 0; plusos_syscall(SYS_reapany, &r); } while (r.pid > 0); }

        int k = poll_key();
        Window *fw = front();
        if (k && fw && fw->kind == WK_APP && !focus_desktop && !fw->app_dead) {
            post_ev(fw->app_id, PLUSOS_EV_KEY, 0, 0, k);   /* typing goes to the front program */
            k = 0;
        }
        if (k) {
            int cmd = cmd_down();
            if (cmd && (k == 'c' || k == 'C')) cmd_copy();
            else if (cmd && (k == 'v' || k == 'V')) cmd_paste();
            else if (cmd && (k == 'd' || k == 'D')) cmd_duplicate();
            else if (cmd && (k == 'o' || k == 'O')) cmd_open();
            else if (cmd && (k == 'q' || k == 'Q')) quit_requested = 1;
            else if (cmd && k >= '1' && k <= '6') set_theme(k - '1');
            else if (k == KEY_RETURN || k == KEY_ENTER) cmd_open();
            else if (k == KEY_BACKSPACE || k == KEY_DELETE) cmd_delete();
            else if (k == KEY_UP || k == KEY_DOWN || k == KEY_LEFT || k == KEY_RIGHT) {
                int old = icon_sel;
                if (icon_sel < 0) icon_sel = 0;
                else icon_sel = (icon_sel + ((k == KEY_DOWN || k == KEY_RIGHT) ? 1 : nicons - 1)) % nicons;
                focus_desktop = 1;
                if (old >= 0) refresh_icon(old);
                refresh_icon(icon_sel);
            } else if (k == 'p') {                     /* profile compositor stages, 10x each */
                gfx_target(BACK); gfx_noclip();
                uint32_t t0 = now();
                for (int n = 0; n < 10; n++) {
                    draw_wallpaper_rect(0, MENUBAR_H, SCREEN_W, desk_bottom());
                }
                uint32_t t1 = now();
                for (int n = 0; n < 10; n++)
                    for (int i = 0; i < nicons; i++)
                        gfx_blit_masked(&icons[i].img, &icons[i].mask, 0, 0, ICON_W, ICON_H, icons[i].x, icons[i].y);
                uint32_t t2 = now();
                for (int n = 0; n < 10; n++)
                    for (int i = 0; i < nwin; i++) gfx_blit(&z[i]->surf, 0, 0, z[i]->w, z[i]->h, z[i]->x, z[i]->y, GFX_COPY);
                uint32_t t3 = now();
                for (int n = 0; n < 10; n++) gfx_present_rows(0, SCREEN_H);
                uint32_t t4 = now();
                snprintf(bench_str, sizeof bench_str, "b%lu i%lu w%lu p%lu",
                         (unsigned long)((t1 - t0) * 100 / 60), (unsigned long)((t2 - t1) * 100 / 60),
                         (unsigned long)((t3 - t2) * 100 / 60), (unsigned long)((t4 - t3) * 100 / 60));
                bench_until = now() + 900;
                mb_dirty = 1;
                redraw();
            } else if (k == 'b') {                     /* benchmark: 5 full repaints */
                uint32_t t0 = now();
                for (int i = 0; i < 5; i++) redraw();
                snprintf(bench_str, sizeof bench_str, "redraw %lums",
                         (unsigned long)((now() - t0) * 1000 / 60 / 5));
                bench_until = now() + 600;
                mb_dirty = 1;
                refresh(0, 0, SCREEN_W, MENUBAR_H);
            }
        }

        uint32_t t = now();
        (void)last_m;                                /* the renderer thread animates */
        if (t - last_sec >= 60) {
            last_sec = t;
            /* Live sync with registry so `theme <name>` CLI commands switch themes immediately */
            int reg_theme = plusos_theme_get();
            if (reg_theme != cur_theme) {
                set_theme(reg_theme);
            }
            ULONGINT secs; ReadDateTime(&secs);
            DateTimeRec d; SecondsToDate(secs, &d);
            if (d.minute != mb_minute || (bench_str[0] && t > bench_until)) {
                if (bench_str[0] && t > bench_until) bench_str[0] = 0;
                mb_dirty = 1;
                tb_dirty = 1;
                refresh(0, 0, SCREEN_W, MENUBAR_H);
                if (theme_has_taskbar()) refresh(0, SCREEN_H - TASKBAR_H, SCREEN_W, SCREEN_H);
            }
            Window *f = front();
            if (f && f->live) refresh_win(f);
        }
        nap(1);
    }
    rset("sys.desktop", "");                 /* back to the shell: no compositor */
    HideCursor();
    con_clear();
    return 0;
}
