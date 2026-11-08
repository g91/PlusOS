/* macrun.c - Classic Macintosh ('APPL' / 'CODE' Resource) Inspector & Launcher
 *
 * Usage:
 *   macrun --info <file>    Inspect Resource Map, 'CODE' 0 A5 World & Jump Table
 *   macrun --makedemo       Build a real 68000 Macintosh 'APPL' ("MacDemo") on
 *                           the HFS volume with 'CODE' 0 and 'CODE' 1 resources
 *   macrun <file>           Inspect and launch a classic Macintosh application
 */
#include <plusos/plusos.h>
#include <plusos/syscall.h>
#include <plusrom/FileMgr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FOURCC_CODE 0x434F4445UL

/* Machine code for our authentic 68000 Macintosh 'APPL' demo (CODE 1):
 * Calls ROM QuickDraw traps directly to draw a classic Macintosh screen,
 * plays a tone with _SysBeep, waits for a mouse click or keypress, and
 * exits cleanly via _ExitToShell ($A9F4).
 *
 * Segment header (4 bytes):
 *   0x0000, 0x0001 (first JT offset = 0, 1 entry)
 * Entry routine (+4):
 *   link    a6, #-32
 *   pea     -16(a6)             ; Rect r1 = {0, 0, 342, 512}
 *   clr.w   -(sp)
 *   clr.w   -(sp)
 *   move.w  #512, -(sp)
 *   move.w  #342, -(sp)
 *   .short  0xA8A7              ; _SetRect
 *   pea     -16(a6)
 *   .short  0xA8A3              ; _EraseRect
 *   pea     -16(a6)             ; Outer border
 *   move.w  #20, -(sp)
 *   move.w  #20, -(sp)
 *   move.w  #492, -(sp)
 *   move.w  #322, -(sp)
 *   .short  0xA8A7              ; _SetRect
 *   pea     -16(a6)
 *   .short  0xA8A1              ; _FrameRect
 *   pea     -16(a6)             ; Center oval
 *   move.w  #156, -(sp)
 *   move.w  #86, -(sp)
 *   move.w  #356, -(sp)
 *   move.w  #256, -(sp)
 *   .short  0xA8A7              ; _SetRect
 *   pea     -16(a6)
 *   .short  0xA8B7              ; _FrameOval
 *   move.w  #130, -(sp)
 *   move.w  #60, -(sp)
 *   .short  0xA893              ; _MoveTo
 *   pea     title_str(pc)
 *   .short  0xA884              ; _DrawString
 *   move.w  #136, -(sp)
 *   move.w  #295, -(sp)
 *   .short  0xA893              ; _MoveTo
 *   pea     prompt_str(pc)
 *   .short  0xA884              ; _DrawString
 *   move.w  #8, -(sp)
 *   .short  0xA9C8              ; _SysBeep
 * wait_click:
 *   clr.w   -(sp)
 *   .short  0xA974              ; _Button
 *   tst.b   (sp)+
 *   beq.s   wait_click
 * wait_up:
 *   clr.w   -(sp)
 *   .short  0xA974              ; _Button
 *   tst.b   (sp)+
 *   bne.s   wait_up
 *   unlk    a6
 *   .short  0xA9F4              ; _ExitToShell
 */
