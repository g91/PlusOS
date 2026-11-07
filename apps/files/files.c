/* files - a graphical file and disk explorer for the PlusOS desktop.
 *
 * Opens a window and lists the mounted disks; open a disk to see its files and
 * folders, open a folder to go in, use the path bar / Up button to go back.
 * Opening a program runs it; opening any other file opens it in the hex editor.
 * It is a GUI program: it draws into its window with PlusGFX and never touches
 * the console. Run without a desktop it says so and exits.
 */
#include <plusos/plusos.h>
#include <plusos/syscall.h>
#include <plusos/gfx.h>
#include <plusos/thread.h>
#include <plusrom/FileMgr.h>

#define WINW 360
#define WINH 272
#define ROWH 13
#define BTN_T 40
#define BTN_B 58
#define LIST_TOP 66
#define LIST_BOT (WINH - 26)
#define VISROWS ((LIST_BOT - LIST_TOP) / ROWH)

typedef struct { char name[32]; int is_dir; long id; unsigned long size; } Entry;

static GWindow win;
static Entry   entries[128];
static int     nent, sel, scroll;
static INTEGER cur_vref;
static long    cur_dir;             /* 0 = the list of disks */
static char    path[96];

/* toolbar action buttons */
enum { B_NEW, B_REN, B_DEL, B_MOVE, B_PASTE, NBTN };
static const char *btn_label[NBTN] = { "New Folder", "Rename", "Delete", "Move", "Paste" };
static int btn_x0[NBTN], btn_x1[NBTN];

/* "Move": the item put aside by Move, dropped into a folder by Paste */
static int     mv_active;
static INTEGER mv_vref;
static long    mv_dir;
static char    mv_name[32];

static int in_rect(int x, int y, int l, int t, int r, int b) { return x >= l && x < r && y >= t && y < b; }

static void c2p(unsigned char *p, const char *c) { int n = 0; while (c[n] && n < 254) { p[n+1] = (unsigned char)c[n]; n++; } p[0] = (unsigned char)n; }
static int ieq(const char *a, const char *b) { for (;; a++, b++) { int x=*a,y=*b; if(x>='A'&&x<='Z')x+=32; if(y>='A'&&y<='Z')y+=32; if(x!=y)return 0; if(!x)return 1; } }
static void scopy(char *d, const char *s, int n) { int i=0; for(;s[i]&&i<n-1;i++)d[i]=s[i]; d[i]=0; }

/* ---- read the current level into entries[] ---- */
static int ent_cmp(const Entry *a, const Entry *b)
{
    if (a->is_dir != b->is_dir) return b->is_dir - a->is_dir;    /* folders first */
    const char *x = a->name, *y = b->name;
    for (;; x++, y++) { int cx=*x,cy=*y; if(cx>='A'&&cx<='Z')cx+=32; if(cy>='A'&&cy<='Z')cy+=32; if(cx!=cy)return cx-cy; if(!cx)return 0; }
}

static void sort_entries(void)
{
    for (int i = 1; i < nent; i++) {               /* insertion sort: lists are short */
        Entry e = entries[i]; int j = i - 1;
        while (j >= 0 && ent_cmp(&entries[j], &e) > 0) { entries[j+1] = entries[j]; j--; }
        entries[j+1] = e;
    }
}

/* `path` is a real HFS path: "" at the disk list, "Vol:" at a disk root,
 * "Vol:folder:" inside a folder - so a file opens as path + its name. */
static void path_push(const char *comp) { int n = (int)strlen(path); snprintf(path + n, sizeof path - n, "%s:", comp); }
static void path_pop(void)
{
    int n = (int)strlen(path);
    if (n == 0) return;
    path[--n] = 0;                                  /* drop trailing ':' */
    while (n > 0 && path[n-1] != ':') n--;
    path[n] = 0;
}

static void load_disks(void)
{
    nent = 0;
    for (int idx = 1; idx < 32 && nent < 128; idx++) {
        HParamBlockRec pb; unsigned char nm[64];
        memset(&pb, 0, sizeof pb);
        pb.volumeParam.ioNamePtr = nm;
        pb.volumeParam.ioVolIndex = idx;
        if (PBHGetVInfoSync(&pb) != 0) break;
        Entry *e = &entries[nent++];
        int k = nm[0] > 31 ? 31 : nm[0];
        for (int i = 0; i < k; i++) e->name[i] = (char)nm[i+1];
        e->name[k] = 0;
        e->is_dir = 2;                             /* 2 = a disk */
        e->id = pb.volumeParam.ioVRefNum;
        e->size = (unsigned long)pb.volumeParam.ioVNmAlBlks * (unsigned long)pb.volumeParam.ioVAlBlkSiz;
    }
    cur_dir = 0;
    path[0] = 0;
}

