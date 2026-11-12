/* macloader.c - Classic Macintosh ('APPL' / 'CODE' Resource) Executable Loader
 *
 * Implements the Macintosh Segment Loader & A5 World runtime architecture:
 *
 *   1. Locates the executable's Resource Fork via:
 *      - HFS Resource Fork (OpenRF on ":bin:<name>" or "<name>")
 *      - MacBinary I/II (.bin) container in the HFS Data Fork
 *      - Raw Macintosh Resource Fork (.rsrc) in the HFS Data Fork
 *   2. Parses the Macintosh Resource Header & Resource Map to locate 'CODE'
 *      resources (type 0x434F4445).
 *   3. Reads 'CODE' ID 0 (A5 World descriptor + Jump Table):
 *        +0x00: uint32_t aboveA5   (bytes above A5, including Jump Table)
 *        +0x04: uint32_t belowA5   (bytes below A5 for app globals)
 *        +0x08: uint32_t jtSize    (Jump Table size in bytes)
 *        +0x0C: uint32_t jtOffset  (Jump Table offset from A5, usually 0x20)
 *        +0x10: uint8_t  jt[jtSize]
 *   4. Allocates the application's A5 World, sets CurrentA5 ($0904),
 *      CurJTOffset ($0934), CurApName ($0910), preloads 'CODE' segments 1..N,
 *      and patches every 8-byte Jump Table entry from:
 *        dc.w segOffset, $3F3C (MOVE.W #seg,-(SP)), segID, $A9F0 (_LoadSeg)
 *      to the loaded jump form:
 *        dc.w segID, $4EF9 (JMP.L), segAddr + 4 + segOffset
 *   5. Hooks _LoadSeg ($A9F0), _UnLoadSeg ($A9F1), _GetAppParms ($A9F5), and
 *      _ExitToShell ($A9F4) so the classic Mac program returns cleanly to
 *      PlusOS when it quits.
 */
#include "kernel.h"
#include "proc.h"

#define FOURCC_CODE 0x434F4445UL    /* 'CODE' */
#define MAX_MAC_SEGS 32

typedef struct {
    int16_t   id;
    uint32_t  size;
    uint8_t  *data;                 /* kmalloc'd copy of segment (including 4-byte hdr) */
} MacSeg;

typedef struct {
    uint8_t  *rf_buf;               /* raw resource fork image */
    uint32_t  rf_len;
    uint8_t  *a5_alloc;             /* kmalloc'd A5 world block */
    uint8_t  *app_a5;               /* A5 pointer inside a5_alloc */
    uint32_t  above_a5;
    uint32_t  below_a5;
    uint32_t  jt_size;
    uint32_t  jt_offset;
    MacSeg    segs[MAX_MAC_SEGS];
    int       nsegs;
    char      ap_name[32];
} MacApp;

/* Saved kernel state across a classic Mac application execution */
static uint32_t mac_saved_sp;
static uint32_t mac_saved_a5;
static uint32_t mac_saved_cur_a5;
static uint16_t mac_saved_jt_off;
static void    *mac_orig_exittoshell;
static void    *mac_orig_loadseg;
static void    *mac_orig_unloadseg;
static void    *mac_orig_getappparms;
static MacApp  *mac_active_app;

static int has_colon(const char *s)
{
    for (; *s; s++) if (*s == ':') return 1;
    return 0;
}

/* Validate whether [buf, len) begins with a Macintosh Resource Fork header */
static int is_valid_rsrc_fork(const uint8_t *buf, uint32_t len)
{
    if (!buf || len < 64) return 0;
    uint32_t data_off = *(const uint32_t *)(buf + 0);
    uint32_t map_off  = *(const uint32_t *)(buf + 4);
    uint32_t data_len = *(const uint32_t *)(buf + 8);
    uint32_t map_len  = *(const uint32_t *)(buf + 12);
    if (data_off < 16 || data_off >= len) return 0;
    if (map_off < 16 || map_off + 28 > len) return 0;
    if (data_off + data_len > len || map_off + map_len > len) return 0;
    return 1;
}

