#include "testlib.h"

/* ---- Pascal convention checked against hand-written assembly ----
 * pascal short PAdd(short a, long b, char c):
 *   stack on entry: 0 ret, 4 c (byte in the high half of a word), 6 b, 10 a, 12 result word */
__asm__(
    "\t.text\n\t.even\n\t.globl PAdd\n"
    "PAdd:\n"
    "\tmove.b 4(%sp),%d0\n\text.w %d0\n"
    "\tadd.w 8(%sp),%d0\n"          /* low word of b */
    "\tadd.w 10(%sp),%d0\n"
    "\tmove.w %d0,12(%sp)\n"
    "\tmove.l (%sp)+,%a0\n\taddq.l #8,%sp\n\tjmp (%a0)\n"
    /* pascal long PPtr(char *p, unsigned char flag) : returns p[1] + flag*256 */
    "\t.globl PPtr\n"
    "PPtr:\n"
    "\tmoveq #0,%d0\n\tmove.b 4(%sp),%d0\n\tlsl.l #8,%d0\n"
    "\tmove.l 6(%sp),%a0\n\tmoveq #0,%d1\n\tmove.b 1(%a0),%d1\n\tadd.l %d1,%d0\n"
    "\tmove.l %d0,10(%sp)\n"
    "\tmove.l (%sp)+,%a0\n\taddq.l #6,%sp\n\tjmp (%a0)\n"
    /* register routine: d0 = d0 * 2 + (a0)  ; also trashes d1/d2/a1 like a ROM trap */
    "\t.globl RegFn\n"
    "RegFn:\n"
    "\tadd.l %d0,%d0\n\tadd.l (%a0),%d0\n\tmoveq #-1,%d1\n\tmoveq #-1,%d2\n\tsuba.l %a1,%a1\n\trts\n"
    /* calls a pascal callback: pascal long cb(short x, long y) with (7, 100000) */
    "\t.globl CallCB\n"
    "CallCB:\n"
    "\tmove.l 4(%sp),%a0\n"
    "\tsubq.l #4,%sp\n\tmove.w #7,-(%sp)\n\tmove.l #100000,-(%sp)\n\tjsr (%a0)\n\tmove.l (%sp)+,%d0\n\trts\n");

pascal short PAdd(short a, long b, char c);
pascal long PPtr(char *p, unsigned char flag);
#pragma parameter __D0 RegFn(__D0, __A0)
long RegFn(long v, long *p);
typedef pascal long (*CBProc)(short x, long y);
long CallCB(CBProc cb);

static pascal long myCallback(short x, long y) { return y * 2 + x; }
static pascal void pvoid(short *out, short v) { *out = (short)(v + 1); }
static pascal char pchar(char c, short n) { return (char)(c + n); }
struct Pt { short v, h; };
static pascal long ptsum(struct Pt p, short k) { return p.v * 1000L + p.h + k; }

/* inline machine code "traps": words are executed in place of a call */
pascal short InlTwice(short v) = {0x301F, 0xD040, 0x3E80};   /* move.w (sp)+,d0 ; add.w d0,d0 ; move.w d0,(sp) */
#pragma parameter __D0 InlNeg(__D0)
long InlNeg(long v) = {0x4480};                               /* neg.l d0 */
#pragma parameter __A0 InlPtr(__A0, __D0)
char *InlPtr(char *base, long idx) = {0xD1C0};                /* adda.l d0,a0 */

static unsigned short mulu16(unsigned short a, unsigned short b) {
    unsigned long r = a;
    __asm__("mulu.w %1,%0" : "+d"(r) : "dmi"(b) : "cc");
    return (unsigned short)(r >> 8);
}
static short getsr(void) {
    short sr;
    __asm__ volatile("move.w %%sr,%0" : "=d"(sr));
    return sr;
}
static long viaRegs(long *pb, long n) {
    register long *a0 __asm__("a0") = pb;
    register long d0 __asm__("d0") = n;
    __asm__ volatile("jsr RegFn" : "+d"(d0), "+a"(a0) : : "d1", "d2", "a1", "cc", "memory");
    return d0;
}
static int bitset(volatile unsigned char *p) {
    unsigned char was;
    __asm__ volatile("bset #7,%1\n\tseq %0" : "=d"(was), "+m"(*p) : : "cc");
    return was != 0;
}

int main(void) {
    CHECK_EQ(PAdd(100, 70000L + 23, 5), (short)(100 + 23 + 4464 + 5));
    CHECK_EQ(PAdd(-1, 0, -1), -2);
    char s[4] = {1, 2, 3, 4};
    CHECK_EQ(PPtr(s, 3), 2 + 3 * 256);
    long cell = 5;
    long keep1 = cell * 3, keep2 = cell * 7, keep3 = cell + 100;   /* live across the calls */
    CHECK_EQ(RegFn(10, &cell), 25);
    CHECK_EQ(viaRegs(&cell, 4), 13);
    CHECK_EQ(keep1 + keep2 + keep3, 15 + 35 + 105);
    CHECK_EQ(CallCB(myCallback), 200007);
    CBProc cb = myCallback;
    CHECK_EQ(cb(3, 10), 23);
    short out = 0; pvoid(&out, 41); CHECK_EQ(out, 42);
    CHECK_EQ(pchar('a', 2), 'c');
    struct Pt pt = {3, 4}; CHECK_EQ(ptsum(pt, 5), 3009);
    CHECK_EQ(InlTwice(21), 42);
    CHECK_EQ(InlNeg(5), -5);
    CHECK_EQ(InlPtr(s, 2)[0], 3);
    CHECK_EQ(mulu16(300, 300), (300UL * 300) >> 8);
    CHECK_EQ(getsr() & 0x2000, 0x2000);
    volatile unsigned char flag = 1;
    CHECK_EQ(bitset(&flag), 1); CHECK_EQ(flag, 0x81); CHECK_EQ(bitset(&flag), 0);
    if (failures == 0) puts_("t05 ok\n");
    return failures;
}
