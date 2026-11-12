/* lc3_hfs_cache.h - Macintosh LC III HFS Disk Buffer Cache & Node Allocator
 * Fully reverse-engineered from Universal ROM $067C (lc3_rom.map)
 * Offsets 0x0137B0 - 0x01464E
 */
#ifndef LC3_HFS_CACHE_H
#define LC3_HFS_CACHE_H

#include <stdint.h>
#include <stdbool.h>

#define HFS_NODE_SIZE        512
#define HFS_CACHE_MAX_BUFS   32

/* Cache Buffer Descriptor */
typedef struct LC3HFSBufDesc {
    struct LC3HFSBufDesc *next;
    struct LC3HFSBufDesc *prev;
    int16_t               vRefNum;      /* Volume reference */
    uint32_t              fileNum;      /* File ID */
    uint32_t              blockNum;     /* Allocation block */
    bool                  isDirty;      /* Modified in RAM */
    bool                  inUse;
    uint8_t               buffer[HFS_NODE_SIZE];
} LC3HFSBufDesc;

/* HFS Disk Cache API */
void           lc3_hfs_init_cache(void);
LC3HFSBufDesc *lc3_hfs_get_block(int16_t vRefNum, uint32_t fileNum, uint32_t blockNum);
void           lc3_hfs_mark_block(LC3HFSBufDesc *buf);
void           lc3_hfs_rel_block(LC3HFSBufDesc *buf);
void           lc3_hfs_flush_cache(int16_t vRefNum);
void           lc3_hfs_trash_blocks(int16_t vRefNum, uint32_t fileNum);
void           lc3_hfs_trash_vblks(int16_t vRefNum);
int16_t        lc3_hfs_alloc_node(uint32_t btreeFileNum, uint32_t *newNodeNum);
int16_t        lc3_hfs_free_node(uint32_t btreeFileNum, uint32_t nodeNum);

#endif /* LC3_HFS_CACHE_H */