static void load_dir(INTEGER vref, long dir)
{
    nent = 0;
    for (int idx = 1; idx < 400 && nent < 128; idx++) {
        CInfoPBRec pb; unsigned char nm[64];
        memset(&pb, 0, sizeof pb);
        pb.hFileInfo.ioNamePtr = nm;
        pb.hFileInfo.ioVRefNum = vref;
        pb.hFileInfo.ioDirID = dir;
        pb.hFileInfo.ioFDirIndex = idx;
        if (PBGetCatInfoSync(&pb) != 0) break;
        Entry *e = &entries[nent++];
        int k = nm[0] > 31 ? 31 : nm[0];
        for (int i = 0; i < k; i++) e->name[i] = (char)nm[i+1];
        e->name[k] = 0;
        e->is_dir = (pb.hFileInfo.ioFlAttrib & 0x10) != 0;
        e->id = e->is_dir ? pb.dirInfo.ioDrDirID : 0;
        e->size = e->is_dir ? 0 : (unsigned long)pb.hFileInfo.ioFlLgLen;
    }
    sort_entries();
    cur_vref = vref; cur_dir = dir;
}

/* ---- drawing ---- */
static int btn_enabled(int i)
{
    int have = sel >= 0 && sel < nent && entries[sel].is_dir != 2;   /* not a disk */
    switch (i) {
    case B_NEW:   return cur_dir >= 2;                                /* inside a volume */
    case B_REN:
    case B_DEL:
    case B_MOVE:  return have;
    case B_PASTE: return mv_active && cur_dir >= 2;
    }
    return 1;
}

static void draw_buttons(void)
{
    int x = 6;
    for (int i = 0; i < NBTN; i++) {
        int w = gfx_text_width(btn_label[i]) + 14;
        btn_x0[i] = x; btn_x1[i] = x + w;
        gfx_frame(x, BTN_T, x + w, BTN_B);
        gfx_text(x + 7, BTN_T + 5, btn_label[i], 0);
        if (!btn_enabled(i)) gfx_dim(x + 1, BTN_T + 1, x + w - 1, BTN_B - 1);
        x += w + 4;
    }
}

static void draw(void)
{
    gfx_target(&win.surf);
    int W = win.surf.w, H = win.surf.h;
    gfx_fill(0, 0, W, H, 0);
    /* toolbar: path row + action buttons */
    gfx_fill(0, 0, W, LIST_TOP - 6, 0);
    gfx_frame(6, 6, 54, 24);
    gfx_text(16, 11, "Up", 0);
    gfx_frame(60, 6, W - 8, 24);
    const char *pp = path[0] ? path : "Disks";
    int maxc = (W - 76) / 8;                          /* show the tail if it's long */
    if ((int)strlen(pp) > maxc) pp += strlen(pp) - maxc;
    gfx_text(66, 11, pp, 0);
    draw_buttons();
    gfx_hline(0, W, LIST_TOP - 5, 1);
    /* list */
    gfx_fill(0, LIST_TOP - 2, W, LIST_BOT, 0);
    for (int i = 0; i < VISROWS; i++) {
        int idx = scroll + i;
        if (idx >= nent) break;
        Entry *e = &entries[idx];
        int y = LIST_TOP + i * ROWH;
        if (idx == sel) gfx_fill(4, y - 1, W - 12, y + ROWH - 2, 1);
        int inv = idx == sel;
        int ix = 10;
        /* tiny type glyph */
        if (e->is_dir == 2) { gfx_frame(ix, y, ix + 11, y + 8); gfx_fill(ix + 2, y + 2, ix + 9, y + 4, !inv); }   /* disk */
        else if (e->is_dir) { gfx_frame(ix, y + 1, ix + 12, y + 9); gfx_hline(ix, ix + 6, y, 1); }                 /* folder */
        else { gfx_frame(ix + 1, y, ix + 10, y + 9); }                                                            /* file */
        char nm[34]; scopy(nm, e->name, 28);
        gfx_text(ix + 18, y, nm, inv);
        if (e->is_dir != 1) {
            char s[16];
            unsigned long kb = e->size >> 10;
            snprintf(s, sizeof s, "%luK", kb ? kb : (e->size ? 1 : 0));
            if (e->is_dir == 0 || e->is_dir == 2) gfx_text(W - 14 - gfx_text_width(s), y, s, inv);
        }
    }
    /* scrollbar */
    if (nent > VISROWS) {
        int sx = W - 10;
        gfx_frame(sx, LIST_TOP - 2, sx + 8, LIST_BOT);
        int span = LIST_BOT - LIST_TOP;
        int th = span * VISROWS / nent; if (th < 8) th = 8;
        int ty = LIST_TOP + span * scroll / nent;
        gfx_fill(sx + 1, ty, sx + 7, ty + th, 1);
    }
    /* status bar */
    gfx_hline(0, W, H - 22, 1);
    char st[40];
    snprintf(st, sizeof st, "%d item%s", nent, nent == 1 ? "" : "s");
    gfx_text(8, H - 18, st, 0);
    if (mv_active) {
        char ms[48]; snprintf(ms, sizeof ms, "Moving: %s  (open a folder, Paste)", mv_name);
        gfx_text(84, H - 18, ms, 0);
    }
    gfx_frame(W - 70, H - 20, W - 8, H - 4);
    gfx_text(W - 60, H - 16, "Open", 0);
    gwin_update(&win);
}

