/* libc_ext.c - the libc/POSIX pieces CPython needs that the PlusOS SDK libc does not have:
 * wide-character strings, locale, clocks, small POSIX stubs and the compiler-builtin helpers
 * (the p68 compiler lowers an unknown __builtin_X to a call of X). */
#include <pyconfig.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <wchar.h>
#include <wctype.h>
#include <locale.h>
#include <langinfo.h>
#include <sys/time.h>
#include <sys/times.h>
#include <signal.h>
#include <plusos/plusos.h>
#include <plusos/syscall.h>

char **environ;

/* ---- wide characters (wchar_t is 32-bit; the locale is always UTF-8) ---- */
size_t wcslen(const wchar_t *s) { const wchar_t *p = s; while (*p) p++; return (size_t)(p - s); }
size_t wcsnlen(const wchar_t *s, size_t n) { size_t i = 0; while (i < n && s[i]) i++; return i; }
wchar_t *wcscpy(wchar_t *d, const wchar_t *s) { wchar_t *r = d; while ((*d++ = *s++)) {} return r; }
wchar_t *wcsncpy(wchar_t *d, const wchar_t *s, size_t n)
{
    size_t i = 0;
    for (; i < n && s[i]; i++) d[i] = s[i];
    for (; i < n; i++) d[i] = 0;
    return d;
}
wchar_t *wcscat(wchar_t *d, const wchar_t *s) { wcscpy(d + wcslen(d), s); return d; }
int wcscmp(const wchar_t *a, const wchar_t *b)
{
    while (*a && *a == *b) { a++; b++; }
    return (*a > *b) - (*a < *b);
}
int wcsncmp(const wchar_t *a, const wchar_t *b, size_t n)
{
    for (; n; n--, a++, b++) { if (*a != *b) return (*a > *b) - (*a < *b); if (!*a) break; }
    return 0;
}
int wcscoll(const wchar_t *a, const wchar_t *b) { return wcscmp(a, b); }
size_t wcsxfrm(wchar_t *d, const wchar_t *s, size_t n) { size_t l = wcslen(s); if (n) wcsncpy(d, s, n); return l; }
wchar_t *wcschr(const wchar_t *s, wchar_t c) { for (;; s++) { if (*s == c) return (wchar_t *)s; if (!*s) return 0; } }
wchar_t *wcsrchr(const wchar_t *s, wchar_t c)
{
    const wchar_t *r = 0;
    for (;; s++) { if (*s == c) r = s; if (!*s) break; }
    return (wchar_t *)r;
}
wchar_t *wcstok(wchar_t *s, const wchar_t *delim, wchar_t **save)
{
    if (!s) s = *save;
    while (*s && wcschr(delim, *s)) s++;
    if (!*s) { *save = s; return 0; }
    wchar_t *tok = s;
    while (*s && !wcschr(delim, *s)) s++;
    if (*s) *s++ = 0;
    *save = s;
    return tok;
}
long wcstol(const wchar_t *s, wchar_t **end, int base)
{
    char buf[80];
    int i = 0;
    while (s[i] && i < 79 && s[i] < 128) { buf[i] = (char)s[i]; i++; }
    buf[i] = 0;
    char *e;
    long v = strtol(buf, &e, base);
    if (end) *end = (wchar_t *)s + (e - buf);
    return v;
}
wchar_t *wmemcpy(wchar_t *d, const wchar_t *s, size_t n) { memcpy(d, s, n * sizeof(wchar_t)); return d; }
wchar_t *wmemmove(wchar_t *d, const wchar_t *s, size_t n) { memmove(d, s, n * sizeof(wchar_t)); return d; }
wchar_t *wmemset(wchar_t *d, wchar_t c, size_t n) { size_t i; for (i = 0; i < n; i++) d[i] = c; return d; }
int wmemcmp(const wchar_t *a, const wchar_t *b, size_t n)
{
    for (; n; n--, a++, b++) if (*a != *b) return (*a > *b) - (*a < *b);
    return 0;
}
wchar_t *wmemchr(const wchar_t *s, wchar_t c, size_t n)
{
    for (; n; n--, s++) if (*s == c) return (wchar_t *)s;
    return 0;
}

