/* desktop_icons.c - PlusOS Desktop File and System Icon Engine.
 *
 * Implements icon layout, theme-customized artwork, drag-and-drop placement,
 * and HFS file manipulation (Open, Copy, Paste, Duplicate, Delete, Rename).
 */
#include "desktop.h"

void c2p(unsigned char *p, const char *c)
{
    int n = 0;
    while (c[n] && n < 254) { p[n + 1] = (unsigned char)c[n]; n++; }
    p[0] = (unsigned char)n;
}

long dir_id(const char *name)
{
    return dir_id_under(ROOT_DIR, name);
}

/* Find a child directory named `name` under `parent`; 0 if not present. */
long dir_id_under(long parent, const char *name)
{
    for (int idx = 1; idx < 200; idx++) {
        CInfoPBRec pb; unsigned char nm[64]; char c[64];
        memset(&pb, 0, sizeof pb);
        pb.dirInfo.ioNamePtr = nm;
        pb.dirInfo.ioVRefNum = vref;
        pb.dirInfo.ioDrDirID = parent;
        pb.dirInfo.ioFDirIndex = idx;
        if (PBGetCatInfoSync(&pb) != 0) break;
        int n = nm[0]; for (int i = 0; i < n; i++) c[i] = (char)nm[i + 1]; c[n] = 0;
        if ((pb.dirInfo.ioFlAttrib & 0x10) && ieq(c, name)) return pb.dirInfo.ioDrDirID;
    }
    return 0;
}

/* Resolve the PlusOS Desktop for the current user. The result is both an HFS
 * dir id (for list_files) and an HFS colon-form path prefix (for building
 * file paths such as ":home:bob:Desktop:notes"). Cached across calls.
 *
 * Lookup order:
 *   1. $HOME env var, usually "/home/<user>", translated to HFS form and
 *      followed by "Desktop".
 *   2. If $HOME is empty (no login yet), fall back to ":Desktop" at the
 *      volume root so a fresh / unconfigured disk still shows something.
 * The Desktop subfolder of $HOME is created by the firstrun wizard; if it
 * somehow isn't there we also create it on demand. */
static long  g_desk_dir_id;                           /* 0 until resolved */
static char  g_desk_prefix[64] = ":Desktop";          /* HFS path prefix, no trailing colon */

static long ensure_dir_under(long parent, const char *name)
{
    long d = dir_id_under(parent, name);
    if (d) return d;
    HParamBlockRec pb; unsigned char nm[64];
    memset(&pb, 0, sizeof pb);
    int n = 0;
    while (name[n] && n < 63) { nm[n + 1] = (unsigned char)name[n]; n++; }
    nm[0] = (unsigned char)n;
    pb.fileParam.ioNamePtr = nm;
    pb.fileParam.ioVRefNum = vref;
    pb.fileParam.ioDirID = parent;
    if (PBDirCreateSync(&pb) != 0) return 0;
    return dir_id_under(parent, name);
}

static void resolve_desktop(void)
{
    if (g_desk_dir_id) return;

    char home[64] = "";
    struct plusos_env_args e = { PLUSOS_ENV_GET, "HOME", home, sizeof home };
    plusos_syscall(SYS_getenv, &e);

    long parent = ROOT_DIR;
    int po = 0;                                        /* offset in g_desk_prefix */
    g_desk_prefix[0] = 0;

    if (home[0] == '/') {
        /* Walk /home/<user> one segment at a time, mirroring the HFS form
         * into g_desk_prefix as we go. */
        const char *p = home + 1;
        while (*p) {
            char seg[32]; int s = 0;
            while (*p && *p != '/' && s < 31) seg[s++] = *p++;
            seg[s] = 0;
            if (*p == '/') p++;
            if (!seg[0]) continue;
            long next = dir_id_under(parent, seg);
            if (!next) { parent = 0; break; }
            parent = next;
            if (po < (int)sizeof g_desk_prefix - 2) g_desk_prefix[po++] = ':';
            for (int i = 0; seg[i] && po < (int)sizeof g_desk_prefix - 2; i++)
                g_desk_prefix[po++] = seg[i];
            g_desk_prefix[po] = 0;
        }
    }

    if (parent == ROOT_DIR || parent == 0) {
        /* Fallback: :Desktop at the volume root. */
        strcpy(g_desk_prefix, ":Desktop");
        g_desk_dir_id = ensure_dir_under(ROOT_DIR, "Desktop");
        return;
    }

    /* Found the home dir; make sure Desktop is inside it. */
    if (po < (int)sizeof g_desk_prefix - 9) {
        g_desk_prefix[po++] = ':';
        const char *d = "Desktop";
        while (*d && po < (int)sizeof g_desk_prefix - 1) g_desk_prefix[po++] = *d++;
        g_desk_prefix[po] = 0;
    }
    g_desk_dir_id = ensure_dir_under(parent, "Desktop");
}