static const uint16_t demo_code1[] = {
    0x0000, 0x0001,                         /* segment header */
    0x4E56, 0xFFE0,                         /* link a6, #-32 */
    0x486E, 0xFFF0,                         /* pea -16(a6) */
    0x4267,                                 /* clr.w -(sp) */
    0x4267,                                 /* clr.w -(sp) */
    0x3F3C, 0x0200,                         /* move.w #512, -(sp) */
    0x3F3C, 0x0156,                         /* move.w #342, -(sp) */
    0xA8A7,                                 /* _SetRect */
    0x486E, 0xFFF0,                         /* pea -16(a6) */
    0xA8A3,                                 /* _EraseRect */
    0x486E, 0xFFF0,                         /* pea -16(a6) */
    0x3F3C, 0x0014,                         /* move.w #20, -(sp) */
    0x3F3C, 0x0014,                         /* move.w #20, -(sp) */
    0x3F3C, 0x01EC,                         /* move.w #492, -(sp) */
    0x3F3C, 0x0142,                         /* move.w #322, -(sp) */
    0xA8A7,                                 /* _SetRect */
    0x486E, 0xFFF0,                         /* pea -16(a6) */
    0xA8A1,                                 /* _FrameRect */
    0x486E, 0xFFF0,                         /* pea -16(a6) */
    0x3F3C, 0x009C,                         /* move.w #156, -(sp) */
    0x3F3C, 0x0056,                         /* move.w #86, -(sp) */
    0x3F3C, 0x0164,                         /* move.w #356, -(sp) */
    0x3F3C, 0x0100,                         /* move.w #256, -(sp) */
    0xA8A7,                                 /* _SetRect */
    0x486E, 0xFFF0,                         /* pea -16(a6) */
    0xA8B7,                                 /* _FrameOval */
    0x3F3C, 0x0078,                         /* move.w #120, -(sp) */
    0x3F3C, 0x003C,                         /* move.w #60, -(sp) */
    0xA893,                                 /* _MoveTo */
    0x487A, 0x002C,                         /* pea title_str(pc) */
    0xA884,                                 /* _DrawString */
    0x3F3C, 0x0088,                         /* move.w #136, -(sp) */
    0x3F3C, 0x0127,                         /* move.w #295, -(sp) */
    0xA893,                                 /* _MoveTo */
    0x487A, 0x0046,                         /* pea prompt_str(pc) */
    0xA884,                                 /* _DrawString */
    0x3F3C, 0x0008,                         /* move.w #8, -(sp) */
    0xA9C8,                                 /* _SysBeep */
    0x4267,                                 /* wait_click: clr.w -(sp) */
    0xA974,                                 /* _Button */
    0x4A1F,                                 /* tst.b (sp)+ */
    0x67F8,                                 /* beq.s wait_click */
    0x4267,                                 /* wait_up: clr.w -(sp) */
    0xA974,                                 /* _Button */
    0x4A1F,                                 /* tst.b (sp)+ */
    0x66F8,                                 /* bne.s wait_up */
    0x4E5E,                                 /* unlk a6 */
    0xA9F4,                                 /* _ExitToShell */
    /* title_str (Pascal string, 39 bytes + 1 pad = 40 bytes): */
    0x274D, 0x6163, 0x4465, 0x6D6F, 0x3A20, 0x4E61, 0x7469, 0x7665,
    0x2036, 0x3830, 0x3030, 0x2027, 0x4150, 0x504C, 0x2720, 0x2843,
    0x4F44, 0x4520, 0x302F, 0x3129,
    /* prompt_str (Pascal string, 35 bytes + 1 pad = 36 bytes): */
    0x2343, 0x6C69, 0x636B, 0x206D, 0x6F75, 0x7365, 0x2062, 0x7574,
    0x746F, 0x6E20, 0x666F, 0x7220, 0x5F45, 0x7869, 0x7454, 0x6F53,
    0x6865, 0x6C6C
};