/* ---- actions ---- */
static void go_up(void)
{
    if (cur_dir == 0) return;                        /* already at the disk list */
    if (cur_dir == 2) { load_disks(); sel = scroll = 0; return; }
    CInfoPBRec pb; unsigned char nm[64];
    memset(&pb, 0, sizeof pb);
    pb.dirInfo.ioNamePtr = nm;
    pb.dirInfo.ioVRefNum = cur_vref;
    pb.dirInfo.ioDrDirID = cur_dir;
    pb.dirInfo.ioFDirIndex = -1;                     /* info about this directory itself */
    long parent = (PBGetCatInfoSync(&pb) == 0) ? pb.dirInfo.ioDrParID : 2;
    path_pop();
    load_dir(cur_vref, parent < 2 ? 2 : parent);
    sel = scroll = 0;
}

static int is_program_path(const char *full)
{
    unsigned char p[256]; INTEGER ref; char h[8];
    c2p(p, full);
    if (FSOpen(p, cur_vref, &ref) != 0) return 0;
    LONGINT n = 8;
    int ok = FSRead(ref, &n, h) == 0 && n == 8 && h[4]=='P' && h[5]=='L' && h[6]=='O' && h[7]=='S';
    FSClose(ref);
    return ok;
}

static void open_sel(void)
{
    if (sel < 0 || sel >= nent) return;
    Entry *e = &entries[sel];
    if (e->is_dir == 2) {                            /* a disk: show its root */
        load_dir((INTEGER)e->id, 2);
        path[0] = 0; path_push(e->name);
        sel = scroll = 0;
        return;
    }
    if (e->is_dir) {                                 /* a folder: go in */
        path_push(e->name);
        load_dir(cur_vref, e->id);
        sel = scroll = 0;
        return;
    }
    char full[128];
    snprintf(full, sizeof full, "%s%s", path, e->name);   /* "Vol:...:name" */
    if (is_program_path(full)) {
        char *av[] = { full, 0 };
        struct plusos_spawn_args s = { full, 1, av };
        plusos_syscall(SYS_termspawn, &s);
    } else {
        char *av[] = { "hexedit", full, 0 };
        struct plusos_spawn_args s = { "hexedit", 2, av };
        plusos_syscall(SYS_termspawn, &s);
    }
}

static int row_at(int y) { if (y < LIST_TOP || y >= LIST_BOT) return -1; int i = scroll + (y - LIST_TOP) / ROWH; return i < nent ? i : -1; }

static void clamp_scroll(void)
{
    if (sel >= 0) { if (sel < scroll) scroll = sel; if (sel >= scroll + VISROWS) scroll = sel - VISROWS + 1; }
    if (scroll > nent - VISROWS) scroll = nent - VISROWS;
    if (scroll < 0) scroll = 0;
}

