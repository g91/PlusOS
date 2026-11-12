/* lc3_hfs_btree.h - Macintosh LC III HFS B-Tree & Catalog Manager Engine
 * Fully reverse-engineered from Universal ROM $067C (lc3_rom.map)
 * Offsets 0x0123E0 - 0x0136AC
 */
#ifndef LC3_HFS_BTREE_H
#define LC3_HFS_BTREE_H

#include <stdint.h>
#include <stdbool.h>

#define BT_NODE_SIZE         512
#define BT_MAX_DEPTH         8

#define BT_NODE_TYPE_INDEX   0x00
#define BT_NODE_TYPE_HEADER  0x01
#define BT_NODE_TYPE_MAP     0x02
#define BT_NODE_TYPE_LEAF    0xFF

/* B-Tree Node Descriptor */
typedef struct {
    uint32_t fLink;             /* Forward link to next sibling node */
    uint32_t bLink;             /* Backward link to prior sibling node */
    uint8_t  nodeType;          /* BT_NODE_TYPE_* */
    uint8_t  nodeHeight;        /* 1 for leaf, >1 for index */
    uint16_t numRecords;        /* Number of records in this node */
    uint16_t reserved;
} LC3BTNodeDesc;

/* B-Tree Header Record */
typedef struct {
    uint16_t treeDepth;         /* Current height of B-tree */
    uint32_t rootNode;          /* Node number of root */
    uint32_t numRecords;        /* Total number of data records */
    uint32_t firstLeaf;         /* First leaf node number */
    uint32_t lastLeaf;          /* Last leaf node number */
    uint16_t nodeSize;          /* Bytes per node: 512 */
    uint16_t maxKeyLen;         /* Max key length */
    uint32_t numNodes;          /* Total nodes in B-tree file */
    uint32_t freeNodes;         /* Unallocated nodes */
    uint8_t  mapRecords[256];   /* Bitmap of free/allocated nodes */
} LC3BTHeaderRec;

/* Catalog Record Key */
typedef struct {
    uint8_t  keyLen;            /* Length of key */
    uint8_t  reserved;
    uint32_t parentDirID;       /* Parent directory ID */
    char     cName[32];         /* Pascal filename string */
} LC3CatKey;

/* Extents Record Key */
typedef struct {
    uint8_t  keyLen;
    uint8_t  forkType;          /* 0 = data fork, 0xFF = rsrc fork */
    uint32_t fileNum;           /* File ID */
    uint16_t startBlock;        /* Starting file allocation block */
} LC3ExtentKey;

/* Extent Descriptor */
typedef struct {
    uint16_t startBlock;
    uint16_t blockCount;
} LC3ExtentDesc;

/* HFS B-Tree API */
void     lc3_bt_open(uint32_t btreeFileNum, LC3BTHeaderRec *hdr);
void     lc3_bt_close(uint32_t btreeFileNum);
int16_t  lc3_bt_get_record(uint32_t btreeFileNum, const void *key, void *recordData, uint16_t *dataLen);
int16_t  lc3_bt_insert(uint32_t btreeFileNum, const void *key, const void *recordData, uint16_t dataLen);
int16_t  lc3_bt_delete(uint32_t btreeFileNum, const void *key);
void     lc3_bt_flush(uint32_t btreeFileNum);

/* Catalog Engine */
int16_t  lc3_cat_create(uint32_t parentDirID, const char *name, uint32_t *newDirID, bool isDir);
int16_t  lc3_cat_delete(uint32_t parentDirID, const char *name);
int16_t  lc3_cat_move(uint32_t srcDirID, const char *srcName, uint32_t destDirID);
int16_t  lc3_cat_rename(uint32_t parentDirID, const char *oldName, const char *newName);

/* Extents Engine */
int16_t  lc3_extent_extend(uint32_t fileNum, uint8_t forkType, uint32_t reqBlocks, uint32_t *actBlocks);
int16_t  lc3_extent_truncate(uint32_t fileNum, uint8_t forkType, uint32_t newBlocks);
int16_t  lc3_extent_map_block(uint32_t fileNum, uint8_t forkType, uint32_t logBlock, uint32_t *physBlock);

#endif /* LC3_HFS_BTREE_H */