int list_files(long dir, char names[][32], int max)
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

int file_exists(const char *path)
{
    unsigned char p[256]; FInfo fi;
    c2p(p, path);
    return GetFInfo(p, vref, &fi) == 0;
}

int is_program(const char *path)
{
    unsigned char p[256]; INTEGER ref; char h[8];
    c2p(p, path);
    if (FSOpen(p, vref, &ref) != 0) return 0;
    LONGINT n = 8;
    int ok = FSRead(ref, &n, h) == 0 && n == 8 && h[4] == 'P' && h[5] == 'L' && h[6] == 'O' && h[7] == 'S';
    FSClose(ref);
    return ok;
}

int is_text(const char *path)
{
    unsigned char p[256]; FInfo fi;
    c2p(p, path);
    if (GetFInfo(p, vref, &fi) == 0 && fi.fdType == 0x54455854UL) return 1; /* 'TEXT' */
    const char *dot = 0; for (const char *s = path; *s; s++) if (*s == '.') dot = s;
    if (dot && (ieq(dot,".txt")||ieq(dot,".md")||ieq(dot,".cfg")||ieq(dot,".c")||
                ieq(dot,".h")||ieq(dot,".log")||ieq(dot,".ini")||ieq(dot,".sh"))) return 1;
    INTEGER ref; char h[64];
    if (FSOpen(p, vref, &ref) != 0) return 0;
    LONGINT n = 64; int ok = FSRead(ref, &n, h) == 0;
    FSClose(ref);
    if (!ok || n <= 0) return 0;
    for (int i = 0; i < n; i++) {
        unsigned char ch = (unsigned char)h[i];
        if (ch != '\n' && ch != '\r' && ch != '\t' && (ch < 0x20 || ch > 0x7E)) return 0;
    }
    return 1;
}

int copy_file(const char *src, const char *dst)
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

int delete_file(const char *path)
{
    unsigned char p[256]; c2p(p, path);
    int rc = FSDelete(p, vref);
    FlushVol(0, vref);
    return rc;
}

int rename_file(const char *from, const char *to)
{
    unsigned char a[256], b[256]; c2p(a, from); c2p(b, to);
    int rc = Rename(a, vref, b);
    FlushVol(0, vref);
    return rc;
}

/* Negative x means right-aligned (SCREEN_W + x); Trash y is computed via desk_bottom(). */
static const struct { int kind; const char *id, *label; int x, y; } sys_icons[] = {
    { K_PROGRAMS, "programs", "Programs",   -80, 28 },
    { K_GAMES,    "games",    "Games",       24, 28 },
    { K_SHELL,    "shell",    "Shell",      -80, 76 },
    { K_FILES,    "files",    "Files",      -80, 124 },
    { K_HEX,      "hexedit",  "Hex Edit",   -80, 170 },
    { K_TASKS,    "tasks",    "Tasks",      -80, 216 },
    { K_SETTINGS, "settings", "Settings",   -80, 262 },
    { K_TRASH,    "trash",    "Trash",      -80, -1 },
};
#define NSYS ((int)(sizeof sys_icons / sizeof sys_icons[0]))

static void icon_key(const Icon *ic, char *k) { snprintf(k, 40, "icon.%s", ic->id); }

static void save_icon_pos(const Icon *ic)
{
    if (ic->kind != K_FILE) return;
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
    if (x < 0 || x > SCREEN_W - ICON_W || y < MENUBAR_H || y > desk_bottom() - ICON_H) return 0;
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
    for (int y = 200; y + ICON_H <= desk_bottom(); y += 52)
        for (int x = 16; x + ICON_W <= SCREEN_W - 150; x += 80)
            if (spot_free(x, y, self)) { ic->x = x; ic->y = y; return; }
    ic->x = 16; ic->y = 200;
}

void free_icon(Icon *ic) { gs_free(&ic->img); gs_free(&ic->mask); }