/* Build a complete Macintosh Resource Fork in `buf` containing CODE 0 and CODE 1 */
static uint32_t build_demo_rsrc_fork(uint8_t *buf)
{
    memset(buf, 0, 1024);
    uint32_t data_off = 256;

    /* CODE 0 (24 bytes): aboveA5=0x28, belowA5=0x100, jtSize=8, jtOffset=0x20,
     * JT[0]: offset=0x0000, $3F3C, segID=1, $A9F0 (_LoadSeg) */
    uint8_t code0[24] = {
        0x00, 0x00, 0x00, 0x28,             /* aboveA5 = 40 */
        0x00, 0x00, 0x01, 0x00,             /* belowA5 = 256 */
        0x00, 0x00, 0x00, 0x08,             /* jtSize = 8 */
        0x00, 0x00, 0x00, 0x20,             /* jtOffset = 32 */
        0x00, 0x00, 0x3F, 0x3C, 0x00, 0x01, 0xA9, 0xF0
    };
    uint32_t code0_len = sizeof(code0);
    uint32_t code1_len = sizeof(demo_code1);

    /* Resource Data section at +256 */
    uint32_t p = data_off;
    *(uint32_t *)(buf + p) = code0_len; p += 4;
    memcpy(buf + p, code0, code0_len);  p += code0_len;

    uint32_t code1_rel = p - data_off;
    *(uint32_t *)(buf + p) = code1_len; p += 4;
    memcpy(buf + p, demo_code1, code1_len); p += code1_len;

    uint32_t data_len = p - data_off;
    uint32_t map_off  = (p + 15u) & ~15u;

    /* Resource Map at map_off:
     * +0..15: copy of 16-byte header
     * +16..21: nextMap(4), refNum(2)
     * +22..23: attrs(2) = 0
     * +24..25: typeListOffset = 28
     * +26..27: nameListOffset = 28 + 2 + 8 + 24 = 62
     */
    uint8_t *map = buf + map_off;
    *(uint16_t *)(map + 24) = 28;
    *(uint16_t *)(map + 26) = 62;

    uint8_t *tlist = map + 28;
    *(int16_t  *)(tlist + 0) = 0;               /* 1 resource type - 1 = 0 */
    *(uint32_t *)(tlist + 2) = FOURCC_CODE;     /* 'CODE' */
    *(uint16_t *)(tlist + 6) = 1;               /* 2 resources - 1 = 1 */
    *(uint16_t *)(tlist + 8) = 10;              /* refList offset from tlist */

    uint8_t *rlist = tlist + 10;
    /* Ref 0: CODE ID 0 at data offset 0 */
    *(int16_t  *)(rlist + 0) = 0;
    *(int16_t  *)(rlist + 2) = -1;
    *(uint32_t *)(rlist + 4) = 0;
    /* Ref 1: CODE ID 1 at data offset code1_rel */
    *(int16_t  *)(rlist + 12) = 1;
    *(int16_t  *)(rlist + 14) = -1;
    *(uint32_t *)(rlist + 16) = code1_rel;

    uint32_t map_len = 64;

    /* Resource Fork Header at +0 (and copy in map[0..15]) */
    *(uint32_t *)(buf + 0)  = data_off;
    *(uint32_t *)(buf + 4)  = map_off;
    *(uint32_t *)(buf + 8)  = data_len;
    *(uint32_t *)(buf + 12) = map_len;
    memcpy(map, buf, 16);

    return map_off + map_len;
}

static int make_demo_app(void)
{
    uint8_t rf[1024];
    uint32_t rf_len = build_demo_rsrc_fork(rf);
    INTEGER vref = (INTEGER)plusos->boot_vrefnum;
    Str255 pname;
    c2pstr_buf(pname, ":Desktop:MacDemo");

    FSDelete(pname, vref);
    OSErr err = Create(pname, vref, 0x4D444D4FUL /* 'MDMO' */, 0x4150504CUL /* 'APPL' */);
    if (err != 0) {
        c2pstr_buf(pname, "MacDemo");
        FSDelete(pname, vref);
        err = Create(pname, vref, 0x4D444D4FUL, 0x4150504CUL);
        if (err != 0) {
            printf("macrun: Create failed (OSErr %d)\n", err);
            return 1;
        }
    }
    INTEGER ref;
    if (OpenRF(pname, vref, &ref) != 0) {
        printf("macrun: OpenRF failed\n");
        return 1;
    }
    LONGINT wlen = (LONGINT)rf_len;
    FSWrite(ref, &wlen, rf);
    SetEOF(ref, (LONGINT)rf_len);
    FSClose(ref);
    FlushVol(0, vref);
    printf("Created classic Macintosh 'APPL' executable (:Desktop:MacDemo, %lu bytes RF).\n",
           (unsigned long)rf_len);
    printf("Run it with:  macrun :Desktop:MacDemo   or   :Desktop:MacDemo\n");
    return 0;
}

