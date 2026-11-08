/* scrap.c - PlusOS System-Wide Scrap / Clipboard Manager
 *
 * Implements a persistent system-wide clipboard modeled on the Macintosh
 * Scrap Manager (low-memory ScrapSize at $0960, ScrapCount at $0968, and
 * "Clipboard File" on the boot HFS volume).
 */
#include "kernel.h"
#include <plusos/syscall.h>

#define SCRAP_MAX 4096

static uint8_t  scrap_buf[SCRAP_MAX];
static int      scrap_len;
static uint32_t scrap_type = 0x54455854UL;      /* 'TEXT' */
static int      scrap_count = 1;
static int      scrap_loaded;
static const unsigned char scrap_file[] = "\x0E" "Clipboard File";

static void sync_lowmem(void)
{
    LM_LONG(0x960) = (uint32_t)scrap_len;       /* ScrapSize */
    LM_WORD(0x968) = (uint16_t)scrap_count;     /* ScrapCount */
}

static void scrap_load_disk(void)
{
    if (scrap_loaded) return;
    if (!(g_flags & PLUSOS_HAVE_FS)) return;
    scrap_loaded = 1;
    INTEGER ref;
    if (FSOpen(scrap_file, g_boot_vref, &ref) != 0) return;
    LONGINT n = SCRAP_MAX - 1;
    FSRead(ref, &n, scrap_buf);
    FSClose(ref);
    if (n < 0) n = 0;
    scrap_len = (int)n;
    scrap_buf[scrap_len] = 0;
    sync_lowmem();
}

static void scrap_save_disk(void)
{
    if (!(g_flags & PLUSOS_HAVE_FS)) return;
    INTEGER ref;
    if (FSOpen(scrap_file, g_boot_vref, &ref) != 0) {
        Create(scrap_file, g_boot_vref, 0x504C4F53 /* 'PLOS' */, 0x434C4950 /* 'CLIP' */);
        if (FSOpen(scrap_file, g_boot_vref, &ref) != 0) return;
    }
    LONGINT n = scrap_len;
    SetFPos(ref, 1, 0);
    if (n > 0) FSWrite(ref, &n, scrap_buf);
    SetEOF(ref, scrap_len);
    FSClose(ref);
    FlushVol(0, g_boot_vref);
}

long scrap_syscall(struct plusos_scrap_args *a)
{
    if (!a) return -1;
    scrap_load_disk();
    switch (a->op) {
    case PLUSOS_SCRAP_CLEAR:
        scrap_len = 0;
        scrap_buf[0] = 0;
        scrap_count++;
        sync_lowmem();
        scrap_save_disk();
        return 0;
    case PLUSOS_SCRAP_SET:
        if (!a->buf || a->size < 0) return -1;
        scrap_len = a->size > SCRAP_MAX - 1 ? SCRAP_MAX - 1 : a->size;
        memcpy(scrap_buf, a->buf, (size_t)scrap_len);
        scrap_buf[scrap_len] = 0;
        scrap_type = a->type ? a->type : 0x54455854UL;
        scrap_count++;
        sync_lowmem();
        scrap_save_disk();
        return scrap_len;
    case PLUSOS_SCRAP_GET: {
        if (!a->buf || a->size <= 0) return scrap_len;
        int copy = scrap_len < a->size - 1 ? scrap_len : a->size - 1;
        memcpy(a->buf, scrap_buf, (size_t)copy);
        ((char *)a->buf)[copy] = 0;
        a->type  = scrap_type;
        a->count = scrap_count;
        return copy;
    }
    case PLUSOS_SCRAP_INFO:
        a->type  = scrap_type;
        a->size  = scrap_len;
        a->count = scrap_count;
        return scrap_len;
    }
    return -1;
}
