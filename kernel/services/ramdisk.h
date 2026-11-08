/* ramdisk.h - PlusOS Virtual RAM Disk */
#ifndef PLUSOS_RAMDISK_H
#define PLUSOS_RAMDISK_H

#include <stdint.h>
#include <plusos/syscall.h>

#define RAMDISK_BLOCKS 128
#define RAMDISK_BLOCK_SIZE 512

void ramdisk_init(void);
int  ramdisk_read(uint32_t block, uint32_t count, void *buf);
int  ramdisk_write(uint32_t block, uint32_t count, const void *buf);
void ramdisk_clear(void);
void ramdisk_status(uint32_t *total_blocks, uint32_t *used_blocks);

#endif
