/* ramdisk.h - PlusOS Virtual RAM Disk */
#ifndef PLUSOS_RAMDISK_H
#define PLUSOS_RAMDISK_H

#include <stdint.h>
#include <plusos/syscall.h>
#include "kconfig.h"

#define RAMDISK_BLOCKS 128
#define RAMDISK_BLOCK_SIZE 512

#if CONFIG_RAMDISK
void ramdisk_init(void);
int  ramdisk_read(uint32_t block, uint32_t count, void *buf);
int  ramdisk_write(uint32_t block, uint32_t count, const void *buf);
void ramdisk_clear(void);
void ramdisk_status(uint32_t *total_blocks, uint32_t *used_blocks);
#else
static inline void ramdisk_init(void) {}
static inline int  ramdisk_read(uint32_t b, uint32_t c, void *p) { (void)b; (void)c; (void)p; return -1; }
static inline int  ramdisk_write(uint32_t b, uint32_t c, const void *p) { (void)b; (void)c; (void)p; return -1; }
static inline void ramdisk_clear(void) {}
static inline void ramdisk_status(uint32_t *t, uint32_t *u) { if (t) *t = 0; if (u) *u = 0; }
#endif

#endif