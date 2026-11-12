/* notes - the PlusOS TextEdit.
 *
 * A classic Macintosh text editor with multiple document tabs: open several
 * files at once, switch between them with the tab bar, and edit with a blinking
 * caret, click-drag selection, word select on double-click, Cut / Copy / Paste
 * through the system scrap, and a vertical scrollbar. The File and Edit menus
 * drop down from the window's top bar; Open and Save As use a graphical file
 * browser that can reach every mounted disk.
 *
 * It is a GUI program: it draws into its window with PlusGFX. While the mouse
 * is held down it polls the low-memory mouse globals directly (the window
 * server only posts press/release), so selection drags, menus and the scrollbar
 * track live.
 */
#include <plusos/plusos.h>
#include <plusos/syscall.h>
#include <plusos/gfx.h>
#include <plusrom/FileMgr.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ---- window geometry ------------------------------------------------------ */
#define WINW    464
#define WINH    280
#define MENU_B  18                 /* menu bar: [0,18)             */
#define TAB_T   18                 /* tab bar:  [18,36)            */
#define TAB_B   36
#define CT      36                 /* text content top             */
#define CB      262                /* text content bottom          */
#define VIEWH   (CB - CT)
#define SBW     16
#define SBX     (WINW - SBW)       /* vertical scrollbar x         */
#define TEXTX   6                  /* text left margin             */
#define TEXTW   (SBX - TEXTX - 2)
#define MAXCOLS (TEXTW / 8)        /* characters per wrapped line  */
#define LINEH   12
#define VISLINES (VIEWH / LINEH)
#define STATUS_T 262

#define NDOC    8                  /* maximum open tabs            */
#define DOC_MAX 32768              /* maximum bytes per document   */

typedef struct {
    char *buf;
    int   len, cap;
    int   caret, anchor;           /* selection = [min..max] of the two */
    int   scroll;                  /* top visible line index       */
    char  name[32];                /* tab label / file name        */
    char  path[160];               /* full HFS path, empty = unsaved */
    INTEGER vref;
    int   modified;
    int   used;
} Doc;

typedef struct { int off, len; } LineInfo;

static GWindow win;
static Doc     doc[NDOC];
static int     active;
static int     want_quit;
static int     origin_x, origin_y;     /* window content origin on screen */

static LineInfo *lines;                /* visual-line index of active doc */
static int     nlines, lines_cap;

static GSurface cmd_glyph;
static int      art_ready;
static char     paste_buf[4096];

/* ---- tiny helpers --------------------------------------------------------- */
static int in_rect(int x, int y, int l, int t, int r, int b) { return x >= l && x < r && y >= t && y < b; }
static int mini(int a, int b) { return a < b ? a : b; }
static int maxi(int a, int b) { return a > b ? a : b; }
static int ieq(const char *a, const char *b)
{ for (;; a++, b++) { int x=*a, y=*b; if (x>='A'&&x<='Z') x+=32; if (y>='A'&&y<='Z') y+=32; if (x!=y) return 0; if (!x) return 1; } }
static void scopy(char *d, const char *s, int n) { int i=0; for (; s[i] && i<n-1; i++) d[i]=s[i]; d[i]=0; }
static void c2p(unsigned char *p, const char *c) { int n=0; while (c[n] && n<254) { p[n+1]=(unsigned char)c[n]; n++; } p[0]=(unsigned char)n; }
static int mouse_x(void) { return *(volatile int16_t *)0x832; }
static int mouse_y(void) { return *(volatile int16_t *)0x830; }
static int mouse_down(void) { return (*(volatile uint8_t *)0x172 & 0x80) == 0; }
static void nap(int t) { plusos_syscall(SYS_sleep, (void *)(intptr_t)t); }
static uint32_t now(void) { return plusos->ticks(); }
static int cmd_down(void) { return (plusos->key_mods() & 0x0100) != 0; }
static int shift_down(void) { return (plusos->key_mods() & 0x0200) != 0; }
static void pstr_to_c(const unsigned char *p, char *out, int max)
{ int n = p[0]; if (n > max-1) n = max-1; for (int i=0;i<n;i++) out[i]=(char)p[i+1]; out[n]=0; }

static void trunc_label(const char *name, char *out, int maxpx)
{
    int maxc = maxpx / 8;
    if (maxc < 1) { out[0] = 0; return; }
    if ((int)strlen(name) <= maxc) { scopy(out, name, 32); return; }
    int keep = maxc - 2; if (keep < 1) keep = 1;
    int i = 0; for (; i < keep && name[i]; i++) out[i] = name[i];
    out[i++] = '.'; out[i++] = '.'; out[i] = 0;
}

/* ---- volume / path queries ------------------------------------------------ */
static void vol_name(INTEGER vref, char *out, int max)
{
    HParamBlockRec pb; unsigned char nm[64];
    memset(&pb, 0, sizeof pb);
    pb.volumeParam.ioNamePtr = nm; pb.volumeParam.ioVRefNum = vref; pb.volumeParam.ioVolIndex = 0;
    if (PBHGetVInfoSync(&pb) == 0) pstr_to_c(nm, out, max);
    else scopy(out, "Disk", max);
}
/* Build "Vol:a:b:" (trailing colon) for a directory id by walking to the root. */
static void build_dir_path(INTEGER vref, long dir, char *out, int outsize)
{
    char comps[16][32]; int n = 0; long d = dir;
    while (d > 2 && n < 16) {
        CInfoPBRec pb; unsigned char nm[64];
        memset(&pb, 0, sizeof pb);
        pb.dirInfo.ioNamePtr = nm; pb.dirInfo.ioVRefNum = vref;
        pb.dirInfo.ioDrDirID = d; pb.dirInfo.ioFDirIndex = -1;
        if (PBGetCatInfoSync(&pb) != 0) break;
        pstr_to_c(nm, comps[n++], 32);
        d = pb.dirInfo.ioDrParID;
    }
    char vol[32]; vol_name(vref, vol, sizeof vol);
    int o = 0;
    o += snprintf(out+o, outsize-o, "%s:", vol);
    for (int i = n-1; i >= 0 && o < outsize-2; i--)
        o += snprintf(out+o, outsize-o, "%s:", comps[i]);
}
/* Resolve the volume reference for a full "Vol:..." path by matching the name. */
static INTEGER resolve_vref(const char *path)
{
    char vol[32]; int n = 0;
    while (path[n] && path[n] != ':' && n < 31) { vol[n] = path[n]; n++; }
    vol[n] = 0;
    if (path[n] != ':') return plusos->boot_vrefnum;
    for (int idx = 1; idx < 32; idx++) {
        HParamBlockRec pb; unsigned char nm[64]; char vn[32];
        memset(&pb, 0, sizeof pb);
        pb.volumeParam.ioNamePtr = nm; pb.volumeParam.ioVolIndex = idx;
        if (PBHGetVInfoSync(&pb) != 0) break;
        pstr_to_c(nm, vn, sizeof vn);
        if (ieq(vn, vol)) return pb.volumeParam.ioVRefNum;
    }
    return plusos->boot_vrefnum;
}
static const char *path_leaf(const char *path)
{
    const char *leaf = path;
    for (const char *s = path; *s; s++) if (*s == ':') leaf = s + 1;
    return leaf;
}
static long parent_dir(INTEGER vref, long dir)
{
    CInfoPBRec pb; unsigned char nm[64];
    memset(&pb, 0, sizeof pb);
    pb.dirInfo.ioNamePtr = nm; pb.dirInfo.ioVRefNum = vref;
    pb.dirInfo.ioDrDirID = dir; pb.dirInfo.ioFDirIndex = -1;
    if (PBGetCatInfoSync(&pb) != 0) return 2;
    return pb.dirInfo.ioDrParID;
}

