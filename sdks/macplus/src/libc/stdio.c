/* libc_stdio.c - PlusOS libc: printf family, putchar/puts/getchar.
 * The formatter writes through a callback so printf has no buffer limit. */
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* back-end: provided by the kernel (console.c / kbd.c) or by rt_app.c */
extern void __plusos_con_putc(int c);
extern void __plusos_con_write(const char *s, size_t n);
extern int  __plusos_key_get(void);
extern int  __plusos_key_line(char *buf, int size);

typedef void (*out_fn)(int c, void *ctx);

static int emit_str(out_fn out, void *ctx, const char *s, int len, int width, int left, char pad)
{
    int n = 0;
    if (!left) for (; len < width; width--) { out(pad, ctx); n++; }
    for (int i = 0; i < len; i++) { out(s[i], ctx); n++; }
    if (left) for (; len < width; width--) { out(' ', ctx); n++; }
    return n;
}

int __plusos_vformat(out_fn out, void *ctx, const char *fmt, va_list ap)
{
    int total = 0;
    char buf[24];
    for (; *fmt; fmt++) {
        if (*fmt != '%') { out(*fmt, ctx); total++; continue; }
        fmt++;
        int left = 0, plus = 0, space = 0, alt = 0;
        char pad = ' ';
        for (;; fmt++) {
            if (*fmt == '-') left = 1;
            else if (*fmt == '0') pad = '0';
            else if (*fmt == '+') plus = 1;
            else if (*fmt == ' ') space = 1;
            else if (*fmt == '#') alt = 1;
            else break;
        }
        int width = 0, prec = -1;
        if (*fmt == '*') { width = va_arg(ap, int); fmt++; if (width < 0) { left = 1; width = -width; } }
        else while (*fmt >= '0' && *fmt <= '9') width = width * 10 + (*fmt++ - '0');
        if (*fmt == '.') {
            fmt++; prec = 0;
            if (*fmt == '*') { prec = va_arg(ap, int); fmt++; }
            else while (*fmt >= '0' && *fmt <= '9') prec = prec * 10 + (*fmt++ - '0');
        }
        int lng = 0;
        while (*fmt == 'l' || *fmt == 'h' || *fmt == 'z' || *fmt == 't') { if (*fmt == 'l') lng++; fmt++; }
        if (left) pad = ' ';

        switch (*fmt) {
        case 'c': buf[0] = (char)va_arg(ap, int); total += emit_str(out, ctx, buf, 1, width, left, ' '); break;
        case 's': {
            const char *s = va_arg(ap, const char *);
            if (!s) s = "(null)";
            int len = (int)strlen(s);
            if (prec >= 0 && len > prec) len = prec;
            total += emit_str(out, ctx, s, len, width, left, ' ');
            break;
        }
        case 'P': {             /* Pascal string (Str255) - handy for Mac ROM calls */
            const unsigned char *p = va_arg(ap, const unsigned char *);
            total += emit_str(out, ctx, p ? (const char *)p + 1 : "", p ? p[0] : 0, width, left, ' ');
            break;
        }
        case 'd': case 'i': case 'u': case 'x': case 'X': case 'o': case 'b': case 'p': {
            unsigned long long v;
            int neg = 0, base = 10, upper = 0;
            if (*fmt == 'p') { v = (uintptr_t)va_arg(ap, void *); base = 16; alt = 1; }
            else if (*fmt == 'd' || *fmt == 'i') {
                long long sv = lng >= 2 ? va_arg(ap, long long) : lng ? va_arg(ap, long) : va_arg(ap, int);
                if (sv < 0) { neg = 1; v = (unsigned long long)(-sv); } else v = (unsigned long long)sv;
            } else {
                v = lng >= 2 ? va_arg(ap, unsigned long long) : lng ? va_arg(ap, unsigned long) : va_arg(ap, unsigned);
                if (*fmt == 'x') base = 16;
                if (*fmt == 'X') { base = 16; upper = 1; }
                if (*fmt == 'o') base = 8;
                if (*fmt == 'b') base = 2;
            }
            int i = sizeof buf;
            const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
            if (v == 0 && prec != 0) buf[--i] = '0';
            while (v) {
                /* 32-bit fast path avoids pulling 64-bit division into every binary */
                if (v <= 0xFFFFFFFFULL) { uint32_t w = (uint32_t)v; buf[--i] = digits[w % base]; v = w / base; }
                else { buf[--i] = digits[v % base]; v /= base; }
            }
            while (prec > 0 && (int)sizeof buf - i < prec) buf[--i] = '0';
            char prefix[3]; int pl = 0;
            if (neg) prefix[pl++] = '-';
            else if (plus && (*fmt == 'd' || *fmt == 'i')) prefix[pl++] = '+';
            else if (space && (*fmt == 'd' || *fmt == 'i')) prefix[pl++] = ' ';
            if (alt && base == 16) { prefix[pl++] = '0'; prefix[pl++] = upper ? 'X' : 'x'; }
            int len = (int)sizeof buf - i;
            if (pad == '0') {           /* sign/prefix before zero padding */
                for (int k = 0; k < pl; k++) { out(prefix[k], ctx); total++; }
                total += emit_str(out, ctx, buf + i, len, width - pl, left, '0');
            } else {
                int w = width - pl - len;
                if (!left) for (; w > 0; w--) { out(' ', ctx); total++; }
                for (int k = 0; k < pl; k++) { out(prefix[k], ctx); total++; }
                total += emit_str(out, ctx, buf + i, len, 0, 0, ' ');
                if (left) for (; w > 0; w--) { out(' ', ctx); total++; }
            }
            break;
        }
        case '%': out('%', ctx); total++; break;
        case 0: return total;
        default: out('%', ctx); out(*fmt, ctx); total += 2; break;
        }
    }
    return total;
}

static void out_console(int c, void *ctx) { (void)ctx; __plusos_con_putc(c); }

struct bufctx { char *buf; size_t size, pos; };
static void out_buffer(int c, void *ctx)
{
    struct bufctx *b = ctx;
    if (b->pos + 1 < b->size) b->buf[b->pos] = (char)c;
    b->pos++;
}

int vprintf(const char *fmt, va_list ap) { return __plusos_vformat(out_console, 0, fmt, ap); }

int printf(const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    int n = vprintf(fmt, ap);
    va_end(ap);
    return n;
}

int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap)
{
    struct bufctx b = { buf, size, 0 };
    int n = __plusos_vformat(out_buffer, &b, fmt, ap);
    if (size) buf[b.pos < size ? b.pos : size - 1] = 0;
    return n;
}

int snprintf(char *buf, size_t size, const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    int n = vsnprintf(buf, size, fmt, ap);
    va_end(ap);
    return n;
}

int sprintf(char *buf, const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    int n = vsnprintf(buf, 0x7FFFFFFF, fmt, ap);
    va_end(ap);
    return n;
}

int putchar(int c) { __plusos_con_putc(c); return c & 0xFF; }

int puts(const char *s)
{
    __plusos_con_write(s, strlen(s));
    __plusos_con_putc('\n');
    return 0;
}

int getchar(void) { return __plusos_key_get(); }

char *gets_s(char *buf, size_t size)
{
    __plusos_key_line(buf, (int)size);
    return buf;
}
