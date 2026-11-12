/* softfloat.c - IEEE 754 single and double precision arithmetic in software, for
 * 680x0 machines without an FPU. Part of the P68 runtime (libp68rt.a).
 * Round-to-nearest-even, with infinities, NaNs and denormals. Only integer operations
 * are used, so this file never calls the routines it defines. */

typedef unsigned long u32;
typedef long s32;
typedef unsigned long long u64;
typedef long long s64;
typedef union { float f; u32 u; } FU;
typedef union { double d; u64 u; struct { u32 hi, lo; } w; } DU;

/* ------------------------------------------------------------------ single */
#define F_SIGN 0x80000000UL
#define F_INF  0x7F800000UL
#define F_QNAN 0x7FC00000UL

/* mant: significand with the leading one at bit 26 (3 extra low bits: guard, round, sticky);
 * exp: biased exponent that goes with that position */
static u32 packf(u32 sign, int exp, u32 mant) {
    if (mant == 0) return sign;
    while (mant >= (1UL << 27)) { mant = (mant >> 1) | (mant & 1); exp++; }
    while (mant < (1UL << 26) && exp > 1) { mant <<= 1; exp--; }
    if (exp < 1) {
        int sh = 1 - exp;
        if (sh > 27) mant = 1;
        else mant = (mant >> sh) | ((mant & ((1UL << sh) - 1)) != 0);
        exp = 1;
    }
    /* round to nearest even on the low 3 bits */
    {
        u32 low = mant & 7;
        mant >>= 3;
        if (low > 4 || (low == 4 && (mant & 1))) mant++;
    }
    if (mant >= (1UL << 24)) { mant >>= 1; exp++; }
    if (exp >= 255) return sign | F_INF;
    if (mant < (1UL << 23)) return sign | mant;          /* denormal (exp field 0) */
    return sign | ((u32)exp << 23) | (mant & 0x7FFFFFUL);
}

static int unpackf(u32 x, int *exp, u32 *mant) { /* returns class: 0 zero, 1 finite, 2 inf, 3 nan */
    int e = (int)((x >> 23) & 0xFF);
    u32 m = x & 0x7FFFFFUL;
    if (e == 255) { *exp = e; *mant = m; return m ? 3 : 2; }
    if (e == 0) {
        if (m == 0) { *exp = 0; *mant = 0; return 0; }
        e = 1;
    } else
        m |= 1UL << 23;
    *exp = e;
    *mant = m << 3;
    return 1;
}

static u32 addf(u32 a, u32 b) {
    int ea, eb, ca, cb;
    u32 ma, mb, sa = a & F_SIGN, sb = b & F_SIGN;
    ca = unpackf(a, &ea, &ma);
    cb = unpackf(b, &eb, &mb);
    if (ca == 3 || cb == 3) return F_QNAN;
    if (ca == 2) return (cb == 2 && sa != sb) ? F_QNAN : a;
    if (cb == 2) return b;
    if (ca == 0) return cb == 0 ? (sa & sb) : b;
    if (cb == 0) return a;
    if (ea < eb || (ea == eb && ma < mb)) {
        int te = ea; u32 tm = ma, ts = sa;
        ea = eb; ma = mb; sa = sb;
        eb = te; mb = tm; sb = ts;
    }
    {
        int d = ea - eb;
        if (d > 27) mb = 1;
        else if (d) mb = (mb >> d) | ((mb & ((1UL << d) - 1)) != 0);
    }
    if (sa == sb) ma += mb;
    else {
        ma -= mb;
        if (ma == 0) return 0;
    }
    return packf(sa, ea, ma);
}

float __addsf3(float a, float b) { FU x, y; x.f = a; y.f = b; x.u = addf(x.u, y.u); return x.f; }
float __subsf3(float a, float b) { FU x, y; x.f = a; y.f = b; x.u = addf(x.u, y.u ^ F_SIGN); return x.f; }
float __negsf2(float a) { FU x; x.f = a; x.u ^= F_SIGN; return x.f; }

