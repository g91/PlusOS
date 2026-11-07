/* libc_stdlib.c - PlusOS libc: number parsing, rand, qsort, malloc. */
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

extern void *__plusos_mem_alloc(uint32_t n);
extern void  __plusos_mem_free(void *p);

/* Each block carries its size in front, so realloc works on any heap. */
void *malloc(size_t n)
{
    uint32_t *p = __plusos_mem_alloc((uint32_t)n + 4);
    if (!p) return 0;
    p[0] = (uint32_t)n;
    return p + 1;
}

void free(void *p)
{
    if (p) __plusos_mem_free((uint32_t *)p - 1);
}

void *calloc(size_t count, size_t size)
{
    size_t n = count * size;
    if (size && n / size != count) return 0;
    void *p = malloc(n);
    if (p) memset(p, 0, n);
    return p;
}

void *realloc(void *p, size_t n)
{
    if (!p) return malloc(n);
    if (!n) { free(p); return 0; }
    uint32_t old = ((uint32_t *)p)[-1];
    if (n <= old) return p;
    void *q = malloc(n);
    if (!q) return 0;
    memcpy(q, p, old);
    free(p);
    return q;
}

unsigned long strtoul(const char *s, char **end, int base)
{
    while (isspace((unsigned char)*s)) s++;
    if (*s == '+') s++;
    if ((base == 0 || base == 16) && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) { s += 2; base = 16; }
    else if ((base == 0 || base == 16) && s[0] == '$') { s++; base = 16; }   /* Mac-style hex */
    else if (base == 0 && s[0] == '0') base = 8;
    else if (base == 0) base = 10;
    unsigned long v = 0;
    for (;; s++) {
        int d;
        if (isdigit((unsigned char)*s)) d = *s - '0';
        else if (isalpha((unsigned char)*s)) d = tolower((unsigned char)*s) - 'a' + 10;
        else break;
        if (d >= base) break;
        v = v * base + d;
    }
    if (end) *end = (char *)s;
    return v;
}

long strtol(const char *s, char **end, int base)
{
    while (isspace((unsigned char)*s)) s++;
    int neg = 0;
    if (*s == '-') { neg = 1; s++; }
    unsigned long v = strtoul(s, end, base);
    return neg ? -(long)v : (long)v;
}

int  atoi(const char *s) { return (int)strtol(s, 0, 10); }

/* Convert a long to text in base 2..36. Returns buf. */
char *ltoa(long value, char *buf, int base)
{
    if (base < 2 || base > 36) { buf[0] = 0; return buf; }
    char tmp[34];
    int i = 0, neg = 0;
    unsigned long v;
    if (value < 0 && base == 10) { neg = 1; v = (unsigned long)(-value); } else v = (unsigned long)value;
    do { int d = (int)(v % (unsigned long)base); tmp[i++] = (char)(d < 10 ? '0' + d : 'a' + d - 10); v /= (unsigned long)base; } while (v);
    if (neg) tmp[i++] = '-';
    int j = 0;
    while (i) buf[j++] = tmp[--i];
    buf[j] = 0;
    return buf;
}
char *itoa(int value, char *buf, int base) { return ltoa(value, buf, base); }
long atol(const char *s) { return strtol(s, 0, 10); }
int  abs(int v)          { return v < 0 ? -v : v; }
long labs(long v)        { return v < 0 ? -v : v; }

static uint32_t rand_state = 1;
void srand(unsigned seed) { rand_state = seed ? seed : 1; }
int rand(void)
{
    rand_state = rand_state * 1103515245UL + 12345;
    return (int)((rand_state >> 16) & RAND_MAX);
}

static void swap_bytes(char *a, char *b, size_t n)
{
    while (n--) { char t = *a; *a++ = *b; *b++ = t; }
}

/* insertion sort for small runs, quicksort above - no recursion depth issues */
void qsort(void *base, size_t n, size_t size, int (*cmp)(const void *, const void *))
{
    char *b = base;
    while (n > 8) {
        char *pivot = b + (n / 2) * size;
        swap_bytes(pivot, b + (n - 1) * size, size);
        pivot = b + (n - 1) * size;
        size_t store = 0;
        for (size_t i = 0; i < n - 1; i++)
            if (cmp(b + i * size, pivot) < 0) { swap_bytes(b + i * size, b + store * size, size); store++; }
        swap_bytes(b + store * size, pivot, size);
        /* recurse into the smaller half, loop on the larger */
        if (store < n - store - 1) { qsort(b, store, size, cmp); b += (store + 1) * size; n -= store + 1; }
        else { qsort(b + (store + 1) * size, n - store - 1, size, cmp); n = store; }
    }
    for (size_t i = 1; i < n; i++)
        for (size_t j = i; j > 0 && cmp(b + (j - 1) * size, b + j * size) > 0; j--)
            swap_bytes(b + (j - 1) * size, b + j * size, size);
}
