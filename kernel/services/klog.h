/* klog.h - PlusOS Kernel Event Ring Buffer Logger */
#ifndef PLUSOS_KLOG_H
#define PLUSOS_KLOG_H

#include <stdint.h>
#include <plusos/syscall.h>

#define KLOG_BUF_SIZE 4096

void klog_init(void);
void klog_write(int level, const char *msg, int len);
int  klog_read(char *buf, int max_len, int offset);
void klog(int level, const char *fmt, ...);

#endif
