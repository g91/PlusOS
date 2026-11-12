/* files - the PlusOS Finder.
 *
 * A classic Macintosh Finder window: browse every mounted disk by icon or by
 * list, drag files and folders onto folders to move them, rubber-band select,
 * rename in place, and use the File / Edit / View / Special drop-down menus.
 * Double-click a program to run it, a text file to edit it, anything else to
 * inspect it in the hex editor.
 *
 * It is a GUI program: it draws into its window with PlusGFX. While the mouse
 * is held down it polls the low-memory mouse globals directly (the window
 * server only posts press/release), so drags, menus and scrollbars track live.
 */
#include <plusos/plusos.h>
#include <plusos/syscall.h>
#include <plusos/gfx.h>
#include <plusos/thread.h>
#include <plusrom/FileMgr.h>
#include <plusrom/OSUtil.h>

/* ---- window geometry ------------------------------------------------------ */
#define WINW   464
#define WINH   280
#define MENU_B 18                 /* menu bar: [0,18)            */
#define INFO_T 18
#define INFO_B 34                 /* path / item-count strip     */
#define CT     35                 /* content top                 */
#define CB     262                /* content bottom              */
#define VIEWH  (CB - CT)
#define SBW    16
#define SBX    (WINW - SBW)       /* vertical scrollbar x        */
#define AREAW  SBX                /* icon area width             */
#define COLS   6
#define CW     74                 /* icon cell width             */
#define CH     56                 /* icon cell height            */
#define ROWH   12                 /* list row height             */
#define LHDR   14                 /* list header height          */
#define MAXENT 256

enum { K_DISK, K_FOLDER, K_DOC, K_TEXT, K_APP };

typedef struct {
    char name[32];
    int  kind;
    long id;                      /* folder: dir id; disk: vref num */
    unsigned long size;
    unsigned long mdate;
    int  x, y;                    /* icon position on the virtual canvas */
} Entry;

typedef struct { char name[32]; long dir; INTEGER vref; } PathComp;

static GWindow win;
static Entry   ent[MAXENT];
static int     nent;
static unsigned char selset[MAXENT];
static int     nsels, anchor;
static PathComp pstack[16];
static int     pdepth;            /* 0 = the disk list */
static INTEGER cur_vref;
static long    cur_dir;
static int     scroll_px;
static int     view_mode;         /* 0 = icons, 1 = list */
static int     sort_key;          /* 0 name, 1 size, 2 kind, 3 date */
static int     sort_asc = 1;
static int     origin_x, origin_y;   /* window content origin on screen */
static int     want_quit;            /* set by do_quit; main loop exits on it */

/* clipboard for Cut / Copy / Paste of files */
static struct { INTEGER vref; long dir; char name[32]; int cut; int active; } clip;

/* cached icon art (32x32) and the command-key glyph */
static GSurface art[5];
static GSurface cmd_glyph;
static int      art_ready;

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

/* ---- selection ------------------------------------------------------------ */
static void sel_clear(void) { memset(selset, 0, sizeof selset); nsels = 0; anchor = -1; }
static void sel_add(int i) { if (i>=0 && i<nent && !selset[i]) { selset[i]=1; nsels++; } anchor = i; }
static void sel_only(int i) { sel_clear(); sel_add(i); }
static void sel_toggle(int i) { if (i<0||i>=nent) return; if (selset[i]) { selset[i]=0; nsels--; } else { selset[i]=1; nsels++; } anchor=i; }
static int sel_first(void) { for (int i=0;i<nent;i++) if (selset[i]) return i; return -1; }