float __mulsf3(float a, float b) {
    FU x, y;
    int ea, eb, ca, cb;
    u32 ma, mb, sign;
    x.f = a; y.f = b;
    sign = (x.u ^ y.u) & F_SIGN;
    ca = unpackf(x.u, &ea, &ma);
    cb = unpackf(y.u, &eb, &mb);
    if (ca == 3 || cb == 3) { x.u = F_QNAN; return x.f; }
    if (ca == 2 || cb == 2) { x.u = (ca == 0 || cb == 0) ? F_QNAN : (sign | F_INF); return x.f; }
    if (ca == 0 || cb == 0) { x.u = sign; return x.f; }
    ma >>= 3; mb >>= 3;                       /* 24-bit significands */
    while (ma < (1UL << 23)) { ma <<= 1; ea--; }
    while (mb < (1UL << 23)) { mb <<= 1; eb--; }
    {
        /* 24x24 -> 48 bit product in hi:lo */
        u32 al = ma & 0xFFFF, ah = ma >> 16, bl = mb & 0xFFFF, bh = mb >> 16;
        u32 ll = al * bl, lh = al * bh, hl = ah * bl, hh = ah * bh;
        u32 mid = lh + hl;                    /* < 2^25, no carry */
        u32 lo = ll + (mid << 16);
        u32 hi = hh + (mid >> 16) + (lo < ll);
        /* product has its leading one at bit 46 or 47; keep 27 bits + sticky */
        u32 m = (hi << 11) | (lo >> 21);      /* bits 47..21 -> 26..0 */
        if (lo & 0x1FFFFFUL) m |= 1;
        x.u = packf(sign, ea + eb - 127 + 1, m);
    }
    return x.f;
}

float __divsf3(float a, float b) {
    FU x, y;
    int ea, eb, ca, cb, i;
    u32 ma, mb, sign, q;
    x.f = a; y.f = b;
    sign = (x.u ^ y.u) & F_SIGN;
    ca = unpackf(x.u, &ea, &ma);
    cb = unpackf(y.u, &eb, &mb);
    if (ca == 3 || cb == 3) { x.u = F_QNAN; return x.f; }
    if (ca == 2) { x.u = cb == 2 ? F_QNAN : (sign | F_INF); return x.f; }
    if (cb == 2) { x.u = sign; return x.f; }
    if (cb == 0) { x.u = ca == 0 ? F_QNAN : (sign | F_INF); return x.f; }
    if (ca == 0) { x.u = sign; return x.f; }
    ma >>= 3; mb >>= 3;
    while (ma < (1UL << 23)) { ma <<= 1; ea--; }
    while (mb < (1UL << 23)) { mb <<= 1; eb--; }
    if (ma < mb) { ma <<= 1; ea--; }
    q = 0;
    for (i = 0; i < 27; i++) {                /* 27 quotient bits, leading one at bit 26 */
        q <<= 1;
        if (ma >= mb) { ma -= mb; q |= 1; }
        ma <<= 1;
    }
    if (ma) q |= 1;
    x.u = packf(sign, ea - eb + 127, q);
    return x.f;
}

/* -1 less, 0 equal, 1 greater, 2 unordered */
static int cmpf(u32 a, u32 b) {
    u32 aa = a & ~F_SIGN, ab = b & ~F_SIGN;
    if (aa > F_INF || ab > F_INF) return 2;
    if (aa == 0 && ab == 0) return 0;
    if ((a ^ b) & F_SIGN) return (a & F_SIGN) ? -1 : 1;
    if (aa == ab) return 0;
    if (a & F_SIGN) return aa > ab ? -1 : 1;
    return aa < ab ? -1 : 1;
}
int __eqsf2(float a, float b) { FU x, y; x.f = a; y.f = b; return cmpf(x.u, y.u) != 0; }
int __nesf2(float a, float b) { FU x, y; x.f = a; y.f = b; return cmpf(x.u, y.u) != 0; }
int __ltsf2(float a, float b) { FU x, y; int c; x.f = a; y.f = b; c = cmpf(x.u, y.u); return c == 2 ? 1 : c; }
int __lesf2(float a, float b) { FU x, y; int c; x.f = a; y.f = b; c = cmpf(x.u, y.u); return c == 2 ? 1 : c; }
int __gtsf2(float a, float b) { FU x, y; int c; x.f = a; y.f = b; c = cmpf(x.u, y.u); return c == 2 ? -1 : c; }
int __gesf2(float a, float b) { FU x, y; int c; x.f = a; y.f = b; c = cmpf(x.u, y.u); return c == 2 ? -1 : c; }

