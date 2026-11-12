/* video.c - Slot/Video Driver Discovery & Multi-Depth Video Driver.
 *
 * Implements the standard Macintosh display discovery chain:
 *   1. Inspect Color QuickDraw GDevices (MainDevice $08A4, DeviceList $08A8)
 *      and their PixMaps / active Device Manager video driver entries.
 *   2. Query the Slot Manager (_SlotManager $A06E, selectors sNextTypeSRsrc,
 *      sRsrcInfo, sReadDrvrName, sGetCString, sFindDevBase) across both Slot $0
 *      (built-in motherboard video: RBV/V8/DAFB/Sonora/Civic/Tim/VSC) and
 *      NuBus Slots $9..$E, matching standard Slot Manager discovery.
 *   3. Probe open video drivers via Device Manager _Status (cscGetMode /
 *      cscGetCurMode / cscGetPageCnt / cscGetConnection) and support depth/mode
 *      switching via _Control (cscSetMode / cscGrayPage).
 *   4. Fall back cleanly to ROM low-memory geometry ($0824 ScrnBase, $0106
 *      ScreenRow, $0C20 RowBits, $0C22 ColLines) or compact 512x342x1bpp.
 */
#include "kernel.h"

#define FB_DEFAULT_ROWBYTES 64
#define FB_DEFAULT_ROWS     342
#define FB_DEFAULT_WIDTH    512
#define FB_DEFAULT_DEPTH    1

#define MAX_VIDEO_CARDS     8

/* Slot Manager & Video Driver constants and packed records */
#define CAT_BOARD           1
#define TYP_BOARD           0
#define CAT_DISPLAY         3
#define TYP_VIDEO           1
#define DRSW_APPLE          1
#define S_RSRC_NAME         2

#define CSC_SET_MODE        2
#define CSC_GRAY_PAGE       5
#define CSC_GET_MODE        2
#define CSC_GET_PAGE_CNT    4
#define CSC_GET_CUR_MODE    10
#define CSC_GET_CONNECTION  12

#define SEL_SGET_CSTRING    0x0003
#define SEL_SFIND_STRUCT    0x0006
#define SEL_SNEXT_TYPE      0x0015
#define SEL_SRSRC_INFO      0x0016
#define SEL_SREAD_DRVRNAME  0x0019
#define SEL_SFIND_DEVBASE   0x001B

#pragma pack(push, 2)
typedef struct SysSpBlock {
    int32_t  spResult;
    void    *spsPointer;
    int32_t  spSize;
    int32_t  spOffsetData;
    void    *spIOFileName;
    void    *spsExecPBlk;
    int32_t  spParamData;
    int32_t  spMisc;
    int32_t  spReserved;
    int16_t  spIOReserved;
    int16_t  spRefNum;
    int16_t  spCategory;
    int16_t  spCType;
    int16_t  spDrvrSW;
    int16_t  spDrvrHW;
    int8_t   spTBMask;
    uint8_t  spSlot;
    uint8_t  spID;
    uint8_t  spExtDev;
    uint8_t  spHwDev;
    uint8_t  spByteLanes;
    uint8_t  spFlags;
    uint8_t  spKey;
} SysSpBlock;

typedef struct SysVDSwitchInfo {
    uint16_t csMode;
    uint32_t csData;
    uint16_t csPage;
    void    *csBaseAddr;
    uint32_t csReserved;
} SysVDSwitchInfo;

typedef struct SysVDConnectInfo {
    uint16_t csDisplayType;
    uint16_t csConnectTagged;
    uint32_t csConnectFlags;
    uint32_t csDisplayComponent;
    uint32_t csConnectReserved;
} SysVDConnectInfo;
#pragma pack(pop)

static uint8_t *front;
static uint8_t *back;
static int      s_rowbytes = FB_DEFAULT_ROWBYTES;
static int      s_rows     = FB_DEFAULT_ROWS;
static int      s_width    = FB_DEFAULT_WIDTH;
static int      s_depth    = FB_DEFAULT_DEPTH;
static uint32_t s_bytes    = FB_DEFAULT_ROWBYTES * FB_DEFAULT_ROWS;

static VideoCardInfo s_cards[MAX_VIDEO_CARDS];
static int           s_ncards;

