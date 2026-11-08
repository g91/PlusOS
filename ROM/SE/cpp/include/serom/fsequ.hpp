#pragma once
#ifndef SEROM_FSEQU_HPP
#define SEROM_FSEQU_HPP
//
// serom/fsequ.hpp -- Macintosh File System Equates (VCB, FCB, HFS B-Trees)
// Transcribed from F:\mac_rom\include\fsequ.inc
//
#include "serom/machine.hpp"
#include "serom/sysequ.hpp"

namespace serom {
namespace fs {

static const UInt16 kMFSSigWord = 0xD2D7; // MFS
static const UInt16 kHFSSigWord = 0x4244; // 'BD' HFS

// Volume Control Block (VCB)
struct VCB {
    Addr   qLink;
    SInt16 qType;
    UInt16 vcbFlags;
    UInt16 vcbSigWord;
    UInt32 vcbCrDate;
    UInt32 vcbLsMod;
    UInt16 vcbAtrb;
    UInt16 vcbNmFls;
    SInt16 vcbVRefNum;
    UInt16 vcbFSID;
    UInt16 vcbDRExt;
    UInt16 vcbAlBlSt;
    UInt32 vcbAllocPtr;
    UInt16 vcbVolBkUp;
    UInt16 vcbVSeqNum;
    UInt32 vcbWrCnt;
    UInt32 vcbFileClp;
    UInt32 vcbDirClp;
    UInt8  vcbVN[28];
    UInt16 vcbDrvNum;
    SInt16 vcbDRefNum;
    SInt16 vcbFSSt;
    UInt32 vcbVAlBlkSiz;
    UInt32 vcbVNmAlBlks;
    UInt32 vcbVFrBlk;
};

// File Control Block (FCB)
struct FCB {
    UInt32 fcbFlNum;     // file number (DirID/CNID)
    UInt8  fcbMdDat;     // flags
    UInt8  fcbTyp;
    UInt16 fcbSBlk;
    UInt32 fcbEOF;       // logical EOF
    UInt32 fcbPLen;      // physical EOF
    UInt32 fcbCrPs;      // mark position
    Addr   fcbVPtr;      // pointer to VCB
    Addr   fcbBfAdr;     // file buffer pointer
    UInt16 fcbClmpSize;
    UInt32 fcbBTCBPtr;
    UInt32 fcbExtRec[3]; // extent descriptors
};

} // namespace fs
} // namespace serom

#endif // SEROM_FSEQU_HPP