/* ---- document memory ------------------------------------------------------ */
static int doc_ensure_cap(Doc *d, int need)
{
    if (need <= d->cap) return 1;
    if (need > DOC_MAX + 1) need = DOC_MAX + 1;
    int nc = d->cap ? d->cap : 1024;
    while (nc < need) nc *= 2;
    if (nc > DOC_MAX + 1) nc = DOC_MAX + 1;
    char *nb = realloc(d->buf, (size_t)nc);
    if (!nb) return 0;
    d->buf = nb; d->cap = nc;
    return 1;
}
static void doc_free(Doc *d)
{
    if (d->buf) free(d->buf);
    d->buf = 0; d->cap = 0; d->len = 0;
}
static int count_used(void) { int n=0; for (int i=0;i<NDOC;i++) if (doc[i].used) n++; return n; }

/* ---- visual line index (character wrap at MAXCOLS, '\n' breaks) ----------- */
static void add_line(int off, int ln)
{
    if (nlines >= lines_cap) {
        int nc = lines_cap ? lines_cap * 2 : 256;
        LineInfo *nl = realloc(lines, (size_t)nc * sizeof(LineInfo));
        if (!nl) return;
        lines = nl; lines_cap = nc;
    }
    lines[nlines].off = off; lines[nlines].len = ln; nlines++;
}
static void rebuild_lines(Doc *d)
{
    nlines = 0;
    if (!d->buf) { add_line(0, 0); return; }
    int start = 0, i = 0, len = d->len;
    const char *b = d->buf;
    while (i < len) {
        if (b[i] == '\n') { add_line(start, i - start); start = i + 1; i++; }
        else if (i - start == MAXCOLS) { add_line(start, MAXCOLS); start = i; }
        else i++;
    }
    add_line(start, len - start);
}
static void caret_linecol(Doc *d, int off, int *line, int *col)
{
    int L = 0, c = 0;
    for (int i = 0; i < nlines; i++) {
        int lo = lines[i].off, ln = lines[i].len;
        if (off < lo + ln) { L = i; c = off - lo; break; }
        if (off == lo + ln) {
            if (off == d->len || d->buf[off] == '\n') { L = i; c = ln; break; }
            L = i + 1; c = 0;
            if (L >= nlines) { L = nlines - 1; c = lines[L].len; }
            break;
        }
    }
    *line = L; *col = c;
}
static int offset_at(int line, int col)
{
    if (nlines == 0) return 0;
    if (line < 0) line = 0;
    if (line >= nlines) line = nlines - 1;
    int c = col; if (c > lines[line].len) c = lines[line].len; if (c < 0) c = 0;
    return lines[line].off + c;
}
static int max_scroll(void) { int m = nlines - VISLINES; return m > 0 ? m : 0; }
static void clamp_scroll(Doc *d) { if (d->scroll > max_scroll()) d->scroll = max_scroll(); if (d->scroll < 0) d->scroll = 0; }
static void ensure_visible(Doc *d)
{
    int cl, cc; caret_linecol(d, d->caret, &cl, &cc);
    if (cl < d->scroll) d->scroll = cl;
    if (cl >= d->scroll + VISLINES) d->scroll = cl - VISLINES + 1;
    clamp_scroll(d);
}

/* ---- selection & editing -------------------------------------------------- */
static void sel_range(Doc *d, int *s0, int *s1)
{
    *s0 = mini(d->anchor, d->caret);
    *s1 = maxi(d->anchor, d->caret);
}
static void set_caret(Doc *d, int off, int extend)
{
    if (off < 0) off = 0; if (off > d->len) off = d->len;
    d->caret = off;
    if (!extend) d->anchor = off;
}
static void doc_delete_range(Doc *d, int a, int b)
{
    if (b <= a) return;
    memmove(d->buf + a, d->buf + b, (size_t)(d->len - b + 1));
    d->len -= (b - a);
    d->caret = a; d->anchor = a;
    d->modified = 1;
    rebuild_lines(d); clamp_scroll(d); ensure_visible(d);
}
static int doc_insert(Doc *d, const char *text, int n)
{
    if (n <= 0) return 0;
    int s0, s1; sel_range(d, &s0, &s1);
    int newlen = d->len - (s1 - s0) + n;
    if (newlen > DOC_MAX) return 0;
    if (!doc_ensure_cap(d, newlen + 1)) return 0;
    memmove(d->buf + s0 + n, d->buf + s1, (size_t)(d->len - s1 + 1));
    memcpy(d->buf + s0, text, (size_t)n);
    d->len = newlen; d->caret = s0 + n; d->anchor = d->caret;
    d->modified = 1;
    rebuild_lines(d); clamp_scroll(d); ensure_visible(d);
    return 1;
}
static void doc_backspace(Doc *d)
{
    int s0, s1; sel_range(d, &s0, &s1);
    if (s1 > s0) doc_delete_range(d, s0, s1);
    else if (d->caret > 0) doc_delete_range(d, d->caret - 1, d->caret);
}
static void doc_delete_fwd(Doc *d)
{
    int s0, s1; sel_range(d, &s0, &s1);
    if (s1 > s0) doc_delete_range(d, s0, s1);
    else if (d->caret < d->len) doc_delete_range(d, d->caret, d->caret + 1);
}
static int word_ch(char c)
{
    return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_';
}
static void select_word(Doc *d, int off)
{
    if (off >= d->len) { d->caret = d->len; d->anchor = d->len; return; }
    int a = off, b = off;
    while (a > 0 && word_ch(d->buf[a-1])) a--;
    while (b < d->len && word_ch(d->buf[b])) b++;
    if (a == b) b = a + 1;
    d->anchor = a; d->caret = b;
}

/* ---- system scrap (clipboard) --------------------------------------------- */
static void scrap_set_text(const char *buf, int len)
{
    struct plusos_scrap_args a; memset(&a, 0, sizeof a);
    a.op = PLUSOS_SCRAP_SET; a.type = 0x54455854UL; a.buf = (void *)buf; a.size = len;
    plusos_syscall(SYS_scrap, &a);
}
static int scrap_get_text(char *buf, int cap)
{
    struct plusos_scrap_args a; memset(&a, 0, sizeof a);
    a.op = PLUSOS_SCRAP_GET; a.buf = buf; a.size = cap;
    return (int)plusos_syscall(SYS_scrap, &a);
}

/* ---- file I/O ------------------------------------------------------------- */
/* Normalize CRLF / CR to LF and expand tabs to 4 spaces, in place. */
static void normalize_buf(Doc *d)
{
    int r = 0, w = 0;
    while (r < d->len) {
        char c = d->buf[r++];
        if (c == '\r') {
            if (r < d->len && d->buf[r] == '\n') r++;
            c = '\n';
        }
        if (c == '\t') {
            for (int k = 0; k < 4 && w < DOC_MAX; k++) d->buf[w++] = ' ';
            continue;
        }
        if (w < DOC_MAX) d->buf[w++] = c;
    }
    d->len = w;
    if (d->buf) d->buf[d->len] = 0;
}
static int load_doc(Doc *d, const char *path)
{
    INTEGER vref = resolve_vref(path);
    unsigned char p[300]; c2p(p, path);
    INTEGER ref;
    if (FSOpen(p, vref, &ref) != 0) return -1;
    d->len = 0;
    for (;;) {
        if (!doc_ensure_cap(d, d->len + 512 + 1)) break;
        if (d->cap < d->len + 512 + 1) break;
        LONGINT n = 512;
        OSErr e = FSRead(ref, &n, d->buf + d->len);
        if (n > 0) d->len += (int)n;
        if (e != 0 || d->len >= DOC_MAX) break;
    }
    FSClose(ref);
    if (d->buf) d->buf[d->len] = 0;
    normalize_buf(d);
    d->caret = 0; d->anchor = 0; d->scroll = 0; d->modified = 0;
    d->vref = vref;
    scopy(d->path, path, sizeof d->path);
    scopy(d->name, path_leaf(path), sizeof d->name);
    if (!d->name[0]) scopy(d->name, "untitled", sizeof d->name);
    return 0;
}
static int save_doc(Doc *d)
{
    if (!d->path[0]) return -1;
    INTEGER vref = d->vref ? d->vref : resolve_vref(d->path);
    unsigned char p[300]; c2p(p, d->path);
    FInfo fi;
    if (GetFInfo(p, vref, &fi) != 0) {
        if (Create(p, vref, 0x74747874UL /*'ttxt'*/, 0x54455854UL /*'TEXT'*/) != 0) return -1;
    }
    INTEGER ref;
    if (FSOpen(p, vref, &ref) != 0) return -1;
    SetFPos(ref, 1, 0);
    LONGINT n = d->len;
    if (n > 0) FSWrite(ref, &n, d->buf);
    SetEOF(ref, d->len);
    FSClose(ref);
    FlushVol(0, vref);
    d->modified = 0;
    return 0;
}