/* ---- modal dialogs, drawn into the window over the list ---- */
/* has_text: a one-line text field + OK/Cancel. Otherwise l2 + <ok>/Cancel. */
static int modal_run(int has_text, const char *title, const char *l2,
                     const char *ok, char *buf, int size)
{
    int W = win.surf.w, H = win.surf.h;
    int bl = 24, br = W - 24, bt = H / 2 - 42, bb = H / 2 + 42;
    int n = buf ? (int)strlen(buf) : 0, res = -1;
    int by0 = bb - 24, by1 = bb - 8;
    int okw = gfx_text_width(ok) + 24, cw = gfx_text_width("Cancel") + 16;
    int okl = br - 12 - okw, cxl = okl - 10 - cw;
    while (res < 0) {
        gfx_target(&win.surf);
        gfx_fill(bl, bt, br, bb, 0);
        gfx_frame(bl, bt, br, bb);
        gfx_frame(bl + 1, bt + 1, br - 1, bb - 1);
        gfx_text(bl + 14, bt + 12, title, 0);
        if (l2) gfx_text(bl + 14, bt + 26, l2, 0);
        if (has_text) {
            gfx_frame(bl + 14, bt + 36, br - 14, bt + 52);
            char vis[42]; int off = n > 38 ? n - 38 : 0; scopy(vis, buf + off, sizeof vis);
            int cx = gfx_text(bl + 18, bt + 40, vis, 0);
            gfx_fill(cx, bt + 39, cx + 1, bt + 49, 1);      /* caret */
        }
        gfx_frame(cxl, by0, cxl + cw, by1);
        gfx_text(cxl + (cw - gfx_text_width("Cancel")) / 2, by0 + 4, "Cancel", 0);
        gfx_frame(okl, by0, okl + okw, by1);
        gfx_frame(okl - 2, by0 - 2, okl + okw + 2, by1 + 2);  /* default ring */
        gfx_text(okl + (okw - gfx_text_width(ok)) / 2, by0 + 4, ok, 0);
        gwin_update(&win);
        GEvent e;
        if (gwin_event(&win, &e, 1) <= 0) continue;
        if (e.type == GEV_CLOSE) res = 0;
        else if (e.type == GEV_KEY) {
            if (e.key == KEY_RETURN || e.key == KEY_ENTER) res = 1;
            else if (e.key == KEY_ESC) res = 0;
            else if (has_text && e.key == KEY_BACKSPACE) { if (n) buf[--n] = 0; }
            else if (has_text && e.key >= 0x20 && e.key < 0x7F && e.key != ':' && n < size - 1) {
                buf[n++] = (char)e.key; buf[n] = 0;
            }
        } else if (e.type == GEV_MOUSEDOWN) {
            if (in_rect(e.x, e.y, okl, by0, okl + okw, by1)) res = 1;
            else if (in_rect(e.x, e.y, cxl, by0, cxl + cw, by1)) res = 0;
        }
    }
    draw();
    return res;
}
static int ask_text(const char *title, char *buf, int size) { return modal_run(1, title, 0, "OK", buf, size) && buf[0]; }
static int ask_delete(const char *l1, const char *l2) { return modal_run(0, l1, l2, "Delete", 0, 0); }
static void note(const char *l1, const char *l2)
{
    int W = win.surf.w, H = win.surf.h;
    int bl = 24, br = W - 24, bt = H / 2 - 30, bb = H / 2 + 30;
    int okw = gfx_text_width("OK") + 24, okl = br - 12 - okw, by0 = bb - 24, by1 = bb - 8;
    for (;;) {
        gfx_target(&win.surf);
        gfx_fill(bl, bt, br, bb, 0); gfx_frame(bl, bt, br, bb); gfx_frame(bl + 1, bt + 1, br - 1, bb - 1);
        gfx_text(bl + 14, bt + 12, l1, 0);
        if (l2) gfx_text(bl + 14, bt + 26, l2, 0);
        gfx_frame(okl, by0, okl + okw, by1); gfx_frame(okl - 2, by0 - 2, okl + okw + 2, by1 + 2);
        gfx_text(okl + (okw - gfx_text_width("OK")) / 2, by0 + 4, "OK", 0);
        gwin_update(&win);
        GEvent e;
        if (gwin_event(&win, &e, 1) <= 0) continue;
        if (e.type == GEV_CLOSE) break;
        if (e.type == GEV_KEY && (e.key == KEY_RETURN || e.key == KEY_ENTER || e.key == KEY_ESC)) break;
        if (e.type == GEV_MOUSEDOWN && in_rect(e.x, e.y, okl, by0, okl + okw, by1)) break;
    }
    draw();
}

