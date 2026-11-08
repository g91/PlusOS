#pragma once
#ifndef SEROM_MAC_MEMORY_HPP
#define SEROM_MAC_MEMORY_HPP
//
// serom/mac_memory.hpp -- Memory Manager data structures (zone + block layout).
//
// Field offsets transcribed from include/sysequ.inc and the Memory Manager
// commentary in se/os/memory_mgr.s (F:\mac_rom), matching Inside Macintosh:
// Memory, chapter 2.  Behavioral ports of the Memory Manager operate on these
// exact byte layouts so that handles remain real master-pointer handles and the
// lock/purge/resource flag bits live where the rest of the ROM expects them.
//
#include "serom/machine.hpp"

namespace serom {
namespace mem {

// ---- OSErr result codes (Inside Macintosh: Memory) ---------------------------
enum : OSErr {
    noErr        =    0,
    paramErr     =  -50,
    memFullErr   = -108,   // not enough room in heap zone
    nilHandleErr = -109,   // NIL master pointer
    memWZErr     = -111,   // attempt to operate on a free block / bad WhichZone
    memPurErr    = -112,   // attempt to purge a locked block
    memAdrErr    = -110,   // bad address
    memAZErr     = -113,   // address in zone check failed
    memPCErr     = -114,   // pointer check failed
    memBCErr     = -115,   // block check failed
    memSCErr     = -116,   // size check failed
    memLockedErr = -117,   // block is locked
};

// ---- Heap zone header (52 bytes; first block begins at zone + 52) ------------
struct Zone {
    Addr   bkLim;       //  0  first byte after the zone (trailing sentinel block)
    Addr   purgePtr;    //  4  roving purge placeholder
    Addr   hFstFree;    //  8  first free master pointer
    SInt32 zcbFree;     // 12  free bytes in zone
    Addr   gzProc;      // 16  grow-zone procedure
    SInt16 moreMast;    // 20  master pointers to allocate at a time
    SInt16 flags;       // 22  zone flags
    SInt16 cntRel;      // 24  allocated relocatable blocks
    SInt16 maxRel;      // 26  max relocatable blocks
    SInt16 cntNRel;     // 28  allocated nonrelocatable blocks
    SInt16 maxNRel;     // 30  max nonrelocatable blocks
    SInt16 cntEmpty;    // 32  empty handles
    SInt16 cntHandles;  // 34  total handles
    SInt32 minCBFree;   // 36  minimum free bytes seen
    Addr   purgeProc;   // 40  purge-warning procedure
    Addr   sparePtr;    // 44  reserved (StdGZ on this ROM)
    Addr   allocPtr;    // 48  roving allocator placeholder
    // heapData: first usable byte is at offset 52 (sizeof(Zone))
};

static const UInt32 kZoneHeaderSize = 52;

// ---- Block header (8 bytes; minimum block size 12) ---------------------------
struct BlkHeader {
    UInt32 tagBC;       // 0  tag byte (type + size correction) + 24-bit phys size
    Addr   handle;      // 4  rel: master-pointer address; nonrel: zone address
    // block data begins at offset 8 (blkData)
};

static const UInt32 kBlkHeaderSize = 8;
static const UInt32 kMinBlkSize    = 12;

// Block type, encoded in the top two bits of the tag byte.
static const UInt8 kTypeMask   = 0xC0;
static const UInt8 kTypeFree   = 0x00;
static const UInt8 kTypeNonRel = 0x40;
static const UInt8 kTypeRel    = 0x80;
static const UInt8 kSizeCorrMask = 0x0F;   // slop bytes at end of block

// Master-pointer flag bits (top byte of the master pointer long).
static const UInt32 kLockBit     = 0x80000000ul;
static const UInt32 kPurgeBit    = 0x40000000ul;
static const UInt32 kResourceBit = 0x20000000ul;

// ---- block header accessors --------------------------------------------------
inline UInt8  BlkTag (const BlkHeader* b) { return static_cast<UInt8>(b->tagBC >> 24); }
inline UInt8  BlkType(const BlkHeader* b) { return BlkTag(b) & kTypeMask; }
inline UInt32 BlkPhysSize(const BlkHeader* b) { return b->tagBC & kLo3Bytes; }
inline UInt8  BlkSizeCorr (const BlkHeader* b) { return BlkTag(b) & kSizeCorrMask; }
// Logical (requested) size = physical - header - size correction.
inline UInt32 BlkLogSize(const BlkHeader* b) {
    return BlkPhysSize(b) - kBlkHeaderSize - BlkSizeCorr(b);
}
inline void SetBlk(BlkHeader* b, UInt8 type, UInt8 sizeCorr, UInt32 physSize) {
    b->tagBC = (static_cast<UInt32>((type & kTypeMask) | (sizeCorr & kSizeCorrMask)) << 24)
             | (physSize & kLo3Bytes);
}
inline BlkHeader* NextBlk(BlkHeader* b) {
    return AsPtr<BlkHeader>(ToAddr(b) + BlkPhysSize(b));
}

// ---- master-pointer accessors (a Handle is the address of one of these) ------
inline Addr   MPData (MacHandle h) { return PeekL(h) & kLo3Bytes; }
inline UInt32 MPFlags(MacHandle h) { return PeekL(h) & kFlagByte; }
inline void   MPSetData (MacHandle h, Addr data) { PokeL(h, (PeekL(h) & kFlagByte) | (data & kLo3Bytes)); }
inline void   MPSetFlags(MacHandle h, UInt32 flags) { PokeL(h, (PeekL(h) & kLo3Bytes) | (flags & kFlagByte)); }
inline bool   MPIsLocked(MacHandle h)   { return (PeekL(h) & kLockBit) != 0; }
inline bool   MPIsPurgeable(MacHandle h){ return (PeekL(h) & kPurgeBit) != 0; }
inline bool   MPIsPurged(MacHandle h)   { return MPData(h) == 0; }

// Compile-time proof that the layout matches the assembly offsets.
static_assert(__builtin_offsetof(Zone, bkLim)      ==  0, "bkLim");
static_assert(__builtin_offsetof(Zone, hFstFree)   ==  8, "hFstFree");
static_assert(__builtin_offsetof(Zone, zcbFree)    == 12, "zcbFree");
static_assert(__builtin_offsetof(Zone, moreMast)   == 20, "moreMast");
static_assert(__builtin_offsetof(Zone, flags)      == 22, "flags");
static_assert(__builtin_offsetof(Zone, minCBFree)  == 36, "minCBFree");
static_assert(__builtin_offsetof(Zone, purgeProc)  == 40, "purgeProc");
static_assert(__builtin_offsetof(Zone, allocPtr)   == 48, "allocPtr");
static_assert(sizeof(Zone) == kZoneHeaderSize, "zone header size");
static_assert(sizeof(BlkHeader) == kBlkHeaderSize, "block header size");

} // namespace mem
} // namespace serom
#endif // SEROM_MAC_MEMORY_HPP