/* ---- forward declarations ------------------------------------------------- */
static void draw(void);
static void run_action(int id);
static void do_quit(void);
static void message(const char *l1, const char *l2);
static int  dialog(const char *title, const char *body, const char *const *btns, int nbtns,
                   char *edit, int editsize, int select_all);
static int  pick_run(char *out, int outsize, int mode);
static void close_doc(int i);

/* ---- tabs ----------------------------------------------------------------- */
static void set_title(void) { gwin_title(&win, doc[active].name); }
static int new_doc(void)
{
    int i = -1;
    for (int k = 0; k < NDOC; k++) if (!doc[k].used) { i = k; break; }
    if (i < 0) return -1;
    Doc *d = &doc[i];
    memset(d, 0, sizeof *d);
    d->used = 1;
    char nm[32]; int n = 1;
    for (;;) {
        if (n == 1) scopy(nm, "untitled", sizeof nm);
        else snprintf(nm, sizeof nm, "untitled %d", n);
        int dup = 0;
        for (int k = 0; k < NDOC; k++) if (doc[k].used && k != i && ieq(doc[k].name, nm)) dup = 1;
        if (!dup) break;
        n++;
    }
    scopy(d->name, nm, sizeof d->name);
    active = i;
    rebuild_lines(d);
    set_title();
    return i;
}
static void activate(int i)
{
    if (i < 0 || i >= NDOC || !doc[i].used) return;
    active = i;
    rebuild_lines(&doc[i]);
    clamp_scroll(&doc[i]);
    set_title();
    draw();
}
static int save_doc_interactive(int i)
{
    Doc *d = &doc[i];
    if (!d->path[0]) {
        char path[160];
        if (!pick_run(path, sizeof path, 1)) return 0;
        scopy(d->path, path, sizeof d->path);
        d->vref = resolve_vref(path);
        scopy(d->name, path_leaf(path), sizeof d->name);
    }
    if (save_doc(d) != 0) { message("Could not save the file.", "The disk may be full or locked."); return 0; }
    set_title();
    return 1;
}
static void close_doc(int i)
{
    Doc *d = &doc[i];
    if (!d->used) return;
    if (d->modified) {
        active = i; rebuild_lines(d); set_title(); draw();
        char msg[80]; snprintf(msg, sizeof msg, "Save changes to \"%s\"?", d->name);
        const char *b[] = { "Don't Save", "Cancel", "Save" };
        int res = dialog("TextEdit", msg, b, 3, 0, 0, 0);
        if (res == 1 || res < 0) return;
        if (res == 2 && !save_doc_interactive(i)) return;
    }
    doc_free(d);
    d->used = 0; d->path[0] = 0; d->modified = 0;
    if (count_used() == 0) { want_quit = 1; return; }
    if (active == i) { int j = i; while (!doc[j].used) j = (j + 1) % NDOC; active = j; }
    rebuild_lines(&doc[active]); clamp_scroll(&doc[active]); set_title(); draw();
}

/* ---- art ------------------------------------------------------------------ */
static void build_art(void)
{
    if (art_ready) return;
    if (gs_init(&cmd_glyph, 8, 8) == 0) {
        gfx_target(&cmd_glyph); gfx_fill(0, 0, 8, 8, 0);
        gfx_frame(2, 2, 6, 6);
        gfx_fill_circle(1, 1, 1, 1); gfx_fill_circle(6, 1, 1, 1);
        gfx_fill_circle(1, 6, 1, 1); gfx_fill_circle(6, 6, 1, 1);
    }
    art_ready = 1;
    gfx_target(&win.surf);
}
static void draw_cmd(int x, int y) { if (cmd_glyph.bits) gfx_blit(&cmd_glyph, 0, 0, 8, 8, x, y, GFX_COPY); }
static void mini_folder(int x, int y)
{ gfx_frame(x, y+2, x+12, y+10); gfx_hline(x, x+5, y+1, 1); gfx_frame(x, y+1, x+5, y+3); }
static void mini_page(int x, int y)
{ gfx_frame(x+2, y, x+10, y+12); gfx_hline(x+4, x+8, y+4, 1); gfx_hline(x+4, x+8, y+7, 1); gfx_hline(x+4, x+8, y+10, 1); }

/* ---- menus ---------------------------------------------------------------- */
typedef struct { const char *label; char shortcut; int id; } MItem;
typedef struct { const char *title; MItem items[12]; int n; } MenuDef;
enum {
    A_NEW, A_OPEN, A_CLOSE, A_SAVE, A_SAVEAS, A_QUIT,
    A_UNDO, A_CUT, A_COPY, A_PASTE, A_SELALL
};
static const MenuDef menus[] = {
    { "File", { {"New",'N',A_NEW},{"Open...",'O',A_OPEN},{"Close",'W',A_CLOSE},{"-",0,0},
                {"Save",'S',A_SAVE},{"Save As...",0,A_SAVEAS},{"-",0,0},{"Quit",'Q',A_QUIT} }, 8 },
    { "Edit", { {"Undo",'Z',A_UNDO},{"-",0,0},{"Cut",'X',A_CUT},{"Copy",'C',A_COPY},
                {"Paste",'V',A_PASTE},{"-",0,0},{"Select All",'A',A_SELALL} }, 7 },
};
#define NMENU 2
static int menu_x[NMENU], menu_w[NMENU];

