/* sys.c - reboot and shutdown. */
#include "kernel.h"

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
    flush_volume();
    /* restart through the ROM: initial SP and PC are the first two longs of ROM */
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
    flush_volume();
    if (g_flags & PLUSOS_HAVE_QD) HideCursor();
    con_clear();
    /* The Mac Plus has no software power switch: show the classic message. */
    const char *msg = "You may now switch off your Macintosh safely.";
    con_goto((CON_COLS - (int)strlen(msg)) / 2, CON_ROWS / 2);
    con_write(msg, strlen(msg));
    for (;;) {}
}