float __floatunsisf(u32 v) {
    FU x;
    u32 m = v, sticky = 0;
    int exp = 127 + 26;
    if (v == 0) { x.u = 0; return x.f; }
    while (m >= (1UL << 27)) { sticky |= m & 1; m >>= 1; exp++; }
    x.u = packf(0, exp, m | sticky);
    return x.f;
}
float __floatsisf(s32 v) {
    FU x;
    if (v < 0) { x.f = __floatunsisf(0UL - (u32)v); x.u |= F_SIGN; return x.f; }
    return __floatunsisf((u32)v);
}
u32 __fixunssfsi(float a) {
    FU x;
    int e;
    u32 m;
    x.f = a;
    if (x.u & F_SIGN) return 0;
    e = (int)((x.u >> 23) & 0xFF);
    if (e < 127) return 0;
    if (e >= 127 + 32) return 0xFFFFFFFFUL;
    m = (x.u & 0x7FFFFFUL) | (1UL << 23);
    e -= 127 + 23;
    return e >= 0 ? m << e : m >> -e;
}
s32 __fixsfsi(float a) {
    FU x;
    int e;
    u32 m;
    x.f = a;
    e = (int)((x.u >> 23) & 0xFF);
    if (e < 127) return 0;
    if (e >= 127 + 31) return (x.u & F_SIGN) ? (s32)0x80000000UL : 0x7FFFFFFFL;
    m = (x.u & 0x7FFFFFUL) | (1UL << 23);
    e -= 127 + 23;
    m = e >= 0 ? m << e : m >> -e;
    return (x.u & F_SIGN) ? -(s32)m : (s32)m;
}

/* ------------------------------------------------------------------ double */
#define D_SIGN 0x8000000000000000ULL
#define D_INF  0x7FF0000000000000ULL
#define D_QNAN 0x7FF8000000000000ULL

/* mant: leading one at bit 55 (52 fraction bits + 3 rounding bits) */
static u64 packd(u64 sign, int exp, u64 mant) {
    if (mant == 0) return sign;
    while (mant >= (1ULL << 56)) { mant = (mant >> 1) | (mant & 1); exp++; }
    while (mant < (1ULL << 55) && exp > 1) { mant <<= 1; exp--; }
    if (exp < 1) {
        int sh = 1 - exp;
        if (sh > 56) mant = 1;
        else mant = (mant >> sh) | ((mant & ((1ULL << sh) - 1)) != 0);
        exp = 1;
    }
    {
        u32 low = (u32)mant & 7;
        mant >>= 3;
        if (low > 4 || (low == 4 && ((u32)mant & 1))) mant++;
    }
    if (mant >= (1ULL << 53)) { mant >>= 1; exp++; }
    if (exp >= 2047) return sign | D_INF;
    if (mant < (1ULL << 52)) return sign | mant;
    return sign | ((u64)exp << 52) | (mant & 0xFFFFFFFFFFFFFULL);
}

static int unpackd(u64 x, int *exp, u64 *mant) {
    int e = (int)((x >> 52) & 0x7FF);
    u64 m = x & 0xFFFFFFFFFFFFFULL;
    if (e == 2047) { *exp = e; *mant = m; return m ? 3 : 2; }
    if (e == 0) {
        if (m == 0) { *exp = 0; *mant = 0; return 0; }
        e = 1;
    } else
        m |= 1ULL << 52;
    *exp = e;
    *mant = m << 3;
    return 1;
}

static u64 addd(u64 a, u64 b) {
    int ea, eb, ca, cb;
    u64 ma, mb, sa = a & D_SIGN, sb = b & D_SIGN;
    ca = unpackd(a, &ea, &ma);
    cb = unpackd(b, &eb, &mb);
    if (ca == 3 || cb == 3) return D_QNAN;
    if (ca == 2) return (cb == 2 && sa != sb) ? D_QNAN : a;
    if (cb == 2) return b;
    if (ca == 0) return cb == 0 ? (sa & sb) : b;
    if (cb == 0) return a;
    if (ea < eb || (ea == eb && ma < mb)) {
        int te = ea; u64 tm = ma, ts = sa;
        ea = eb; ma = mb; sa = sb;
        eb = te; mb = tm; sb = ts;
    }
    {
        int d = ea - eb;
        if (d > 56) mb = 1;
        else if (d) mb = (mb >> d) | ((mb & ((1ULL << d) - 1)) != 0);
    }
    if (sa == sb) ma += mb;
    else {
        ma -= mb;
        if (ma == 0) return 0;
    }
    return packd(sa, ea, ma);
}

double __adddf3(double a, double b) { DU x, y; x.d = a; y.d = b; x.u = addd(x.u, y.u); return x.d; }
double __subdf3(double a, double b) { DU x, y; x.d = a; y.d = b; x.u = addd(x.u, y.u ^ D_SIGN); return x.d; }
double __negdf2(double a) { DU x; x.d = a; x.u ^= D_SIGN; return x.d; }