static void layout_menus(void)
{
    int x = 6;
    for (int m = 0; m < NMENU; m++) {
        menu_x[m] = x; menu_w[m] = gfx_text_width(menus[m].title) + 16;
        x += menu_w[m];
    }
}
static int menu_item_enabled(int m, int i)
{
    int id = menus[m].items[i].id;
    Doc *d = &doc[active];
    int s0, s1; sel_range(d, &s0, &s1);
    switch (id) {
    case A_NEW: return count_used() < NDOC;
    case A_CLOSE: return d->used;
    case A_SAVE: case A_SAVEAS: return d->used;
    case A_CUT: case A_COPY: return s1 > s0;
    case A_PASTE: return 1;
    case A_SELALL: return d->len > 0;
    case A_UNDO: return 0;
    default: return 1;
    }
}
static void draw_menubar(int open)
{
    gfx_fill(0, 0, WINW, MENU_B, 0);
    gfx_hline(0, WINW, MENU_B-1, 1);
    for (int m = 0; m < NMENU; m++) {
        int inv = m == open;
        if (inv) gfx_fill(menu_x[m], 1, menu_x[m]+menu_w[m], MENU_B-1, 1);
        gfx_text(menu_x[m]+8, 5, menus[m].title, inv);
    }
}
static int menu_geom(int m, int *l, int *t, int *r, int *b, int *iw)
{
    int wmax = 0;
    for (int i = 0; i < menus[m].n; i++) {
        const MItem *it = &menus[m].items[i];
        int w = gfx_text_width(it->label) + 44;
        if (it->shortcut) w += 18;
        if (w > wmax) wmax = w;
    }
    *l = menu_x[m] - 2; *t = MENU_B; *iw = wmax; *r = *l + wmax;
    int hh = 4;
    for (int i = 0; i < menus[m].n; i++) hh += menus[m].items[i].label[0]=='-' ? 7 : 15;
    *b = *t + hh;
    return hh;
}
static void draw_menu_open(int m, int hi)
{
    int l, t, r, b, iw;
    menu_geom(m, &l, &t, &r, &b, &iw);
    gfx_target(&win.surf);
    draw_menubar(m);
    gfx_fill(l+3, t+3, r+3, b+3, 1);
    gfx_fill(l, t, r, b, 0);
    gfx_frame(l, t, r, b);
    int y = t + 2;
    for (int i = 0; i < menus[m].n; i++) {
        const MItem *it = &menus[m].items[i];
        if (it->label[0] == '-') { gfx_pattern(l+2, y+3, r-2, y+4, 0xAA, 0xAA); y += 7; continue; }
        if (i == hi) gfx_fill(l+1, y, r-1, y+15, 1);
        int inv = i == hi;
        gfx_text(l+18, y+4, it->label, inv);
        if (it->shortcut) {
            int sx = r - 22;
            if (inv) gfx_fill(sx-1, y+3, sx+17, y+12, 0);
            draw_cmd(sx, y+3);
            char s[2] = { it->shortcut, 0 };
            gfx_text(sx+9, y+4, s, 0);
        }
        if (!menu_item_enabled(m, i)) gfx_dim(l+1, y+1, r-1, y+14);
        y += 15;
    }
    gwin_update(&win);
}
static int menu_item_at(int m, int mx, int my)
{
    int l, t, r, b, iw;
    menu_geom(m, &l, &t, &r, &b, &iw);
    if (!in_rect(mx, my, l, t, r, b)) return -1;
    int y = t + 2;
    for (int i = 0; i < menus[m].n; i++) {
        int hh = menus[m].items[i].label[0]=='-' ? 7 : 15;
        if (my >= y && my < y+hh) {
            if (menus[m].items[i].label[0]=='-' || !menu_item_enabled(m, i)) return -1;
            return i;
        }
        y += hh;
    }
    return -1;
}
static int menu_title_at(int mx, int my)
{
    if (my >= MENU_B) return -1;
    for (int m = 0; m < NMENU; m++) if (mx >= menu_x[m] && mx < menu_x[m]+menu_w[m]) return m;
    return -1;
}
static void menu_flash(int m, int i)
{
    for (int k = 0; k < 2; k++) {
        int l, t, r, b, iw; menu_geom(m, &l, &t, &r, &b, &iw);
        int y = t+2;
        for (int j = 0; j < i; j++) y += menus[m].items[j].label[0]=='-' ? 7 : 15;
        gfx_target(&win.surf);
        gfx_invert(l+1, y, r-1, y+15);
        gwin_update(&win);
        nap(3);
    }
}
static void menu_run(int m)
{
    int hi = -1, sticky = 0;
    draw_menu_open(m, hi);
    for (;;) {
        if (!sticky) {
            while (mouse_down()) {
                int mx = mouse_x() - origin_x, my = mouse_y() - origin_y;
                int mt = menu_title_at(mx, my);
                if (mt >= 0 && mt != m) { m = mt; hi = -1; draw_menu_open(m, hi); }
                int nh = menu_item_at(m, mx, my);
                if (nh != hi) { hi = nh; draw_menu_open(m, hi); }
                nap(1);
            }
            if (hi >= 0) { menu_flash(m, hi); draw(); run_action(menus[m].items[hi].id); return; }
            int mx = mouse_x() - origin_x, my = mouse_y() - origin_y;
            if (menu_title_at(mx, my) == m) { sticky = 1; continue; }
            draw(); return;
        } else {
            int mx = mouse_x() - origin_x, my = mouse_y() - origin_y;
            int nh = menu_item_at(m, mx, my);
            if (nh != hi) { hi = nh; draw_menu_open(m, hi); }
            GEvent e;
            if (gwin_event(&win, &e, 0) > 0) {
                if (e.type == GEV_CLOSE) { draw(); do_quit(); return; }
                if (e.type == GEV_KEY && e.key == KEY_ESC) { draw(); return; }
                if (e.type == GEV_MOUSEDOWN) {
                    int mt = menu_title_at(e.x, e.y);
                    if (mt >= 0 && mt != m) { m = mt; hi = -1; sticky = 0; draw_menu_open(m, hi); continue; }
                    int it = menu_item_at(m, e.x, e.y);
                    if (it >= 0) { menu_flash(m, it); draw(); run_action(menus[m].items[it].id); return; }
                    draw(); return;
                }
            }
            nap(1);
        }
    }
}

/* ---- text field widget (dialogs) ------------------------------------------ */
typedef struct { char *buf; int size; int len; int caret; int s0, s1; int x, y, w; } Field;
static void field_init(Field *f, char *buf, int size, int x, int y, int w, int select_all)
{
    f->buf = buf; f->size = size; f->len = (int)strlen(buf);
    f->caret = f->len; f->s0 = select_all ? 0 : f->len; f->s1 = f->len;
    f->x = x; f->y = y; f->w = w;
}
static void field_draw(Field *f)
{
    gfx_fill(f->x, f->y, f->x+f->w, f->y+14, 0);
    gfx_frame(f->x, f->y, f->x+f->w, f->y+14);
    int maxc = (f->w - 8) / 8;
    int off = 0;
    if (f->caret > maxc) off = f->caret - maxc;
    char vis[64]; int n = f->len - off; if (n > maxc) n = maxc; if (n < 0) n = 0;
    for (int i = 0; i < n; i++) vis[i] = f->buf[off+i];
    vis[n] = 0;
    int a = f->s0 - off, b = f->s1 - off;
    if (a < 0) a = 0;
    if (b > n) b = n;
    if (b > a) gfx_fill(f->x+4+a*8, f->y+3, f->x+4+b*8, f->y+11, 1);
    gfx_text(f->x+4, f->y+3, vis, 0);
    if (b > a) {
        char seg[64]; int sn = b-a; for (int i=0;i<sn;i++) seg[i]=vis[a+i]; seg[sn]=0;
        gfx_text(f->x+4+a*8, f->y+3, seg, 1);
    }
    if ((now()/24) & 1) gfx_fill(f->x+4+(f->caret-off)*8, f->y+2, f->x+5+(f->caret-off)*8, f->y+12, 1);
}
static void field_replace_sel(Field *f, const char *ins)
{
    int a = f->s0, b = f->s1, il = (int)strlen(ins);
    int newlen = f->len - (b-a) + il;
    if (newlen >= f->size) return;
    memmove(f->buf + a + il, f->buf + b, f->len - b + 1);
    memcpy(f->buf + a, ins, il);
    f->len = newlen; f->caret = a + il; f->s0 = f->s1 = f->caret;
}
static int field_key(Field *f, int key)
{
    if (key == KEY_BACKSPACE || key == KEY_DELETE) {
        if (f->s1 > f->s0) field_replace_sel(f, "");
        else if (key == KEY_BACKSPACE && f->caret > 0) { f->s0 = f->caret-1; f->s1 = f->caret; field_replace_sel(f, ""); }
        else if (key == KEY_DELETE && f->caret < f->len) { f->s0 = f->caret; f->s1 = f->caret+1; field_replace_sel(f, ""); }
        return 1;
    }
    if (key == KEY_LEFT) { f->caret = f->caret > 0 ? f->caret-1 : 0; if (!shift_down()) f->s0 = f->s1 = f->caret; else { if (f->caret < f->s0) f->s0 = f->caret; else f->s1 = f->caret; } return 1; }
    if (key == KEY_RIGHT) { f->caret = f->caret < f->len ? f->caret+1 : f->len; if (!shift_down()) f->s0 = f->s1 = f->caret; else { if (f->caret > f->s1) f->s1 = f->caret; else f->s0 = f->caret; } return 1; }
    if (key == KEY_HOME) { f->caret = 0; f->s0 = f->s1 = 0; return 1; }
    if (key == KEY_END) { f->caret = f->len; f->s0 = f->s1 = f->len; return 1; }
    if (key >= 0x20 && key < 0x7F && key != ':') { char s[2] = {(char)key, 0}; field_replace_sel(f, s); return 1; }
    return 0;
}

