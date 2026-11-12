#include "testlib.h"

typedef union { float f; unsigned long u; } FU;
typedef union { double d; struct { unsigned long hi, lo; } w; } DU;
static unsigned long fb(float f) { FU x; x.f = f; return x.u; }
static unsigned long dhi(double d) { DU x; x.d = d; return x.w.hi; }
static unsigned long dlo(double d) { DU x; x.d = d; return x.w.lo; }
static float fsq(float x) { return x * x; }
static double poly(double x) { return 3.0 * x * x - 2.0 * x + 0.5; }
static float fsum(const float *p, int n) { float s = 0; for (int i = 0; i < n; i++) s += p[i]; return s; }
static const float ftab[4] = {1.5f, -2.25f, 100.0f, 0.125f};
static float mysqrt(float x) { float g = x * 0.5f; for (int i = 0; i < 12; i++) g = (g + x / g) * 0.5f; return g; }

int main(void) {
    volatile float a = 1.5f, b = 2.25f;
    CHECK_EQ(fb(a + b), 0x40700000);          /* 3.75 */
    CHECK_EQ(fb(a - b), 0xBF400000);          /* -0.75 */
    CHECK_EQ(fb(a * b), 0x40580000);          /* 3.375 */
    CHECK_EQ(fb(b / a), 0x3FC00000);          /* 1.5 */
    CHECK_EQ(fb(1.0f / 3.0f), 0x3EAAAAAB);
    volatile float one = 1.0f, three = 3.0f, tenth = 0.1f;
    CHECK_EQ(fb(one / three), 0x3EAAAAAB);
    CHECK_EQ(fb(tenth * three), 0x3E99999A);
    CHECK_EQ(fb(tenth + tenth + tenth), 0x3E99999A);
    CHECK(a < b); CHECK(!(a > b)); CHECK(a <= a); CHECK(a != b); CHECK(a == a); CHECK(-a < a);
    CHECK_EQ((int)(a * 10), 15); CHECK_EQ((int)-3.99f, -3); CHECK_EQ((unsigned)3e9f, 3000000000u);
    volatile int iv = -7; volatile unsigned uv = 4000000000u;
    CHECK_EQ(fb((float)iv), 0xC0E00000); CHECK_EQ(fb((float)uv), 0x4F6E6B28);
    CHECK_EQ(fb(fsq(12.5f)), 0x431C4000);     /* 156.25 */
    CHECK_EQ(fb(fsum(ftab, 4)), 0x42C6C000);  /* 99.375 */
    CHECK_EQ((int)(mysqrt(2.0f) * 100000), 141421);
    CHECK_EQ(fb(-a), 0xBFC00000);
    float big = 3e38f; big = big * 10; CHECK_EQ(fb(big), 0x7F800000);
    float tiny = 1e-38f; tiny = tiny / 1000; CHECK(tiny > 0); CHECK(tiny < 1e-40f);
    /* double */
    volatile double x = 1.5, y = 0.1;
    CHECK_EQ(dhi(x + y), 0x3FF99999); CHECK_EQ(dlo(x + y), 0x9999999A);
    CHECK_EQ(dhi(x * y), 0x3FC33333); CHECK_EQ(dlo(x * y), 0x33333334);
    CHECK_EQ(dhi(x / y), 0x402E0000); CHECK_EQ(dlo(x / y), 0x00000000);
    CHECK_EQ(dhi(y - x), 0xBFF66666); CHECK_EQ(dlo(y - x), 0x66666666);
    CHECK_EQ(dhi(poly(2.0)), 0x40210000);     /* 8.5 */
    CHECK(x > y); CHECK(y < x); CHECK(x == 1.5); CHECK(x != y);
    CHECK_EQ((long)(x * 1000000.0), 1500000);
    CHECK_EQ(fb((float)y), 0x3DCCCCCD);
    CHECK_EQ(dhi((double)a), 0x3FF80000);
    CHECK_EQ(dhi((double)iv), 0xC01C0000);
    CHECK_EQ((int)(float)(double)123456, 123456);
    long long ll = 1234567890123LL; double dl = (double)ll; CHECK((long long)dl == ll);
    if (failures == 0) puts_("t04 ok\n");
    return failures;
}
