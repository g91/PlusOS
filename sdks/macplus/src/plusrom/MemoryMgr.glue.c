/* MemoryMgr.glue.c - GENERATED glue from multiversal */
#include <plusrom/MemoryMgr.h>
pascal Size MaxMem(Size *growp){register Size _retval __asm__("%d0");register Size _growp __asm__("%a0");
// clang-format off
    __asm__ volatile(".short 0xA11D"
        : "=r"(_retval), "=r"(_growp)
        : 
        : "%d1", "%d2", "%a1", "cc", "memory");
// clang-format on
*growp = _growp;return _retval;}pascal Size MaxMemSys(Size *growp){register Size _retval __asm__("%d0");register Size _growp __asm__("%a0");
// clang-format off
    __asm__ volatile(".short 0xA51D"
        : "=r"(_retval), "=r"(_growp)
        : 
        : "%d1", "%d2", "%a1", "cc", "memory");
// clang-format on
*growp = _growp;return _retval;}pascal void PurgeSpace(Size *totalp, Size *contigp){register Size _totalp __asm__("%d0");register Size _contigp __asm__("%a0");
// clang-format off
    __asm__ volatile(".short 0xA062"
        : "=r"(_totalp), "=r"(_contigp)
        : 
        : "%d1", "%d2", "%a1", "cc", "memory");
// clang-format on
*totalp = _totalp;*contigp = _contigp;}pascal void PurgeSpaceSys(Size *totalp, Size *contigp){register Size _totalp __asm__("%d0");register Size _contigp __asm__("%a0");
// clang-format off
    __asm__ volatile(".short 0xA462"
        : "=r"(_totalp), "=r"(_contigp)
        : 
        : "%d1", "%d2", "%a1", "cc", "memory");
// clang-format on
*totalp = _totalp;*contigp = _contigp;}