/* ---- dialogs -------------------------------------------------------------- */
static void dlg_shadow(int l, int t, int r, int b)
{
    gfx_fill(l+3, t+3, r+3, b+3, 1);
    gfx_fill_round_rect(l, t, r, b, 6, 0);
    gfx_round_rect(l, t, r, b, 6, 1);
}
static void dlg_button(int l, int t, int r, int b, const char *label, int dflt)
{
    gfx_fill_round_rect(l, t, r, b, 5, 0);
    gfx_round_rect(l, t, r, b, 5, 1);
    if (dflt) gfx_round_rect(l-2, t-2, r+2, b+2, 6, 1);
    gfx_text(l + (r-l-gfx_text_width(label))/2, t + (b-t-8)/2, label, 0);
}
static int dialog(const char *title, const char *body, const char *const *btns, int nbtns,
                  char *edit, int editsize, int select_all)
{
    int W = WINW, H = WINH;
    int bw[4]; int total = 0;
    for (int i = 0; i < nbtns; i++) { bw[i] = gfx_text_width(btns[i]) + 28; total += bw[i] + 10; }
    int tw = gfx_text_width(title), bw2 = body ? gfx_text_width(body) : 0;
    int w = maxi(maxi(tw, bw2), total) + 48;
    if (edit) w = maxi(w, 260);
    if (w > W - 32) w = W - 32;
    int h = 92 + (edit ? 24 : 0) + (body ? 12 : 0);
    int l = (W-w)/2, t = (H-h)/2, r = l+w, b = t+h;
    Field f; if (edit) field_init(&f, edit, editsize, l+24, t+44, w-48, select_all);
    int bx[4]; int x = r - 16;
    for (int i = nbtns-1; i >= 0; i--) { x -= bw[i]; bx[i] = x; x -= 10; }
    int by0 = b - 30, by1 = b - 12;
    int res = -1;
    for (;;) {
        gfx_target(&win.surf);
        draw();
        dlg_shadow(l, t, r, b);
        gfx_text(l + (w-tw)/2, t+14, title, 0);
        if (body) gfx_text(l + (w-bw2)/2, t+30, body, 0);
        if (edit) field_draw(&f);
        for (int i = 0; i < nbtns; i++)
            dlg_button(bx[i], by0, bx[i]+bw[i], by1, btns[i], i == nbtns-1);
        gwin_update(&win);
        GEvent e;
        if (gwin_event(&win, &e, 0) > 0) {
            if (e.type == GEV_CLOSE) { res = -1; break; }
            if (e.type == GEV_KEY) {
                if (e.key == KEY_RETURN || e.key == KEY_ENTER) { res = nbtns-1; break; }
                if (e.key == KEY_ESC) { res = -1; break; }
                if (edit) field_key(&f, e.key);
            } else if (e.type == GEV_MOUSEDOWN) {
                for (int i = 0; i < nbtns; i++)
                    if (in_rect(e.x, e.y, bx[i]-2, by0-2, bx[i]+bw[i]+2, by1+2)) { res = i; break; }
                if (res >= 0) break;
            }
        }
        nap(2);
    }
    draw();
    return res;
}
static void message(const char *l1, const char *l2)
{
    const char *b[] = {"OK"};
    dialog(l1, l2, b, 1, 0, 0, 0);
}

/* ---- graphical file browser (Open / Save As) ------------------------------ */
#define PICK_MAX  128
#define PICK_ROWS 9
#define PICK_ROWH 15
typedef struct { char name[32]; int is_dir; long id; INTEGER vref; } PEnt;
static PEnt    pent[PICK_MAX];
static int     npent, pk_scroll, pk_sel;
static INTEGER pk_vref;
static long    pk_dir;                 /* 0,0 = the disk list */
static char    pk_name[32];

static void pick_load_disks(void)
{
    npent = 0;
    for (int idx = 1; idx < 32 && npent < PICK_MAX; idx++) {
        HParamBlockRec pb; unsigned char nm[64];
        memset(&pb, 0, sizeof pb);
        pb.volumeParam.ioNamePtr = nm; pb.volumeParam.ioVolIndex = idx;
        if (PBHGetVInfoSync(&pb) != 0) break;
        PEnt *e = &pent[npent++];
        pstr_to_c(nm, e->name, sizeof e->name);
        e->is_dir = 1; e->id = pb.volumeParam.ioVRefNum; e->vref = pb.volumeParam.ioVRefNum;
    }
    pk_vref = 0; pk_dir = 0; pk_scroll = 0; pk_sel = 0;
}
static void pick_load_dir(INTEGER vref, long dir)
{
    npent = 0;
    for (int idx = 1; idx < 512 && npent < PICK_MAX; idx++) {
        CInfoPBRec pb; unsigned char nm[64];
        memset(&pb, 0, sizeof pb);
        pb.hFileInfo.ioNamePtr = nm; pb.hFileInfo.ioVRefNum = vref;
        pb.hFileInfo.ioDirID = dir; pb.hFileInfo.ioFDirIndex = idx;
        if (PBGetCatInfoSync(&pb) != 0) break;
        PEnt *e = &pent[npent++];
        pstr_to_c(nm, e->name, sizeof e->name);
        if (pb.hFileInfo.ioFlAttrib & 0x10) { e->is_dir = 1; e->id = pb.dirInfo.ioDrDirID; }
        else { e->is_dir = 0; e->id = 0; }
        e->vref = vref;
    }
    pk_vref = vref; pk_dir = dir; pk_scroll = 0; pk_sel = 0;
}
static void pick_path(char *out, int size)
{
    if (pk_vref == 0) { scopy(out, "Disks", size); return; }
    build_dir_path(pk_vref, pk_dir, out, size);
}
static void pick_up(void)
{
    if (pk_vref == 0) return;
    if (pk_dir == 2) pick_load_disks();
    else pick_load_dir(pk_vref, parent_dir(pk_vref, pk_dir));
}
static void pick_enter(int i, char *out, int outsize, int mode, int *done)
{
    if (i < 0 || i >= npent) return;
    PEnt *e = &pent[i];
    if (e->is_dir) {
        if (pk_vref == 0) pick_load_dir(e->vref, 2);
        else pick_load_dir(pk_vref, e->id);
        return;
    }
    if (mode == 0) {
        char dp[256]; pick_path(dp, sizeof dp);
        snprintf(out, outsize, "%s%s", dp, e->name);
        *done = 1;
    } else {
        scopy(pk_name, e->name, sizeof pk_name);
    }
}
static int pick_max_scroll(void) { int m = npent - PICK_ROWS; return m > 0 ? m : 0; }

static void pick_draw(int mode, Field *f, int dl, int dt, int dr, int db,
                      int ll, int lt, int lr, int lb, int sl, int sr)
{
    gfx_target(&win.surf);
    draw();
    dlg_shadow(dl, dt, dr, db);
    const char *title = mode ? "Save As" : "Open";
    gfx_text(dl + (dr-dl-gfx_text_width(title))/2, dt+12, title, 0);
    char pp[256]; pick_path(pp, sizeof pp);
    char tp[64]; trunc_label(pp, tp, (dr-90)-(dl+14));
    gfx_text(dl+14, dt+28, tp, 0);
    dlg_button(dr-72, dt+22, dr-14, dt+38, "Up", 0);
    gfx_frame(ll-1, lt-1, lr+1, lb+1);
    gfx_fill(ll, lt, lr, lb, 0);
    for (int r = 0; r < PICK_ROWS; r++) {
        int i = pk_scroll + r;
        if (i >= npent) break;
        int y = lt + r*PICK_ROWH;
        int selrow = (i == pk_sel);
        if (selrow) gfx_fill(ll, y, lr, y+PICK_ROWH, 1);
        if (pent[i].is_dir) mini_folder(ll+3, y+2);
        else mini_page(ll+3, y+1);
        char lbl[40]; trunc_label(pent[i].name, lbl, lr-ll-24);
        gfx_text(ll+18, y+3, lbl, selrow);
    }
    gfx_frame(sl, lt, sr, lb);
    gfx_fill(sl+1, lt+1, sr-1, lb-1, 0);
    int span = (lb-1)-(lt+1);
    if (npent > PICK_ROWS) {
        int th = span * PICK_ROWS / npent; if (th < 12) th = 12;
        int ms = pick_max_scroll();
        int ty = lt+1 + (ms ? (span-th)*pk_scroll/ms : 0);
        gfx_fill(sl+1, ty, sr-1, ty+th, 1);
    } else {
        gfx_fill(sl+1, lt+1, sr-1, lb-1, 1);
    }
    if (mode == 1) {
        gfx_text(dl+14, db-44, "Name:", 0);
        field_draw(f);
        dlg_button(dr-146, db-30, dr-80, db-12, "Save", 1);
    } else {
        dlg_button(dr-146, db-30, dr-80, db-12, "Open", 1);
    }
    dlg_button(dr-72, db-30, dr-14, db-12, "Cancel", 0);
    gwin_update(&win);
}

