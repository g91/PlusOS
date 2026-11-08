#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#pragma pack(push, 1)

// Apple Driver Descriptor Record (Block 0)
struct AppleDDR {
    uint16_t sbSig;       // 0x4552 ('ER')
    uint16_t sbBlkSize;   // 512
    uint32_t sbBlkCount;  // total blocks
    uint16_t sbDevType;
    uint16_t sbDevId;
    uint32_t sbData;
    uint16_t sbDrvrCount;
    // ... rest is driver descriptor array
};

// Apple Partition Map Entry (Blocks 1..n)
struct ApplePartition {
    uint16_t pmSig;       // 0x504D ('PM')
    uint16_t pmSigPad;
    uint32_t pmMapBlkCnt; // total entries in partition map
    uint32_t pmPyPartStart; // physical start block
    uint32_t pmPartBlkCnt;  // physical block count
    char     pmPartName[32]; // partition name
    char     pmPartType[32]; // e.g. "Apple_HFS", "Apple_partition_map"
    uint32_t pmLgDataStart;
    uint32_t pmDataCnt;
    uint32_t pmPartStatus;
    uint32_t pmLgBootStart;
    uint32_t pmBootSize;
    uint32_t pmBootAddr;
    uint32_t pmBootAddr2;
    uint32_t pmBootEntry;
    uint32_t pmBootEntry2;
    uint32_t pmBootCksum;
    char     pmProcessor[16];
    uint16_t pad[188];
};

// HFS Extent Descriptor
struct HFSExtentDescriptor {
    uint16_t xdrStABN;    // first allocation block number
    uint16_t xdrNumABlks; // number of allocation blocks
};
using HFSExtentRecord = HFSExtentDescriptor[3];

// HFS Master Directory Block (MDB) - at sector 2 (and sector count - 2)
struct HFSMDB {
    uint16_t drSigWord;   // 0x4244 ('BD')
    uint32_t drCrDate;    // date and time of volume creation
    uint32_t drLsMod;     // date and time of last modification
    uint16_t drAtrb;      // volume attributes
    uint16_t drNmFls;     // number of files in root directory
    uint16_t drVBMSt;     // first allocation block of volume bitmap
    uint16_t drAllocPtr;  // start of next allocation search
    uint16_t drNmAlBlks;  // total number of allocation blocks
    uint32_t drAlBlkSiz;  // allocation block size in bytes (e.g. 512, 1024, etc.)
    uint32_t drClpSiz;    // clump size
    uint16_t drAlBlSt;    // first 512-byte sector of allocation block area
    uint32_t drNxtCNID;   // next unused Catalog Node ID
    uint16_t drFreeBks;   // number of unused allocation blocks
    uint8_t  drVN[28];    // volume name as Pascal string (byte 0 = len)
    uint32_t drVolBkUp;
    uint16_t drVSeqNum;
    uint32_t drWrCnt;
    uint32_t drXTClpSiz;
    uint32_t drCTClpSiz;
    uint16_t drNmRtDirs;  // number of directories in root
    uint32_t drFilCnt;    // total number of files in volume
    uint32_t drDirCnt;    // total number of directories in volume
    uint8_t  drFndrInfo[32]; // Finder info
    uint16_t drVCSize;
    uint16_t drVBMCSize;
    uint16_t drCtlCSize;
    uint32_t drXTFlSize;  // Extents Overflow file size in bytes
    HFSExtentRecord drXTExtRec; // Extents Overflow file extent record
    uint32_t drCTFlSize;  // Catalog file size in bytes
    HFSExtentRecord drCTExtRec; // Catalog file extent record
};

// B*-Tree Node Descriptor (14 bytes at start of every node)
struct BTNodeDescriptor {
    uint32_t ndFLink;     // forward link (node number)
    uint32_t ndBLink;     // backward link (node number)
    int8_t   ndType;      // node type: 0 = index, 1 = header, 2 = map, -1 = leaf
    uint8_t  ndNHeight;   // node level in tree (1 = leaf)
    uint16_t ndNRecs;     // number of records in node
    uint16_t ndResv2;     // reserved
};

// B*-Tree Header Record (starts at offset 14 in Node 0)
struct BTHeaderRecord {
    uint16_t bthDepth;    // current depth of tree
    uint32_t bthRoot;     // root node number
    uint32_t bthNRecs;    // number of data records
    uint32_t bthFNode;    // first leaf node number
    uint32_t bthLNode;    // last leaf node number
    uint16_t bthNodeSize; // node size in bytes (usually 512)
    uint16_t bthKeyLen;   // max key length
    uint32_t bthNNodes;   // total number of nodes in file
    uint32_t bthFree;     // number of free nodes
    uint8_t  bthResv[76];
};

// HFS Catalog Key
struct HFSCatalogKey {
    uint8_t  ckrKeyLen;   // key length
    uint8_t  ckrResrv1;   // reserved
    uint32_t ckrParID;    // parent Directory CNID (1 = root parent, 2 = root dir)
    uint8_t  ckrCName[32];// Pascal string (byte 0 = len)
};