/* Locate a resource by (type, id) in a raw Macintosh Resource Fork buffer */
static const uint8_t *rsrc_find(const uint8_t *rf, uint32_t rf_len,
                                uint32_t target_type, int16_t target_id,
                                uint32_t *out_size)
{
    if (!is_valid_rsrc_fork(rf, rf_len)) return 0;
    uint32_t data_off = *(const uint32_t *)(rf + 0);
    uint32_t map_off  = *(const uint32_t *)(rf + 4);
    const uint8_t *map = rf + map_off;
    uint16_t type_list_off = *(const uint16_t *)(map + 24);
    if (map_off + type_list_off + 2 > rf_len) return 0;

    const uint8_t *tlist = map + type_list_off;
    int16_t num_types_m1 = *(const int16_t *)tlist;
    if (num_types_m1 < 0) return 0;
    int num_types = (int)num_types_m1 + 1;

    const uint8_t *tentry = tlist + 2;
    for (int t = 0; t < num_types; t++, tentry += 8) {
        if ((uint32_t)(tentry - rf) + 8 > rf_len) break;
        uint32_t rtype = *(const uint32_t *)(tentry + 0);
        int num_refs   = (int)(*(const uint16_t *)(tentry + 4)) + 1;
        uint16_t ref_off = *(const uint16_t *)(tentry + 6);
        if (rtype != target_type) continue;

        const uint8_t *ref = tlist + ref_off;
        for (int r = 0; r < num_refs; r++, ref += 12) {
            if ((uint32_t)(ref - rf) + 12 > rf_len) break;
            int16_t rid = *(const int16_t *)(ref + 0);
            if (rid != target_id) continue;
            uint32_t attr_doff = *(const uint32_t *)(ref + 4);
            uint32_t roff = data_off + (attr_doff & 0x00FFFFFFUL);
            if (roff + 4 > rf_len) return 0;
            uint32_t rsize = *(const uint32_t *)(rf + roff);
            if (roff + 4 + rsize > rf_len) return 0;
            if (out_size) *out_size = rsize;
            return rf + roff + 4;
        }
    }
    return 0;
}

/* Enumerate all resources of a given type by index (0..N-1) */
static const uint8_t *rsrc_get_by_index(const uint8_t *rf, uint32_t rf_len,
                                        uint32_t target_type, int idx,
                                        int16_t *out_id, uint32_t *out_size)
{
    if (!is_valid_rsrc_fork(rf, rf_len)) return 0;
    uint32_t data_off = *(const uint32_t *)(rf + 0);
    uint32_t map_off  = *(const uint32_t *)(rf + 4);
    const uint8_t *map = rf + map_off;
    uint16_t type_list_off = *(const uint16_t *)(map + 24);
    if (map_off + type_list_off + 2 > rf_len) return 0;

    const uint8_t *tlist = map + type_list_off;
    int16_t num_types_m1 = *(const int16_t *)tlist;
    if (num_types_m1 < 0) return 0;
    int num_types = (int)num_types_m1 + 1;

    const uint8_t *tentry = tlist + 2;
    for (int t = 0; t < num_types; t++, tentry += 8) {
        if ((uint32_t)(tentry - rf) + 8 > rf_len) break;
        uint32_t rtype = *(const uint32_t *)(tentry + 0);
        int num_refs   = (int)(*(const uint16_t *)(tentry + 4)) + 1;
        uint16_t ref_off = *(const uint16_t *)(tentry + 6);
        if (rtype != target_type) continue;
        if (idx < 0 || idx >= num_refs) return 0;

        const uint8_t *ref = tlist + ref_off + idx * 12;
        if ((uint32_t)(ref - rf) + 12 > rf_len) return 0;
        int16_t rid = *(const int16_t *)(ref + 0);
        uint32_t attr_doff = *(const uint32_t *)(ref + 4);
        uint32_t roff = data_off + (attr_doff & 0x00FFFFFFUL);
        if (roff + 4 > rf_len) return 0;
        uint32_t rsize = *(const uint32_t *)(rf + roff);
        if (roff + 4 + rsize > rf_len) return 0;
        if (out_id)   *out_id = rid;
        if (out_size) *out_size = rsize;
        return rf + roff + 4;
    }
    return 0;
}

