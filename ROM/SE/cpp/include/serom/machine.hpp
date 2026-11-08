#pragma once
#ifndef SEROM_MACHINE_HPP
#define SEROM_MACHINE_HPP
//
// serom/machine.hpp -- 68000 machine / ABI foundation for the SE ROM C++ port.
//
// WHY THIS EXISTS
// ---------------
// The scaffold declares every ROM assembly label as a `void foo()` C++ function
// with no parameters.  Real 68000 ROM code passes everything in registers and
// uses absolute low-memory globals, so those bare `void()` functions have no way
// to receive arguments, return results, or chain to one another.  This header is
// the missing layer: a shared 68000 register context plus memory accessors that
// give the ported routines a real, uniform ABI -- the same register-based calling
// convention Apple's traps use (e.g. _NewHandle: D0 in = byte count, A0 out =
// handle, D0 out = OSErr).
//
// TARGET
// ------
// Compiled with the PlusOS Retro68 toolchain (m68k-apple-macos-g++), freestanding,
// -fno-exceptions -fno-rtti.  The m68k target is big-endian and uses 32-bit longs,
// matching classic Mac memory layout exactly, so Mac structures can be overlaid on
// memory with no byte-swapping.
//
namespace serom {

// ---- Fixed-width Mac integral types (68k: short=16, long=32) -----------------
typedef signed char        SInt8;
typedef unsigned char      UInt8;
typedef short              SInt16;
typedef unsigned short     UInt16;
typedef long               SInt32;
typedef unsigned long      UInt32;

typedef UInt8              Byte;
typedef SInt16             OSErr;
typedef UInt32             Size;        // a byte count
typedef UInt32             LongWord;

// A 68k address.  We keep addresses in UInt32 (what an address register holds)
// and dereference through the Peek/Poke helpers below.  On a real/emulated SE the
// heap lives in the low 16 MB (24-bit bus), so the classic tagged master-pointer
// encoding (flags in the top byte, 24-bit address in the low 3 bytes) is valid.
typedef UInt32             Addr;

// A Pascal Ptr / Handle, as the Memory Manager hands them out.
typedef Addr               MacPtr;      // pointer to a nonrelocatable block's data
typedef Addr               MacHandle;   // pointer to a master pointer

// ---- 24-bit addressing constants --------------------------------------------
static const UInt32 kLo3Bytes = 0x00FFFFFFul;  // Lo3Bytes: mask off the flag byte
static const UInt32 kFlagByte = 0xFF000000ul;  // high byte of a master pointer

// ---- Memory accessors (big-endian native on m68k) ----------------------------
inline UInt8  PeekB(Addr a) { return *reinterpret_cast<volatile UInt8*> (static_cast<unsigned long>(a)); }
inline UInt16 PeekW(Addr a) { return *reinterpret_cast<volatile UInt16*>(static_cast<unsigned long>(a)); }
inline UInt32 PeekL(Addr a) { return *reinterpret_cast<volatile UInt32*>(static_cast<unsigned long>(a)); }
inline void   PokeB(Addr a, UInt8  v) { *reinterpret_cast<volatile UInt8*> (static_cast<unsigned long>(a)) = v; }
inline void   PokeW(Addr a, UInt16 v) { *reinterpret_cast<volatile UInt16*>(static_cast<unsigned long>(a)) = v; }
inline void   PokeL(Addr a, UInt32 v) { *reinterpret_cast<volatile UInt32*>(static_cast<unsigned long>(a)) = v; }

// Turn an Addr into a typed pointer for structured access to Mac records.
template <class T> inline T* AsPtr(Addr a) {
    return reinterpret_cast<T*>(static_cast<unsigned long>(a));
}
inline Addr ToAddr(const void* p) {
    return static_cast<Addr>(reinterpret_cast<unsigned long>(p));
}

// ---- 68000 register context ---------------------------------------------------
// The shared ABI between the trap dispatcher and the ported routines.  D0..D7 are
// the data registers, A0..A7 the address registers (A7 == SP).  The condition
// codes are kept so that routines transcribed closely from the asm can branch on
// the result of the last flag-setting operation.
struct Regs {
    UInt32 D[8];
    UInt32 A[8];        // A[7] is the stack pointer
    struct {
        bool c;         // carry
        bool v;         // overflow
        bool z;         // zero
        bool n;         // negative
        bool x;         // extend
    } ccr;
};

// The single machine context.  Defined in support.cpp.
extern Regs g_cpu;

// Convenience references for readability in ported routines.
inline UInt32& D(int n) { return g_cpu.D[n]; }
inline UInt32& A(int n) { return g_cpu.A[n]; }
inline UInt32& SP()     { return g_cpu.A[7]; }

// Set Z/N condition codes from a 32-bit result (the common case after a MOVE/op).
inline void SetNZ(UInt32 result) {
    g_cpu.ccr.z = (result == 0);
    g_cpu.ccr.n = (result & 0x80000000ul) != 0;
}

} // namespace serom
#endif // SEROM_MACHINE_HPP