/* UTF-8 <-> wchar_t */
size_t mbrtowc(wchar_t *pwc, const char *s, size_t n, mbstate_t *ps)
{
    const unsigned char *p = (const unsigned char *)s;
    if (!s) return 0;
    if (n == 0) return (size_t)-2;
    unsigned c = p[0];
    int len;
    unsigned long cp;
    if (c < 0x80) { if (pwc) *pwc = c; return c ? 1 : 0; }
    else if ((c & 0xE0) == 0xC0) { len = 2; cp = c & 0x1F; }
    else if ((c & 0xF0) == 0xE0) { len = 3; cp = c & 0x0F; }
    else if ((c & 0xF8) == 0xF0) { len = 4; cp = c & 0x07; }
    else { errno = EILSEQ; return (size_t)-1; }
    if ((size_t)len > n) return (size_t)-2;
    int i;
    for (i = 1; i < len; i++) {
        if ((p[i] & 0xC0) != 0x80) { errno = EILSEQ; return (size_t)-1; }
        cp = (cp << 6) | (p[i] & 0x3F);
    }
    if (pwc) *pwc = (wchar_t)cp;
    return (size_t)len;
}
size_t wcrtomb(char *s, wchar_t wc, mbstate_t *ps)
{
    unsigned long c = (unsigned long)wc;
    if (!s) return 1;
    if (c < 0x80) { s[0] = (char)c; return 1; }
    if (c < 0x800) { s[0] = (char)(0xC0 | (c >> 6)); s[1] = (char)(0x80 | (c & 0x3F)); return 2; }
    if (c < 0x10000) {
        s[0] = (char)(0xE0 | (c >> 12)); s[1] = (char)(0x80 | ((c >> 6) & 0x3F)); s[2] = (char)(0x80 | (c & 0x3F));
        return 3;
    }
    if (c < 0x110000) {
        s[0] = (char)(0xF0 | (c >> 18)); s[1] = (char)(0x80 | ((c >> 12) & 0x3F));
        s[2] = (char)(0x80 | ((c >> 6) & 0x3F)); s[3] = (char)(0x80 | (c & 0x3F));
        return 4;
    }
    errno = EILSEQ;
    return (size_t)-1;
}
int mbtowc(wchar_t *pwc, const char *s, size_t n) { if (!s) return 0; size_t r = mbrtowc(pwc, s, n, 0); return (r == (size_t)-1 || r == (size_t)-2) ? -1 : (int)r; }
int wctomb(char *s, wchar_t wc) { if (!s) return 0; size_t r = wcrtomb(s, wc, 0); return r == (size_t)-1 ? -1 : (int)r; }
size_t mbrlen(const char *s, size_t n, mbstate_t *ps) { return mbrtowc(0, s, n, ps); }
int mbsinit(const mbstate_t *ps) { return 1; }
size_t mbstowcs(wchar_t *d, const char *s, size_t n)
{
    size_t cnt = 0;
    for (;;) {
        wchar_t w;
        size_t r = mbrtowc(&w, s, 4, 0);
        if (r == (size_t)-1 || r == (size_t)-2) return (size_t)-1;
        if (d) { if (cnt >= n) return cnt; d[cnt] = w; }
        if (r == 0) return cnt;
        cnt++; s += r;
    }
}
size_t wcstombs(char *d, const wchar_t *s, size_t n)
{
    size_t cnt = 0;
    char tmp[4];
    for (;; s++) {
        size_t r = wcrtomb(tmp, *s, 0);
        if (r == (size_t)-1) return (size_t)-1;
        if (*s == 0) { if (d && cnt < n) d[cnt] = 0; return cnt; }
        if (d) { if (cnt + r > n) return cnt; memcpy(d + cnt, tmp, r); }
        cnt += r;
    }
}
int iswalpha(wint_t c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c > 0xBF; }
int iswupper(wint_t c) { return c >= 'A' && c <= 'Z'; }
int iswlower(wint_t c) { return c >= 'a' && c <= 'z'; }
int iswdigit(wint_t c) { return c >= '0' && c <= '9'; }
int iswalnum(wint_t c) { return iswalpha(c) || iswdigit(c); }
int iswspace(wint_t c) { return c == ' ' || (c >= 9 && c <= 13); }
int iswpunct(wint_t c) { return c > 32 && c < 127 && !iswalnum(c); }
int iswprint(wint_t c) { return c >= 32 && c != 127; }
wint_t towlower(wint_t c) { return iswupper(c) ? c + 32 : c; }
wint_t towupper(wint_t c) { return iswlower(c) ? c - 32 : c; }

/* ---- locale: always "C" with UTF-8 ---- */
char *setlocale(int cat, const char *loc) { return (char *)"C"; }
static struct lconv g_lconv = { (char *)".", (char *)"", (char *)"", (char *)"", (char *)"", (char *)"", (char *)"",
                                (char *)"", (char *)"", (char *)"", 127, 127, 127, 127, 127, 127, 127, 127 };