uint8_t *video_front(void)    { return front; }
uint8_t *video_back(void)     { return back ? back : front; }
int      video_rowbytes(void) { return s_rowbytes; }
int      video_rows(void)     { return s_rows; }
int      video_width(void)    { return s_width; }
int      video_depth(void)    { return s_depth; }
uint32_t video_bytes(void)    { return s_bytes; }

int video_card_count(void) { return s_ncards; }
const VideoCardInfo *video_card_at(int index)
{
    if (index < 0 || index >= s_ncards) return 0;
    return &s_cards[index];
}

extern void video_copy(uint8_t *dst, const uint8_t *src, uint32_t n);   /* vblit.S */

static void blit(uint8_t *dst, const uint8_t *src, uint32_t n)
{
    uint8_t was_busy = LM_BYTE(0x8CD);
    LM_BYTE(0x8CD) = 1;
    video_copy(dst, src, n);
    LM_BYTE(0x8CD) = was_busy;
}

static void wait_vblank(void)
{
    volatile uint32_t *ticks = (volatile uint32_t *)0x16A;
    uint32_t t = *ticks;
    uint32_t spins = 0;
    while (*ticks == t && ++spins < 200000) { }
}

void video_present(void)
{
    if (!back) return;
    wait_vblank();
    blit(front, back, s_bytes);
}

void video_present_rect(int top, int bottom)
{
    if (!back) return;
    if (top < 0) top = 0;
    if (bottom > s_rows) bottom = s_rows;
    if (top >= bottom) return;
    blit(front + (uint32_t)top * (uint32_t)s_rowbytes,
         back + (uint32_t)top * (uint32_t)s_rowbytes,
         (uint32_t)(bottom - top) * (uint32_t)s_rowbytes);
}

static int ptr_looks_sane(uint32_t p)
{
    return p >= 0x100 && p != 0xFFFFFFFFUL && (p & 1u) == 0;
}

/* Issue _SlotManager ($A06E) call: D0 = selector, A0 = &sp */
static int16_t slot_call(uint16_t sel, SysSpBlock *sp)
{
    register uint32_t d0 __asm__("d0") = sel;
    register SysSpBlock *a0 __asm__("a0") = sp;
    __asm__ volatile(".short 0xA06E"
                     : "+d"(d0), "+a"(a0)
                     :
                     : "d1", "d2", "a1", "cc", "memory");
    return (int16_t)d0;
}

/* Read Pascal driver name from UTableBase ($011C) for negative ioRefNum */
static void lookup_driver_name(int16_t refnum, char *out, int out_sz)
{
    out[0] = 0;
    if (refnum >= 0) return;
    int unit = -(refnum + 1);
    int ntry = (int)(uint16_t)LM_WORD(0x1D2);           /* UnitNtryCnt */
    DCtlHandlePtr utbl = (DCtlHandlePtr)LM_PTR(0x11C);  /* UTableBase */
    if (!utbl || unit < 0 || (ntry > 0 && unit >= ntry)) return;
    DCtlHandle dceh = utbl[unit];
    if (!ptr_looks_sane((uint32_t)(uintptr_t)dceh)) return;
    DCtlPtr dce = *dceh;
    if (!ptr_looks_sane((uint32_t)(uintptr_t)dce)) return;
    const uint8_t *hdr = 0;
    if (dce->dCtlFlags & 0x0040) {                      /* dRAMBased: Handle */
        const uint8_t * const *h = (const uint8_t * const *)dce->dCtlDriver;
        if (ptr_looks_sane((uint32_t)(uintptr_t)h)) hdr = *h;
    } else {
        hdr = (const uint8_t *)dce->dCtlDriver;
    }
    if (!ptr_looks_sane((uint32_t)(uintptr_t)hdr)) return;
    p2cstr_buf(out, hdr + 18);                          /* drvrName at +18 */
}

static int drv_status(int16_t refnum, int16_t cs_code, void *param_ptr)
{
    if (refnum >= 0) return -1;
    ParamBlockRec pb;
    memset(&pb, 0, sizeof pb);
    pb.cntrlParam.ioCRefNum = refnum;
    pb.cntrlParam.csCode = cs_code;
    memcpy(pb.cntrlParam.csParam, &param_ptr, sizeof(param_ptr));
    return PBStatusSync(&pb);
}