/* ---- file operations (PB-level traps: the HFS glue is not in the Plus ROM) ---- */
static int make_dir(INTEGER vref, long parent, const char *name)
{
    HParamBlockRec pb; unsigned char nm[64];
    memset(&pb, 0, sizeof pb); c2p(nm, name);
    pb.fileParam.ioNamePtr = nm; pb.fileParam.ioVRefNum = vref; pb.fileParam.ioDirID = parent;
    return PBDirCreateSync(&pb);
}
static int hdel(INTEGER vref, long parent, const char *name)
{
    HParamBlockRec pb; unsigned char nm[64];
    memset(&pb, 0, sizeof pb); c2p(nm, name);
    pb.fileParam.ioNamePtr = nm; pb.fileParam.ioVRefNum = vref; pb.fileParam.ioDirID = parent;
    return PBHDeleteSync(&pb);
}
static int delete_tree(INTEGER vref, long dir)      /* empty a folder, depth first */
{
    for (;;) {
        CInfoPBRec pb; unsigned char nm[64]; char c[64];
        memset(&pb, 0, sizeof pb);
        pb.hFileInfo.ioNamePtr = nm; pb.hFileInfo.ioVRefNum = vref;
        pb.hFileInfo.ioDirID = dir; pb.hFileInfo.ioFDirIndex = 1;   /* list shrinks as we go */
        if (PBGetCatInfoSync(&pb) != 0) break;
        int n = nm[0]; for (int i = 0; i < n; i++) c[i] = (char)nm[i + 1]; c[n] = 0;
        if (pb.hFileInfo.ioFlAttrib & 0x10) { if (delete_tree(vref, pb.dirInfo.ioDrDirID) != 0) return -1; }
        if (hdel(vref, dir, c) != 0) return -1;
    }
    return 0;
}
static int rename_entry(const char *oldn, const char *newn)
{
    HParamBlockRec pb; unsigned char a[64], b[64];
    memset(&pb, 0, sizeof pb); c2p(a, oldn); c2p(b, newn);
    pb.fileParam.ioNamePtr = a; pb.fileParam.ioVRefNum = cur_vref; pb.fileParam.ioDirID = cur_dir;
    pb.ioParam.ioMisc = (Ptr)b;                     /* HRename's new name */
    return PBHRenameSync(&pb);
}
static int move_entry(INTEGER vref, long src, const char *name, long dst)
{
    CMovePBRec pb; unsigned char nm[64];
    memset(&pb, 0, sizeof pb); c2p(nm, name);
    pb.ioNamePtr = nm; pb.ioVRefNum = vref; pb.ioDirID = src; pb.ioNewDirID = dst; pb.ioNewName = 0;
    return PBCatMoveSync(&pb);
}
static void reselect(const char *name)
{
    sel = 0;
    for (int i = 0; i < nent; i++) if (ieq(entries[i].name, name)) sel = i;
    clamp_scroll();
}