struct lconv *localeconv(void) { return &g_lconv; }
char *nl_langinfo(int item)
{
    static const char *days[] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
    static const char *mons[] = { "January", "February", "March", "April", "May", "June", "July", "August",
                                  "September", "October", "November", "December" };
    if (item == CODESET) return (char *)"UTF-8";
    if (item >= DAY_1 && item <= DAY_7) return (char *)days[item - DAY_1];
    if (item >= ABDAY_1 && item <= ABDAY_1 + 6) { static char b[4]; memcpy(b, days[item - ABDAY_1], 3); b[3] = 0; return b; }
    if (item >= MON_1 && item <= MON_12) return (char *)mons[item - MON_1];
    if (item >= ABMON_1 && item <= ABMON_12) { static char b[4]; memcpy(b, mons[item - ABMON_1], 3); b[3] = 0; return b; }
    if (item == RADIXCHAR) return (char *)".";
    if (item == AM_STR) return (char *)"AM";
    if (item == PM_STR) return (char *)"PM";
    if (item == D_FMT) return (char *)"%m/%d/%y";
    if (item == T_FMT) return (char *)"%H:%M:%S";
    if (item == D_T_FMT) return (char *)"%a %b %e %H:%M:%S %Y";
    if (item == YESEXPR) return (char *)"^[yY]";
    if (item == NOEXPR) return (char *)"^[nN]";
    return (char *)"";
}

/* ---- time ---- */
long timezone = 0;
int daylight = 0;
char *tzname[2] = { (char *)"UTC", (char *)"UTC" };
void tzset(void) {}

static unsigned long g_ticks0;
static unsigned long g_wall0;
static int g_clock_init;
static void clock_init(void)
{
    if (g_clock_init) return;
    g_clock_init = 1;
    g_wall0 = (unsigned long)time(0);
    g_ticks0 = (unsigned long)ticks();
}
/* 60 Hz ticks since boot -> wall clock anchored at first use */
int clock_gettime(int clk, struct timespec *ts)
{
    clock_init();
    unsigned long t = (unsigned long)ticks();
    unsigned long dt = t - g_ticks0;
    unsigned long sec = dt / 60UL, rem = dt % 60UL;
    if (clk == CLOCK_REALTIME) ts->tv_sec = (time_t)(g_wall0 + sec);
    else ts->tv_sec = (time_t)(t / 60UL);
    ts->tv_nsec = (long)(((clk == CLOCK_REALTIME ? rem : t % 60UL) * 1000000UL / 60UL) * 1000UL);
    return 0;
}
int clock_getres(int clk, struct timespec *ts) { ts->tv_sec = 0; ts->tv_nsec = 16666667L; return 0; }
int gettimeofday(struct timeval *tv, struct timezone *tz)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    tv->tv_sec = ts.tv_sec; tv->tv_usec = ts.tv_nsec / 1000;
    if (tz) { tz->tz_minuteswest = 0; tz->tz_dsttime = 0; }
    return 0;
}
clock_t clock(void) { return (clock_t)((unsigned long)ticks() * (1000000UL / 60UL)); }
clock_t times(struct tms *b)
{
    clock_t c = (clock_t)(ticks());
    if (b) { b->tms_utime = c; b->tms_stime = 0; b->tms_cutime = 0; b->tms_cstime = 0; }
    return c;
}
struct tm *gmtime_r(const time_t *t, struct tm *out) { struct tm *r = gmtime(t); if (!r) return 0; *out = *r; return out; }
struct tm *localtime_r(const time_t *t, struct tm *out) { struct tm *r = localtime(t); if (!r) return 0; *out = *r; return out; }

/* ---- small POSIX bits ---- */
void _exit(int code) { plusos_syscall(SYS_exit, (void *)(long)code); for (;;) {} }
int fcntl(int fd, int cmd, ...) { return 0; }
int pause(void) { for (;;) plusos_syscall(SYS_sleep, (void *)60L); }
int raise(int sig) { return kill(getpid(), sig); }
void setbuf(void *f, char *buf) {}
char *strpbrk(const char *s, const char *accept)
{
    for (; *s; s++) { const char *a; for (a = accept; *a; a++) if (*a == *s) return (char *)s; }
    return 0;
}

/* ---- compiler builtins that lower to calls ---- */
int clz(unsigned int x) { int n = 0; if (!x) return 32; while (!(x & 0x80000000UL)) { x <<= 1; n++; } return n; }
int clzl(unsigned long x) { return clz((unsigned int)x); }
int ctz(unsigned int x) { int n = 0; if (!x) return 32; while (!(x & 1)) { x >>= 1; n++; } return n; }
int popcount(unsigned int x) { int n = 0; while (x) { n += x & 1; x >>= 1; } return n; }
void *frame_address(int level)
{
    /* the stack pointer of the caller is close enough: CPython only uses it for stack-depth checks */
    char probe;
    return &probe;
}
double nan(const char *s) { return -(1e999 - 1e999); }
double huge_val(void) { return 1e999; }

/* ---- pieces of CPython this port leaves out ---- */
int _PyFaulthandler_Init(int enable) { return 0; }
void _PyFaulthandler_Fini(void) {}
int _PyPerfTrampoline_Init(int activate) { return 0; }
int _PyPerfTrampoline_Fini(void) { return 0; }
int _PySysRemoteDebug_SendExec(int pid, int tid, const char *debugger_script_path) { return -1; }