double __muldf3(double a, double b) {
    DU x, y;
    int ea, eb, ca, cb;
    u64 ma, mb, sign;
    x.d = a; y.d = b;
    sign = (x.u ^ y.u) & D_SIGN;
    ca = unpackd(x.u, &ea, &ma);
    cb = unpackd(y.u, &eb, &mb);
    if (ca == 3 || cb == 3) { x.u = D_QNAN; return x.d; }
    if (ca == 2 || cb == 2) { x.u = (ca == 0 || cb == 0) ? D_QNAN : (sign | D_INF); return x.d; }
    if (ca == 0 || cb == 0) { x.u = sign; return x.d; }
    ma >>= 3; mb >>= 3;                        /* 53-bit significands */
    while (ma < (1ULL << 52)) { ma <<= 1; ea--; }
    while (mb < (1ULL << 52)) { mb <<= 1; eb--; }
    {
        /* 53x53 -> 106 bits via 32-bit limbs: (ah:al) * (bh:bl) */
        u64 al = ma & 0xFFFFFFFFULL, ah = ma >> 32, bl = mb & 0xFFFFFFFFULL, bh = mb >> 32;
        u64 ll = al * bl, lh = al * bh, hl = ah * bl, hh = ah * bh;
        u64 mid = lh + hl;                     /* < 2^54: no overflow */
        u64 lo = ll + (mid << 32);
        u64 hi = hh + (mid >> 32) + (lo < ll);
        /* leading one at bit 104 or 105 of hi:lo; take bits 105..50 -> 55..0 */
        u64 m = (hi << 14) | (lo >> 50);
        if (lo & ((1ULL << 50) - 1)) m |= 1;
        x.u = packd(sign, ea + eb - 1023 + 1, m);
    }
    return x.d;
}

double __divdf3(double a, double b) {
    DU x, y;
    int ea, eb, ca, cb, i;
    u64 ma, mb, sign, q;
    x.d = a; y.d = b;
    sign = (x.u ^ y.u) & D_SIGN;
    ca = unpackd(x.u, &ea, &ma);
    cb = unpackd(y.u, &eb, &mb);
    if (ca == 3 || cb == 3) { x.u = D_QNAN; return x.d; }
    if (ca == 2) { x.u = cb == 2 ? D_QNAN : (sign | D_INF); return x.d; }
    if (cb == 2) { x.u = sign; return x.d; }
    if (cb == 0) { x.u = ca == 0 ? D_QNAN : (sign | D_INF); return x.d; }
    if (ca == 0) { x.u = sign; return x.d; }
    ma >>= 3; mb >>= 3;
    while (ma < (1ULL << 52)) { ma <<= 1; ea--; }
    while (mb < (1ULL << 52)) { mb <<= 1; eb--; }
    if (ma < mb) { ma <<= 1; ea--; }
    q = 0;
    for (i = 0; i < 56; i++) {
        q <<= 1;
        if (ma >= mb) { ma -= mb; q |= 1; }
        ma <<= 1;
    }
    if (ma) q |= 1;
    x.u = packd(sign, ea - eb + 1023, q);
    return x.d;
}

static int cmpd(u64 a, u64 b) {
    u64 aa = a & ~D_SIGN, ab = b & ~D_SIGN;
    if (aa > D_INF || ab > D_INF) return 2;
    if (aa == 0 && ab == 0) return 0;
    if ((a ^ b) & D_SIGN) return (a & D_SIGN) ? -1 : 1;
    if (aa == ab) return 0;
    if (a & D_SIGN) return aa > ab ? -1 : 1;
    return aa < ab ? -1 : 1;
}
int __eqdf2(double a, double b) { DU x, y; x.d = a; y.d = b; return cmpd(x.u, y.u) != 0; }
int __nedf2(double a, double b) { DU x, y; x.d = a; y.d = b; return cmpd(x.u, y.u) != 0; }
int __ltdf2(double a, double b) { DU x, y; int c; x.d = a; y.d = b; c = cmpd(x.u, y.u); return c == 2 ? 1 : c; }
int __ledf2(double a, double b) { DU x, y; int c; x.d = a; y.d = b; c = cmpd(x.u, y.u); return c == 2 ? 1 : c; }
int __gtdf2(double a, double b) { DU x, y; int c; x.d = a; y.d = b; c = cmpd(x.u, y.u); return c == 2 ? -1 : c; }
int __gedf2(double a, double b) { DU x, y; int c; x.d = a; y.d = b; c = cmpd(x.u, y.u); return c == 2 ? -1 : c; }

