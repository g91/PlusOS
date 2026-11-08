/* ramdisk.c - PlusOS Virtual RAM Disk */
#include "kernel.h"
#include "ramdisk.h"

static uint8_t *ramdisk_data = 0;
static uint32_t ramdisk_bitmap[RAMDISK_BLOCKS / 32];

void ramdisk_init(void)
{
    /* Allocate 64KB for RAM disk if not already allocated */
    if (!ramdisk_data) {
        ramdisk_data = (uint8_t *)kmalloc(RAMDISK_BLOCKS * RAMDISK_BLOCK_SIZE);
        if (ramdisk_data) {
            memset(ramdisk_data, 0, RAMDISK_BLOCKS * RAMDISK_BLOCK_SIZE);
        }
    }
    memset(ramdisk_bitmap, 0, sizeof ramdisk_bitmap);
}

int ramdisk_read(uint32_t block, uint32_t count, void *buf)
{
    if (!ramdisk_data || !buf) return -1;
    if (block + count > RAMDISK_BLOCKS) return -1;

    memcpy(buf, ramdisk_data + (block * RAMDISK_BLOCK_SIZE), count * RAMDISK_BLOCK_SIZE);
    return 0;
}

int ramdisk_write(uint32_t block, uint32_t count, const void *buf)
{
    if (!ramdisk_data || !buf) return -1;
    if (block + count > RAMDISK_BLOCKS) return -1;

    memcpy(ramdisk_data + (block * RAMDISK_BLOCK_SIZE), buf, count * RAMDISK_BLOCK_SIZE);
    for (uint32_t i = 0; i < count; i++) {
        uint32_t b = block + i;
        ramdisk_bitmap[b / 32] |= (1UL << (b % 32));
    }
    return 0;
}

void ramdisk_clear(void)
{
    if (ramdisk_data) {
        memset(ramdisk_data, 0, RAMDISK_BLOCKS * RAMDISK_BLOCK_SIZE);
    }
    memset(ramdisk_bitmap, 0, sizeof ramdisk_bitmap);
}

void ramdisk_status(uint32_t *total_blocks, uint32_t *used_blocks)
{
    if (total_blocks) *total_blocks = RAMDISK_BLOCKS;
    if (used_blocks) {
        uint32_t used = 0;
        for (int i = 0; i < RAMDISK_BLOCKS; i++) {
            if (ramdisk_bitmap[i / 32] & (1UL << (i % 32))) used++;
        }
        *used_blocks = used;
    }
}
