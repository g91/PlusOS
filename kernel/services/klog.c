/* klog.c - PlusOS Kernel Event Ring Buffer Logger */
#include "kernel.h"
#include "proc.h"
#include "klog.h"
#include <stdarg.h>

static char     klog_ring[KLOG_BUF_SIZE];
static uint32_t klog_head = 0;
static uint32_t klog_total = 0;

void klog_init(void)
{
    klog_head = 0;
    klog_total = 0;
    memset(klog_ring, 0, sizeof klog_ring);
    klog(PLUSOS_LOG_INFO, "PlusOS Kernel Event Logger initialized.");
}

void klog_write(int level, const char *msg, int len)
{
    if (!msg || len <= 0) return;
    uint16_t sr = int_disable();

    char prefix[32];
    uint32_t sec = tick_count() / 60;
    int plen = snprintf(prefix, sizeof prefix, "[%5lu.%02lu] <%d> ",
                        (unsigned long)sec, (unsigned long)((tick_count() % 60) * 100 / 60), level);

    for (int i = 0; i < plen; i++) {
        klog_ring[klog_head % KLOG_BUF_SIZE] = prefix[i];
        klog_head++;
        klog_total++;
    }

    for (int i = 0; i < len; i++) {
        klog_ring[klog_head % KLOG_BUF_SIZE] = msg[i];
        klog_head++;
        klog_total++;
    }

    if (msg[len - 1] != '\n') {
        klog_ring[klog_head % KLOG_BUF_SIZE] = '\n';
        klog_head++;
        klog_total++;
    }

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
