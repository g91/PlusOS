/* sys.c - ShutDown Manager aware reboot and shutdown. */
#include "kernel.h"
#include "sys7.h"

static void flush_volume(void)
{
    if (g_flags & PLUSOS_HAVE_FS) {
        ParamBlockRec pb;
        memset(&pb, 0, sizeof pb);
        pb.volumeParam.ioVRefNum = g_boot_vref;
        PBFlushVolSync(&pb);
    }
}

void sys_reboot(void)
{
    kmod_shutdown();                    /* modules' fini() hooks, last started first */
    sys7_run_shutdown_hooks(1);         /* ShutDown Manager restart hooks */
    flush_volume();
    if (hw_rom_has_trap(0xA895)) {      /* _ShutDown: ShutDwnStart (selector 2) */
        __asm__ volatile(
            "move.w #2, -(%%sp)\n\t"
            ".short 0xA895" ::: "d0", "d1", "d2", "a0", "a1", "cc", "memory");
    }
    /* fallback restart through ROM reset vector */
    __asm__ volatile(
        "move.w  #0x2700, %%sr\n\t"
        "movea.l 0x2AE, %%a0\n\t"          /* ROMBase */
        "movea.l (%%a0), %%sp\n\t"
        "movea.l 4(%%a0), %%a0\n\t"
        "jmp     (%%a0)" ::: "a0", "memory");
    __builtin_unreachable();
}

void sys_shutdown(void)
{
    kmod_shutdown();
    sys7_run_shutdown_hooks(0);         /* ShutDown Manager power-off hooks */
    flush_volume();
    if (g_flags & PLUSOS_HAVE_QD) HideCursor();
    if (hw_rom_has_trap(0xA895)) {      /* _ShutDown: ShutDwnPower (selector 1) */
        __asm__ volatile(
            "move.w #1, -(%%sp)\n\t"
            ".short 0xA895" ::: "d0", "d1", "d2", "a0", "a1", "cc", "memory");
    }
    con_clear();
    const char *msg = "You may now switch off your Macintosh safely.";
    int cols = con_cols(), rows = con_rows();
    int cx = (cols - (int)strlen(msg)) / 2;
    con_goto(cx > 0 ? cx : 0, rows / 2);
    con_write(msg, strlen(msg));
    for (;;) {}
}
