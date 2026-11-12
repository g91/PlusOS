#include "testlib.h"
#include <stdarg.h>

/* ---- structs, unions, bitfields, initialisers ---- */
struct Rect { short top, left, bottom, right; };
struct Node { struct Node *next; int val; char name[6]; };
union U { long l; short s[2]; unsigned char b[4]; };
struct BF { unsigned a : 3; unsigned b : 5; int c : 4; unsigned d : 12; unsigned e : 8; };
struct Packed { char c; long l; } __attribute__((packed));
struct Nest { struct Rect r; struct { int x; int y; } pt; int arr[3]; };
enum Color { RED, GREEN = 5, BLUE };
static const struct Rect rects[] = {{1, 2, 3, 4}, {.right = 9, .top = 7}, {0}};
static struct Nest gn = {.pt = {.y = 22}, .arr = {[1] = 5, 6}, .r = {10, 20, 30, 40}};
static int tab[] = {[3] = 3, [1] = 1, 9};
static char msg[] = "hey";
static const char *const strs[2] = {"ab", "cde"};
static int (*fptab[3])(int, int);
static long long gll = 0x123456789ALL;

static int f_add(int a, int b) { return a + b; }
static int f_sub(int a, int b) { return a - b; }
static int f_mul(int a, int b) { return a * b; }
static int apply(int (*f)(int, int), int a, int b) { return f(a, b); }

static int vsum(int n, ...) {
    va_list ap;
    int s = 0;
    va_start(ap, n);
    while (n--) s += va_arg(ap, int);
    va_end(ap);
    return s;
}
static long vmix(const char *fmt, ...) {
    va_list ap;
    long s = 0;
    va_start(ap, fmt);
    for (; *fmt; fmt++) {
        if (*fmt == 'i') s += va_arg(ap, int);
        else if (*fmt == 'l') s += (long)va_arg(ap, long long);
        else if (*fmt == 's') s += va_arg(ap, char *)[0];
    }
    va_end(ap);
    return s;
}

static int area(struct Rect r) { return (r.bottom - r.top) * (r.right - r.left); }
static struct Rect inset(struct Rect r, int d) { r.top += d; r.left += d; r.bottom -= d; r.right -= d; return r; }
static void rswap(struct Rect *a, struct Rect *b) { struct Rect t = *a; *a = *b; *b = t; }

static int counter(void) { static int n = 10; return n++; }
static int collatz(unsigned n) { int steps = 0; while (n != 1) { n = (n & 1) ? 3 * n + 1 : n / 2; steps++; } return steps; }
static int gotoTest(int n) {
    int s = 0;
again:
    s += n;
    if (--n > 0) goto again;
    return s;
}
static int dense(int x) {
    switch (x) {
    case 10: return 1; case 11: return 2; case 12: return 3; case 13: return 4; case 14: return 5;
    case 15: return 6; case 16: return 7; case 18: return 9; case 20 ... 25: return 99;
    }
    return 0;
}
static int fallthrough(int x) {
    int r = 0;
    switch (x) {
    case 1: r += 1;
    case 2: r += 2;
    case 3: r += 4; break;
    default: r = -1;
    }
    return r;
}
static unsigned char sat(int v) { return v < 0 ? 0 : v > 255 ? 255 : (unsigned char)v; }
static int strcmp_(const char *a, const char *b) { while (*a && *a == *b) { a++; b++; } return (unsigned char)*a - (unsigned char)*b; }
static void *memset_(void *d, int c, unsigned long n) { unsigned char *p = d; while (n--) *p++ = (unsigned char)c; return d; }
static void rev(char *s, int n) { for (int i = 0, j = n - 1; i < j; i++, j--) { char t = s[i]; s[i] = s[j]; s[j] = t; } }
static int many(int a, int b, int c, int d, int e, int f, int g, int h, int i, int j) { return a - b + c - d + e - f + g - h + i - j; }
static short narrow(short a, unsigned char b, signed char c) { return (short)(a + b * c); }

