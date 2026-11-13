/* api.c - the service table given to apps, and the kernel's memory, time and
 * raw block I/O services. */
#include "kernel.h"
#include "syscall.h"
#include "kmem.h"

PlusOSAPI g_api;
const PlusOSAPI *plusos;

extern const uint8_t plusos_font8x8[95 * 8];

void *kmalloc(uint32_t n)
{
    return kmem_alloc(n, 0);
}

void kfree(void *p) { kmem_free(p); }

static uint32_t ticks(void)          { return (uint32_t)TickCount(); }
static uint32_t seconds(void)        { return LM_LONG(0x20C); }     /* Time */

/* Raw 512-byte blocks on the boot drive, relative to the start of the HFS
 * partition - the same addressing the boot block uses for the kernel. */
static int blk_io(int write, uint32_t block, uint32_t count, void *buf)
{
    ParamBlockRec pb;
    memset(&pb, 0, sizeof pb);
    pb.ioParam.ioRefNum = g_boot_refnum;
    pb.ioParam.ioVRefNum = g_boot_drive;
    pb.ioParam.ioBuffer = buf;
    pb.ioParam.ioReqCount = (LONGINT)(count * 512);
    pb.ioParam.ioPosMode = 1;                         /* fsFromStart */
    pb.ioParam.ioPosOffset = (LONGINT)(block * 512);
    return write ? PBWriteSync(&pb) : PBReadSync(&pb);
}

int blk_read(uint32_t block, uint32_t count, void *buf)        { return blk_io(0, block, count, buf); }
int blk_write(uint32_t block, uint32_t count, const void *buf) { return blk_io(1, block, count, (void *)buf); }

void api_init(void)
{
    PlusOSAPI *a = &g_api;
    a->magic = PLUSOS_API_MAGIC;
    a->version = PLUSOS_API_VERSION;
    a->flags = g_flags;
    /* The console and the keyboard queue are shared kernel state, and user
     * programs are preemptible, so they are reached through the syscall gate. */
    a->con_putc = ktrap_con_putc;
    a->con_write = ktrap_con_write;
    a->con_clear = ktrap_con_clear;
    a->con_goto = ktrap_con_goto;
    a->con_getpos = ktrap_con_getpos;
    a->con_invert = ktrap_con_invert;
    a->con_cursor = ktrap_con_cursor;
    a->key_get = ktrap_getkey;
    a->key_poll = ktrap_pollkey;
    a->key_line = ktrap_readline;
    a->key_mods = kbd_mods;
    a->mem_alloc = ktrap_malloc;        /* ROM Memory Manager runs in supervisor via syscall */
    a->mem_free = ktrap_free;
    a->mem_free_bytes = ktrap_memfree;   /* ROM Memory/File Manager calls go via the gate */
    a->ticks = ticks;
    a->seconds = seconds;
    a->sleep_ticks = ktrap_sleep;
    a->blk_read = ktrap_blk_read;
    a->blk_write = ktrap_blk_write;
    a->reboot = ktrap_reboot;           /* privileged / no-return -> syscall */
    a->shutdown = ktrap_shutdown;
    a->run = ktrap_spawn;               /* runs the program as a ring-2 process */
    a->qd = &qd;
    /* PlusOS graphics (desktop, gfx.c) are 1-bpp. The back buffer is 1-bpp and
     * video_present expands it to the hardware depth. So apps must see the
     * 1-bpp rowBytes for their off-screen drawing. On a 1-bpp display this is
     * just the real rowBytes. We also point `screen` at the back buffer (not the
     * raw color framebuffer): an app that draws to `screen` then presents works,
     * and nothing draws 1-bpp directly into a color front buffer. */
    a->screen_rowbytes = video_back_rowbytes();     /* 1-bpp back-buffer stride */
    a->screen_width    = video_width();
    a->screen_height   = video_rows();
    a->boot_vrefnum = g_boot_vref;
    a->font8x8 = plusos_font8x8;
    a->screen_back = video_back();      /* off-screen 1-bpp buffer for flicker-free drawing */
    a->screen      = video_back();      /* draw target == back buffer (expanded on present) */
    a->present = video_present;
    a->present_rect = video_present_rect;
    a->sleep_ticks = ktrap_sleep;
    a->screen_depth = 1;                /* what apps draw in; hardware depth hidden by present */
    a->con_cols     = (int16_t)con_cols();
    a->con_rows     = (int16_t)con_rows();
    plusos = a;
}

/* libc back-end for the kernel */
void *__plusos_mem_alloc(uint32_t n) { return kmalloc(n); }
void  __plusos_mem_free(void *p)     { kfree(p); }
