/* int64.c - 64-bit integer helpers of the P68 runtime (libp68rt.a).
 * 8-byte values are passed on the stack and returned in d0:d1; the compiler calls these
 * for the long long operations it does not expand inline. Written with 32-bit halves only
 * so that this file never needs the helpers it defines. */

typedef unsigned long u32;
typedef long s32;
typedef union { unsigned long long v; struct { u32 hi, lo; } w; } U64;

unsigned long long __adddi3(unsigned long long a, unsigned long long b) {
    U64 x, y;
    x.v = a; y.v = b;
    u32 lo = x.w.lo + y.w.lo;
    x.w.hi = x.w.hi + y.w.hi + (lo < x.w.lo);
    x.w.lo = lo;
    return x.v;
}

unsigned long long __subdi3(unsigned long long a, unsigned long long b) {
    U64 x, y;
    x.v = a; y.v = b;
    u32 lo = x.w.lo - y.w.lo;
    x.w.hi = x.w.hi - y.w.hi - (x.w.lo < y.w.lo);
    x.w.lo = lo;
    return x.v;
}

unsigned long long __negdi2(unsigned long long a) {
    U64 x;
    x.v = a;
    x.w.lo = 0 - x.w.lo;
    x.w.hi = 0 - x.w.hi - (x.w.lo != 0);
    return x.v;
}

/* 0 if a < b, 1 if equal, 2 if a > b */
int __ucmpdi2(unsigned long long a, unsigned long long b) {
    U64 x, y;
    x.v = a; y.v = b;
    if (x.w.hi != y.w.hi) return x.w.hi < y.w.hi ? 0 : 2;
    if (x.w.lo != y.w.lo) return x.w.lo < y.w.lo ? 0 : 2;
    return 1;
}
int __cmpdi2(long long a, long long b) {
    U64 x, y;
    x.v = (unsigned long long)a; y.v = (unsigned long long)b;
    if (x.w.hi != y.w.hi) return (s32)x.w.hi < (s32)y.w.hi ? 0 : 2;
    if (x.w.lo != y.w.lo) return x.w.lo < y.w.lo ? 0 : 2;
    return 1;
}

unsigned long long __ashldi3(unsigned long long a, int n) {
    U64 x;
    x.v = a;
    n &= 63;
    if (n == 0) return a;
    if (n >= 32) { x.w.hi = x.w.lo << (n - 32); x.w.lo = 0; }
    else { x.w.hi = (x.w.hi << n) | (x.w.lo >> (32 - n)); x.w.lo <<= n; }
    return x.v;
}
unsigned long long __lshrdi3(unsigned long long a, int n) {
    U64 x;
    x.v = a;
    n &= 63;
    if (n == 0) return a;
    if (n >= 32) { x.w.lo = x.w.hi >> (n - 32); x.w.hi = 0; }
    else { x.w.lo = (x.w.lo >> n) | (x.w.hi << (32 - n)); x.w.hi >>= n; }
    return x.v;
}
long long __ashrdi3(long long a, int n) {
    U64 x;
    x.v = (unsigned long long)a;
    n &= 63;
    if (n == 0) return a;
    if (n >= 32) { x.w.lo = (u32)((s32)x.w.hi >> (n - 32)); x.w.hi = (u32)((s32)x.w.hi >> 31); }
    else { x.w.lo = (x.w.lo >> n) | (x.w.hi << (32 - n)); x.w.hi = (u32)((s32)x.w.hi >> n); }
    return (long long)x.v;
}

/* 32x32 -> 64 from 16-bit pieces */
static void mul32(u32 a, u32 b, U64 *r) {
    u32 al = a & 0xFFFF, ah = a >> 16, bl = b & 0xFFFF, bh = b >> 16;
    u32 ll = al * bl, lh = al * bh, hl = ah * bl, hh = ah * bh;
    u32 mid = lh + hl;
    u32 carry = mid < lh ? 0x10000UL : 0;
    u32 lo = ll + (mid << 16);
    r->w.hi = hh + (mid >> 16) + carry + (lo < ll);
    r->w.lo = lo;
}
unsigned long long __muldi3(unsigned long long a, unsigned long long b) {
    U64 x, y, r;
    x.v = a; y.v = b;
    mul32(x.w.lo, y.w.lo, &r);
    r.w.hi += x.w.lo * y.w.hi + x.w.hi * y.w.lo;
    return r.v;
}

static void udivmod(U64 n, U64 d, U64 *q, U64 *rem) {
    U64 r;
    int i;
    r.w.hi = r.w.lo = 0;
    q->w.hi = q->w.lo = 0;
    for (i = 63; i >= 0; i--) {
        /* r = r << 1 | bit i of n */
        r.w.hi = (r.w.hi << 1) | (r.w.lo >> 31);
        r.w.lo = (r.w.lo << 1) | (i >= 32 ? (n.w.hi >> (i - 32)) & 1 : (n.w.lo >> i) & 1);
        if (r.w.hi > d.w.hi || (r.w.hi == d.w.hi && r.w.lo >= d.w.lo)) {
            u32 lo = r.w.lo - d.w.lo;
            r.w.hi = r.w.hi - d.w.hi - (r.w.lo < d.w.lo);
            r.w.lo = lo;
            if (i >= 32) q->w.hi |= 1UL << (i - 32);
            else q->w.lo |= 1UL << i;
        }
    }
    *rem = r;
}
static void neg64(U64 *x) {
    x->w.lo = 0 - x->w.lo;
    x->w.hi = 0 - x->w.hi - (x->w.lo != 0);
}
unsigned long long __udivdi3(unsigned long long a, unsigned long long b) {
    U64 n, d, q, r;
    n.v = a; d.v = b;
    udivmod(n, d, &q, &r);
    return q.v;
}
unsigned long long __umoddi3(unsigned long long a, unsigned long long b) {
    U64 n, d, q, r;
    n.v = a; d.v = b;
    udivmod(n, d, &q, &r);
    return r.v;
}
long long __divdi3(long long a, long long b) {
    U64 n, d, q, r;
    int neg = 0;
    n.v = (unsigned long long)a; d.v = (unsigned long long)b;
    if ((s32)n.w.hi < 0) { neg64(&n); neg = !neg; }
    if ((s32)d.w.hi < 0) { neg64(&d); neg = !neg; }
    udivmod(n, d, &q, &r);
    if (neg) neg64(&q);
    return (long long)q.v;
}
long long __moddi3(long long a, long long b) {
    U64 n, d, q, r;
    int neg = 0;
    n.v = (unsigned long long)a; d.v = (unsigned long long)b;
    if ((s32)n.w.hi < 0) { neg64(&n); neg = 1; }
    if ((s32)d.w.hi < 0) neg64(&d);
    udivmod(n, d, &q, &r);
    if (neg) neg64(&r);
    return (long long)r.v;
}