static void inspect_rsrc(const uint8_t *rf, uint32_t len, const char *label)
{
    uint32_t data_off = *(const uint32_t *)(rf + 0);
    uint32_t map_off  = *(const uint32_t *)(rf + 4);
    uint32_t data_len = *(const uint32_t *)(rf + 8);
    uint32_t map_len  = *(const uint32_t *)(rf + 12);
    printf("Macintosh Resource Fork: %s (%lu bytes)\n", label, (unsigned long)len);
    printf("  Data Offset: 0x%06lx (%lu bytes)   Map Offset: 0x%06lx (%lu bytes)\n",
           (unsigned long)data_off, (unsigned long)data_len,
           (unsigned long)map_off, (unsigned long)map_len);

    if (map_off + 28 > len) return;
    const uint8_t *map = rf + map_off;
    uint16_t tlist_off = *(const uint16_t *)(map + 24);
    uint16_t nlist_off = *(const uint16_t *)(map + 26);
    const uint8_t *tlist = map + tlist_off;
    int16_t ntypes_m1 = *(const int16_t *)tlist;
    if (ntypes_m1 < 0) return;
    int ntypes = (int)ntypes_m1 + 1;
    printf("  Resource Types (%d):\n", ntypes);

    const uint8_t *tentry = tlist + 2;
    for (int t = 0; t < ntypes; t++, tentry += 8) {
        uint32_t rtype = *(const uint32_t *)(tentry + 0);
        int nrefs = (int)(*(const uint16_t *)(tentry + 4)) + 1;
        uint16_t roff = *(const uint16_t *)(tentry + 6);
        printf("    '%c%c%c%c' (%d resource%s):\n",
               (char)(rtype >> 24), (char)(rtype >> 16), (char)(rtype >> 8), (char)rtype,
               nrefs, nrefs == 1 ? "" : "s");
        const uint8_t *ref = tlist + roff;
        for (int r = 0; r < nrefs; r++, ref += 12) {
            int16_t id = *(const int16_t *)(ref + 0);
            int16_t noff = *(const int16_t *)(ref + 2);
            uint32_t doff = data_off + (*(const uint32_t *)(ref + 4) & 0x00FFFFFFUL);
            uint32_t rsz = (doff + 4 <= len) ? *(const uint32_t *)(rf + doff) : 0;
            char rname[36] = "";
            if (noff >= 0 && map_off + nlist_off + (uint32_t)noff < len) {
                const uint8_t *np = map + nlist_off + noff;
                int nl = np[0] < 32 ? np[0] : 32;
                memcpy(rname, np + 1, (size_t)nl);
                rname[nl] = 0;
            }
            printf("      ID %6d  size %6lu bytes  %s\n", id, (unsigned long)rsz, rname);
            if (rtype == FOURCC_CODE && id == 0 && rsz >= 16 && doff + 4 + rsz <= len) {
                const uint8_t *c0 = rf + doff + 4;
                uint32_t above  = *(const uint32_t *)(c0 + 0);
                uint32_t below  = *(const uint32_t *)(c0 + 4);
                uint32_t jtsize = *(const uint32_t *)(c0 + 8);
                uint32_t jtoff  = *(const uint32_t *)(c0 + 12);
                printf("        [CODE 0] aboveA5=%lu  belowA5=%lu  jtSize=%lu (%lu entries)  jtOffset=0x%lx\n",
                       (unsigned long)above, (unsigned long)below,
                       (unsigned long)jtsize, (unsigned long)(jtsize / 8), (unsigned long)jtoff);
            }
        }
    }
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage:\n");
        printf("  macrun --makedemo       Create sample 68000 Mac 'APPL' (:Desktop:MacDemo)\n");
        printf("  macrun --info <file>    Inspect Macintosh Resource Map & 'CODE' segments\n");
        printf("  macrun <file>           Inspect and launch a classic Macintosh application\n");
        return 0;
    }

    if (!strcasecmp(argv[1], "--makedemo"))
        return make_demo_app();

    int info_only = 0;
    const char *target = argv[1];
    if (!strcasecmp(argv[1], "--info") && argc >= 3) {
        info_only = 1;
        target = argv[2];
    }

    INTEGER vref = (INTEGER)plusos->boot_vrefnum;
    Str255 pname;
    c2pstr_buf(pname, target);
    INTEGER ref;
    int opened = (OpenRF(pname, vref, &ref) == 0);
    if (!opened) {
        char alt[64];
        snprintf(alt, sizeof alt, ":Desktop:%s", target);
        c2pstr_buf(pname, alt);
        if (OpenRF(pname, vref, &ref) == 0) {
            opened = 1;
            target = alt;
        }
    }
    if (opened) {
        LONGINT eof = 0;
        GetEOF(ref, &eof);
        if (eof >= 64 && eof < 256 * 1024) {
            uint8_t *buf = malloc((size_t)eof);
            if (buf) {
                LONGINT n = eof;
                if (FSRead(ref, &n, buf) == 0)
                    inspect_rsrc(buf, (uint32_t)n, target);
                free(buf);
            }
        }
        FSClose(ref);
    }

    if (!info_only) {
        printf("Launching Macintosh application '%s'...\n", target);
        char *av[2] = { (char *)target, 0 };
        int pid = plusos->run(target, 1, av);
        if (pid < 0) {
            printf("macrun: could not launch '%s' (error %d)\n", target, pid);
            return 1;
        }
        plusos_syscall(SYS_wait, (void *)(intptr_t)pid);
        con_clear();
        printf("Macintosh application '%s' exited via _ExitToShell.\n", target);
    }
    return 0;
}
