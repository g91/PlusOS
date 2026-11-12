/* testlib.h - tiny host interface for compiler tests running under emulator/tools/run68k.mjs */
#ifndef TESTLIB_H
#define TESTLIB_H
static void putch(int c) { *(volatile unsigned char *)0x00FF0000 = (unsigned char)c; }
static void puts_(const char *s) { while (*s) putch(*s++); }
static void putnum(long v) {
    char buf[16];
    int n = 0;
    unsigned long u = v < 0 ? -(unsigned long)v : (unsigned long)v;
    if (v < 0) putch('-');
    do { buf[n++] = (char)('0' + u % 10); u /= 10; } while (u);
    while (n) putch(buf[--n]);
}
static void puthex(unsigned long v) {
    int i;
    for (i = 28; i >= 0; i -= 4) putch("0123456789ABCDEF"[(v >> i) & 15]);
}
static int failures;
#define CHECK(cond) do { if (!(cond)) { puts_("FAIL line "); putnum(__LINE__); puts_(": " #cond "\n"); failures++; } } while (0)
#define CHECK_EQ(a, b) do { long _a = (long)(a), _b = (long)(b); if (_a != _b) { puts_("FAIL line "); putnum(__LINE__); puts_(": " #a " = "); putnum(_a); puts_(" expected "); putnum(_b); putch('\n'); failures++; } } while (0)
#endif