static int try_open_path(const char *path, int rsrc_fork, INTEGER *ref)
{
    Str255 pname;
    c2pstr_buf(pname, path);
    if (rsrc_fork) return OpenRF(pname, g_boot_vref, ref) == 0 ? 0 : -1;
    return FSOpen(pname, g_boot_vref, ref) == 0 ? 0 : -1;
}

static int open_candidate(const char *name, int rsrc_fork, INTEGER *ref)
{
    if (!has_colon(name)) {
        char path[64];
        snprintf(path, sizeof path, ":bin:%s", name);
        if (try_open_path(path, rsrc_fork, ref) == 0) return 0;
        snprintf(path, sizeof path, ":Desktop:%s", name);
        if (try_open_path(path, rsrc_fork, ref) == 0) return 0;
    }
    return try_open_path(name, rsrc_fork, ref);
}

/* Load the resource fork of `name` (from HFS resource fork, raw .rsrc data
 * fork, or MacBinary .bin container) into a kmalloc'd buffer. */
static int load_rsrc_fork(const char *name, uint8_t **out_buf, uint32_t *out_len)
{
    if (!(g_flags & PLUSOS_HAVE_FS)) return -1;
    INTEGER ref;

    /* 1. Try HFS Resource Fork via OpenRF */
    if (open_candidate(name, 1, &ref) == 0) {
        LONGINT eof = 0;
        if (GetEOF(ref, &eof) == 0 && eof >= 64 && eof < 512 * 1024) {
            uint8_t *buf = kmalloc((uint32_t)eof);
            if (buf) {
                LONGINT n = eof;
                if (SetFPos(ref, 1, 0) == 0 && FSRead(ref, &n, buf) == 0 && n == eof) {
                    if (is_valid_rsrc_fork(buf, (uint32_t)eof) &&
                        rsrc_find(buf, (uint32_t)eof, FOURCC_CODE, 0, 0)) {
                        FSClose(ref);
                        *out_buf = buf;
                        *out_len = (uint32_t)eof;
                        return 0;
                    }
                }
                kfree(buf);
            }
        }
        FSClose(ref);
    }

    /* 2. Try HFS Data Fork (.rsrc or MacBinary .bin) */
    if (open_candidate(name, 0, &ref) == 0) {
        LONGINT eof = 0;
        if (GetEOF(ref, &eof) == 0 && eof >= 64 && eof < 512 * 1024) {
            uint8_t hdr[128];
            LONGINT hn = eof < 128 ? eof : 128;
            if (SetFPos(ref, 1, 0) == 0 && FSRead(ref, &hn, hdr) == 0) {
                uint32_t rf_offset = 0;
                uint32_t rf_size   = (uint32_t)eof;
                /* Check for 128-byte MacBinary header */
                if (hn == 128 && hdr[0] == 0 && hdr[1] >= 1 && hdr[1] <= 63 && hdr[74] == 0) {
                    uint32_t df_len = *(uint32_t *)(hdr + 83);
                    uint32_t rlen   = *(uint32_t *)(hdr + 87);
                    uint32_t roff   = 128 + ((df_len + 127u) & ~127u);
                    if (rlen >= 64 && roff + rlen <= (uint32_t)eof) {
                        rf_offset = roff;
                        rf_size   = rlen;
                    }
                }
                uint8_t *buf = kmalloc(rf_size);
                if (buf) {
                    LONGINT n = (LONGINT)rf_size;
                    if (SetFPos(ref, 1, (LONGINT)rf_offset) == 0 &&
                        FSRead(ref, &n, buf) == 0 && (uint32_t)n == rf_size) {
                        if (is_valid_rsrc_fork(buf, rf_size) &&
                            rsrc_find(buf, rf_size, FOURCC_CODE, 0, 0)) {
                            FSClose(ref);
                            *out_buf = buf;
                            *out_len = rf_size;
                            return 0;
                        }
                    }
                    kfree(buf);
                }
            }
        }
        FSClose(ref);
    }
    return -1;
}

