#include "testlib.h"

static int add(int a, int b) { return a + b; }
static int fact(int n) { return n <= 1 ? 1 : n * fact(n - 1); }
static int fib(int n) { int a = 0, b = 1; while (n-- > 0) { int t = a + b; a = b; b = t; } return a; }
int gcounter;
static int garr[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
static const char *names[] = {"zero", "one", "two"};
struct P { short x, y; long z; char tag; };
static struct P gp = {3, -4, 100000, 'q'};

static int sum(const int *p, int n) { int s = 0; for (int i = 0; i < n; i++) s += p[i]; return s; }
static int slen(const char *s) { const char *p = s; while (*p) p++; return (int)(p - s); }
static void scopy(char *d, const char *s) { while ((*d++ = *s++) != 0) {} }
static unsigned udiv(unsigned a, unsigned b) { return a / b; }
static int sdiv(int a, int b) { return a / b; }
static int smod(int a, int b) { return a % b; }
static long mul(long a, long b) { return a * b; }
static int sw(int x) {
    switch (x) {
    case 0: return 10;
    case 1: return 11;
    case 2: return 12;
    case 3: return 13;
    case 4: return 14;
    case 7: return 17;
    case 100: return 5;
    default: return -1;
    }
}
static struct P mk(int a) { struct P p; p.x = (short)a; p.y = (short)(a * 2); p.z = a * 1000L; p.tag = 'm'; return p; }
static int psum(struct P p) { return p.x + p.y + (int)p.z + p.tag; }

int main(void) {
    CHECK_EQ(add(2, 3), 5);
    CHECK_EQ(fact(10), 3628800);
    CHECK_EQ(fib(20), 6765);
    CHECK_EQ(sum(garr, 10), 55);
    CHECK_EQ(slen("hello"), 5);
    CHECK_EQ(slen(names[2]), 3);
    char buf[16];
    scopy(buf, "abcdef");
    CHECK_EQ(buf[3], 'd');
    CHECK_EQ(slen(buf), 6);
    CHECK_EQ(gp.x, 3);
    CHECK_EQ(gp.y, -4);
    CHECK_EQ(gp.z, 100000);
    CHECK_EQ(gp.tag, 'q');
    CHECK_EQ(sizeof(struct P), 10);
    gp.z += gp.x;
    CHECK_EQ(gp.z, 100003);
    CHECK_EQ(udiv(1000000, 7), 142857);
    CHECK_EQ(udiv(4000000000u, 100000), 40000);
    CHECK_EQ(sdiv(-1000000, 7), -142857);
    CHECK_EQ(smod(-1000003, 7), -1000003 % 7);
    CHECK_EQ(smod(1000003, -7), 1000003 % -7);
    CHECK_EQ(mul(123456, 789), 97406784);
    CHECK_EQ(mul(-5, 70000), -350000);
    CHECK_EQ(sw(0), 10); CHECK_EQ(sw(4), 14); CHECK_EQ(sw(7), 17); CHECK_EQ(sw(100), 5); CHECK_EQ(sw(5), -1); CHECK_EQ(sw(-3), -1);
    struct P q = mk(7);
    CHECK_EQ(q.x, 7); CHECK_EQ(q.y, 14); CHECK_EQ(q.z, 7000); CHECK_EQ(q.tag, 'm');
    CHECK_EQ(psum(q), 7 + 14 + 7000 + 'm');
    struct P r = q;
    r.x++;
    CHECK_EQ(r.x, 8); CHECK_EQ(q.x, 7);
    for (int i = 0; i < 10; i++) gcounter += i;
    CHECK_EQ(gcounter, 45);
    unsigned char uc = 250; uc += 10; CHECK_EQ(uc, 4);
    signed char sc = 127; sc++; CHECK_EQ(sc, -128);
    short sh = -5; CHECK_EQ(sh * 3, -15); CHECK_EQ((unsigned short)sh, 65531);
    int x = 0x12345678;
    CHECK_EQ(x >> 8, 0x123456); CHECK_EQ(x << 4, 0x23456780); CHECK_EQ((x >> 16) & 0xFF, 0x34);
    CHECK_EQ(-x >> 20, (-0x12345678) >> 20);
    unsigned ux = 0x87654321u;
    CHECK_EQ(ux >> 28, 8); CHECK_EQ(ux >> 17, 0x87654321u >> 17);
    CHECK((ux > 5) && !(x < 0) && (sc < 0));
    int t = x > 5 ? 1 : 2; CHECK_EQ(t, 1);
    int arr2[3][4]; for (int i = 0; i < 3; i++) for (int j = 0; j < 4; j++) arr2[i][j] = i * 10 + j;
    CHECK_EQ(arr2[2][3], 23); CHECK_EQ(sizeof(arr2), 48);
    int *pp = &arr2[1][0]; CHECK_EQ(pp[5], 21); CHECK_EQ(*(pp + 2), 12);
    long big = 100000; big *= big / 1000; CHECK_EQ(big, 10000000);
    CHECK_EQ('ABCD', 0x41424344);
    if (failures == 0) puts_("t01 ok\n");
    return failures;
}
