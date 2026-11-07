/* BinaryDecimal.glue.c - GENERATED glue from multiversal */
#include <plusrom/BinaryDecimal.h>
pascal void StringToNum(ConstStringPtr s, LONGINT *lp){register ConstStringPtr _s __asm__("%a0");register LONGINT _lp __asm__("%d0");_s = s;
// clang-format off
    __asm__ volatile(".short 0x3F3C, 0x0001, 0xA9EE"
        : "=r"(_lp)
        : "r"(_s)
        : "%d1", "%d2", "%a1", "cc", "memory");
// clang-format on
*lp = _lp;}