/* Patch all Jump Table entries belonging to `seg_id` so they jump directly
 * to `seg_base + 4 + offset`. */
static void patch_jump_table(MacApp *app, int16_t seg_id, uint8_t *seg_base)
{
    uint8_t *jt = app->app_a5 + app->jt_offset;
    uint32_t nentries = app->jt_size / 8;
    for (uint32_t i = 0; i < nentries; i++) {
        uint8_t *e = jt + i * 8;
        uint16_t w0 = *(uint16_t *)(e + 0);
        uint16_t w1 = *(uint16_t *)(e + 2);
        int16_t  w2 = *(int16_t  *)(e + 4);
        uint16_t w3 = *(uint16_t *)(e + 6);
        /* Unloaded entry: offset, $3F3C (MOVE.W #id,-(SP)), segID, $A9F0 (_LoadSeg) */
        if (w1 == 0x3F3C && w2 == seg_id && w3 == 0xA9F0) {
            uint32_t target = (uint32_t)(seg_base + 4 + w0);
            *(int16_t  *)(e + 0) = seg_id;
            *(uint16_t *)(e + 2) = 0x4EF9;      /* JMP.L abs.L */
            *(uint32_t *)(e + 4) = target;
        }
    }
}

/* C helper called by our _LoadSeg ($A9F0) trap handler if an unpatched Jump
 * Table entry is ever invoked at runtime. Returns the target PC to jump to. */
uint32_t mac_c_loadseg(int16_t seg_id, uint8_t *ret_pc)
{
    MacApp *app = mac_active_app;
    if (!app) return (uint32_t)ret_pc;
    for (int i = 0; i < app->nsegs; i++) {
        if (app->segs[i].id == seg_id) {
            patch_jump_table(app, seg_id, app->segs[i].data);
            cache_flush();
            return (uint32_t)(ret_pc - 6);      /* re-execute the patched JMP.L */
        }
    }
    return (uint32_t)ret_pc;
}

/* C helper for _GetAppParms ($A9F5): Pascal void GetAppParms(Str255 apName,
 * INTEGER *apRefNum, Handle *apParam) */
void mac_c_getappparms(uint8_t *ap_name, int16_t *ap_ref, void **ap_param)
{
    if (ap_name) {
        const char *nm = mac_active_app ? mac_active_app->ap_name : "MacApp";
        c2pstr_buf(ap_name, nm);
    }
    if (ap_ref)   *ap_ref = 0;
    if (ap_param) *ap_param = (void *)LM_LONG(0xAEC);
}

/* Assembly trap handlers for _ExitToShell ($A9F4), _LoadSeg ($A9F0),
 * _UnLoadSeg ($A9F1), and _GetAppParms ($A9F5). */
extern void mac_trap_exittoshell(void);
extern void mac_trap_loadseg(void);
extern void mac_trap_unloadseg(void);
extern void mac_trap_getappparms(void);
extern void mac_enter_app(void *entry_jt, void *app_a5, uint32_t *save_sp, uint32_t *save_a5);