/* ---- toolbar actions ---- */
static void act_new_folder(void)
{
    if (cur_dir < 2) return;
    char nm[32]; scopy(nm, "untitled folder", sizeof nm);
    if (!ask_text("Name the new folder:", nm, 28)) return;
    if (make_dir(cur_vref, cur_dir, nm) != 0) { note("Could not create the folder.", "A folder with that name may exist."); return; }
    load_dir(cur_vref, cur_dir); reselect(nm); draw();
}
static void act_rename(void)
{
    if (sel < 0 || sel >= nent || entries[sel].is_dir == 2) return;
    char oldn[32], nm[32];
    scopy(oldn, entries[sel].name, sizeof oldn); scopy(nm, oldn, sizeof nm);
    if (!ask_text("Rename to:", nm, 28) || ieq(nm, oldn)) return;
    if (rename_entry(oldn, nm) != 0) { note("Could not rename the item.", "A file with that name may exist."); return; }
    load_dir(cur_vref, cur_dir); reselect(nm); draw();
}
static void act_delete(void)
{
    if (sel < 0 || sel >= nent || entries[sel].is_dir == 2) return;
    Entry e = entries[sel];
    char l1[48]; snprintf(l1, sizeof l1, "Delete \"%s\"?", e.name);
    if (!ask_delete(l1, e.is_dir ? "The folder and everything inside it." : "It will be erased from the disk.")) return;
    int bad = (e.is_dir && delete_tree(cur_vref, e.id) != 0) || hdel(cur_vref, cur_dir, e.name) != 0;
    load_dir(cur_vref, cur_dir);
    if (sel >= nent) sel = nent - 1;
    clamp_scroll();
    if (bad) note("Could not delete the item.", 0); else draw();
}
static void act_move(void)
{
    if (sel < 0 || sel >= nent || entries[sel].is_dir == 2) return;
    mv_active = 1; mv_vref = cur_vref; mv_dir = cur_dir;
    scopy(mv_name, entries[sel].name, sizeof mv_name);
    draw();
}
static void act_paste(void)
{
    if (!mv_active || cur_dir < 2) return;
    if (mv_vref != cur_vref) { note("Can't move between disks yet.", "Try Open to copy instead."); return; }
    if (mv_dir == cur_dir) { mv_active = 0; draw(); return; }        /* already here */
    if (move_entry(mv_vref, mv_dir, mv_name, cur_dir) != 0) { note("Could not move the item.", "A folder can't move inside itself."); return; }
    char kept[32]; scopy(kept, mv_name, sizeof kept);
    mv_active = 0; load_dir(cur_vref, cur_dir); reselect(kept); draw();
}

int main(void)
{
    char flag[8];
    struct plusos_reg_args ra; memset(&ra, 0, sizeof ra);
    ra.key = "sys.desktop"; ra.vbuf = flag; ra.vsize = sizeof flag;
    if (plusos_syscall(SYS_regget, &ra) <= 0) {      /* no compositor: nothing to show into */
        printf("Files is a desktop program - open it from the desktop.\n");
        return 0;
    }
    if (gwin_open(&win, "Files", WINW, WINH) != 0) return 1;
    load_disks();
    if (nent == 1) open_sel();                        /* one disk: show it straight away */
    draw();

    uint32_t last = 0, click_t = 0; int click_row = -1;
    for (;;) {
        GEvent e;
        int r = gwin_event(&win, &e, 0);
        if (r < 0) break;                             /* window closed */
        if (r > 0) {
            if (e.type == GEV_CLOSE) break;
            if (e.type == GEV_MOUSEDOWN) {
                if (e.y >= 6 && e.y < 24 && e.x >= 6 && e.x < 54) { go_up(); draw(); }
                else if (e.y >= BTN_T && e.y < BTN_B) {
                    for (int i = 0; i < NBTN; i++)
                        if (e.x >= btn_x0[i] && e.x < btn_x1[i] && btn_enabled(i)) {
                            if (i == B_NEW) act_new_folder();
                            else if (i == B_REN) act_rename();
                            else if (i == B_DEL) act_delete();
                            else if (i == B_MOVE) act_move();
                            else act_paste();
                            break;
                        }
                }
                else if (e.y >= win.surf.h - 20 && e.x >= win.surf.w - 70) { open_sel(); draw(); }
                else {
                    int row = row_at(e.y);
                    if (row >= 0) {
                        uint32_t t = plusos->ticks();
                        int dbl = row == click_row && t - click_t < 32;
                        sel = row; click_row = row; click_t = t;
                        draw();
                        if (dbl) { open_sel(); click_row = -1; draw(); }
                    }
                }
            } else if (e.type == GEV_KEY) {
                if (e.key == KEY_UP && sel > 0) { sel--; clamp_scroll(); draw(); }
                else if (e.key == KEY_DOWN && sel < nent - 1) { sel++; clamp_scroll(); draw(); }
                else if (e.key == KEY_RETURN || e.key == KEY_ENTER) { open_sel(); draw(); }
                else if (e.key == KEY_BACKSPACE) { go_up(); draw(); }
            }
            continue;
        }
        uint32_t t = plusos->ticks();               /* idle: nothing to poll, just wait */
        (void)last; last = t;
        gthread_sleep(2);
    }
    gwin_close(&win);
    return 0;
}
