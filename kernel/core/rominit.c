/* rominit.c - bring up the ROM's subsystems.
 *
 * Our boot block took over before the ROM's disk bootstrapper
 * got to set up the machine. We do the same steps, in the same order:
 *   1. heaps     _SetApplBase: grow the system heap, create the application heap
 *   2. events    _InitEvents(20): OS event queue -> keyboard/mouse events
 *   3. files     _InitFS(10) + _MountVol(boot drive): File Manager + boot volume
 *   4. QuickDraw _InitGraf/_OpenPort on the screen
 * Each step prints first, so a hang shows where it happened.
 */
#include "kernel.h"
#include "../services/klog.h"

#define SYS_HEAP_SIZE  0x18000          /* system heap: 96 KB (drivers, FCBs, VCBs) */

static void step(const char *what) { printf("  %-24s", what); }

const char *cpu_name(void)
{
    return hw_cpu_name();
}

uint32_t ram_bytes(void) { return LM_LONG(0x108); }      /* MemTop */

static void init_heap(void)
{
    step("heap (SetApplBase)");
    uint32_t memtop = LM_LONG(0x108);                   /* MemTop */
    uint32_t bufptr = LM_LONG(0x10C);                   /* BufPtr: top of usable RAM below the ROM's buffers */
    uint32_t sysz = LM_LONG(0x2A6);                     /* SysZone */
    uint32_t cur_appl = LM_LONG(0x2AA);                 /* ApplZone */
    uint32_t stack_bottom = __plusos_load_addr - KERNEL_STACK;

    /* The whole memory map, logged before we touch anything - this is the data
     * needed to diagnose a heap-setup failure on an unfamiliar machine. */
    klog(PLUSOS_LOG_INFO, "heap: MemTop=%06lx BufPtr=%06lx SysZone=%06lx ApplZone=%06lx",
         (unsigned long)memtop, (unsigned long)bufptr, (unsigned long)sysz, (unsigned long)cur_appl);
    klog(PLUSOS_LOG_INFO, "heap: load_addr=%06lx stack_bottom=%06lx (kstack=%lxh)",
         (unsigned long)__plusos_load_addr, (unsigned long)stack_bottom, (unsigned long)KERNEL_STACK);

    uint32_t base = (sysz + SYS_HEAP_SIZE + 15) & ~15UL;
    if (cur_appl > sysz && cur_appl + 0x18000 < stack_bottom) {
        uint32_t grown = (cur_appl + 0x8000 + 15) & ~15UL;
        if (grown > base) base = grown;
    }
    klog(PLUSOS_LOG_INFO, "heap: requested base=%06lx (need base+64K < stack_bottom=%06lx)",
         (unsigned long)base, (unsigned long)stack_bottom);

    if (base + 0x10000 >= stack_bottom) {
        /* No gap between the system heap and the kernel image. Almost always
         * means the loader placed the kernel too low in RAM for this machine. */
        printf("skipped: no room (base %06lx, stack %06lx)\n",
               (unsigned long)base, (unsigned long)stack_bottom);
        klog(PLUSOS_LOG_ERR, "heap: NO ROOM - base %06lx + 64K >= stack_bottom %06lx; "
             "kernel loaded too low (load_addr %06lx of %06lx RAM)",
             (unsigned long)base, (unsigned long)stack_bottom,
             (unsigned long)__plusos_load_addr, (unsigned long)memtop);
        return;
    }
    LM_LONG(0x130) = stack_bottom - 0x400;              /* Ensure ApplLimit is above base before SetApplBase */
    int16_t err = plusrom_set_appl_base((void *)base);  /* Also resets ApplLimit to its default */
    LM_LONG(0x130) = stack_bottom - 0x400;              /* Re-clamp ApplLimit below our stack and image */
    LM_LONG(0x118) = LM_LONG(0x2A6);                    /* TheZone = SysZone for the next steps */
    uint32_t final_appl = LM_LONG(0x2AA);
    if (final_appl <= sysz || final_appl + 0x10000 >= stack_bottom) {
        printf("FAILED (ApplZone %06lx, err %d)\n", (unsigned long)final_appl, (int)err);
        klog(PLUSOS_LOG_ERR, "heap: SetApplBase($A057) FAILED err=%d ApplZone=%06lx "
             "(wanted >%06lx and +64K<%06lx); _SetApplBase may be unsupported on this ROM",
             (int)err, (unsigned long)final_appl, (unsigned long)sysz, (unsigned long)stack_bottom);
        return;
    }
    g_flags |= PLUSOS_HAVE_HEAP;
    printf("ok  system %luK, apps %06lx-%06lx\n", (unsigned long)((final_appl - sysz) >> 10),
           (unsigned long)final_appl, (unsigned long)LM_LONG(0x130));
    klog(PLUSOS_LOG_INFO, "heap: ok system %luK apps %06lx-%06lx",
         (unsigned long)((final_appl - sysz) >> 10), (unsigned long)final_appl,
         (unsigned long)LM_LONG(0x130));
}