static int drv_control(int16_t refnum, int16_t cs_code, void *param_ptr)
{
    if (refnum >= 0) return -1;
    ParamBlockRec pb;
    memset(&pb, 0, sizeof pb);
    pb.cntrlParam.ioCRefNum = refnum;
    pb.cntrlParam.csCode = cs_code;
    memcpy(pb.cntrlParam.csParam, &param_ptr, sizeof(param_ptr));
    return PBControlSync(&pb);
}

static void read_board_name(uint8_t slot, char *out, int out_sz)
{
    SysSpBlock sp;
    memset(&sp, 0, sizeof sp);
    sp.spSlot = slot;
    sp.spCategory = CAT_BOARD;
    sp.spCType = TYP_BOARD;
    sp.spTBMask = 0x03;                                 /* ignore DrvrSW/DrvrHW */
    if (slot_call(SEL_SNEXT_TYPE, &sp) == 0 && sp.spSlot == slot) {
        sp.spID = S_RSRC_NAME;
        if (slot_call(SEL_SGET_CSTRING, &sp) == 0 && sp.spResult) {
            const char *s = (const char *)(uintptr_t)sp.spResult;
            strlcpy(out, s, (size_t)out_sz);
            DisposePtr((Ptr)(uintptr_t)sp.spResult);
        }
    }
}

