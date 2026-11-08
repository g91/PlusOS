/* rominit.c - bring up the ROM's subsystems.
 *
 * Our boot block took over before the ROM's BootFromDisk (boot/boot_load.s)
 * got to set up the machine. We do the same steps, in the same order:
 *   1. heaps     _SetApplBase: grow the system heap, create the application heap
 *   2. events    _InitEvents(20): OS event queue -> keyboard/mouse events
 *   3. files     _InitFS(10) + _MountVol(boot drive): File Manager + boot volume
 *   4. QuickDraw _InitGraf/_OpenPort on the screen
 * Each step prints first, so a hang shows where it happened.
 */
#include "kernel.h"

#define SYS_HEAP_SIZE  0x18000          /* system heap: 96 KB (drivers, FCBs, VCBs) */

static void step(const char *what) { printf("  %-24s", what); }

const char *cpu_name(void)
{
    switch (LM_BYTE(0x12F)) {           /* CPUFlag, set by the ROM's MOVEC probe */
    case 0:  return "68000";
    case 1:  return "68010";
    case 2:  return "68020";
    case 3:  return "68030";
    default: return "680x0";
    }
}

uint32_t ram_bytes(void) { return LM_LONG(0x108); }      /* MemTop */

static void init_heap(void)
{
    step("heap (SetApplBase)");
    uint32_t sysz = LM_LONG(0x2A6);                     /* SysZone */
    uint32_t stack_bottom = __plusos_load_addr - KERNEL_STACK;
    uint32_t base = (sysz + SYS_HEAP_SIZE + 15) & ~15UL;
    if (base + 0x10000 >= stack_bottom) {
        printf("skipped: not enough RAM\n");
        return;
    }
    plusrom_set_appl_base((void *)base);                /* also resets ApplLimit to its default */
    LM_LONG(0x130) = stack_bottom - 0x400;              /* ApplLimit: the heap must stop below our
                                                           stack and image (set AFTER SetApplBase) */
    LM_LONG(0x118) = LM_LONG(0x2A6);                    /* TheZone = SysZone for the next steps */
    if (LM_LONG(0x2AA) != base) {                       /* ApplZone */
        printf("FAILED (ApplZone %08lx)\n", (unsigned long)LM_LONG(0x2AA));
        return;
    }
    g_flags |= PLUSOS_HAVE_HEAP;
    printf("ok  system %luK, apps %06lx-%06lx\n", (unsigned long)((base - sysz) >> 10),
           (unsigned long)base, (unsigned long)LM_LONG(0x130));
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
    uint8_t *scr = (uint8_t *)LM_LONG(0x824);
    if ((uint8_t *)g_port.portBits.baseAddr != scr) {   /* should not happen; be safe */
        g_port.portBits.baseAddr = (Ptr)scr;
        g_port.portBits.rowBytes = 64;
        SetRect(&g_port.portBits.bounds, 0, 0, 512, 342);
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