__asm__(
    "    .text\n"
    "    .align 2\n"
    "    .globl mac_enter_app\n"
    "mac_enter_app:\n"
    "    movem.l %d2-%d7/%a2-%a6, -(%sp)\n"
    "    movea.l 48(%sp), %a0\n"        /* entry_jt */
    "    movea.l 52(%sp), %a1\n"        /* app_a5 */
    "    movea.l 56(%sp), %a2\n"        /* save_sp */
    "    movea.l 60(%sp), %a3\n"        /* save_a5 */
    "    move.l  %sp, (%a2)\n"
    "    move.l  %a5, (%a3)\n"
    "    movea.l %a1, %a5\n"
    "    jsr     (%a0)\n"
    "    .globl mac_trap_exittoshell\n"
    "mac_trap_exittoshell:\n"
    "    movea.l mac_saved_a5, %a5\n"
    "    movea.l mac_saved_sp, %sp\n"
    "    movem.l (%sp)+, %d2-%d7/%a2-%a6\n"
    "    rts\n"
    "\n"
    "    .globl mac_trap_loadseg\n"
    "mac_trap_loadseg:\n"
    "    move.l  (%sp)+, %a0\n"         /* return PC (just after _LoadSeg) */
    "    move.w  (%sp)+, %d0\n"         /* segID pushed by MOVE.W #id,-(SP) */
    "    move.l  %a0, -(%sp)\n"
    "    ext.l   %d0\n"
    "    move.l  %d0, -(%sp)\n"
    "    jsr     mac_c_loadseg\n"
    "    addq.l  #8, %sp\n"
    "    movea.l %d0, %a0\n"
    "    jmp     (%a0)\n"
    "\n"
    "    .globl mac_trap_unloadseg\n"
    "mac_trap_unloadseg:\n"
    "    move.l  (%sp)+, %a0\n"         /* return address */
    "    addq.l  #4, %sp\n"             /* pop Ptr routineAddr */
    "    jmp     (%a0)\n"
    "\n"
    "    .globl mac_trap_getappparms\n"
    "mac_trap_getappparms:\n"
    "    move.l  (%sp)+, %a1\n"         /* return address */
    "    move.l  (%sp)+, %d0\n"         /* apParam */
    "    move.l  (%sp)+, %d1\n"         /* apRefNum */
    "    move.l  (%sp)+, %d2\n"         /* apName */
    "    move.l  %a1, -(%sp)\n"
    "    move.l  %d0, -(%sp)\n"
    "    move.l  %d1, -(%sp)\n"
    "    move.l  %d2, -(%sp)\n"
    "    jsr     mac_c_getappparms\n"
    "    lea     12(%sp), %sp\n"
    "    rts\n"
);

