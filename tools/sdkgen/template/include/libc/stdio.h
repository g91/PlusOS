/* stdio.h - PlusOS libc. Output goes to the PlusOS text console. */
#ifndef PLUSOS_STDIO_H
#define PLUSOS_STDIO_H
#include <stddef.h>
#include <stdarg.h>
#ifdef __cplusplus
extern "C" {
#endif

#define EOF (-1)

int putchar(int c);
int puts(const char *s);                 /* writes s and a newline */
int printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
int vprintf(const char *fmt, va_list ap);
int snprintf(char *buf, size_t size, const char *fmt, ...) __attribute__((format(printf, 3, 4)));
int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap);
int sprintf(char *buf, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

int getchar(void);                       /* blocks for one key (ASCII) */
char *gets_s(char *buf, size_t size);    /* line editor with echo; returns buf */

#ifdef __cplusplus
}
#endif
#endif
