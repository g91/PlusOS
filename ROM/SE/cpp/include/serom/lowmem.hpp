#pragma once
#ifndef SEROM_LOWMEM_HPP
#define SEROM_LOWMEM_HPP
//
// serom/lowmem.hpp -- Macintosh SE low-memory global variables.
//
// Addresses transcribed from the ROM source tree's include/lowmem.inc,
// include/lowmem_se.inc and include/sysequ.inc (F:\mac_rom).  These are absolute
// locations in the low 2 KB of RAM; the ROM and the Toolbox communicate through
// them.  Keeping the same addresses means code ported from the asm -- and any
// classic Mac software that reads these globals directly -- sees the same layout.
//
#include "serom/machine.hpp"

namespace serom {
namespace lowmem {

// --- addresses (hex offsets from 0) ------------------------------------------
enum : Addr {
    // System / boot
    MacJmp        = 0x0120,
    ScreenRow     = 0x0106,
    MemTop        = 0x0108,   // top of RAM (last byte + 1)
    BufPtr        = 0x010C,   // top of application memory
    StackLowPoint = 0x0110,
    HeapEnd       = 0x0114,   // end of application heap zone
    TheZone       = 0x0118,   // current heap zone
    UTableBase    = 0x011C,
    CPUFlag       = 0x012F,
    ApplLimit     = 0x0130,   // application heap limit
    SysEvtMask    = 0x0144,
    RndSeed       = 0x0156,   // random number seed (long)
    SysVersion    = 0x015A,
    Ticks         = 0x016A,   // tick count since boot (long, 60.15 Hz)
    MemErr        = 0x0220,   // last Memory Manager error (word)
    BootDrive     = 0x0210,
    Time          = 0x020C,   // seconds since 1904-01-01 (long)

    // Memory Manager zones / masks
    MmInOK        = 0x012E,   // initial memory-mgr checks ok? (byte)
    HiHeapMark    = 0x0BAE,   // highest address used by a zone below sp (long)
    MaskBC        = 0x031A,   // byte-count / handle / pointer mask (= Lo3Bytes)
    SysZone       = 0x02A6,   // system heap zone
    ApplZone      = 0x02AA,   // application heap zone
    ROMBase       = 0x02AE,   // base of ROM

    // SE-era (lowmem_se.inc)
    ResetVector   = 0x0004,
    WarmStart     = 0x0CFC,
    SCSIBase      = 0x0C00,
    ADBBase       = 0x0CF8,
    ToolTable     = 0x0E00,   // SE: moved from 0x0C00 (Plus)
};

// --- typed accessors ---------------------------------------------------------
template <class T> inline T   Get(Addr a);
template <class T> inline void Set(Addr a, T v);

template <> inline UInt8  Get<UInt8>(Addr a)  { return PeekB(a); }
template <> inline UInt16 Get<UInt16>(Addr a) { return PeekW(a); }
template <> inline UInt32 Get<UInt32>(Addr a) { return PeekL(a); }
template <> inline SInt16 Get<SInt16>(Addr a) { return static_cast<SInt16>(PeekW(a)); }
template <> inline SInt32 Get<SInt32>(Addr a) { return static_cast<SInt32>(PeekL(a)); }

template <> inline void Set<UInt8>(Addr a, UInt8 v)   { PokeB(a, v); }
template <> inline void Set<UInt16>(Addr a, UInt16 v) { PokeW(a, v); }
template <> inline void Set<UInt32>(Addr a, UInt32 v) { PokeL(a, v); }
template <> inline void Set<SInt16>(Addr a, SInt16 v) { PokeW(a, static_cast<UInt16>(v)); }
template <> inline void Set<SInt32>(Addr a, SInt32 v) { PokeL(a, static_cast<UInt32>(v)); }

// Named helpers for the globals the Memory Manager touches most.
inline Addr   GetSysZone()    { return PeekL(SysZone); }
inline void   SetSysZone(Addr z)   { PokeL(SysZone, z); }
inline Addr   GetApplZone()   { return PeekL(ApplZone); }
inline void   SetApplZone(Addr z)  { PokeL(ApplZone, z); }
inline Addr   GetTheZone()    { return PeekL(TheZone); }
inline void   SetTheZone(Addr z)   { PokeL(TheZone, z); }
inline Addr   GetHeapEnd()    { return PeekL(HeapEnd); }
inline void   SetHeapEnd(Addr a)   { PokeL(HeapEnd, a); }
inline Addr   GetApplLimit()  { return PeekL(ApplLimit); }
inline void   SetApplLimit(Addr a) { PokeL(ApplLimit, a); }
inline OSErr  GetMemErr()     { return static_cast<OSErr>(PeekW(MemErr)); }
inline void   SetMemErr(OSErr e)   { PokeW(MemErr, static_cast<UInt16>(e)); }

} // namespace lowmem
} // namespace serom
#endif // SEROM_LOWMEM_HPP