static int mac_execute_app(MacApp *app)
{
    const uint8_t *code0 = rsrc_find(app->rf_buf, app->rf_len, FOURCC_CODE, 0, 0);
    if (!code0) return -1002;

    app->above_a5  = *(const uint32_t *)(code0 + 0);
    app->below_a5  = *(const uint32_t *)(code0 + 4);
    app->jt_size   = *(const uint32_t *)(code0 + 8);
    app->jt_offset = *(const uint32_t *)(code0 + 12);

    if (app->jt_size < 8 || app->above_a5 > 0x40000 || app->below_a5 > 0x40000)
        return -1002;

    uint32_t total_a5 = app->below_a5 + app->above_a5 + 256;
    app->a5_alloc = kmalloc(total_a5);
    if (!app->a5_alloc) return -1003;
    memset(app->a5_alloc, 0, total_a5);
    app->app_a5 = app->a5_alloc + ((app->below_a5 + 15u) & ~15u);

    /* Copy initial Jump Table into A5 + jtOffset */
    memcpy(app->app_a5 + app->jt_offset, code0 + 16, app->jt_size);

    /* Point 0(A5) at the screen GrafPort pointer so QuickDraw works immediately
     * even if the application draws before calling InitGraf. */
    *(void **)app->app_a5 = &qd.thePort;

    /* Load all 'CODE' segments (ID != 0) and patch the Jump Table */
    for (int idx = 0; idx < MAX_MAC_SEGS; idx++) {
        int16_t seg_id = 0;
        uint32_t seg_sz = 0;
        const uint8_t *seg_ptr = rsrc_get_by_index(app->rf_buf, app->rf_len,
                                                   FOURCC_CODE, idx, &seg_id, &seg_sz);
        if (!seg_ptr) break;
        if (seg_id == 0 || seg_sz < 6) continue;
        uint8_t *sbuf = kmalloc(seg_sz);
        if (!sbuf) continue;
        memcpy(sbuf, seg_ptr, seg_sz);
        app->segs[app->nsegs].id   = seg_id;
        app->segs[app->nsegs].size = seg_sz;
        app->segs[app->nsegs].data = sbuf;
        app->nsegs++;
        patch_jump_table(app, seg_id, sbuf);
    }

    if (app->nsegs == 0) {
        kfree(app->a5_alloc);
        return -1002;
    }

    /* Save kernel low-memory globals and Segment Loader trap vectors */
    mac_saved_cur_a5 = LM_LONG(0x904);
    mac_saved_jt_off = LM_WORD(0x934);
    mac_orig_exittoshell = *plusrom_trap_slot(0xA9F4);
    mac_orig_loadseg     = *plusrom_trap_slot(0xA9F0);
    mac_orig_unloadseg   = *plusrom_trap_slot(0xA9F1);
    mac_orig_getappparms = *plusrom_trap_slot(0xA9F5);

    LM_LONG(0x904) = (uint32_t)app->app_a5;             /* CurrentA5 */
    LM_WORD(0x934) = (uint16_t)app->jt_offset;          /* CurJTOffset */
    c2pstr_buf((uint8_t *)0x910, app->ap_name);         /* CurApName */

    *plusrom_trap_slot(0xA9F4) = (void *)mac_trap_exittoshell;
    *plusrom_trap_slot(0xA9F0) = (void *)mac_trap_loadseg;
    *plusrom_trap_slot(0xA9F1) = (void *)mac_trap_unloadseg;
    *plusrom_trap_slot(0xA9F5) = (void *)mac_trap_getappparms;

    mac_active_app = app;
    cache_flush();

    /* Enter the Macintosh application at Jump Table entry 0 (+2 skips segID) */
    void *entry_jt = (void *)(app->app_a5 + app->jt_offset + 2);
    mac_enter_app(entry_jt, app->app_a5, &mac_saved_sp, &mac_saved_a5);

    /* Restore kernel Segment Loader traps, A5 World, and QuickDraw port */
    mac_active_app = 0;
    *plusrom_trap_slot(0xA9F4) = mac_orig_exittoshell;
    *plusrom_trap_slot(0xA9F0) = mac_orig_loadseg;
    *plusrom_trap_slot(0xA9F1) = mac_orig_unloadseg;
    *plusrom_trap_slot(0xA9F5) = mac_orig_getappparms;
    LM_LONG(0x904) = mac_saved_cur_a5;
    LM_WORD(0x934) = mac_saved_jt_off;

    InitGraf(&qd.thePort);
    SetPort(&g_port);
    HideCursor();

    for (int i = 0; i < app->nsegs; i++)
        kfree(app->segs[i].data);
    kfree(app->a5_alloc);
    return 0;
}

int mac_try_run(const char *name, int argc, char **argv)
{
    (void)argc; (void)argv;
    uint8_t *rf_buf = 0;
    uint32_t rf_len = 0;
    if (load_rsrc_fork(name, &rf_buf, &rf_len) != 0)
        return -1001;

    MacApp *app = kmalloc(sizeof(MacApp));
    if (!app) { kfree(rf_buf); return -1003; }
    memset(app, 0, sizeof(*app));
    app->rf_buf = rf_buf;
    app->rf_len = rf_len;

    const char *leaf = name;
    for (const char *s = name; *s; s++) if (*s == ':') leaf = s + 1;
    strlcpy(app->ap_name, leaf, sizeof app->ap_name);

    int rc = mac_execute_app(app);
    kfree(rf_buf);
    kfree(app);
    return rc;
}
