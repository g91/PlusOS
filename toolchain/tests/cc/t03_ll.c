#include "testlib.h"
typedef unsigned long u32;
typedef union { unsigned long long v; struct { u32 hi, lo; } w; } U64;
static unsigned long long id(unsigned long long x) { return x; }
static unsigned long long mk(u32 hi, u32 lo) { U64 u; u.w.hi = hi; u.w.lo = lo; return u.v; }
static u32 hi(unsigned long long x) { U64 u; u.v = x; return u.w.hi; }
static u32 lo(unsigned long long x) { return (u32)x; }
int main(void) {
    unsigned long long a = mk(0x12, 0x3456789A);
    CHECK_EQ(hi(a), 0x12); CHECK_EQ(lo(a), 0x3456789A);
    CHECK_EQ(hi(id(a)), 0x12);
    unsigned long long b = a + a;
    CHECK_EQ(hi(b), 0x24); CHECK_EQ(lo(b), 0x68ACF134);
    unsigned long long c = a * 3;
    CHECK_EQ(hi(c), 0x36); CHECK_EQ(lo(c), 0x9D0369CE);
    unsigned long long d = mk(0, 0xFFFFFFFF) * mk(0, 0xFFFFFFFF);
    CHECK_EQ(hi(d), 0xFFFFFFFE); CHECK_EQ(lo(d), 1);
    long long n = -5; long long nn = n * n;
    CHECK_EQ(hi(nn), 0); CHECK_EQ(lo(nn), 25);
    CHECK_EQ(lo(a / 1000), (u32)(0x123456789AULL / 1000)); CHECK_EQ(lo(a % 1000), (u32)(0x123456789AULL % 1000));
    CHECK_EQ(hi(a << 4), 0x123); CHECK_EQ(lo(a >> 4), 0x23456789);
    CHECK(a == mk(0x12, 0x3456789A)); CHECK(a != b); CHECK(a < b); CHECK(!(b <= a));
    if (failures == 0) puts_("t03 ok\n");
    return failures;
}