// HFS Directory Record (cdrDirRec) - record type 1
struct HFSCatalogFolder {
    int8_t   cdrType;     // 1 = directory record
    int8_t   cdrResrv2;   // reserved (0)
    uint16_t dirFlags;    // directory flags
    uint16_t dirVal;      // number of items directly in directory
    uint32_t dirDirID;    // directory CNID
    uint32_t dirCrDat;    // creation date
    uint32_t dirMdDat;    // modification date
    uint32_t dirBkDat;    // backup date
    uint8_t  dirUsrInfo[16]; // DInfo (rect, etc.)
    uint8_t  dirFndrInfo[16]; // DXInfo
    uint8_t  dirResrv[16];
};

// HFS File Record (cdrFilRec) - record type 2
struct HFSCatalogFile {
    int8_t   cdrType;     // 2 = file record
    int8_t   cdrResrv2;   // reserved (0)
    uint8_t  filFlags;    // file flags (e.g. bit 1: record used, bit 7: file locked)
    uint8_t  filTyp;      // file type (0)
    uint32_t fdType;      // Finder 4-char type (e.g. 'APPL', 'PLOS', 'TEXT')
    uint32_t fdCreator;   // Finder 4-char creator (e.g. 'PLOS', 'ttxt')
    uint16_t fdFlags;     // Finder flags (e.g. bit 13: bundle, bit 8: invisible)
    uint16_t fdLocationV; // icon location V
    uint16_t fdLocationH; // icon location H
    uint16_t fdFldr;      // folder ID
    uint32_t filFlNum;    // file CNID
    uint16_t filStBlk;    // first alloc block of data fork (first extent)
    uint32_t filLgLen;    // data fork logical length in bytes
    uint32_t filPyLen;    // data fork physical length in bytes
    uint16_t filRStBlk;   // first alloc block of resource fork
    uint32_t filRLgLen;   // resource fork logical length in bytes
    uint32_t filRPyLen;   // resource fork physical length in bytes
    uint32_t filCrDat;    // creation date
    uint32_t filMdDat;    // modification date
    uint32_t filBkDat;    // backup date
    uint8_t  filFndrInfo[16]; // FXInfo
    uint16_t filClpSize;  // clump size
    HFSExtentRecord filExtRec;  // first 3 extents of data fork
    HFSExtentRecord filRExtRec; // first 3 extents of resource fork
    uint32_t filResrv;
};

// HFS Thread Record (cdrThdRec / cdrFThdRec) - record types 3 and 4
struct HFSCatalogThread {
    int16_t  recordType;  // 3 = directory thread, 4 = file thread
    int32_t  reserved[2];
    uint32_t thdParID;    // parent CNID
    uint8_t  thdCName[32];// Pascal string name
};

// MacBinary II Header (128 bytes)
struct MacBinaryHeader {
    uint8_t  oldVersionZero;      // 0
    uint8_t  filenameLength;      // 1..63
    char     filename[63];
    uint32_t fileType;            // FourCC
    uint32_t fileCreator;         // FourCC
    uint8_t  finderFlagsHigh;
    uint8_t  zeroByte1;
    uint16_t locationV;
    uint16_t locationH;
    uint16_t folderID;
    uint8_t  protectedFlag;
    uint8_t  zeroByte2;
    uint32_t dataForkLength;
    uint32_t resourceForkLength;
    uint32_t creationDate;
    uint32_t modifiedDate;
    uint16_t commentLength;
    uint8_t  finderFlagsLow;
    uint8_t  reservedSignature[14]; // "mBIN" or zeros
    uint8_t  scriptCode;
    uint8_t  extendedFinderFlags;
    uint8_t  padding[8];
    uint32_t totalFilesLength;
    uint16_t secondHeaderLength;
    uint8_t  macBinaryVersion;    // 129
    uint8_t  minMacBinaryVersion; // 129
    uint16_t crc16;               // CRC of bytes 0..123
    uint16_t reservedEnd;
};

#pragma pack(pop)

// Big-Endian Byte Order conversion helpers
inline uint16_t swap16(uint16_t v) {
    return (uint16_t)((v << 8) | (v >> 8));
}
inline uint32_t swap32(uint32_t v) {
    return ((v << 24) & 0xFF000000u) |
           ((v << 8)  & 0x00FF0000u) |
           ((v >> 8)  & 0x0000FF00u) |
           ((v >> 24) & 0x000000FFu);
}

// In-place or value big-endian reads
inline uint16_t be16(uint16_t v) { return swap16(v); }
inline uint32_t be32(uint32_t v) { return swap32(v); }

// Helper to convert Mac 4-byte FourCC to readable string
inline std::string fourcc_to_str(uint32_t fcc) {
    char s[5];
    s[0] = (char)((fcc >> 24) & 0xFF);
    s[1] = (char)((fcc >> 16) & 0xFF);
    s[2] = (char)((fcc >> 8) & 0xFF);
    s[3] = (char)(fcc & 0xFF);
    s[4] = '\0';
    for (int i = 0; i < 4; i++) {
        if ((unsigned char)s[i] < 32 || (unsigned char)s[i] > 126) s[i] = '?';
    }
    return std::string(s);
}

// Helper to convert 4-char string to FourCC
inline uint32_t str_to_fourcc(const std::string& str) {
    std::string s = str;
    while (s.length() < 4) s += ' ';
    return ((uint32_t)(uint8_t)s[0] << 24) |
           ((uint32_t)(uint8_t)s[1] << 16) |
           ((uint32_t)(uint8_t)s[2] << 8)  |
           ((uint32_t)(uint8_t)s[3]);
}
