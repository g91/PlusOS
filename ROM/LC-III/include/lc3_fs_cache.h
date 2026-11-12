/* lc3_fs_cache.h - Macintosh LC III File System Cache & B-Tree Dispatch Engine
 * Fully reverse-engineered from Universal ROM $067C (lc3_rom.map)
 * Offsets 0x00B520 - 0x00C14A
 */
#ifndef LC3_FS_CACHE_H
#define LC3_FS_CACHE_H

#include <stdint.h>
#include <stdbool.h>

#define FS_CACHE_BLOCK_SIZE  512
#define FS_CACHE_BLOCKS      16

/* File System Cache Block */
typedef struct LC3CacheBlock {
    struct LC3CacheBlock *next;
    struct LC3CacheBlock *prev;
    int16_t               vRefNum;
    uint32_t              blockNum;
    bool                  isDirty;
    uint8_t               data[FS_CACHE_BLOCK_SIZE];
} LC3CacheBlock;

/* File System Cache API */
void     lc3_fs_cache_init(void);
uint8_t *lc3_fs_fetch_block(int16_t vRefNum, uint32_t blockNum);
void     lc3_fs_stash_block(int16_t vRefNum, uint32_t blockNum, const uint8_t *data);
void     lc3_fs_flush_cache(int16_t vRefNum);
void     lc3_fs_io_done(int16_t vRefNum, int16_t result);
int16_t  lc3_cmp_string(const char *s1, const char *s2);
int16_t  lc3_rel_string(const char *s1, const char *s2, bool caseSens, bool diacSens);
void     lc3_delay(uint32_t ticksToDelay);

#endif /* LC3_FS_CACHE_H */