void build_icons(void)
{
    static char names[MAX_ICONS][32];
    static uint8_t progs[MAX_ICONS];
    resolve_desktop();
    int nfiles = g_desk_dir_id ? list_files(g_desk_dir_id, names, MAX_ICONS - NSYS) : 0;
    for (int i = 0; i < nfiles; i++) {
        char path[96];
        snprintf(path, sizeof path, "%s:%s", g_desk_prefix, names[i]);
        progs[i] = (uint8_t)is_program(path);
    }
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
        ic->x = sys_icons[i].x < 0 ? (SCREEN_W + sys_icons[i].x) : sys_icons[i].x;
        ic->y = sys_icons[i].y < 0 ? (desk_bottom() - ICON_H - 4) : sys_icons[i].y;
        ic->rendered = -1;
    }
    for (int i = 0; i < nfiles; i++) {
        Icon *ic = &icons[nicons];
        memset(ic, 0, sizeof *ic);
        ic->kind = K_FILE;
        ic->rendered = -1;
        scopy(ic->label, names[i], sizeof ic->label);
        snprintf(ic->id, sizeof ic->id, "f/%s", names[i]);
        snprintf(ic->path, sizeof ic->path, "%s:%s", g_desk_prefix, names[i]);
        ic->program = progs[i];
        nicons++;
        if (!load_icon_pos(ic)) place_free(ic, nicons - 1);
    }
    icon_sel = -1;
    for (int i = 0; i < nicons; i++) if (keep[0] && !strcmp(icons[i].id, keep)) icon_sel = i;
    cover_dirty = 1;
    UNLOCK();
}