/* mode 0 = Open (returns a chosen file path), 1 = Save As (dir + typed name).
 * Returns 1 and fills out on success, 0 on cancel. */
static int pick_run(char *out, int outsize, int mode)
{
    pick_load_disks();
    pk_name[0] = 0;
    int W = WINW, H = WINH;
    int w = 384, h = 232;
    int dl = (W-w)/2, dt = (H-h)/2, dr = dl+w, db = dt+h;
    int ll = dl+14, lt = dt+46, lr = dr-30, lb = lt + PICK_ROWS*PICK_ROWH;
    int sl = dr-26, sr = dr-14;
    Field f; field_init(&f, pk_name, sizeof pk_name, dl+58, db-29, (dr-156)-(dl+58), 1);
    int done = 0, res = 0;
    uint32_t last_t = 0; int last_i = -1;
    for (;;) {
        if (mode == 1) { f.buf = pk_name; f.len = (int)strlen(pk_name); }
        pick_draw(mode, &f, dl, dt, dr, db, ll, lt, lr, lb, sl, sr);
        GEvent e;
        if (gwin_event(&win, &e, 0) > 0) {
            if (e.type == GEV_CLOSE) break;
            if (e.type == GEV_KEY) {
                if (e.key == KEY_ESC) break;
                if (mode == 1 && e.key != KEY_RETURN && e.key != KEY_ENTER) { field_key(&f, e.key); continue; }
                if (e.key == KEY_RETURN || e.key == KEY_ENTER) {
                    if (mode == 1) {
                        if (pk_vref != 0 && pk_name[0]) {
                            char dp[256]; pick_path(dp, sizeof dp);
                            snprintf(out, outsize, "%s%s", dp, pk_name);
                            res = 1; break;
                        }
                    } else { pick_enter(pk_sel, out, outsize, mode, &done); if (done) { res = 1; break; } }
                    continue;
                }
                if (e.key == KEY_UP) { if (pk_sel > 0) pk_sel--; if (pk_sel < pk_scroll) pk_scroll = pk_sel; continue; }
                if (e.key == KEY_DOWN) { if (pk_sel < npent-1) pk_sel++; if (pk_sel >= pk_scroll+PICK_ROWS) pk_scroll = pk_sel-PICK_ROWS+1; continue; }
            } else if (e.type == GEV_MOUSEDOWN) {
                int mx = e.x, my = e.y;
                origin_x = mouse_x() - mx; origin_y = mouse_y() - my;
                if (in_rect(mx, my, dr-72, dt+22, dr-14, dt+38)) { pick_up(); continue; }
                if (in_rect(mx, my, dr-72, db-30, dr-14, db-12)) break;          /* Cancel */
                if (in_rect(mx, my, dr-146, db-30, dr-80, db-12)) {               /* Open / Save */
                    if (mode == 1) {
                        if (pk_vref != 0 && pk_name[0]) {
                            char dp[256]; pick_path(dp, sizeof dp);
                            snprintf(out, outsize, "%s%s", dp, pk_name);
                            res = 1; break;
                        }
                    } else { pick_enter(pk_sel, out, outsize, mode, &done); if (done) { res = 1; break; } }
                    continue;
                }
                if (mode == 1 && in_rect(mx, my, f.x, f.y, f.x+f.w, f.y+14)) {
                    int c = (mx - f.x - 4 + 4) / 8; if (c < 0) c = 0; if (c > f.len) c = f.len;
                    f.caret = c; f.s0 = f.s1 = c; continue;
                }
                if (in_rect(mx, my, sl, lt, sr, lb)) {                            /* scrollbar */
                    int span = (lb-1)-(lt+1);
                    int th = npent > PICK_ROWS ? span*PICK_ROWS/npent : span; if (th < 12) th = 12;
                    int ms = pick_max_scroll();
                    int ty = lt+1 + (ms ? (span-th)*pk_scroll/ms : 0);
                    if (my < ty) pk_scroll -= PICK_ROWS;
                    else if (my >= ty+th) pk_scroll += PICK_ROWS;
                    else {
                        int grab = my - ty;
                        while (mouse_down()) {
                            int my2 = mouse_y() - origin_y;
                            int nty = my2 - grab;
                            if (nty < lt+1) nty = lt+1;
                            if (nty > lb-1-th) nty = lb-1-th;
                            pk_scroll = (ms && span>th) ? (nty-(lt+1))*ms/(span-th) : 0;
                            if (pk_scroll < 0) pk_scroll = 0; if (pk_scroll > ms) pk_scroll = ms;
                            nap(1);
                        }
                        continue;
                    }
                    if (pk_scroll < 0) pk_scroll = 0; if (pk_scroll > ms) pk_scroll = ms;
                    continue;
                }
                if (in_rect(mx, my, ll, lt, lr, lb)) {                            /* list */
                    int i = pk_scroll + (my - lt)/PICK_ROWH;
                    if (i >= 0 && i < npent) {
                        uint32_t t = now();
                        int dbl = (i == last_i && t - last_t < 32);
                        last_i = i; last_t = t;
                        pk_sel = i;
                        if (dbl) { last_i = -1; pick_enter(i, out, outsize, mode, &done); if (done) { res = 1; break; } }
                    }
                    continue;
                }
            }
        }
        nap(2);
    }
    draw();
    return res;
}

/* ---- actions -------------------------------------------------------------- */
static void do_new(void) { if (new_doc() < 0) message("No more tabs.", "Close a document to open another."); draw(); }
static void do_open(void)
{
    char path[160];
    if (!pick_run(path, sizeof path, 0)) return;
    for (int i = 0; i < NDOC; i++)
        if (doc[i].used && doc[i].path[0] && ieq(doc[i].path, path)) { activate(i); return; }
    int slot = -1;
    for (int k = 0; k < NDOC; k++) if (!doc[k].used) { slot = k; break; }
    if (slot < 0) { message("No more tabs.", "Close a document to open another."); return; }
    Doc *d = &doc[slot];
    memset(d, 0, sizeof *d);
    if (load_doc(d, path) != 0) { message("Could not open the file.", path_leaf(path)); doc_free(d); return; }
    d->used = 1;
    active = slot;
    rebuild_lines(d); set_title(); draw();
}
static void do_save(void) { if (save_doc_interactive(active)) draw(); }
static void do_saveas(void)
{
    char path[160];
    if (!pick_run(path, sizeof path, 1)) return;
    Doc *d = &doc[active];
    scopy(d->path, path, sizeof d->path);
    d->vref = resolve_vref(path);
    scopy(d->name, path_leaf(path), sizeof d->name);
    if (save_doc(d) != 0) message("Could not save the file.", "The disk may be full or locked.");
    set_title(); draw();
}
static void do_cut(void)
{
    Doc *d = &doc[active];
    int s0, s1; sel_range(d, &s0, &s1);
    if (s1 <= s0) return;
    scrap_set_text(d->buf + s0, s1 - s0);
    doc_delete_range(d, s0, s1);
    draw();
}
static void do_copy(void)
{
    Doc *d = &doc[active];
    int s0, s1; sel_range(d, &s0, &s1);
    if (s1 <= s0) return;
    scrap_set_text(d->buf + s0, s1 - s0);
}
static void do_paste(void)
{
    int n = scrap_get_text(paste_buf, sizeof paste_buf);
    if (n <= 0) return;
    if (!doc_insert(&doc[active], paste_buf, n)) message("The document is full.", "");
    draw();
}
static void do_select_all(void)
{
    Doc *d = &doc[active];
    d->anchor = 0; d->caret = d->len;
    draw();
}
static void do_quit(void)
{
    for (int i = 0; i < NDOC; i++) {
        if (doc[i].used && doc[i].modified) {
            active = i; rebuild_lines(&doc[i]); set_title(); draw();
            char msg[80]; snprintf(msg, sizeof msg, "Save changes to \"%s\"?", doc[i].name);
            const char *b[] = { "Don't Save", "Cancel", "Save" };
            int res = dialog("TextEdit", msg, b, 3, 0, 0, 0);
            if (res == 1 || res < 0) { draw(); return; }
            if (res == 2 && !save_doc_interactive(i)) { draw(); return; }
        }
    }
    want_quit = 1;
}
static void run_action(int id)
{
    switch (id) {
    case A_NEW: do_new(); return;
    case A_OPEN: do_open(); return;
    case A_CLOSE: close_doc(active); return;
    case A_SAVE: do_save(); return;
    case A_SAVEAS: do_saveas(); return;
    case A_QUIT: do_quit(); return;
    case A_CUT: do_cut(); return;
    case A_COPY: do_copy(); return;
    case A_PASTE: do_paste(); return;
    case A_SELALL: do_select_all(); return;
    default: return;
    }
}

