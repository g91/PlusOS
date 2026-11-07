/* OSUtil.glue.c - GENERATED glue from multiversal */
#include <serom/OSUtil.h>
pascal OSErr HandToHand(Handle *h){register OSErr _retval __asm__("%d0");register Handle _h __asm__("%a0");_h = *h;
// clang-format off
    __asm__ volatile(".short 0xA9E1"
        : "=r"(_retval), "=r"(_h)
        : "r"(_h)
        : "%d1", "%d2", "%a1", "cc", "memory");
// clang-format on
*h = _h;return _retval;}pascal OSErr PtrToHand(const void *p, Handle *h, LONGINT s){register OSErr _retval __asm__("%d0");register const void* _p __asm__("%a0");register Handle _h __asm__("%a0");register LONGINT _s __asm__("%d0");_p = p;_s = s;
// clang-format off
    __asm__ volatile(".short 0xA9E3"
        : "=r"(_retval), "=r"(_h)
        : "r"(_p), "r"(_s)
        : "%d1", "%d2", "%a1", "cc", "memory");
// clang-format on
*h = _h;return _retval;}pascal void DateToSeconds(DateTimeRec *d, ULONGINT *s){register DateTimeRec* _d __asm__("%a0");register ULONGINT _s __asm__("%d0");_d = d;
// clang-format off
    __asm__ volatile(".short 0xA9C7"
        : "=r"(_s)
        : "r"(_d)
        : "%d1", "%d2", "%a1", "cc", "memory");
// clang-format on
*s = _s;}pascal void Delay(LONGINT n, LONGINT *ftp){register LONGINT _n __asm__("%a0");register LONGINT _ftp __asm__("%d0");_n = n;
// clang-format off
    __asm__ volatile(".short 0xA03B"
        : "=r"(_ftp)
        : "r"(_n)
        : "%d1", "%d2", "%a1", "cc", "memory");
// clang-format on
*ftp = _ftp;}pascal void SwapMMUMode(Byte *bp){register Byte _bp __asm__("%d0");_bp = *bp;
// clang-format off
    __asm__ volatile(".short 0xA05D"
        : "=r"(_bp)
        : "r"(_bp)
        : "%d1", "%d2", "%a0", "%a1", "cc", "memory");
// clang-format on
*bp = _bp;}