void icon_art(const Icon *ic, int bx, int by, int shape)
{
    switch (ic->kind) {
    case K_GAMES:
    case K_PROGRAMS:
        if (cur_theme == PLUSOS_THEME_AMIGA) {
            if (shape) { gfx_fill(bx + 1, by + 3, bx + 31, by + 26, 1); break; }
            gfx_fill(bx + 1, by + 3, bx + 31, by + 26, 0);
            gfx_frame(bx + 1, by + 3, bx + 31, by + 26);
            gfx_frame(bx + 4, by + 8, bx + 28, by + 23);
            gfx_hline(bx + 4, bx + 28, by + 6, 1);
            gfx_fill(bx + 12, by + 13, bx + 20, by + 17, 1);
            gfx_fill(bx + 14, by + 14, bx + 18, by + 16, 0);
            break;
        }
        if (shape) { gfx_fill(bx + 2, by + 4, bx + 14, by + 9, 1); gfx_fill(bx + 2, by + 8, bx + 30, by + 26, 1); break; }
        gfx_fill(bx + 2, by + 4, bx + 14, by + 9, 0); gfx_frame(bx + 2, by + 4, bx + 14, by + 9);
        gfx_fill(bx + 2, by + 8, bx + 30, by + 26, 0); gfx_frame(bx + 2, by + 8, bx + 30, by + 26);
        if (cur_theme == PLUSOS_THEME_WIN95 || cur_theme == PLUSOS_THEME_WIN98 || cur_theme == PLUSOS_THEME_AERO)
            gfx_dither_rect(bx + 3, by + 10, bx + 29, by + 25, 4);
        gfx_hline(bx + 3, bx + 29, by + 11, 1);
        if (ic->kind == K_GAMES) {
            gfx_vline(bx + 16, by + 16, by + 22, 1);
            gfx_fill(bx + 14, by + 21, bx + 19, by + 24, 1);
            gfx_fill(bx + 15, by + 14, bx + 18, by + 17, 1);
        }
        break;
    case K_TRASH:
        if (shape) { gfx_fill(bx + 13, by + 1, bx + 19, by + 3, 1); gfx_fill(bx + 5, by + 3, bx + 27, by + 6, 1);
                     gfx_fill(bx + 7, by + 6, bx + 25, by + 27, 1); break; }
        gfx_fill(bx + 7, by + 6, bx + 25, by + 27, 0); gfx_frame(bx + 7, by + 6, bx + 25, by + 27);
        gfx_fill(bx + 5, by + 3, bx + 27, by + 6, 0); gfx_frame(bx + 5, by + 3, bx + 27, by + 6);
        gfx_fill(bx + 13, by + 1, bx + 19, by + 3, 0); gfx_frame(bx + 13, by + 1, bx + 19, by + 3);
        if (cur_theme == PLUSOS_THEME_WIN95 || cur_theme == PLUSOS_THEME_WIN98 || cur_theme == PLUSOS_THEME_AERO) {
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
    case K_FILES:
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
        gfx_fill(bx + 20, by + 10, bx + 26, by + 13, 1);
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

void render_icon(Icon *ic, int sel)
{
    if (!ic->img.bits && (gs_init(&ic->img, ICON_W, ICON_H) || gs_init(&ic->mask, ICON_W, ICON_H))) return;
    char lab[12]; scopy(lab, ic->label, 9);
    int lw = gfx_text_width(lab), lx = ICON_W / 2 - lw / 2;
    gfx_target(&ic->img);
    gfx_fill(0, 0, ICON_W, ICON_H, 0);
    if (sel) {
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

static int is_protected(const char *name) { return ieq(name, "shell") || ieq(name, "desktop"); }

int get_target(Target *t)
{
    extern int prog_sel;
    extern char prog_names[80][32];
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
    char path[96];
    snprintf(out, size, "%s", base);
    snprintf(path, sizeof path, "%s:%s", g_desk_prefix, out);
    for (int n = 1; file_exists(path) && n < 50; n++) {
        char b[24]; scopy(b, base, 20);
        if (n == 1) snprintf(out, size, "%s copy", b);
        else snprintf(out, size, "%s copy %d", b, n);
        snprintf(path, sizeof path, "%s:%s", g_desk_prefix, out);
    }
}

static void select_icon_named(const char *name)
{
    char id[32]; snprintf(id, sizeof id, "f/%s", name);
    for (int i = 0; i < nicons; i++) if (!strcmp(icons[i].id, id)) icon_sel = i;
    focus_desktop = 1;
}

/* Background copy: real file I/O (copy_file) can take seconds for a large file.
 * Running it on the Desktop's main thread freezes the UI for that whole time
 * even though the kernel preempts us every 50 ms, because the main thread is
 * the one that would otherwise be reading events and repainting. A worker
 * thread does the copy; the final build_icons/redraw happens on it too, under
 * the usual screen mutex so the main thread sees a consistent state. */
struct copy_job { char src[80]; char dst[80]; char nm[32]; };

static void copy_worker(void *arg)
{
    struct copy_job *j = arg;
    int rc = copy_file(j->src, j->dst);
    LOCK();
    if (rc != 0) {
        UNLOCK();
        message("Could not copy the file.", "The disk may be full.");
    } else {
        build_icons();
        select_icon_named(j->nm);
        redraw();
        UNLOCK();
    }
    free(j);
}

void copy_to_desktop(const char *src, const char *name)
{
    resolve_desktop();
    if (!g_desk_dir_id) { message("There is no Desktop folder", "for this user."); return; }
    struct copy_job *j = malloc(sizeof *j);
    if (!j) { message("Not enough memory", "to start the copy."); return; }
    unique_desktop_name(name, j->nm, sizeof j->nm);
    snprintf(j->src, sizeof j->src, "%s", src);
    snprintf(j->dst, sizeof j->dst, "%s:%s", g_desk_prefix, j->nm);
    if (gthread_create_ex(copy_worker, j, 4096, 1) < 0) {
        /* No thread slot: fall back to a foreground copy so the user still gets
         * their file. UI will freeze briefly, as it did before. */
        copy_worker(j);
    }
}

void cmd_copy(void)
{
    Target t;
    if (!get_target(&t)) return;
    scopy(clip_path, t.path, sizeof clip_path);
    scopy(clip_name, t.name, sizeof clip_name);
}
void cmd_paste(void) { if (clip_path[0]) copy_to_desktop(clip_path, clip_name); }
void cmd_duplicate(void) { Target t; if (get_target(&t)) copy_to_desktop(t.path, t.name); }

void cmd_delete(void)
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
        extern int prog_sel;
        prog_sel = -1;
        scan_programs();
        Window *w = find_kind(WK_PROGRAMS);
        if (w) refresh_win(w);
    }
    if (!strcmp(clip_path, t.path)) clip_path[0] = 0;
}

void cmd_rename(void)
{
    Target t;
    if (!get_target(&t)) return;
    if (t.prot) { message("That program is part of PlusOS", "and can't be renamed."); return; }
    char nm[32]; scopy(nm, t.name, sizeof nm);
    if (!prompt("New name:", nm, 28) || !strcmp(nm, t.name)) return;
    char to[96];
    if (t.desktop) snprintf(to, sizeof to, "%s:%s", g_desk_prefix, nm);
    else           snprintf(to, sizeof to, ":bin:%s", nm);
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

void open_icon(int i)
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
        else if (is_text(ic->path)) { char *av[] = { "notes", ic->path, 0 }; start_prog(2, av); }
        else hex_edit(ic->path);
        break;
    }
}

void cmd_open(void)
{
    extern int prog_sel;
    extern char prog_names[80][32];
    Window *f = front();
    if (!focus_desktop && f && f->kind == WK_PROGRAMS && prog_sel >= 0) {
        char *av[] = { prog_names[prog_sel], 0 };
        start_prog(1, av);
    } else if (icon_sel >= 0) open_icon(icon_sel);
}

void clean_up(void)
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

uint32_t last_click_tick;
int      last_click_icon = -1;

int icon_at(int mx, int my)
{
    for (int i = nicons - 1; i >= 0; i--)
        if (in_rect(mx, my, icons[i].x, icons[i].y, icons[i].x + ICON_W, icons[i].y + ICON_H)) return i;
    return -1;
}

void press_icon(int i, int mx, int my)
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
            ic->x = ox; ic->y = oy;
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