static void init_events(void)
{
    step("events (InitEvents)");
    plusrom_init_events(20);
    g_flags |= PLUSOS_HAVE_EVENTS;
    printf("ok  20-entry queue\n");
}

static void init_files(void)
{
    step("files (InitFS/MountVol)");
    int16_t e = plusrom_init_fs(10);
    if (e) { printf("InitFS error %d\n", e); return; }

    ParamBlockRec pb;
    memset(&pb, 0, sizeof pb);
    pb.volumeParam.ioVRefNum = g_boot_drive;
    e = PBMountVol(&pb);
    if (e && e != -55) {                                /* -55 volOnLinErr: already mounted */
        printf("MountVol error %d\n", e);
        return;
    }

    Str255 name;
    memset(&pb, 0, sizeof pb);
    pb.volumeParam.ioNamePtr = name;
    pb.volumeParam.ioVRefNum = g_boot_drive;            /* a drive number works here too */
    e = PBGetVInfoSync(&pb);
    if (e) { printf("GetVInfo error %d\n", e); return; }
    g_boot_vref = pb.volumeParam.ioVRefNum;
    p2cstr_buf(g_volname, name);
    long freek = ((long)pb.volumeParam.ioVFrBlk * pb.volumeParam.ioVAlBlkSiz) >> 10;

    memset(&pb, 0, sizeof pb);
    pb.volumeParam.ioVRefNum = g_boot_vref;
    PBSetVolSync(&pb);                                  /* default volume for file names */
    g_flags |= PLUSOS_HAVE_FS;
    printf("ok  volume \"%s\", %ldK free\n", g_volname, freek);
}

static void init_quickdraw(void)
{
    step("QuickDraw (InitGraf)");
    InitGraf(&qd.thePort);                              /* stores &qd.thePort at (A5) */
    OpenPort(&g_port);
    /* On the Plus/SE the ROM's own QuickDraw port is already 512x342 at
     * rowBytes 64. On the LC III (and any machine with a RAM frame buffer) the
     * geometry is whatever the ROM probed - read it from the shared geometry
     * reader, which reads the real low-memory globals ($0824/$0106/$0C20/
     * $0C22). We call it directly (not the cached video_* accessors) because
     * rom_init() runs BEFORE video_init(). Without this QuickDraw would
     * clip/stretch to 512x342 on a 640x480 screen. */
    uint8_t *scr;
    int rb, w, h;
    video_probe_geometry(&scr, &rb, &w, &h, 0);
    if ((uint8_t *)g_port.portBits.baseAddr != scr ||
        g_port.portBits.rowBytes != rb ||
        g_port.portRect.right != w ||
        g_port.portRect.bottom != h) {
        g_port.portBits.baseAddr = (Ptr)scr;
        g_port.portBits.rowBytes = rb;
        SetRect(&g_port.portBits.bounds, 0, 0, w, h);
        g_port.portRect = g_port.portBits.bounds;
        RectRgn(g_port.visRgn, &g_port.portRect);
        ClipRect(&g_port.portRect);
    }
    SetPort(&g_port);
    HideCursor();                                       /* text mode: no arrow over the text */
    g_flags |= PLUSOS_HAVE_QD;
    printf("ok  %dx%d, rowBytes %d\n", g_port.portRect.right, g_port.portRect.bottom,
           g_port.portBits.rowBytes);
}

void rom_init(void)
{
    /* boot drive and its driver (from the drive queue) */
    g_boot_drive = (int16_t)LM_WORD(0x210);             /* BootDrive */
    for (uint8_t *q = (uint8_t *)LM_LONG(0x30A); q; q = *(uint8_t **)q)   /* DrvQHdr.qHead */
        if (*(int16_t *)(q + 6) == g_boot_drive) { g_boot_refnum = *(int16_t *)(q + 8); break; }
    if (!g_boot_refnum) {
        uint8_t *q = (uint8_t *)LM_LONG(0x30A);
        if (q) {
            g_boot_drive = *(int16_t *)(q + 6);
            g_boot_refnum = *(int16_t *)(q + 8);
        }
    }

    init_heap();
    if (g_flags & PLUSOS_HAVE_HEAP) {
        init_events();
        init_files();
        LM_LONG(0x118) = LM_LONG(0x2AA);                /* TheZone = ApplZone from now on */
        plusrom_max_appl_zone();                        /* grow the heap up to ApplLimit */
        uint32_t heap_end = LM_LONG(0x114);             /* HeapEnd */
        if (heap_end > __plusos_load_addr - KERNEL_STACK)
            printf("  WARNING: heap end %06lx overlaps the kernel stack!\n", (unsigned long)heap_end);
        init_quickdraw();
    } else {
        printf("  (no heap: events, files and QuickDraw stay off)\n");
    }
}