void video_scan_displays(void)
{
    s_ncards = 0;
    memset(s_cards, 0, sizeof s_cards);

    /* Step 1: Enumerate Color QuickDraw GDevices if present */
    uint32_t main_gd_raw = LM_LONG(0x8A4);              /* MainDevice */
    uint32_t dev_list_raw = LM_LONG(0x8A8);             /* DeviceList */
    if (g_hw.has_color_qd && ptr_looks_sane(dev_list_raw)) {
        GDHandle gdh = (GDHandle)(uintptr_t)dev_list_raw;
        int guard = 0;
        while (ptr_looks_sane((uint32_t)(uintptr_t)gdh) && s_ncards < MAX_VIDEO_CARDS && guard++ < 16) {
            GDevice *gd = *gdh;
            if (!ptr_looks_sane((uint32_t)(uintptr_t)gd)) break;
            VideoCardInfo *c = &s_cards[s_ncards];
            c->is_main = ((uint32_t)(uintptr_t)gdh == main_gd_raw) ? 1 : 0;
            c->refnum  = gd->gdRefNum;
            c->mode_id = (uint8_t)(gd->gdMode & 0xFF);
            c->width   = (uint16_t)(gd->gdRect.right - gd->gdRect.left);
            c->height  = (uint16_t)(gd->gdRect.bottom - gd->gdRect.top);
            if (ptr_looks_sane((uint32_t)(uintptr_t)gd->gdPMap)) {
                PixMap *pm = *gd->gdPMap;
                if (ptr_looks_sane((uint32_t)(uintptr_t)pm)) {
                    c->base_addr = (uint32_t)(uintptr_t)pm->baseAddr;
                    c->rowbytes  = (uint16_t)(pm->rowBytes & 0x3FFF);
                    c->depth     = (uint16_t)pm->pixelSize;
                }
            }
            lookup_driver_name(c->refnum, c->driver_name, sizeof c->driver_name);
            strlcpy(c->card_name, c->is_main ? "Primary Display" : "Secondary Display", sizeof c->card_name);

            SysVDConnectInfo ci;
            memset(&ci, 0, sizeof ci);
            if (drv_status(c->refnum, CSC_GET_CONNECTION, &ci) == 0)
                c->conn_type = ci.csDisplayType;

            s_ncards++;
            gdh = gd->gdNextGD;
        }
    }

    /* Step 2: Query Slot Manager (Slot $0 built-in + NuBus $9..$E) for
     * CatDisplay/TypVideo sResources and Declaration ROM driver/card names. */
    if (g_hw.has_slot_mgr && ptr_looks_sane(LM_LONG(0x0CBC))) {
        SysSpBlock sp;
        memset(&sp, 0, sizeof sp);
        sp.spSlot     = 0;
        sp.spID       = 0;
        sp.spCategory = CAT_DISPLAY;
        sp.spCType    = TYP_VIDEO;
        sp.spDrvrSW   = DRSW_APPLE;
        sp.spTBMask   = 1;                              /* wildcard spDrvrHW */

        int guard = 0;
        while (slot_call(SEL_SNEXT_TYPE, &sp) == 0 && guard++ < 16) {
            VideoCardInfo *dst = 0;
            for (int i = 0; i < s_ncards; i++) {
                if (sp.spRefNum < 0 && s_cards[i].refnum == sp.spRefNum) {
                    dst = &s_cards[i];
                    break;
                }
            }
            if (!dst && s_ncards < MAX_VIDEO_CARDS) {
                dst = &s_cards[s_ncards++];
                memset(dst, 0, sizeof *dst);
                dst->refnum = sp.spRefNum;
                if (sp.spSlot == 0 && s_ncards == 1) dst->is_main = 1;
            }
            if (dst) {
                dst->slot    = sp.spSlot;
                dst->sp_id   = sp.spID;
                dst->drvr_hw = (uint16_t)sp.spDrvrHW;

                if (!dst->driver_name[0]) {
                    Str255 dname;
                    memset(dname, 0, sizeof dname);
                    SysSpBlock sp2 = sp;
                    sp2.spResult = (int32_t)(uintptr_t)dname;
                    if (slot_call(SEL_SREAD_DRVRNAME, &sp2) == 0)
                        p2cstr_buf(dst->driver_name, dname);
                }

                char bname[48] = {0};
                read_board_name(sp.spSlot, bname, sizeof bname);
                if (bname[0]) strlcpy(dst->card_name, bname, sizeof dst->card_name);
                else if (sp.spSlot == 0) strlcpy(dst->card_name, "Built-in Motherboard Video", sizeof dst->card_name);
                else snprintf(dst->card_name, sizeof dst->card_name, "NuBus Slot $%X Video", sp.spSlot);

                if (dst->base_addr == 0) {
                    SysSpBlock spb = sp;
                    if (slot_call(SEL_SFIND_DEVBASE, &spb) == 0)
                        dst->base_addr = (uint32_t)spb.spResult;
                }
                if (dst->refnum < 0 && dst->conn_type == 0) {
                    SysVDConnectInfo ci;
                    memset(&ci, 0, sizeof ci);
                    if (drv_status(dst->refnum, CSC_GET_CONNECTION, &ci) == 0)
                        dst->conn_type = ci.csDisplayType;
                    if (dst->mode_id == 0) {
                        VDPgInfo pg;
                        memset(&pg, 0, sizeof pg);
                        if (drv_status(dst->refnum, CSC_GET_MODE, &pg) == 0)
                            dst->mode_id = (uint8_t)(pg.csMode & 0xFF);
                    }
                }
            }
        }
    }

    /* Step 3: Compact Mac fallback (Plus / SE / Classic with no Slot Mgr/CQD) */
    if (s_ncards == 0) {
        VideoCardInfo *c = &s_cards[0];
        c->slot      = 0;
        c->sp_id     = 128;
        c->mode_id   = 128;
        c->is_main   = 1;
        c->width     = (uint16_t)s_width;
        c->height    = (uint16_t)s_rows;
        c->rowbytes  = (uint16_t)s_rowbytes;
        c->depth     = (uint16_t)s_depth;
        c->base_addr = (uint32_t)(uintptr_t)front;
        strlcpy(c->card_name, "Built-in CRT Framebuffer", sizeof c->card_name);
        strlcpy(c->driver_name, ".Builtin_Compact_Video", sizeof c->driver_name);
        s_ncards = 1;
    }
}