/* ---- drawing -------------------------------------------------------------- */
static int tab_width(void)
{
    int nu = count_used(); if (nu < 1) nu = 1;
    int tw = (WINW - 28) / nu;
    if (tw > 96) tw = 96;
    if (tw < 30) tw = 30;
    return tw;
}
static int tab_index_of(int slot) { int k=0; for (int i=0;i<slot;i++) if (doc[i].used) k++; return k; }
static void tab_geom(int slot, int *l, int *t, int *r, int *b)
{
    int tw = tab_width();
    int k = tab_index_of(slot);
    *l = 2 + k*tw; *r = *l + tw - 2;
    *t = TAB_T + (slot == active ? 1 : 3); *b = TAB_B;
}
static void draw_tabs(void)
{
    gfx_fill(0, TAB_T, WINW, TAB_B, 0);
    gfx_hline(0, WINW, TAB_B-1, 1);
    for (int i = 0; i < NDOC; i++) {
        if (!doc[i].used) continue;
        int l, t, r, b; tab_geom(i, &l, &t, &r, &b);
        int act = (i == active);
        if (act) {
            gfx_fill(l, t, r, b, 0);
            gfx_vline(l, t, b, 1); gfx_vline(r-1, t, b, 1); gfx_hline(l, r, t, 1);
            gfx_hline(l+1, r-1, TAB_B-1, 0);
        } else {
            gfx_dither_rect(l, t, r, b-1, 4);
            gfx_frame(l, t, r, b);
        }
        char lbl[32]; trunc_label(doc[i].name, lbl, (r-l)-24);
        gfx_text(l+5, t+5, lbl, 0);
        if (doc[i].modified) gfx_fill(l+3, t+7, l+5, t+9, 1);
        int bx = r-14, by = t+4;
        gfx_frame(bx, by, bx+9, by+9);
        gfx_line(bx+2, by+2, bx+6, by+6, 1);
        gfx_line(bx+6, by+2, bx+2, by+6, 1);
    }
    int pl = WINW-24, pt = TAB_T+3, pr = WINW-3, pb = TAB_B-2;
    gfx_fill_round_rect(pl, pt, pr, pb, 3, 0);
    gfx_round_rect(pl, pt, pr, pb, 3, 1);
    gfx_hline(pl+5, pr-5, (pt+pb)/2, 1);
    gfx_vline((pl+pr)/2, pt+4, pb-4, 1);
}
static void draw_text(void)
{
    Doc *d = &doc[active];
    gfx_clip(TEXTX-2, CT, SBX-1, CB);
    int cl, cc; caret_linecol(d, d->caret, &cl, &cc);
    int s0, s1; sel_range(d, &s0, &s1);
    int blink = (now()/24) & 1;
    char seg[MAXCOLS+1];
    for (int r = 0; r < VISLINES; r++) {
        int li = d->scroll + r;
        if (li >= nlines) break;
        int y = CT + r*LINEH;
        int off = lines[li].off, ln = lines[li].len;
        for (int c = 0; c < ln; c++) seg[c] = d->buf[off+c];
        seg[ln] = 0;
        int a = s0 - off, b = s1 - off;
        if (a < 0) a = 0; if (a > ln) a = ln;
        if (b < 0) b = 0; if (b > ln) b = ln;
        int has_sel = (s1 > s0 && b > a);
        if (!has_sel) {
            gfx_text(TEXTX, y+2, seg, 0);
        } else {
            if (a > 0) { char sv = seg[a]; seg[a] = 0; gfx_text(TEXTX, y+2, seg, 0); seg[a] = sv; }
            gfx_fill(TEXTX+a*8, y+1, TEXTX+b*8, y+LINEH-1, 1);
            char t[MAXCOLS+1]; int sn = b-a;
            for (int i=0;i<sn;i++) t[i]=seg[a+i]; t[sn]=0;
            gfx_text(TEXTX+a*8, y+2, t, 1);
            if (b < ln) { char u[MAXCOLS+1]; int un=ln-b; for (int i=0;i<un;i++) u[i]=seg[b+i]; u[un]=0; gfx_text(TEXTX+b*8, y+2, u, 0); }
        }
        if (li == cl && blink) {
            int cx = TEXTX + cc*8;
            gfx_invert(cx, y+1, cx+1, y+LINEH-1);
        }
    }
    gfx_noclip();
}
static void draw_scrollbar(void)
{
    gfx_frame(SBX, CT, WINW, CB);
    gfx_fill(SBX+1, CT+1, WINW-1, CB-1, 0);
    gfx_hline(SBX, WINW, CT+SBW, 1);
    gfx_hline(SBX, WINW, CB-SBW, 1);
    int cx = SBX + SBW/2;
    int p1[] = {cx-4, CT+11, cx+4, CT+11, cx, CT+5};
    gfx_poly_fill(p1, 3, 1);
    int p2[] = {cx-4, CB-11, cx+4, CB-11, cx, CB-5};
    gfx_poly_fill(p2, 3, 1);
    int tt = CT+SBW+1, tb = CB-SBW-1, span = tb-tt;
    if (nlines > VISLINES) {
        int th = span * VISLINES / nlines; if (th < 12) th = 12;
        int ms = max_scroll();
        int ty = tt + (ms ? (span-th)*doc[active].scroll/ms : 0);
        gfx_fill(SBX+1, ty, WINW-1, ty+th, 1);
    } else {
        gfx_dither_rect(SBX+1, tt, WINW-1, tb, 8);
    }
}
static void draw_status(void)
{
    Doc *d = &doc[active];
    gfx_fill(0, STATUS_T, WINW, WINH, 0);
    gfx_hline(0, WINW, STATUS_T, 1);
    int cl, cc; caret_linecol(d, d->caret, &cl, &cc);
    char left[40]; snprintf(left, sizeof left, "Ln %d, Col %d", cl+1, cc+1);
    gfx_text(8, STATUS_T+6, left, 0);
    char right[48]; snprintf(right, sizeof right, "%d chars", d->len);
    gfx_text(WINW - 8 - gfx_text_width(right), STATUS_T+6, right, 0);
    char mid[64];
    snprintf(mid, sizeof mid, "%s%s", d->name, d->modified ? "  (modified)" : "");
    int mw = gfx_text_width(mid);
    int mx = (WINW - mw)/2;
    if (mx > 150) gfx_text(mx, STATUS_T+6, mid, 0);
}
static void draw(void)
{
    gfx_target(&win.surf);
    gfx_fill(0, 0, WINW, WINH, 0);
    draw_menubar(-1);
    draw_tabs();
    draw_text();
    draw_scrollbar();
    draw_status();
    gwin_update(&win);
}