int main(void) {
    CHECK_EQ(rects[0].right, 4); CHECK_EQ(rects[1].top, 7); CHECK_EQ(rects[1].right, 9); CHECK_EQ(rects[1].left, 0);
    CHECK_EQ(sizeof(rects) / sizeof(rects[0]), 3);
    CHECK_EQ(gn.pt.y, 22); CHECK_EQ(gn.pt.x, 0); CHECK_EQ(gn.arr[1], 5); CHECK_EQ(gn.arr[2], 6); CHECK_EQ(gn.r.bottom, 30);
    CHECK_EQ(sizeof(tab) / sizeof(int), 4); CHECK_EQ(tab[2], 9); CHECK_EQ(tab[3], 3); CHECK_EQ(tab[0], 0);
    CHECK_EQ(sizeof(msg), 4); CHECK_EQ(msg[2], 'y'); CHECK_EQ(strs[1][2], 'e');
    CHECK_EQ(sizeof(struct Packed), 5); CHECK_EQ(sizeof(struct Node), 14); CHECK_EQ(sizeof(union U), 4);
    CHECK_EQ(BLUE, 6);
    union U u; u.l = 0x11223344; CHECK_EQ(u.s[0], 0x1122); CHECK_EQ(u.b[3], 0x44); u.b[0] = 0x7F; CHECK_EQ(u.l, 0x7F223344);
    struct BF bf; bf.a = 5; bf.b = 17; bf.c = -3; bf.d = 0xABC; bf.e = 200;
    CHECK_EQ(bf.a, 5); CHECK_EQ(bf.b, 17); CHECK_EQ(bf.c, -3); CHECK_EQ(bf.d, 0xABC); CHECK_EQ(bf.e, 200);
    bf.b++; bf.a = 9; CHECK_EQ(bf.b, 18); CHECK_EQ(bf.a, 1); CHECK_EQ(bf.c, -3);
    struct Packed pk; pk.c = 1; pk.l = 0x01020304; CHECK_EQ(pk.l, 0x01020304); CHECK_EQ(((unsigned char *)&pk)[4], 4);
    fptab[0] = f_add; fptab[1] = f_sub; fptab[2] = f_mul;
    CHECK_EQ(fptab[2](6, 7), 42); CHECK_EQ(apply(fptab[1], 10, 3), 7); CHECK_EQ((*fptab[0])(1, 2), 3);
    CHECK_EQ(vsum(4, 1, 2, 3, 4), 10); CHECK_EQ(vsum(0), 0);
    CHECK_EQ(vmix("ilsi", 5, 0x100000007LL, "A", 2), 5 + 7 + 65 + 2);
    struct Rect r = {10, 10, 30, 50};
    CHECK_EQ(area(r), 800); CHECK_EQ(area(inset(r, 5)), 300); CHECK_EQ(r.top, 10);
    struct Rect r2 = {1, 1, 2, 2}; rswap(&r, &r2); CHECK_EQ(r.right, 2); CHECK_EQ(r2.right, 50);
    CHECK_EQ(counter(), 10); CHECK_EQ(counter(), 11);
    CHECK_EQ(collatz(27), 111);
    CHECK_EQ(gotoTest(10), 55);
    CHECK_EQ(dense(10), 1); CHECK_EQ(dense(16), 7); CHECK_EQ(dense(17), 0); CHECK_EQ(dense(18), 9); CHECK_EQ(dense(22), 99); CHECK_EQ(dense(26), 0); CHECK_EQ(dense(-5), 0);
    CHECK_EQ(fallthrough(1), 7); CHECK_EQ(fallthrough(2), 6); CHECK_EQ(fallthrough(3), 4); CHECK_EQ(fallthrough(9), -1);
    CHECK_EQ(sat(-5), 0); CHECK_EQ(sat(300), 255); CHECK_EQ(sat(77), 77);
    CHECK(strcmp_("abc", "abd") < 0); CHECK_EQ(strcmp_("x", "x"), 0);
    char buf[40]; memset_(buf, 'z', 39); buf[39] = 0; CHECK_EQ(buf[38], 'z');
    char w[] = "abcdefg"; rev(w, 7); CHECK_EQ(strcmp_(w, "gfedcba"), 0);
    CHECK_EQ(many(1, 2, 3, 4, 5, 6, 7, 8, 9, 10), -5);
    CHECK_EQ(narrow(1000, 200, -3), 400);
    struct Node n1 = {0, 1, "one"}, n2 = {&n1, 2, "two"}, n3 = {&n2, 3, "tri"};
    int tot = 0; for (struct Node *p = &n3; p; p = p->next) tot += p->val * p->name[0];
    CHECK_EQ(tot, 3 * 't' + 2 * 't' + 'o');
    /* long long */
    long long a = gll, b = 1000000007LL;
    CHECK_EQ((long)(a >> 8), 0x12345678); CHECK_EQ((long)(a + b >> 32), (long)((0x123456789ALL + 1000000007LL) >> 32));
    CHECK((a * 3) == 0x369D0369CELL); CHECK(a / 1000 == 0x123456789ALL / 1000); CHECK(a % 1000 == 0x123456789ALL % 1000);
    CHECK(a > b); CHECK(-a < 0); CHECK((a ^ a) == 0); CHECK((unsigned long long)-1 > 5ULL);
    unsigned long long s = 1; s <<= 40; CHECK_EQ((long)(s >> 36), 16);
    long long neg = -5; CHECK_EQ((long)(neg / 2), -2); CHECK_EQ((int)(neg * neg), 25);
    /* misc expressions */
    int i = 5, j; j = i++ + ++i; CHECK_EQ(i, 7); CHECK_EQ(j, 12);
    int k = (i > 3, i - 1); CHECK_EQ(k, 6);
    int z = 0; int e1 = z && (1 / z); int e2 = !z || (1 / z); CHECK_EQ(e1, 0); CHECK_EQ(e2, 1);
    unsigned short us = 65535; us++; CHECK_EQ(us, 0);
    unsigned un = 5; CHECK(un - 6 > 0); CHECK_EQ((int)(un - 6), -1);
    CHECK_EQ(-7 / 2, -3); CHECK_EQ(-7 % 2, -1); CHECK_EQ(7 >> 1, 3); CHECK_EQ(-8 >> 1, -4);
    int vv = -100; CHECK_EQ(vv / 8, -12); CHECK_EQ(vv % 8, -4); CHECK_EQ(vv * 10, -1000); CHECK_EQ(vv / 16, -6);
    int sh = 3; CHECK_EQ(1 << sh, 8); CHECK_EQ(0x80000000u >> sh, 0x10000000); CHECK_EQ(vv >> sh, -13); CHECK_EQ(vv << 17, -100 * 131072);
    char c8 = (char)200; CHECK_EQ(c8, -56); CHECK_EQ((unsigned char)c8, 200); CHECK_EQ(c8 == -56, 1);
    int big[100]; for (int q = 0; q < 100; q++) big[q] = q * q; long bs = 0; for (int q = 99; q >= 0; q--) bs += big[q]; CHECK_EQ(bs, 328350);
    if (failures == 0) puts_("t02 ok\n");
    return failures;
}
