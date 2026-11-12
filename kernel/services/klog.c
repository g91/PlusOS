/* klog.c - PlusOS Kernel Event Ring Buffer Logger
 *
 * klog keeps the last KLOG_BUF_SIZE bytes of boot/runtime output in an in-RAM
 * ring (read live via the dmesg syscall). Once the root filesystem is mounted,
 * klog_open_file() opens /boot.log and back-fills whatever is already in the
 * ring; after that every klog line is *also* appended to the file (a tee), so a
 * full record of the boot survives a reboot and can be inspected by mounting
 * the disk image. The file handle is a raw VFS handle, not a per-process fd. */
#include "kernel.h"
#include "proc.h"
#include "klog.h"
#include "vfs.h"
#include <stdarg.h>

static char     klog_ring[KLOG_BUF_SIZE];
static uint32_t klog_head = 0;
static uint32_t klog_total = 0;

/* Disk tee. -1 = not open yet; >=0 = raw VFS handle for /boot.log. The guard
 * stops the FSWrite path from recursing if it (or the VFS under it) klog()s. */
static int klog_fh = -1;
static int klog_in_flush = 0;

/* Commit the volume so a written line survives a crash/power loss mid-boot -
 * the exact situation the boot log exists to diagnose. Without this the HFS
 * catalog entry and data blocks stay in cache and never reach the disk. */
static void klog_commit(void)
{
    if (g_flags & PLUSOS_HAVE_FS) FlushVol(0, g_boot_vref);
}

/* Append raw bytes to the open /boot.log, if any. Caller holds no lock; this is
 * only reached from klog_write (interrupts already disabled) and klog_open_file
 * (single-threaded at boot), so a plain flag is enough re-entrancy protection. */
static void klog_tee(const char *data, int n)
{
    if (klog_fh < 0 || n <= 0 || klog_in_flush) return;
    klog_in_flush = 1;
    vfs_klog_write(klog_fh, data, n);
    klog_commit();
    klog_in_flush = 0;
}

void klog_init(void)
{
    /* The ring and counters are zero-initialized in BSS, so klog() already works
     * during early ROM bring-up (before this runs). Do NOT reset the ring here:
     * that would discard pre-init messages (e.g. the heap/memory-map diagnostics
     * logged by rom_init), which are exactly the ones worth keeping. The disk
     * tee and file handle are not open yet, so only clear those. */
    klog_fh = -1;
    klog_in_flush = 0;
    klog(PLUSOS_LOG_INFO, "PlusOS Kernel Event Logger initialized.");
}

/* Called once from init_task after the root filesystem is mounted. Opens
 * /boot.log (truncating any prior boot's file) and writes out everything the
 * ring has buffered so far, so pre-mount messages are not lost. */
void klog_open_file(const char *path)
{
    if (klog_fh >= 0) return;                 /* already open */
    int h = vfs_klog_open(path);
    if (h < 0) {
        klog(PLUSOS_LOG_WARN, "klog: could not open %s for logging (err %d)", path, h);
        return;
    }

    /* Flush the existing ring contents (oldest first) before enabling the tee. */
    uint16_t sr = int_disable();
    klog_fh = h;
    uint32_t avail = klog_total > KLOG_BUF_SIZE ? KLOG_BUF_SIZE : klog_total;
    uint32_t start = klog_head - avail;
    klog_in_flush = 1;
    for (uint32_t i = 0; i < avail; ) {
        /* Copy a contiguous chunk out of the ring, then write it. */
        char chunk[128];
        uint32_t c = 0;
        while (c < sizeof chunk && i < avail) {
            chunk[c++] = klog_ring[(start + i) % KLOG_BUF_SIZE];
            i++;
        }
        vfs_klog_write(klog_fh, chunk, (int32_t)c);
    }
    klog_commit();
    klog_in_flush = 0;
    int_restore(sr);

    klog(PLUSOS_LOG_INFO, "klog: boot log now persisting to %s", path);
}

void klog_write(int level, const char *msg, int len)
{
    if (!msg || len <= 0) return;
    uint16_t sr = int_disable();

    /* Build the full line once so the ring and the disk tee stay identical. */
    char line[224];
    uint32_t sec = tick_count() / 60;
    int off = snprintf(line, sizeof line, "[%5lu.%02lu] <%d> ",
                       (unsigned long)sec, (unsigned long)((tick_count() % 60) * 100 / 60), level);
    for (int i = 0; i < len && off < (int)sizeof line - 1; i++)
        line[off++] = msg[i];
    if (off == 0 || line[off - 1] != '\n') {
        if (off < (int)sizeof line - 1) line[off++] = '\n';
        else line[sizeof line - 2] = '\n';
    }

    for (int i = 0; i < off; i++) {
        klog_ring[klog_head % KLOG_BUF_SIZE] = line[i];
        klog_head++;
        klog_total++;
    }

    klog_tee(line, off);

    int_restore(sr);
}

void klog(int level, const char *fmt, ...)
{
    char buf[160];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (n > 0) klog_write(level, buf, n);
}

int klog_read(char *buf, int max_len, int offset)
{
    if (!buf || max_len <= 0) return 0;
    uint16_t sr = int_disable();

    uint32_t avail = klog_total > KLOG_BUF_SIZE ? KLOG_BUF_SIZE : klog_total;
    if ((uint32_t)offset >= avail) {
        int_restore(sr);
        return 0;
    }

    uint32_t start_idx = (klog_head - avail) + offset;
    int to_copy = avail - offset;
    if (to_copy > max_len - 1) to_copy = max_len - 1;

    for (int i = 0; i < to_copy; i++) {
        buf[i] = klog_ring[(start_idx + i) % KLOG_BUF_SIZE];
    }
    buf[to_copy] = '\0';

    int_restore(sr);
    return to_copy;
}