/* ---- scrolling ------------------------------------------------------------ */
static void scroll_by(int dl)
{
    Doc *d = &doc[active];
    d->scroll += dl; clamp_scroll(d); draw();
}
static void scrollbar_run(int cy)
{
    Doc *d = &doc[active];
    if (cy < CT+SBW) scroll_by(-1);
    else if (cy >= CB-SBW) scroll_by(1);
    else {
        int tt = CT+SBW+1, tb = CB-SBW-1, span = tb-tt;
        int th = nlines > VISLINES ? span*VISLINES/nlines : span; if (th < 12) th = 12;
        int ms = max_scroll();
        int ty = tt + (ms ? (span-th)*d->scroll/ms : 0);
        if (cy < ty) scroll_by(-VISLINES);
        else if (cy >= ty+th) scroll_by(VISLINES);
        else {
            int grab = cy - ty;
            while (mouse_down()) {
                int my = mouse_y() - origin_y;
                int nty = my - grab;
                if (nty < tt) nty = tt;
                if (nty > tb-th) nty = tb-th;
                d->scroll = (ms && span>th) ? (nty-tt)*ms/(span-th) : 0;
                clamp_scroll(d); draw();
                nap(1);
            }
            return;
        }
    }
    uint32_t last = now();
    while (mouse_down()) {
        int my = mouse_y() - origin_y;
        int dir = 0;
        if (my < CT+SBW) dir = -1; else if (my >= CB-SBW) dir = 1;
        if (dir && now() - last > 8) { scroll_by(dir); last = now(); }
        nap(1);
    }
}

/* ---- mouse ---------------------------------------------------------------- */
static uint32_t last_click_t; static int last_click_off = -1;
static void text_drag(void)
{
    Doc *d = &doc[active];
    while (mouse_down()) {
        int mx = mouse_x() - origin_x, my = mouse_y() - origin_y;
        if (my < CT && d->scroll > 0) d->scroll--;
        else if (my >= CB && d->scroll < max_scroll()) d->scroll++;
        int li = d->scroll + (my - CT)/LINEH;
        int col = (mx - TEXTX + 4)/8;
        int off = offset_at(li, col);
        if (off != d->caret) { set_caret(d, off, 1); ensure_visible(d); draw(); }
        nap(1);
    }
}
static void on_mousedown(int cx, int cy)
{
    origin_x = mouse_x() - cx; origin_y = mouse_y() - cy;
    if (cy < MENU_B) { int m = menu_title_at(cx, cy); if (m >= 0) menu_run(m); return; }
    if (cy < TAB_B) {
        if (cx >= WINW-24 && cx < WINW-2) { do_new(); return; }
        for (int i = 0; i < NDOC; i++) {
            if (!doc[i].used) continue;
            int l, t, r, b; tab_geom(i, &l, &t, &r, &b);
            if (cx >= l && cx < r) {
                if (cx >= r-14 && cx < r-5 && cy >= t+4 && cy < t+13) { close_doc(i); return; }
                if (i != active) activate(i);
                return;
            }
        }
        return;
    }
    if (cx >= SBX) { if (cy >= CT && cy < CB) scrollbar_run(cy); return; }
    if (cy >= CB) return;
    if (cy >= CT) {
        Doc *d = &doc[active];
        int li = d->scroll + (cy - CT)/LINEH;
        int col = (cx - TEXTX + 4)/8;
        int off = offset_at(li, col);
        uint32_t t = now();
        int dbl = (t - last_click_t < 32 && off == last_click_off);
        last_click_t = t; last_click_off = off;
        if (dbl) { last_click_off = -1; select_word(d, off); draw(); while (mouse_down()) nap(1); return; }
        set_caret(d, off, shift_down());
        ensure_visible(d);
        draw();
        text_drag();
        return;
    }
}

/* ---- keyboard ------------------------------------------------------------- */
static void move_line(Doc *d, int dir, int extend)
{
    int cl, cc; caret_linecol(d, d->caret, &cl, &cc);
    int nl = cl + dir;
    if (nl < 0) set_caret(d, 0, extend);
    else if (nl >= nlines) set_caret(d, d->len, extend);
    else set_caret(d, offset_at(nl, cc), extend);
    ensure_visible(d);
}
static void on_key(int key)
{
    Doc *d = &doc[active];
    int cmd = cmd_down(), shift = shift_down();
    if (cmd) {
        switch (key) {
        case 'n': case 'N': do_new(); return;
        case 'o': case 'O': do_open(); return;
        case 'w': case 'W': close_doc(active); return;
        case 's': case 'S': do_save(); return;
        case 'q': case 'Q': do_quit(); return;
        case 'x': case 'X': do_cut(); return;
        case 'c': case 'C': do_copy(); return;
        case 'v': case 'V': do_paste(); return;
        case 'a': case 'A': do_select_all(); return;
        }
        if (key == KEY_UP || key == KEY_LEFT) { set_caret(d, 0, shift); ensure_visible(d); draw(); return; }
        if (key == KEY_DOWN || key == KEY_RIGHT) { set_caret(d, d->len, shift); ensure_visible(d); draw(); return; }
    }
    switch (key) {
    case KEY_LEFT: set_caret(d, d->caret-1, shift); ensure_visible(d); draw(); return;
    case KEY_RIGHT: set_caret(d, d->caret+1, shift); ensure_visible(d); draw(); return;
    case KEY_UP: move_line(d, -1, shift); draw(); return;
    case KEY_DOWN: move_line(d, 1, shift); draw(); return;
    case KEY_HOME: { int cl,cc; caret_linecol(d,d->caret,&cl,&cc); set_caret(d, offset_at(cl,0), shift); ensure_visible(d); draw(); return; }
    case KEY_END: { int cl,cc; caret_linecol(d,d->caret,&cl,&cc); set_caret(d, offset_at(cl,lines[cl].len), shift); ensure_visible(d); draw(); return; }
    case KEY_PGUP: move_line(d, -VISLINES, shift); draw(); return;
    case KEY_PGDN: move_line(d, VISLINES, shift); draw(); return;
    case KEY_RETURN: case KEY_ENTER: doc_insert(d, "\n", 1); draw(); return;
    case KEY_TAB: doc_insert(d, "    ", 4); draw(); return;
    case KEY_BACKSPACE: doc_backspace(d); draw(); return;
    case KEY_DELETE: doc_delete_fwd(d); draw(); return;
    case KEY_ESC: d->anchor = d->caret; draw(); return;
    }
    if (key >= 0x20 && key < 0x7F) { char s[2] = {(char)key, 0}; doc_insert(d, s, 1); draw(); }
}

/* ---- startup -------------------------------------------------------------- */
static int under_desktop(void)
{
    struct plusos_procinfo p[16];
    struct plusos_proclist_args a = { p, 16 };
    int n = (int)plusos_syscall(SYS_proclist, &a);
    for (int i = 0; i < n; i++)
        if (p[i].state != 4 && !strncmp(p[i].name, "desktop", 7)) return 1;
    return 0;
}

int main(int argc, char **argv)
{
    if (!under_desktop()) {
        if (argc >= 2) {
            Doc d; memset(&d, 0, sizeof d);
            if (load_doc(&d, argv[1]) == 0) { fwrite(d.buf, 1, (size_t)d.len, stdout); doc_free(&d); }
            else printf("cannot open %s\n", argv[1]);
        } else {
            printf("TextEdit is a desktop program - open it from the desktop.\n");
        }
        return 0;
    }
    if (gwin_open(&win, "TextEdit", WINW, WINH) != 0) return 1;
    build_art();
    layout_menus();
    gfx_target(&win.surf);

    memset(doc, 0, sizeof doc);
    if (argc >= 2 && argv[1] && argv[1][0]) {
        Doc *d = &doc[0];
        if (load_doc(d, argv[1]) == 0) { d->used = 1; active = 0; }
        else { new_doc(); message("Could not open the file.", path_leaf(argv[1])); }
    } else {
        new_doc();
    }
    rebuild_lines(&doc[active]);
    set_title();
    draw();

    int last_blink = -1;
    for (;;) {
        if (want_quit) break;
        GEvent e;
        int r = gwin_event(&win, &e, 0);
        if (r < 0) break;
        if (r > 0) {
            if (e.type == GEV_CLOSE) { do_quit(); if (want_quit) break; }
            else if (e.type == GEV_MOUSEDOWN) on_mousedown(e.x, e.y);
            else if (e.type == GEV_KEY) on_key(e.key);
            last_blink = (now()/24) & 1;
        } else {
            int blink = (now()/24) & 1;
            if (blink != last_blink) { last_blink = blink; draw(); }
            nap(2);
        }
    }
    for (int i = 0; i < NDOC; i++) if (doc[i].used) doc_free(&doc[i]);
    if (lines) free(lines);
    gwin_close(&win);
    return 0;
}