/* ---- volume / directory queries ------------------------------------------- */
static void vol_name(INTEGER vref, char *out, int max)
{
    HParamBlockRec pb; unsigned char nm[64];
    memset(&pb, 0, sizeof pb);
    pb.volumeParam.ioNamePtr = nm;
    pb.volumeParam.ioVRefNum = vref;
    pb.volumeParam.ioVolIndex = 0;            /* look up by ref num */
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

static long dir_id_under(INTEGER vref, long parent, const char *name)
{
    for (int idx = 1; idx < 400; idx++) {
        CInfoPBRec pb; unsigned char nm[64]; char c[64];
        memset(&pb, 0, sizeof pb);
        pb.dirInfo.ioNamePtr = nm; pb.dirInfo.ioVRefNum = vref;
        pb.dirInfo.ioDrDirID = parent; pb.dirInfo.ioFDirIndex = idx;
        if (PBGetCatInfoSync(&pb) != 0) break;
        pstr_to_c(nm, c, sizeof c);
        if ((pb.dirInfo.ioFlAttrib & 0x10) && ieq(c, name)) return pb.dirInfo.ioDrDirID;
    }
    return 0;
}

static int kind_of_file(INTEGER vref, long dir, const char *name, int *is_text)
{
    char dp[256], full[300]; unsigned char p[300]; FInfo fi;
    build_dir_path(vref, dir, dp, sizeof dp);
    snprintf(full, sizeof full, "%s%s", dp, name);
    c2p(p, full);
    *is_text = 0;
    if (GetFInfo(p, vref, &fi) == 0) {
        if (fi.fdType == 0x504C4F53UL) return K_APP;          /* 'PLOS' */
        if (fi.fdType == 0x54455854UL) { *is_text = 1; return K_TEXT; } /* 'TEXT' */
    }
    /* extension hints */
    const char *dot = 0; for (const char *s = name; *s; s++) if (*s == '.') dot = s;
    if (dot && (ieq(dot,".txt")||ieq(dot,".md")||ieq(dot,".cfg")||ieq(dot,".c")||
                ieq(dot,".h")||ieq(dot,".log")||ieq(dot,".ini")||ieq(dot,".sh"))) {
        *is_text = 1; return K_TEXT;
    }
    /* magic + content sniff */
    INTEGER ref; char h[64];
    if (FSOpen(p, vref, &ref) == 0) {
        LONGINT n = 64; int ok = FSRead(ref, &n, h) == 0;
        FSClose(ref);
        if (ok && n >= 8 && h[4]=='P'&&h[5]=='L'&&h[6]=='O'&&h[7]=='S') return K_APP;
        if (ok) {
            int text = n > 0;
            for (int i = 0; i < n; i++) {
                unsigned char ch = (unsigned char)h[i];
                if (ch != '\n' && ch != '\r' && ch != '\t' && (ch < 0x20 || ch > 0x7E)) { text = 0; break; }
            }
            if (text) { *is_text = 1; return K_TEXT; }
        }
    }
    return K_DOC;
}

/* ---- sorting -------------------------------------------------------------- */
static int name_cmp(const char *x, const char *y)
{ for (;; x++, y++) { int a=*x, b=*y; if(a>='A'&&a<='Z')a+=32; if(b>='A'&&b<='Z')b+=32; if(a!=b)return a-b; if(!a)return 0; } }

static int ent_cmp(const Entry *a, const Entry *b)
{
    if (a->kind == K_DISK && b->kind != K_DISK) return -1;
    if (b->kind == K_DISK && a->kind != K_DISK) return 1;
    int af = a->kind == K_FOLDER, bf = b->kind == K_FOLDER;
    if (af != bf) return bf - af;                 /* folders first */
    int r = 0;
    switch (sort_key) {
    case 1: r = (a->size < b->size) ? -1 : (a->size > b->size) ? 1 : 0; break;
    case 2: r = (a->kind - b->kind); break;
    case 3: r = (a->mdate < b->mdate) ? -1 : (a->mdate > b->mdate) ? 1 : 0; break;
    default: r = 0; break;
    }
    if (r == 0) r = name_cmp(a->name, b->name);
    return sort_asc ? r : -r;
}

static void sort_entries(void)
{
    for (int i = 1; i < nent; i++) {
        Entry e = ent[i]; int j = i-1;
        while (j >= 0 && ent_cmp(&ent[j], &e) > 0) { ent[j+1] = ent[j]; j--; }
        ent[j+1] = e;
    }
}

static void grid_layout(void)
{
    for (int i = 0; i < nent; i++) { ent[i].x = 4 + (i % COLS) * CW; ent[i].y = (i / COLS) * CH + 4; }
}

/* ---- loading -------------------------------------------------------------- */
static void load_disks(void)
{
    nent = 0;
    for (int idx = 1; idx < 32 && nent < MAXENT; idx++) {
        HParamBlockRec pb; unsigned char nm[64];
        memset(&pb, 0, sizeof pb);
        pb.volumeParam.ioNamePtr = nm; pb.volumeParam.ioVolIndex = idx;
        if (PBHGetVInfoSync(&pb) != 0) break;
        Entry *e = &ent[nent++];
        pstr_to_c(nm, e->name, sizeof e->name);
        e->kind = K_DISK; e->id = pb.volumeParam.ioVRefNum;
        e->size = (unsigned long)pb.volumeParam.ioVNmAlBlks * (unsigned long)pb.volumeParam.ioVAlBlkSiz;
        e->mdate = 0;
    }
    pdepth = 0; cur_vref = 0; cur_dir = 0;
    sort_key = 0; sort_asc = 1;
    sort_entries(); grid_layout(); sel_clear(); scroll_px = 0;
    gwin_title(&win, "Disks");
}

static void load_dir(INTEGER vref, long dir)
{
    nent = 0;
    for (int idx = 1; idx < 512 && nent < MAXENT; idx++) {
        CInfoPBRec pb; unsigned char nm[64];
        memset(&pb, 0, sizeof pb);
        pb.hFileInfo.ioNamePtr = nm; pb.hFileInfo.ioVRefNum = vref;
        pb.hFileInfo.ioDirID = dir; pb.hFileInfo.ioFDirIndex = idx;
        if (PBGetCatInfoSync(&pb) != 0) break;
        Entry *e = &ent[nent++];
        pstr_to_c(nm, e->name, sizeof e->name);
        if (pb.hFileInfo.ioFlAttrib & 0x10) {
            e->kind = K_FOLDER; e->id = pb.dirInfo.ioDrDirID; e->size = 0;
            e->mdate = pb.dirInfo.ioDrMdDat;
        } else {
            int is_text = 0;
            e->kind = kind_of_file(vref, dir, e->name, &is_text);
            e->id = 0; e->size = (unsigned long)pb.hFileInfo.ioFlLgLen;
            e->mdate = pb.hFileInfo.ioFlMdDat;
        }
    }
    cur_vref = vref; cur_dir = dir;
    sort_entries(); grid_layout(); sel_clear(); scroll_px = 0;
    gwin_title(&win, pdepth ? pstack[pdepth-1].name : "Disks");
}

static void reload(void)
{
    char keep[32] = ""; int had = sel_first();
    if (had >= 0) scopy(keep, ent[had].name, sizeof keep);
    if (pdepth == 0) load_disks(); else load_dir(cur_vref, cur_dir);
    if (keep[0]) for (int i=0;i<nent;i++) if (ieq(ent[i].name, keep)) { sel_only(i); break; }
}

/* ---- navigation ----------------------------------------------------------- */
static void nav_disks(void) { load_disks(); }
static void nav_disk(int i)
{
    pdepth = 0;
    scopy(pstack[0].name, ent[i].name, 32);
    pstack[0].dir = 2; pstack[0].vref = (INTEGER)ent[i].id;
    pdepth = 1;
    load_dir((INTEGER)ent[i].id, 2);
}
static void nav_folder(int i)
{
    if (pdepth >= 15) return;
    scopy(pstack[pdepth].name, ent[i].name, 32);
    pstack[pdepth].dir = ent[i].id; pstack[pdepth].vref = cur_vref;
    pdepth++;
    load_dir(cur_vref, ent[i].id);
}
static void nav_up(void)
{
    if (pdepth == 0) return;
    if (pdepth == 1) { load_disks(); return; }
    pdepth--;
    load_dir(pstack[pdepth-1].vref, pstack[pdepth-1].dir);
}
static void nav_crumb(int k)        /* k = -1 for the Disks root */
{
    if (k < 0) { load_disks(); return; }
    pdepth = k+1;
    load_dir(pstack[k].vref, pstack[k].dir);
}

/* ---- file operations ------------------------------------------------------ */
static int make_dir(INTEGER vref, long parent, const char *name)
{
    HParamBlockRec pb; unsigned char nm[64];
    memset(&pb, 0, sizeof pb); c2p(nm, name);
    pb.fileParam.ioNamePtr = nm; pb.fileParam.ioVRefNum = vref; pb.fileParam.ioDirID = parent;
    return PBDirCreateSync(&pb);
}
static int make_file(INTEGER vref, long parent, const char *name)
{
    char dp[256], full[300]; unsigned char p[300];
    build_dir_path(vref, parent, dp, sizeof dp);
    snprintf(full, sizeof full, "%s%s", dp, name);
    c2p(p, full);
    return Create(p, vref, 0x74747874UL /*'ttxt'*/, 0x54455854UL /*'TEXT'*/);
}
static int hdel(INTEGER vref, long parent, const char *name)
{
    HParamBlockRec pb; unsigned char nm[64];
    memset(&pb, 0, sizeof pb); c2p(nm, name);
    pb.fileParam.ioNamePtr = nm; pb.fileParam.ioVRefNum = vref; pb.fileParam.ioDirID = parent;
    return PBHDeleteSync(&pb);
}
static int delete_tree(INTEGER vref, long dir)
{
    for (;;) {
        CInfoPBRec pb; unsigned char nm[64]; char c[64];
        memset(&pb, 0, sizeof pb);
        pb.hFileInfo.ioNamePtr = nm; pb.hFileInfo.ioVRefNum = vref;
        pb.hFileInfo.ioDirID = dir; pb.hFileInfo.ioFDirIndex = 1;
        if (PBGetCatInfoSync(&pb) != 0) break;
        pstr_to_c(nm, c, sizeof c);
        if (pb.hFileInfo.ioFlAttrib & 0x10) { if (delete_tree(vref, pb.dirInfo.ioDrDirID) != 0) return -1; }
        if (hdel(vref, dir, c) != 0) return -1;
    }
    return 0;
}
static int rename_entry(INTEGER vref, long dir, const char *oldn, const char *newn)
{
    HParamBlockRec pb; unsigned char a[64], b[64];
    memset(&pb, 0, sizeof pb); c2p(a, oldn); c2p(b, newn);
    pb.fileParam.ioNamePtr = a; pb.fileParam.ioVRefNum = vref; pb.fileParam.ioDirID = dir;
    pb.ioParam.ioMisc = (Ptr)b;
    return PBHRenameSync(&pb);
}
static int move_same_vol(INTEGER vref, long src, const char *name, long dst)
{
    CMovePBRec pb; unsigned char nm[64];
    memset(&pb, 0, sizeof pb); c2p(nm, name);
    pb.ioNamePtr = nm; pb.ioVRefNum = vref; pb.ioDirID = src; pb.ioNewDirID = dst; pb.ioNewName = 0;
    return PBCatMoveSync(&pb);
}
static int copy_file(const char *src, INTEGER svref, const char *dst, INTEGER dvref)
{
    unsigned char ps[300], pd[300]; FInfo fi;
    c2p(ps, src); c2p(pd, dst);
    OSType creator = 0x74747874UL, type = 0x54455854UL;
    if (GetFInfo(ps, svref, &fi) == 0) { creator = fi.fdCreator; type = fi.fdType; }
    INTEGER in, out;
    if (FSOpen(ps, svref, &in) != 0) return -1;
    if (Create(pd, dvref, creator, type) != 0 || FSOpen(pd, dvref, &out) != 0) { FSClose(in); return -1; }
    char buf[512]; int rc = 0;
    for (;;) {
        LONGINT n = sizeof buf;
        OSErr e = FSRead(in, &n, buf);
        if (n > 0) { LONGINT w = n; if (FSWrite(out, &w, buf) != 0 || w != n) { rc = -1; break; } }
        if (e != 0) break;
    }
    FSClose(in); FSClose(out); FlushVol(0, dvref);
    return rc;
}
static int copy_tree(INTEGER svref, long sdir, const char *name, INTEGER dvref, long ddir);
static int copy_entry(INTEGER svref, long sdir, const char *name, int is_dir, INTEGER dvref, long ddir)
{
    if (is_dir) return copy_tree(svref, sdir, name, dvref, ddir);
    char sp[256], dp[256], sf[300], df[300];
    build_dir_path(svref, sdir, sp, sizeof sp);
    build_dir_path(dvref, ddir, dp, sizeof dp);
    snprintf(sf, sizeof sf, "%s%s", sp, name);
    snprintf(df, sizeof df, "%s%s", dp, name);
    return copy_file(sf, svref, df, dvref);
}
static int copy_tree(INTEGER svref, long sdir, const char *name, INTEGER dvref, long ddir)
{
    if (make_dir(dvref, ddir, name) != 0) return -1;
    long nd = dir_id_under(dvref, ddir, name);
    if (!nd) return -1;
    for (int idx = 1; idx < 512; idx++) {
        CInfoPBRec pb; unsigned char nm[64]; char c[64];
        memset(&pb, 0, sizeof pb);
        pb.hFileInfo.ioNamePtr = nm; pb.hFileInfo.ioVRefNum = svref;
        pb.hFileInfo.ioDirID = sdir; pb.hFileInfo.ioFDirIndex = idx;
        if (PBGetCatInfoSync(&pb) != 0) break;
        pstr_to_c(nm, c, sizeof c);
        int isdir = (pb.hFileInfo.ioFlAttrib & 0x10) != 0;
        long cid = isdir ? pb.dirInfo.ioDrDirID : 0;
        if (copy_entry(svref, isdir ? cid : sdir, c, isdir, dvref, nd) != 0) return -1;
    }
    return 0;
}
/* Move an entry anywhere (same volume: catalog move; across volumes: copy+delete). */
static int move_entry(INTEGER svref, long sdir, const char *name, int is_dir, INTEGER dvref, long ddir)
{
    if (svref == dvref) {
        if (sdir == ddir) return 0;
        return move_same_vol(svref, sdir, name, ddir);
    }
    if (copy_entry(svref, sdir, name, is_dir, dvref, ddir) != 0) return -1;
    if (is_dir) { if (delete_tree(svref, sdir) != 0) return -1; return hdel(svref, sdir, name); }
    return hdel(svref, sdir, name);
}
static int entry_exists(INTEGER vref, long dir, const char *name)
{
    for (int idx = 1; idx < 512; idx++) {
        CInfoPBRec pb; unsigned char nm[64]; char c[64];
        memset(&pb, 0, sizeof pb);
        pb.hFileInfo.ioNamePtr = nm; pb.hFileInfo.ioVRefNum = vref;
        pb.hFileInfo.ioDirID = dir; pb.hFileInfo.ioFDirIndex = idx;
        if (PBGetCatInfoSync(&pb) != 0) break;
        pstr_to_c(nm, c, sizeof c);
        if (ieq(c, name)) return 1;
    }
    return 0;
}
static void unique_name(INTEGER vref, long dir, const char *base, char *out, int size)
{
    scopy(out, base, size);
    for (int n = 1; entry_exists(vref, dir, out) && n < 99; n++) {
        if (n == 1) snprintf(out, size, "%s copy", base);
        else snprintf(out, size, "%s copy %d", base, n);
    }
}

/* ---- icon artwork --------------------------------------------------------- */
static void art_folder(int bx, int by)
{
    gfx_fill(bx+2, by+6, bx+14, by+11, 0); gfx_frame(bx+2, by+6, bx+14, by+11);
    gfx_line(bx+13, by+6, bx+16, by+10, 1);
    gfx_fill(bx+2, by+10, bx+30, by+27, 0); gfx_frame(bx+2, by+10, bx+30, by+27);
    gfx_hline(bx+3, bx+29, by+13, 1);
    gfx_hline(bx+4, bx+28, by+25, 1);
}
static void art_disk(int bx, int by)
{
    gfx_fill_round_rect(bx+2, by+9, bx+30, by+25, 2, 0);
    gfx_round_rect(bx+2, by+9, bx+30, by+25, 2, 1);
    gfx_hline(bx+6, bx+20, by+13, 1);
    gfx_hline(bx+6, bx+22, by+19, 1); gfx_hline(bx+6, bx+22, by+20, 1);
    gfx_fill(bx+25, by+18, bx+27, by+20, 1);
}
static void art_page(int bx, int by, int dense)
{
    gfx_fill(bx+6, by+2, bx+26, by+30, 0); gfx_frame(bx+6, by+2, bx+26, by+30);
    gfx_fill(bx+20, by+2, bx+26, by+8, 0);
    gfx_line(bx+20, by+2, bx+25, by+7, 1);
    gfx_hline(bx+20, bx+26, by+8, 1); gfx_vline(bx+20, by+2, by+8, 1);
    int step = dense ? 2 : 4;
    for (int y = by+12; y < by+27; y += step) gfx_hline(bx+9, bx+23, y, 1);
}
static void art_app(int bx, int by)
{
    gfx_fill(bx+5, by+3, bx+27, by+29, 0); gfx_frame(bx+5, by+3, bx+27, by+29);
    int pts[] = { bx+16,by+8, bx+23,by+16, bx+16,by+24, bx+9,by+16 };
    gfx_poly_frame(pts, 4, 1);
    gfx_hline(bx+10, bx+22, by+16, 1);
    gfx_line(bx+16, by+8, bx+16, by+24, 1);
}
static void build_art(void)
{
    if (art_ready) return;
    for (int k = 0; k < 5; k++) {
        if (gs_init(&art[k], 32, 32)) return;
        gfx_target(&art[k]); gfx_fill(0, 0, 32, 32, 0);
        if (k == K_FOLDER) art_folder(0, 0);
        else if (k == K_DISK) art_disk(0, 0);
        else if (k == K_DOC) art_page(0, 0, 0);
        else if (k == K_TEXT) art_page(0, 0, 1);
        else art_app(0, 0);
    }
    if (gs_init(&cmd_glyph, 8, 8) == 0) {
        gfx_target(&cmd_glyph); gfx_fill(0, 0, 8, 8, 0);
        gfx_frame(2, 2, 6, 6);
        gfx_fill_circle(1, 1, 1, 1); gfx_fill_circle(6, 1, 1, 1);
        gfx_fill_circle(1, 6, 1, 1); gfx_fill_circle(6, 6, 1, 1);
    }
    art_ready = 1;
    gfx_target(&win.surf);
}
static void mini_art(int kind, int x, int y)
{
    switch (kind) {
    case K_FOLDER:
        gfx_frame(x, y+2, x+12, y+10); gfx_hline(x, x+5, y+1, 1); gfx_frame(x, y+1, x+5, y+3); break;
    case K_DISK:
        gfx_frame(x, y+3, x+12, y+10); gfx_hline(x+2, x+8, y+7, 1); gfx_pixel(x+10, y+7, 1); break;
    case K_APP: {
        int pts[] = { x+6,y+1, x+11,y+6, x+6,y+11, x+1,y+6 };
        gfx_poly_frame(pts, 4, 1); break;
    }
    default:
        gfx_frame(x+2, y, x+10, y+12); gfx_hline(x+4, x+8, y+4, 1);
        gfx_hline(x+4, x+8, y+7, 1); gfx_hline(x+4, x+8, y+10, 1); break;
    }
}
static void draw_check(int x, int y) { gfx_line(x, y+3, x+2, y+5, 1); gfx_line(x+2, y+5, x+5, y+1, 1); }
static void draw_cmd(int x, int y) { if (cmd_glyph.bits) gfx_blit(&cmd_glyph, 0, 0, 8, 8, x, y, GFX_COPY); }
static void tri_up(int cx, int cy, int black) { int p[] = {cx-4,cy+3, cx+4,cy+3, cx,cy-3}; gfx_poly_fill(p, 3, black); }
static void tri_down(int cx, int cy, int black) { int p[] = {cx-4,cy-3, cx+4,cy-3, cx,cy+3}; gfx_poly_fill(p, 3, black); }

/* ---- formatting ----------------------------------------------------------- */
static void fmt_size(unsigned long b, char *out, int size)
{
    if (b >= 1048576UL) snprintf(out, size, "%lu.%lu MB", b>>20, ((b>>10)&1023)*10/1024);
    else if (b >= 1024UL) snprintf(out, size, "%lu K", (b+512)>>10);
    else snprintf(out, size, "%lu bytes", b);
}
static void fmt_date(unsigned long macsecs, char *out, int size)
{
    if (!macsecs) { scopy(out, "--", size); return; }
    DateTimeRec d; SecondsToDate(macsecs, &d);
    static const char *mon[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
    int m = d.month; if (m < 1) m = 1; if (m > 12) m = 12;
    snprintf(out, size, "%s %d, %d", mon[m-1], d.day, d.year);
}
static const char *kind_name(int kind)
{
    switch (kind) {
    case K_DISK: return "disk";
    case K_FOLDER: return "folder";
    case K_APP: return "program";
    case K_TEXT: return "text";
    default: return "document";
    }
}
static void trunc_label(const char *name, char *out, int maxpx)
{
    int maxc = maxpx / 8;
    if ((int)strlen(name) <= maxc) { scopy(out, name, 32); return; }
    int keep = maxc - 2; if (keep < 1) keep = 1;
    int i = 0; for (; i < keep && name[i]; i++) out[i] = name[i];
    out[i++] = '.'; out[i++] = '.'; out[i] = 0;
}

/* ---- content metrics ------------------------------------------------------ */
static int content_h(void)
{
    if (view_mode == 1) return nent * ROWH;
    int h = ((nent + COLS - 1) / COLS) * CH;
    for (int i = 0; i < nent; i++) h = maxi(h, ent[i].y + CH);
    return h;
}
static int max_scroll(void) { int m = content_h() - VIEWH; return m > 0 ? m : 0; }
static void clamp_scroll(void) { if (scroll_px > max_scroll()) scroll_px = max_scroll(); if (scroll_px < 0) scroll_px = 0; }

/* icon screen rect for entry i (returns 0 if fully outside the viewport) */
static int icon_rect(int i, int *l, int *t, int *r, int *b)
{
    int sy = CT + ent[i].y - scroll_px;
    *l = ent[i].x; *t = sy; *r = ent[i].x + CW; *b = sy + CH - 4;
    return *b > CT && *t < CB;
}
static int icon_at(int cx, int cy)
{
    if (cx >= SBX || cy < CT || cy >= CB) return -1;
    int vy = cy - CT + scroll_px;
    for (int i = nent-1; i >= 0; i--)
        if (in_rect(cx, vy, ent[i].x, ent[i].y, ent[i].x+CW, ent[i].y+CH-4)) return i;
    return -1;
}

/* ---- drawing -------------------------------------------------------------- */
static void draw_scrollbar(void)
{
    int x = SBX;
    gfx_fill(x, CT, x+SBW, CB, 0);
    gfx_vline(x, CT, CB, 1);
    int over = nent > 0 && max_scroll() > 0;
    /* arrows */
    gfx_frame(x, CT, x+SBW, CT+SBW);
    gfx_frame(x, CB-SBW, x+SBW, CB);
    tri_up(x+SBW/2, CT+SBW/2, 1);
    tri_down(x+SBW/2, CB-SBW/2, 1);
    if (!over) { gfx_dim(x+1, CT+1, x+SBW-1, CT+SBW-1); gfx_dim(x+1, CB-SBW+1, x+SBW-1, CB-1); }
    /* track */
    int tt = CT+SBW, tb = CB-SBW;
    gfx_dither_rect(x+1, tt, x+SBW-1, tb, over ? 3 : 6);
    if (over) {
        int span = tb - tt;
        int ch = content_h();
        int th = span * VIEWH / ch; if (th < 12) th = 12;
        int ty = tt + (span - th) * scroll_px / max_scroll();
        gfx_fill(x+2, ty, x+SBW-2, ty+th, 0);
        gfx_frame(x+2, ty, x+SBW-2, ty+th);
    }
}

static void draw_icon_entry(int i, int dx, int dy, int ghost)
{
    int l, t, r, b;
    if (!icon_rect(i, &l, &t, &r, &b)) return;
    l += dx; t += dy; r += dx; b += dy;
    int ix = l + (CW-32)/2, iy = t + 2;
    int sel = selset[i] && !ghost;
    gfx_clip(0, CT, SBX, CB);
    if (art[ent[i].kind].bits) gfx_blit(&art[ent[i].kind], 0, 0, 32, 32, ix, iy, GFX_COPY);
    if (sel) gfx_invert(ix, iy, ix+32, iy+32);
    char lab[32]; trunc_label(ent[i].name, lab, CW-6);
    int lw = gfx_text_width(lab), lx = l + (CW-lw)/2, ly = t + 37;
    if (sel) { gfx_fill(lx-2, ly-1, lx+lw+2, ly+9, 1); gfx_text(lx, ly, lab, 1); }
    else gfx_text(lx, ly, lab, 0);
    if (ghost) gfx_frame(ix-1, iy-1, ix+33, iy+33);
    gfx_noclip();
}

static void draw_icons(void)
{
    gfx_fill(0, CT, SBX, CB, 0);
    for (int i = 0; i < nent; i++) draw_icon_entry(i, 0, 0, 0);
}

static void draw_list(void)
{
    gfx_fill(0, CT, SBX, CB, 0);
    /* header */
    gfx_dither_rect(0, CT, SBX, CT+LHDR, 3);
    gfx_hline(0, SBX, CT+LHDR-1, 1);
    struct { const char *t; int x; int key; } cols[] = {
        {"Name", 26, 0}, {"Size", 250, 1}, {"Kind", 312, 2}, {"Modified", 372, 3}
    };
    for (int c = 0; c < 4; c++) {
        gfx_text(cols[c].x, CT+3, cols[c].t, 0);
        if (sort_key == cols[c].key) {
            int tx = cols[c].x + gfx_text_width(cols[c].t) + 4;
            if (sort_asc) tri_up(tx+3, CT+7, 1); else tri_down(tx+3, CT+7, 1);
        }
        if (c) gfx_vline(cols[c].x-6, CT+2, CT+LHDR-2, 1);
    }
    gfx_clip(0, CT+LHDR, SBX, CB);
    for (int i = 0; i < nent; i++) {
        int y = CT + LHDR + i*ROWH - scroll_px;
        if (y + ROWH <= CT+LHDR || y >= CB) continue;
        int sel = selset[i];
        if (sel) gfx_fill(0, y, SBX, y+ROWH, 1);
        mini_art(ent[i].kind, 6, y);
        char nm[32]; trunc_label(ent[i].name, nm, 218);
        gfx_text(26, y+2, nm, sel);
        if (ent[i].kind != K_FOLDER) {
            char sz[16]; fmt_size(ent[i].size, sz, sizeof sz);
            gfx_text(304 - gfx_text_width(sz), y+2, sz, sel);
        }
        gfx_text(312, y+2, kind_name(ent[i].kind), sel);
        char dt[20]; fmt_date(ent[i].mdate, dt, sizeof dt);
        gfx_text(372, y+2, dt, sel);
    }
    gfx_noclip();
}

/* breadcrumb segments, recorded while drawing for hit-testing */
static struct { int l, r, k; } crumb[18];
static int ncrumb;

static void draw_info(void)
{
    gfx_fill(0, INFO_T, WINW, INFO_B, 0);
    gfx_hline(0, WINW, INFO_B-1, 1);
    /* up button */
    gfx_frame(4, INFO_T+2, 22, INFO_B-2);
    tri_up(13, INFO_T+8, pdepth ? 1 : 0);
    if (!pdepth) gfx_dim(5, INFO_T+3, 21, INFO_B-3);
    ncrumb = 0;
    int x = 28;
    int limit = WINW - 110;
    /* build the segment list, then drop leading segments until it fits */
    char segs[18][32]; int ns = 0;
    scopy(segs[ns++], "Disks", 32);
    for (int i = 0; i < pdepth; i++) scopy(segs[ns++], pstack[i].name, 32);
    int widths[18]; int total = 0;
    for (int i = 0; i < ns; i++) { widths[i] = gfx_text_width(segs[i]) + 14; total += widths[i]; }
    int start = 0;
    while (start < ns-1 && total > limit - x) { total -= widths[start]; start++; }
    if (start > 0) { gfx_text(x, INFO_T+4, "..", 0); x += 22; }
    for (int i = start; i < ns; i++) {
        int k = i-1;                       /* -1 = Disks root */
        if (i > start) { gfx_text(x, INFO_T+4, ":", 0); x += 8; }
        int l = x;
        gfx_text(x, INFO_T+4, segs[i], 0);
        x += gfx_text_width(segs[i]);
        crumb[ncrumb].l = l; crumb[ncrumb].r = x; crumb[ncrumb].k = k; ncrumb++;
        x += 6;
    }
    char cnt[24]; snprintf(cnt, sizeof cnt, "%d item%s", nent, nent==1?"":"s");
    gfx_text(WINW - 8 - gfx_text_width(cnt), INFO_T+4, cnt, 0);
}

static void draw_status(const char *hint)
{
    gfx_fill(0, CB, WINW, WINH, 0);
    gfx_hline(0, WINW, CB, 1);
    char left[64];
    if (hint) scopy(left, hint, sizeof left);
    else if (nsels == 1) {
        int i = sel_first();
        char sz[16]; fmt_size(ent[i].size, sz, sizeof sz);
        if (ent[i].kind == K_FOLDER) snprintf(left, sizeof left, "\"%s\"  folder", ent[i].name);
        else snprintf(left, sizeof left, "\"%s\"  %s", ent[i].name, sz);
    } else if (nsels > 1) snprintf(left, sizeof left, "%d items selected", nsels);
    else scopy(left, "Drag items onto a folder to move them", sizeof left);
    gfx_text(8, CB+5, left, 0);
    if (pdepth > 0) {
        HParamBlockRec pb; unsigned char nm[64];
        memset(&pb, 0, sizeof pb);
        pb.volumeParam.ioNamePtr = nm; pb.volumeParam.ioVRefNum = cur_vref; pb.volumeParam.ioVolIndex = 0;
        if (PBHGetVInfoSync(&pb) == 0) {
            unsigned long freeb = (unsigned long)pb.volumeParam.ioVFrBlk * (unsigned long)pb.volumeParam.ioVAlBlkSiz;
            char fs[20]; fmt_size(freeb, fs, sizeof fs);
            char right[32]; snprintf(right, sizeof right, "%s available", fs);
            gfx_text(WINW - 8 - gfx_text_width(right), CB+5, right, 0);
        }
    }
}

/* menu bar */
typedef struct { const char *label; char shortcut; int id; } MItem;
typedef struct { const char *title; MItem items[12]; int n; } MenuDef;
enum {
    A_NEWFOLDER, A_NEWFILE, A_OPEN, A_INFO, A_RENAME, A_DUP, A_DELETE, A_QUIT,
    A_UNDO, A_CUT, A_COPY, A_PASTE, A_SELALL,
    A_VICON, A_VNAME, A_VSIZE, A_VDATE, A_VKIND, A_CLEANUP,
    A_NEWWIN, A_CLOSEWIN, A_RESTART, A_SHUTDOWN
};
static const MenuDef menus[] = {
    { "File", { {"New Folder",'N',A_NEWFOLDER},{"New File",'T',A_NEWFILE},{"-",0,0},
                {"Open",'O',A_OPEN},{"Get Info",'I',A_INFO},{"-",0,0},
                {"Rename",0,A_RENAME},{"Duplicate",'D',A_DUP},{"Delete",0,A_DELETE},
                {"-",0,0},{"Quit",'Q',A_QUIT} }, 11 },
    { "Edit", { {"Undo",'Z',A_UNDO},{"-",0,0},{"Cut",'X',A_CUT},{"Copy",'C',A_COPY},
                {"Paste",'V',A_PASTE},{"-",0,0},{"Select All",'A',A_SELALL} }, 7 },
    { "View", { {"by Icon",0,A_VICON},{"by Name",0,A_VNAME},{"by Size",0,A_VSIZE},
                {"by Date",0,A_VDATE},{"by Kind",0,A_VKIND},{"-",0,0},{"Clean Up",0,A_CLEANUP} }, 7 },
    { "Special", { {"New Window",0,A_NEWWIN},{"Close Window",'W',A_CLOSEWIN},{"-",0,0},
                   {"Restart...",0,A_RESTART},{"Shut Down...",0,A_SHUTDOWN} }, 5 },
};
#define NMENU 4
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
    switch (id) {
    case A_NEWFOLDER: case A_NEWFILE: return pdepth > 0;
    case A_OPEN: case A_INFO: case A_DUP: case A_DELETE: case A_CUT: case A_COPY: return nsels > 0;
    case A_RENAME: return nsels == 1;
    case A_PASTE: return clip.active && pdepth > 0;
    case A_SELALL: return nent > 0;
    case A_CLEANUP: return view_mode == 0;
    case A_UNDO: return 0;
    default: return 1;
    }
}
static int menu_item_checked(int m, int i)
{
    int id = menus[m].items[i].id;
    if (m != 2) return 0;
    if (id == A_VICON) return view_mode == 0;
    if (view_mode != 1) return 0;
    return (id == A_VNAME && sort_key == 0) || (id == A_VSIZE && sort_key == 1) ||
           (id == A_VKIND && sort_key == 2) || (id == A_VDATE && sort_key == 3);
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

/* ---- full redraw ---------------------------------------------------------- */
static char status_hint[64];
static void draw(void)
{
    gfx_target(&win.surf);
    gfx_fill(0, 0, WINW, WINH, 0);
    draw_menubar(-1);
    draw_info();
    if (view_mode == 0) draw_icons(); else draw_list();
    draw_scrollbar();
    draw_status(status_hint[0] ? status_hint : 0);
    gwin_update(&win);
}

/* ---- text field widget (dialogs + inline rename) -------------------------- */
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

/* Generic modal dialog. btns: rightmost is the default. edit != 0 adds a field.
 * Returns the index of the button pressed, or -1 for Escape / close. */
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
        draw();                                   /* repaint the window behind */
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
static int confirm(const char *l1, const char *l2, const char *ok)
{
    const char *b[] = {"Cancel", ok};
    return dialog(l1, l2, b, 2, 0, 0, 0) == 1;
}
static int prompt(const char *title, char *buf, int size)
{
    const char *b[] = {"Cancel", "OK"};
    return dialog(title, 0, b, 2, buf, size, 1) == 1 && buf[0];
}

/* ---- Get Info ------------------------------------------------------------- */
static void get_info(int i)
{
    Entry *e = &ent[i];
    char dp[256]; build_dir_path(cur_vref, cur_dir, dp, sizeof dp);
    char lines[7][64];
    scopy(lines[0], e->name, 64);
    snprintf(lines[1], 64, "Kind: %s", kind_name(e->kind));
    if (e->kind == K_FOLDER) {
        CInfoPBRec pb; unsigned char nm[64];
        memset(&pb, 0, sizeof pb);
        pb.dirInfo.ioNamePtr = nm; pb.dirInfo.ioVRefNum = cur_vref;
        pb.dirInfo.ioDrDirID = e->id; pb.dirInfo.ioFDirIndex = -1;
        int items = (PBGetCatInfoSync(&pb)==0) ? pb.dirInfo.ioDrNmFls : 0;
        snprintf(lines[2], 64, "Contents: %d items", items);
    } else if (e->kind == K_DISK) {
        char sz[20]; fmt_size(e->size, sz, sizeof sz);
        snprintf(lines[2], 64, "Capacity: %s", sz);
    } else {
        char sz[20]; fmt_size(e->size, sz, sizeof sz);
        snprintf(lines[2], 64, "Size: %s (%lu bytes)", sz, e->size);
    }
    char cd[24], md[24]; fmt_date(e->mdate, md, sizeof md);
    snprintf(lines[3], 64, "Modified: %s", md);
    snprintf(lines[4], 64, "Where: %s", dp);
    (void)cd;
    int W = WINW, H = WINH;
    int w = 320, h = 150;
    int l = (W-w)/2, t = (H-h)/2, r = l+w, b = t+h;
    int okw = 60, okl = r-16-okw, by0 = b-30, by1 = b-12;
    for (;;) {
        gfx_target(&win.surf);
        draw();
        dlg_shadow(l, t, r, b);
        if (art[e->kind].bits) gfx_blit(&art[e->kind], 0, 0, 32, 32, l+18, t+18, GFX_COPY);
        int ty = t+18;
        for (int k = 0; k < 5; k++) { gfx_text(l+62, ty, lines[k], 0); ty += 14; }
        dlg_button(okl, by0, okl+okw, by1, "OK", 1);
        gwin_update(&win);
        GEvent e2;
        if (gwin_event(&win, &e2, 0) > 0) {
            if (e2.type == GEV_CLOSE) break;
            if (e2.type == GEV_KEY && (e2.key==KEY_RETURN||e2.key==KEY_ENTER||e2.key==KEY_ESC)) break;
            if (e2.type == GEV_MOUSEDOWN && in_rect(e2.x, e2.y, okl-2, by0-2, okl+okw+2, by1+2)) break;
        }
        nap(2);
    }
    draw();
}

/* ---- actions -------------------------------------------------------------- */
static void do_new_folder(void)
{
    if (pdepth == 0) return;
    char nm[32]; unique_name(cur_vref, cur_dir, "untitled folder", nm, sizeof nm);
    if (!prompt("Name the new folder:", nm, sizeof nm)) return;
    if (make_dir(cur_vref, cur_dir, nm) != 0) { message("Could not create the folder.", "A folder with that name may exist."); return; }
    reload();
    for (int i=0;i<nent;i++) if (ieq(ent[i].name, nm)) { sel_only(i); break; }
    draw();
}
static void do_new_file(void)
{
    if (pdepth == 0) return;
    char nm[32]; unique_name(cur_vref, cur_dir, "untitled", nm, sizeof nm);
    if (!prompt("Name the new text file:", nm, sizeof nm)) return;
    if (make_file(cur_vref, cur_dir, nm) != 0) { message("Could not create the file.", "A file with that name may exist."); return; }
    reload();
    for (int i=0;i<nent;i++) if (ieq(ent[i].name, nm)) { sel_only(i); break; }
    draw();
}
static void do_delete(void)
{
    if (!nsels) return;
    char l1[64];
    if (nsels == 1) snprintf(l1, sizeof l1, "Delete \"%s\"?", ent[sel_first()].name);
    else snprintf(l1, sizeof l1, "Delete %d items?", nsels);
    if (!confirm(l1, "This cannot be undone.", "Delete")) return;
    int bad = 0;
    for (int i = 0; i < nent; i++) if (selset[i]) {
        if (ent[i].kind == K_DISK) continue;
        if (ent[i].kind == K_FOLDER && delete_tree(cur_vref, ent[i].id) != 0) bad = 1;
        if (hdel(cur_vref, cur_dir, ent[i].name) != 0) bad = 1;
    }
    reload(); draw();
    if (bad) message("Some items could not be deleted.", 0);
}
static void do_duplicate(void)
{
    if (!nsels || pdepth == 0) return;
    int bad = 0;
    for (int i = 0; i < nent; i++) if (selset[i] && ent[i].kind != K_DISK) {
        char nn[32]; unique_name(cur_vref, cur_dir, ent[i].name, nn, sizeof nn);
        if (copy_entry(cur_vref, ent[i].id, ent[i].name, ent[i].kind==K_FOLDER, cur_vref, cur_dir) != 0) { bad = 1; continue; }
        /* copy_entry used the original name; rename the fresh copy to nn */
        rename_entry(cur_vref, cur_dir, ent[i].name, nn);
    }
    reload(); draw();
    if (bad) message("Some items could not be duplicated.", 0);
}
static void do_copy(int cut)
{
    int i = sel_first();
    if (i < 0 || ent[i].kind == K_DISK) return;
    clip.vref = cur_vref; clip.dir = cur_dir; clip.cut = cut; clip.active = 1;
    scopy(clip.name, ent[i].name, sizeof clip.name);
    snprintf(status_hint, sizeof status_hint, cut ? "Cut \"%s\" - open a folder and Paste" : "Copied \"%s\"", ent[i].name);
    draw();
}
static void do_paste(void)
{
    if (!clip.active || pdepth == 0) return;
    char nn[32]; unique_name(cur_vref, cur_dir, clip.name, nn, sizeof nn);
    int rc;
    if (clip.cut && clip.vref == cur_vref) rc = move_same_vol(clip.vref, clip.dir, clip.name, cur_dir);
    else {
        CInfoPBRec pb; unsigned char nm[64];
        memset(&pb, 0, sizeof pb); c2p(nm, clip.name);
        pb.hFileInfo.ioNamePtr = nm; pb.hFileInfo.ioVRefNum = clip.vref;
        pb.hFileInfo.ioDirID = clip.dir; pb.hFileInfo.ioFDirIndex = 0;
        int isdir = 0; long sid = clip.dir;
        if (PBGetCatInfoSync(&pb) == 0) { isdir = (pb.hFileInfo.ioFlAttrib & 0x10) != 0; if (isdir) sid = pb.dirInfo.ioDrDirID; }
        rc = copy_entry(clip.vref, sid, clip.name, isdir, cur_vref, cur_dir);
        if (rc == 0 && clip.cut) {
            if (isdir) { delete_tree(clip.vref, sid); hdel(clip.vref, clip.dir, clip.name); }
            else hdel(clip.vref, clip.dir, clip.name);
        }
    }
    if (rc != 0) { message("Could not paste the item.", "The disk may be full."); return; }
    if (clip.cut) clip.active = 0;
    status_hint[0] = 0;
    reload();
    for (int i=0;i<nent;i++) if (ieq(ent[i].name, nn)) { sel_only(i); break; }
    draw();
}
static void do_select_all(void) { sel_clear(); for (int i=0;i<nent;i++) sel_add(i); draw(); }
static void do_cleanup(void) { grid_layout(); scroll_px = 0; draw(); }
static void do_quit(void) { want_quit = 1; }

static void spawn_program(const char *path, int gui)
{
    char *av[] = { (char *)path, 0 };
    struct plusos_spawn_args s = { path, 1, av };
    plusos_syscall(gui ? SYS_spawn : SYS_termspawn, &s);
}
static int is_gui_name(const char *n)
{
    return ieq(n,"notes")||ieq(n,"calc")||ieq(n,"taskman")||ieq(n,"pkgmgr")||
           ieq(n,"installer")||ieq(n,"puzzle")||ieq(n,"files");
}
static void open_entry(int i)
{
    Entry *e = &ent[i];
    if (e->kind == K_DISK) { nav_disk(i); draw(); return; }
    if (e->kind == K_FOLDER) { nav_folder(i); draw(); return; }
    char dp[256], full[300];
    build_dir_path(cur_vref, cur_dir, dp, sizeof dp);
    snprintf(full, sizeof full, "%s%s", dp, e->name);
    if (e->kind == K_APP) { spawn_program(full, is_gui_name(e->name)); return; }
    if (e->kind == K_TEXT) {
        char *av[] = { "notes", full, 0 };
        struct plusos_spawn_args s = { "notes", 2, av };
        plusos_syscall(SYS_spawn, &s);
        return;
    }
    char *av[] = { "hexedit", full, 0 };
    struct plusos_spawn_args s = { "hexedit", 2, av };
    plusos_syscall(SYS_termspawn, &s);
}
static void do_open(void) { int i = sel_first(); if (i >= 0) open_entry(i); }

static void do_restart(void)
{
    if (confirm("Restart the computer?", "Unsaved changes will be lost.", "Restart"))
        plusos->reboot();
}
static void do_shutdown(void)
{
    if (confirm("Shut down the computer?", "You can turn it back on later.", "Shut Down"))
        plusos->shutdown();
}
static void do_new_window(void)
{
    char *av[] = { "files", 0 };
    struct plusos_spawn_args s = { "files", 1, av };
    plusos_syscall(SYS_spawn, &s);
}

static void inline_rename(int i);
static void run_action(int id)
{
    switch (id) {
    case A_NEWFOLDER: do_new_folder(); break;
    case A_NEWFILE: do_new_file(); break;
    case A_OPEN: do_open(); break;
    case A_INFO: { int i = sel_first(); if (i>=0) get_info(i); break; }
    case A_RENAME: { int i = sel_first(); if (i>=0) inline_rename(i); break; }
    case A_DUP: do_duplicate(); break;
    case A_DELETE: do_delete(); break;
    case A_QUIT: case A_CLOSEWIN: do_quit(); break;
    case A_CUT: do_copy(1); break;
    case A_COPY: do_copy(0); break;
    case A_PASTE: do_paste(); break;
    case A_SELALL: do_select_all(); break;
    case A_VICON: view_mode = 0; grid_layout(); scroll_px = 0; draw(); break;
    case A_VNAME: view_mode = 1; sort_key = 0; sort_asc = 1; sort_entries(); scroll_px = 0; draw(); break;
    case A_VSIZE: view_mode = 1; sort_key = 1; sort_asc = 0; sort_entries(); scroll_px = 0; draw(); break;
    case A_VDATE: view_mode = 1; sort_key = 3; sort_asc = 0; sort_entries(); scroll_px = 0; draw(); break;
    case A_VKIND: view_mode = 1; sort_key = 2; sort_asc = 1; sort_entries(); scroll_px = 0; draw(); break;
    case A_CLEANUP: do_cleanup(); break;
    case A_NEWWIN: do_new_window(); break;
    case A_RESTART: do_restart(); break;
    case A_SHUTDOWN: do_shutdown(); break;
    }
}

/* ---- drop-down menu tracking ---------------------------------------------- */
static int menu_geom(int m, int *l, int *t, int *r, int *b, int *iw)
{
    int wmax = 0;
    for (int i = 0; i < menus[m].n; i++) {
        const MItem *it = &menus[m].items[i];
        int w = gfx_text_width(it->label) + 44;
        if (it->shortcut) w += 18;
        if (w > wmax) wmax = w;
    }
    *l = menu_x[m] - 2; *t = MENU_B;
    *iw = wmax;
    *r = *l + wmax;
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
        if (menu_item_checked(m, i)) { if (inv) { gfx_line(l+5, y+7, l+7, y+9, 0); gfx_line(l+7, y+9, l+10, y+5, 0); } else draw_check(l+5, y+4); }
        gfx_text(l+18, y+4, it->label, inv);
        if (it->shortcut) {
            int sx = r - 22;
            if (inv) { gfx_fill(sx-1, y+3, sx+17, y+12, 0); }
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
/* Track a menu from a press on its title. Supports press-drag-release and
 * click-to-open (sticky) modes, like the classic Mac menu bar. */
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

/* ---- inline rename -------------------------------------------------------- */
static void inline_rename(int i)
{
    if (i < 0 || i >= nent || ent[i].kind == K_DISK) return;
    char oldn[32], nm[32];
    scopy(oldn, ent[i].name, sizeof oldn); scopy(nm, oldn, sizeof nm);
    int l, t, r, b;
    if (!icon_rect(i, &l, &t, &r, &b) && view_mode == 0) return;
    Field f;
    if (view_mode == 0) field_init(&f, nm, sizeof nm, l+1, t+35, CW-2, 1);
    else field_init(&f, nm, sizeof nm, 24, CT+LHDR+i*ROWH-scroll_px, 222, 1);
    for (;;) {
        gfx_target(&win.surf);
        draw();
        field_draw(&f);
        gwin_update(&win);
        GEvent e;
        if (gwin_event(&win, &e, 0) > 0) {
            if (e.type == GEV_CLOSE) { draw(); return; }
            if (e.type == GEV_KEY) {
                if (e.key == KEY_RETURN || e.key == KEY_ENTER) break;
                if (e.key == KEY_ESC) { draw(); return; }
                field_key(&f, e.key);
            } else if (e.type == GEV_MOUSEDOWN) {
                if (!in_rect(e.x, e.y, f.x, f.y, f.x+f.w, f.y+14)) break;
            }
        }
        nap(2);
    }
    if (nm[0] && !ieq(nm, oldn)) {
        if (rename_entry(cur_vref, cur_dir, oldn, nm) != 0)
            message("Could not rename the item.", "A file with that name may exist.");
    }
    reload();
    for (int j=0;j<nent;j++) if (ieq(ent[j].name, nm)) { sel_only(j); break; }
    draw();
}

/* ---- scrolling ------------------------------------------------------------ */
static void scroll_by(int dy)
{
    scroll_px += dy; clamp_scroll(); draw();
}
static void ensure_visible(int i)
{
    if (view_mode == 0) {
        int top = ent[i].y, bot = ent[i].y + CH;
        if (top < scroll_px) scroll_px = top;
        if (bot > scroll_px + VIEWH) scroll_px = bot - VIEWH;
    } else {
        int top = i*ROWH, bot = top + ROWH;
        if (top < scroll_px) scroll_px = top;
        if (bot > scroll_px + VIEWH) scroll_px = bot - VIEWH;
    }
    clamp_scroll();
}
/* scrollbar press: arrows repeat, track pages, thumb drags */
static void scrollbar_run(int cy)
{
    if (cy < CT+SBW) { scroll_by(-(view_mode?ROWH:CH)); }
    else if (cy >= CB-SBW) { scroll_by(view_mode?ROWH:CH); }
    else {
        int tt = CT+SBW, tb = CB-SBW, span = tb-tt;
        int ch = content_h(), th = span*VIEWH/ch; if (th < 12) th = 12;
        int ty = tt + (span-th)*scroll_px/max_scroll();
        if (cy < ty) { scroll_by(-VIEWH); }
        else if (cy >= ty+th) { scroll_by(VIEWH); }
        else {
            int grab = cy - ty;
            while (mouse_down()) {
                int my = mouse_y() - origin_y;
                int nty = my - grab;
                if (nty < tt) nty = tt;
                if (nty > tb-th) nty = tb-th;
                scroll_px = (nty - tt) * max_scroll() / (span - th);
                clamp_scroll(); draw();
                nap(1);
            }
            return;
        }
    }
    /* arrow auto-repeat */
    uint32_t last = now();
    while (mouse_down()) {
        int my = mouse_y() - origin_y;
        int dir = 0;
        if (my < CT+SBW) dir = -1; else if (my >= CB-SBW) dir = 1;
        if (dir && now() - last > 8) { scroll_by(dir*(view_mode?ROWH:CH)); last = now(); }
        nap(1);
    }
}

/* ---- drag & drop ---------------------------------------------------------- */
static int drop_target(int cx, int cy, int *onto_crumb)
{
    *onto_crumb = -2;
    if (cy >= INFO_T && cy < INFO_B) {
        for (int c = 0; c < ncrumb; c++)
            if (cx >= crumb[c].l && cx <= crumb[c].r) { *onto_crumb = crumb[c].k; return -1; }
        return -1;
    }
    if (view_mode == 0) {
        int i = icon_at(cx, cy);
        if (i >= 0 && (ent[i].kind == K_FOLDER || ent[i].kind == K_DISK) && !selset[i]) return i;
        return -1;
    } else {
        if (cy < CT+LHDR || cy >= CB || cx >= SBX) return -1;
        int i = (cy - CT - LHDR + scroll_px) / ROWH;
        if (i >= 0 && i < nent && ent[i].kind == K_FOLDER && !selset[i]) return i;
        return -1;
    }
}
static void move_selection_to(int target, int crumb_k)
{
    INTEGER dvref; long ddir; char dname[32];
    if (crumb_k != -2) {
        if (crumb_k < 0) { message("Items can't be moved to the disk list.", "Open a disk first."); return; }
        dvref = pstack[crumb_k].vref; ddir = pstack[crumb_k].dir;
        scopy(dname, pstack[crumb_k].name, sizeof dname);
    } else {
        if (target < 0) return;
        if (ent[target].kind == K_DISK) { dvref = (INTEGER)ent[target].id; ddir = 2; scopy(dname, ent[target].name, sizeof dname); }
        else { dvref = cur_vref; ddir = ent[target].id; scopy(dname, ent[target].name, sizeof dname); }
    }
    int bad = 0, moved = 0;
    for (int i = 0; i < nent; i++) if (selset[i] && ent[i].kind != K_DISK) {
        if (dvref == cur_vref && ddir == cur_dir) continue;
        if (move_entry(cur_vref, ent[i].id, ent[i].name, ent[i].kind==K_FOLDER, dvref, ddir) != 0) bad = 1;
        else moved++;
    }
    reload(); draw();
    if (bad) message("Some items could not be moved.", "A folder can't move inside itself.");
    else if (moved) { snprintf(status_hint, sizeof status_hint, "Moved %d item%s to \"%s\"", moved, moved==1?"":"s", dname); draw(); }
}
static void drag_run(int down_cx, int down_cy)
{
    int ox = mouse_x() - down_cx, oy = mouse_y() - down_cy;
    int sx = mouse_x(), sy = mouse_y();
    int target = -1, crumb_k = -2, last_target = -1, last_crumb = -2;
    uint32_t last_scroll = now();
    int dx = 0, dy = 0;
    while (mouse_down()) {
        int cx = mouse_x() - ox, cy = mouse_y() - oy;
        dx = cx - down_cx; dy = cy - down_cy;
        /* auto-scroll near the edges */
        if (now() - last_scroll > 6) {
            if (cy < CT+14 && scroll_px > 0) { scroll_px -= 10; clamp_scroll(); last_scroll = now(); }
            else if (cy > CB-14 && scroll_px < max_scroll()) { scroll_px += 10; clamp_scroll(); last_scroll = now(); }
        }
        crumb_k = -2;
        target = drop_target(cx, cy, &crumb_k);
        if (target != last_target || crumb_k != last_crumb) { last_target = target; last_crumb = crumb_k; }
        /* redraw with ghosts + highlight */
        gfx_target(&win.surf);
        gfx_fill(0, CT, SBX, CB, 0);
        if (view_mode == 0) {
            for (int i = 0; i < nent; i++) {
                if (selset[i]) {
                    int l,t,r,b; if (icon_rect(i,&l,&t,&r,&b)) gfx_frame(l+(CW-32)/2, t+2, l+(CW-32)/2+32, t+34);
                } else draw_icon_entry(i, 0, 0, 0);
            }
            if (target >= 0) {
                int l,t,r,b; if (icon_rect(target,&l,&t,&r,&b)) {
                    gfx_invert(l+(CW-32)/2, t+2, l+(CW-32)/2+32, t+34);
                }
            }
            for (int i = 0; i < nent; i++) if (selset[i]) draw_icon_entry(i, dx, dy, 1);
        } else {
            draw_list();
            if (target >= 0) {
                int y = CT+LHDR+target*ROWH-scroll_px;
                if (y >= CT+LHDR && y < CB) gfx_invert(0, y, SBX, y+ROWH);
            }
        }
        draw_scrollbar();
        char hint[64];
        if (crumb_k != -2) snprintf(hint, sizeof hint, "Move to \"%s\"", crumb_k<0 ? "Disks" : pstack[crumb_k].name);
        else if (target >= 0) snprintf(hint, sizeof hint, "Move %d item%s to \"%s\"", nsels, nsels==1?"":"s", ent[target].name);
        else snprintf(hint, sizeof hint, "Moving %d item%s", nsels, nsels==1?"":"s");
        draw_status(hint);
        gwin_update(&win);
        nap(1);
        (void)sx; (void)sy;
    }
    if (crumb_k != -2 || target >= 0) move_selection_to(target, crumb_k);
    else if (view_mode == 0) {
        /* reposition the dragged icons on the canvas */
        for (int i = 0; i < nent; i++) if (selset[i]) {
            int nx = ent[i].x + dx, ny = ent[i].y + dy;
            if (nx < 0) nx = 0;
            if (nx > AREAW-CW) nx = AREAW-CW;
            if (ny < 0) ny = 0;
            ent[i].x = nx; ent[i].y = ny;
        }
        draw();
    } else draw();
    status_hint[0] = 0;
}
/* rubber-band selection on empty canvas */
static void marquee_run(int down_cx, int down_cy, int add)
{
    int ox = mouse_x() - down_cx, oy = mouse_y() - down_cy;
    int ax = down_cx, ay = down_cy - CT + scroll_px;
    if (!add) sel_clear();
    while (mouse_down()) {
        int cx = mouse_x() - ox, cy = mouse_y() - oy;
        int bx = cx, by = cy - CT + scroll_px;
        int l = mini(ax,bx), r = maxi(ax,bx), t = mini(ay,by), b = maxi(ay,by);
        gfx_target(&win.surf);
        if (view_mode == 0) draw_icons(); else draw_list();
        /* dotted rectangle */
        int st = CT + t - scroll_px, sb = CT + b - scroll_px;
        gfx_clip(0, CT, SBX, CB);
        for (int x = l; x < r; x += 2) { gfx_pixel(x, st, 1); gfx_pixel(x, sb-1, 1); }
        for (int y = st; y < sb; y += 2) { gfx_pixel(l, y, 1); gfx_pixel(r-1, y, 1); }
        gfx_noclip();
        draw_scrollbar();
        gwin_update(&win);
        nap(1);
    }
    int cx = mouse_x() - ox, cy = mouse_y() - oy;
    int bx = cx, by = cy - CT + scroll_px;
    int l = mini(ax,bx), r = maxi(ax,bx), t = mini(ay,by), b = maxi(ay,by);
    if (view_mode == 0) {
        for (int i = 0; i < nent; i++)
            if (ent[i].x < r && ent[i].x+CW > l && ent[i].y < b && ent[i].y+CH-4 > t) sel_add(i);
    } else {
        for (int i = 0; i < nent; i++) {
            int ry = i*ROWH;
            if (ry < b && ry+ROWH > t) sel_add(i);
        }
    }
    draw();
}

/* ---- pointer dispatch ----------------------------------------------------- */
static uint32_t last_click_t; static int last_click_i = -1;

static void on_mousedown(int cx, int cy)
{
    origin_x = mouse_x() - cx; origin_y = mouse_y() - cy;
    status_hint[0] = 0;
    if (cy < MENU_B) { int m = menu_title_at(cx, cy); if (m >= 0) menu_run(m); return; }
    if (cy < INFO_B) {
        if (cx >= 4 && cx < 22) { nav_up(); draw(); return; }
        for (int c = 0; c < ncrumb; c++) if (cx >= crumb[c].l && cx <= crumb[c].r) { nav_crumb(crumb[c].k); draw(); return; }
        return;
    }
    if (cx >= SBX) { if (cy >= CT && cy < CB) scrollbar_run(cy); return; }
    if (cy >= CB) return;
    if (view_mode == 1 && cy < CT+LHDR) {
        struct { int x; int key; } cols[] = {{26,0},{250,1},{312,2},{372,3}};
        for (int c = 3; c >= 0; c--) if (cx >= cols[c].x-6) {
            if (sort_key == cols[c].key) sort_asc = !sort_asc; else { sort_key = cols[c].key; sort_asc = 1; }
            sort_entries(); scroll_px = 0; draw(); return;
        }
        return;
    }
    int i = view_mode == 0 ? icon_at(cx, cy)
                           : (cy >= CT+LHDR ? (cy-CT-LHDR+scroll_px)/ROWH : -1);
    if (i >= nent) i = -1;
    if (i < 0) { marquee_run(cx, cy, shift_down()); return; }
    int was_sel = selset[i];
    if (shift_down()) sel_toggle(i);
    else if (!selset[i]) sel_only(i);
    anchor = i;
    draw();
    uint32_t t = now();
    int dbl = (i == last_click_i && t - last_click_t < 32);
    last_click_i = i; last_click_t = t;
    if (dbl) { last_click_i = -1; open_entry(i); return; }
    /* wait for a drag or a plain release */
    int sx = mouse_x(), sy = mouse_y();
    while (mouse_down()) {
        if (mouse_x()-sx > 3 || sx-mouse_x() > 3 || mouse_y()-sy > 3 || sy-mouse_y() > 3) {
            drag_run(cx, cy); return;
        }
        nap(1);
    }
    /* click on the name of an already-selected single item -> rename */
    if (was_sel && nsels == 1 && !shift_down()) {
        if (view_mode == 0) {
            int l,tt,r,b; icon_rect(i,&l,&tt,&r,&b);
            if (cy >= tt+34) inline_rename(i);
        } else inline_rename(i);
    }
}

/* ---- keyboard ------------------------------------------------------------- */
static char typebuf[16]; static int typelen; static uint32_t typet;
static void type_ahead(int ch)
{
    uint32_t t = now();
    if (t - typet > 60) typelen = 0;
    typet = t;
    if (typelen < 15) typebuf[typelen++] = (char)ch;
    typebuf[typelen] = 0;
    int start = anchor + 1;
    for (int n = 0; n < nent; n++) {
        int i = (start + n) % nent;
        if (!strncasecmp(ent[i].name, typebuf, typelen)) { sel_only(i); ensure_visible(i); draw(); return; }
    }
}
static void on_key(int key)
{
    int cmd = cmd_down();
    if (cmd) {
        switch (key) {
        case 'n': case 'N': do_new_folder(); return;
        case 't': case 'T': do_new_file(); return;
        case 'o': case 'O': do_open(); return;
        case 'i': case 'I': { int i=sel_first(); if (i>=0) get_info(i); return; }
        case 'd': case 'D': do_duplicate(); return;
        case 'x': case 'X': do_copy(1); return;
        case 'c': case 'C': do_copy(0); return;
        case 'v': case 'V': do_paste(); return;
        case 'a': case 'A': do_select_all(); return;
        case 'w': case 'W': case 'q': case 'Q': do_quit(); return;
        }
        if (key == KEY_UP) { nav_up(); draw(); return; }
    }
    switch (key) {
    case KEY_UP: case KEY_DOWN: case KEY_LEFT: case KEY_RIGHT: {
        if (!nent) return;
        int i = anchor < 0 ? 0 : anchor;
        int step = view_mode == 0 ? COLS : 1;
        if (key == KEY_UP) i -= step;
        else if (key == KEY_DOWN) i += step;
        else if (key == KEY_LEFT) i -= 1;
        else i += 1;
        if (i < 0) i = 0;
        if (i >= nent) i = nent-1;
        sel_only(i); ensure_visible(i); draw();
        return;
    }
    case KEY_RETURN: case KEY_ENTER: { int i = sel_first(); if (i>=0) inline_rename(i); return; }
    case KEY_BACKSPACE: case KEY_DELETE: do_delete(); return;
    case KEY_PGUP: scroll_by(-VIEWH); return;
    case KEY_PGDN: scroll_by(VIEWH); return;
    case KEY_HOME: if (nent) { sel_only(0); ensure_visible(0); draw(); } return;
    case KEY_END: if (nent) { sel_only(nent-1); ensure_visible(nent-1); draw(); } return;
    case KEY_ESC: sel_clear(); draw(); return;
    }
    if (key >= 0x20 && key < 0x7F) type_ahead(key);
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

int main(void)
{
    if (!under_desktop()) {
        printf("Files is a desktop program - open it from the desktop.\n");
        return 0;
    }
    if (gwin_open(&win, "Disks", WINW, WINH) != 0) return 1;
    build_art();
    layout_menus();
    gfx_target(&win.surf);
    load_disks();
    if (nent == 1) nav_disk(0);
    draw();

    for (;;) {
        if (want_quit) break;
        GEvent e;
        int r = gwin_event(&win, &e, 0);
        if (r < 0) break;
        if (r > 0) {
            if (e.type == GEV_CLOSE) break;
            if (e.type == GEV_MOUSEDOWN) on_mousedown(e.x, e.y);
            else if (e.type == GEV_KEY) on_key(e.key);
        } else nap(2);
    }
    gwin_close(&win);
    return 0;
}