void video_probe_geometry(uint8_t **base, int *rowbytes, int *width,
                          int *height, int *depth)
{
    uint8_t *b = (uint8_t *)LM_LONG(0x824);             /* ScrnBase */
    int rb = (int)(uint16_t)LM_WORD(0x106);             /* ScreenRow */
    int w = 0, h = 0, d = 0;

    /* Prefer active Color QuickDraw MainDevice PixMap if initialized */
    uint32_t main_raw = LM_LONG(0x8A4);                 /* MainDevice */
    if (g_hw.has_color_qd && ptr_looks_sane(main_raw)) {
        GDHandle mgd = (GDHandle)(uintptr_t)main_raw;
        GDevice *gd = *mgd;
        if (ptr_looks_sane((uint32_t)(uintptr_t)gd) && ptr_looks_sane((uint32_t)(uintptr_t)gd->gdPMap)) {
            PixMap *pm = *gd->gdPMap;
            if (ptr_looks_sane((uint32_t)(uintptr_t)pm) && ptr_looks_sane((uint32_t)(uintptr_t)pm->baseAddr)) {
                int prb = pm->rowBytes & 0x3FFF;
                int pw  = pm->bounds.right - pm->bounds.left;
                int ph  = pm->bounds.bottom - pm->bounds.top;
                int pd  = pm->pixelSize;
                if (prb >= 32 && prb <= 8192 && pw >= 64 && ph >= 64 && pd >= 1 && pd <= 32) {
                    b  = (uint8_t *)pm->baseAddr;
                    rb = prb;
                    w  = pw;
                    h  = ph;
                    d  = pd;
                }
            }
        }
    }

    if (rb < 32 || rb > 8192) {
        int alt = (int)(uint16_t)LM_WORD(0x82A);
        rb = (alt >= 32 && alt <= 8192) ? alt : FB_DEFAULT_ROWBYTES;
    }

    if (w < 64 || h < 64) {
        /* ToolTable occupies $0C00..$0FFF on Plus / 64K ROMs, so $0C20/$0C22
         * (RowBits/ColLines) only exist on SE and later ROMs. */
        if (g_hw.box_flag == BOX_PLUS || (int16_t)g_hw.rom85 < 0) {
            w = FB_DEFAULT_WIDTH;
            h = FB_DEFAULT_ROWS;
        } else {
            w = (int)(uint16_t)LM_WORD(0xC20);          /* RowBits  */
            h = (int)(uint16_t)LM_WORD(0xC22);          /* ColLines */
            if (w < 256 || w > 2048) w = (rb == FB_DEFAULT_ROWBYTES) ? FB_DEFAULT_WIDTH : rb * 8;
            if (h < 200 || h > 2048) h = (rb == FB_DEFAULT_ROWBYTES) ? FB_DEFAULT_ROWS  : 480;
        }
    }

    if (d < 1) {
        d = (w > 0) ? (rb * 8) / w : 1;
        if (d != 1 && d != 2 && d != 4 && d != 8 && d != 16 && d != 32) {
            if (d < 2) d = 1;
            else if (d < 4) d = 2;
            else if (d < 8) d = 4;
            else if (d < 16) d = 8;
            else if (d < 32) d = 16;
            else d = 32;
        }
    }

    if (base)     *base     = b;
    if (rowbytes) *rowbytes = rb;
    if (width)    *width    = w;
    if (height)   *height   = h;
    if (depth)    *depth    = d;
}

int video_set_mode(uint8_t slot, uint8_t mode_id)
{
    if (mode_id < 128 || mode_id > 133) return -1;
    const VideoCardInfo *card = 0;
    for (int i = 0; i < s_ncards; i++) {
        if ((s_cards[i].slot == slot || slot == 0xFF) && s_cards[i].refnum < 0) {
            card = &s_cards[i];
            if (s_cards[i].is_main) break;
        }
    }
    if (!card || card->refnum >= 0) return -1;

    VDPgInfo pg;
    memset(&pg, 0, sizeof pg);
    pg.csMode = mode_id;
    pg.csPage = 0;
    if (drv_control(card->refnum, CSC_SET_MODE, &pg) != 0) return -1;

    VDPgInfo gray = pg;
    drv_control(card->refnum, CSC_GRAY_PAGE, &gray);

    if (pg.csBaseAddr) LM_LONG(0x824) = (uint32_t)(uintptr_t)pg.csBaseAddr;
    video_probe_geometry(&front, &s_rowbytes, &s_width, &s_rows, &s_depth);
    s_bytes = (uint32_t)s_rowbytes * (uint32_t)s_rows;
    video_scan_displays();
    return 0;
}

void video_init(void)
{
    video_probe_geometry(&front, &s_rowbytes, &s_width, &s_rows, &s_depth);
    s_bytes = (uint32_t)s_rowbytes * (uint32_t)s_rows;

    back = 0;
    if (s_bytes <= 32768) {
        back = kmalloc(s_bytes);
        if (back) memcpy(back, front, s_bytes);
    }
    video_scan_displays();
}