double __floatundidf(u64 v) {
    DU x;
    u64 m = v;
    u32 sticky = 0;
    int exp = 1023 + 55;
    if (v == 0) { x.u = 0; return x.d; }
    while (m >= (1ULL << 56)) { sticky |= (u32)m & 1; m >>= 1; exp++; }
    x.u = packd(0, exp, m | sticky);
    return x.d;
}
double __floatdidf(s64 v) {
    DU x;
    if (v < 0) { x.d = __floatundidf(0ULL - (u64)v); x.u |= D_SIGN; return x.d; }
    return __floatundidf((u64)v);
}
double __floatunsidf(u32 v) { return __floatundidf((u64)v); }
double __floatsidf(s32 v) { return __floatdidf((s64)v); }

u64 __fixunsdfdi(double a) {
    DU x;
    int e;
    u64 m;
    x.d = a;
    if (x.u & D_SIGN) return 0;
    e = (int)((x.u >> 52) & 0x7FF);
    if (e < 1023) return 0;
    if (e >= 1023 + 64) return 0xFFFFFFFFFFFFFFFFULL;
    m = (x.u & 0xFFFFFFFFFFFFFULL) | (1ULL << 52);
    e -= 1023 + 52;
    return e >= 0 ? m << e : m >> -e;
}
s64 __fixdfdi(double a) {
    DU x;
    u64 m;
    x.d = a;
    if (x.u & D_SIGN) {
        x.u &= ~D_SIGN;
        m = __fixunsdfdi(x.d);
        return m > 0x8000000000000000ULL ? (s64)0x8000000000000000ULL : -(s64)m;
    }
    m = __fixunsdfdi(a);
    return m > 0x7FFFFFFFFFFFFFFFULL ? 0x7FFFFFFFFFFFFFFFLL : (s64)m;
}
u32 __fixunsdfsi(double a) {
    u64 v = __fixunsdfdi(a);
    return v > 0xFFFFFFFFULL ? 0xFFFFFFFFUL : (u32)v;
}
s32 __fixdfsi(double a) {
    s64 v = __fixdfdi(a);
    if (v > 0x7FFFFFFFLL) return 0x7FFFFFFFL;
    if (v < -0x80000000LL) return (s32)0x80000000UL;
    return (s32)v;
}

double __extendsfdf2(float a) {
    FU x;
    DU r;
    int e, c;
    u32 m;
    u64 sign;
    x.f = a;
    sign = (x.u & F_SIGN) ? D_SIGN : 0;
    c = unpackf(x.u, &e, &m);
    if (c == 0) { r.u = sign; return r.d; }
    if (c == 2) { r.u = sign | D_INF; return r.d; }
    if (c == 3) { r.u = D_QNAN; return r.d; }
    /* m has the leading one at bit 26 (or lower for denormals): move it to bit 55 */
    r.u = packd(sign, e - 127 + 1023, (u64)m << 29);
    return r.d;
}
float __truncdfsf2(double a) {
    DU x;
    FU r;
    int e, c;
    u64 m;
    u32 sign, sm;
    x.d = a;
    sign = (x.u & D_SIGN) ? F_SIGN : 0;
    c = unpackd(x.u, &e, &m);
    if (c == 0) { r.u = sign; return r.f; }
    if (c == 2) { r.u = sign | F_INF; return r.f; }
    if (c == 3) { r.u = F_QNAN; return r.f; }
    while (m < (1ULL << 55)) { m <<= 1; e--; }
    sm = (u32)(m >> 29);
    if (m & ((1ULL << 29) - 1)) sm |= 1;
    e = e - 1023 + 127;
    if (e >= 255) { r.u = sign | F_INF; return r.f; }
    if (e < -30) { r.u = sign; return r.f; }
    r.u = packf(sign, e, sm);
    return r.f;
}

float __floatundisf(u64 v) { return __truncdfsf2(__floatundidf(v)); }
float __floatdisf(s64 v) { return __truncdfsf2(__floatdidf(v)); }
u64 __fixunssfdi(float a) { return __fixunsdfdi(__extendsfdf2(a)); }
s64 __fixsfdi(float a) { return __fixdfdi(__extendsfdf2(a)); }
