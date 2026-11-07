/* libc_string.c - PlusOS libc: memory and string functions. */
#include <string.h>
#include <stdint.h>

void *memcpy(void *dst, const void *src, size_t n)
{
    uint8_t *d = dst;
    const uint8_t *s = src;
    /* long copies when both are even-aligned (68000 faults on odd longs) */
    if ((((uintptr_t)d | (uintptr_t)s) & 1) == 0) {
        while (n >= 4) { *(uint32_t *)d = *(const uint32_t *)s; d += 4; s += 4; n -= 4; }
    }
    while (n--) *d++ = *s++;
    return dst;
}

void *memmove(void *dst, const void *src, size_t n)
{
    uint8_t *d = dst;
    const uint8_t *s = src;
    if (d == s || n == 0) return dst;
    if (d < s || d >= s + n) return memcpy(dst, src, n);
    d += n; s += n;
    if ((((uintptr_t)d | (uintptr_t)s) & 1) == 0) {
        while (n >= 4) { d -= 4; s -= 4; *(uint32_t *)d = *(const uint32_t *)s; n -= 4; }
    }
    while (n--) *--d = *--s;
    return dst;
}

void *memset(void *dst, int c, size_t n)
{
    uint8_t *d = dst;
    uint8_t b = (uint8_t)c;
    if (((uintptr_t)d & 1) == 0) {
        uint32_t w = b * 0x01010101UL;
        while (n >= 4) { *(uint32_t *)d = w; d += 4; n -= 4; }
    }
    while (n--) *d++ = b;
    return dst;
}

int memcmp(const void *a, const void *b, size_t n)
{
    const uint8_t *x = a, *y = b;
    for (; n; n--, x++, y++)
        if (*x != *y) return *x - *y;
    return 0;
}

void *memchr(const void *s, int c, size_t n)
{
    const uint8_t *p = s;
    for (; n; n--, p++)
        if (*p == (uint8_t)c) return (void *)p;
    return 0;
}

size_t strlen(const char *s)               { const char *p = s; while (*p) p++; return p - s; }
size_t strnlen(const char *s, size_t max)  { size_t n = 0; while (n < max && s[n]) n++; return n; }

char *strcpy(char *dst, const char *src)   { char *d = dst; while ((*d++ = *src++)) ; return dst; }

char *strncpy(char *dst, const char *src, size_t n)
{
    size_t i = 0;
    for (; i < n && src[i]; i++) dst[i] = src[i];
    for (; i < n; i++) dst[i] = 0;
    return dst;
}

size_t strlcpy(char *dst, const char *src, size_t size)
{
    size_t len = strlen(src);
    if (size) {
        size_t n = len < size - 1 ? len : size - 1;
        memcpy(dst, src, n);
        dst[n] = 0;
    }
    return len;
}

char *strcat(char *dst, const char *src)   { strcpy(dst + strlen(dst), src); return dst; }

char *strncat(char *dst, const char *src, size_t n)
{
    char *d = dst + strlen(dst);
    while (n-- && *src) *d++ = *src++;
    *d = 0;
    return dst;
}

int strcmp(const char *a, const char *b)
{
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char *a, const char *b, size_t n)
{
    for (; n; n--, a++, b++) {
        if (*a != *b || !*a) return (unsigned char)*a - (unsigned char)*b;
    }
    return 0;
}

static int lower(int c) { return (c >= 'A' && c <= 'Z') ? c + 32 : c; }

int strcasecmp(const char *a, const char *b)
{
    while (*a && lower(*a) == lower(*b)) { a++; b++; }
    return lower((unsigned char)*a) - lower((unsigned char)*b);
}

int strncasecmp(const char *a, const char *b, size_t n)
{
    for (; n; n--, a++, b++) {
        int x = lower((unsigned char)*a), y = lower((unsigned char)*b);
        if (x != y || !x) return x - y;
    }
    return 0;
}

char *strchr(const char *s, int c)
{
    for (;; s++) {
        if (*s == (char)c) return (char *)s;
        if (!*s) return 0;
    }
}

char *strrchr(const char *s, int c)
{
    const char *r = 0;
    for (;; s++) {
        if (*s == (char)c) r = s;
        if (!*s) return (char *)r;
    }
}

char *strstr(const char *hay, const char *needle)
{
    size_t n = strlen(needle);
    if (!n) return (char *)hay;
    for (; *hay; hay++)
        if (*hay == *needle && !strncmp(hay, needle, n)) return (char *)hay;
    return 0;
}

size_t strspn(const char *s, const char *accept)
{
    size_t n = 0;
    while (s[n] && strchr(accept, s[n])) n++;
    return n;
}

size_t strcspn(const char *s, const char *reject)
{
    size_t n = 0;
    while (s[n] && !strchr(reject, s[n])) n++;
    return n;
}

char *strtok_r(char *s, const char *delim, char **save)
{
    if (!s) s = *save;
    s += strspn(s, delim);
    if (!*s) { *save = s; return 0; }
    char *end = s + strcspn(s, delim);
    if (*end) *end++ = 0;
    *save = end;
    return s;
}

unsigned char *c2pstr_buf(unsigned char *dst, const char *src)
{
    size_t n = strlen(src);
    if (n > 255) n = 255;
    dst[0] = (unsigned char)n;
    memcpy(dst + 1, src, n);
    return dst;
}

char *p2cstr_buf(char *dst, const unsigned char *src)
{
    memcpy(dst, src + 1, src[0]);
    dst[src[0]] = 0;
    return dst;
}
