#ifndef PLUSOS_WCHAR_H
#define PLUSOS_WCHAR_H
#include <stddef.h>
#include <stdarg.h>
typedef long wint_t;
typedef struct { int __count; } mbstate_t;
#define WEOF ((wint_t)-1)
#define WCHAR_MIN 0
#define WCHAR_MAX 0x7fffffffL
size_t wcslen(const wchar_t *s);
size_t wcsnlen(const wchar_t *s, size_t n);
wchar_t *wcscpy(wchar_t *d, const wchar_t *s);
wchar_t *wcsncpy(wchar_t *d, const wchar_t *s, size_t n);
wchar_t *wcscat(wchar_t *d, const wchar_t *s);
int wcscmp(const wchar_t *a, const wchar_t *b);
int wcsncmp(const wchar_t *a, const wchar_t *b, size_t n);
int wcscoll(const wchar_t *a, const wchar_t *b);
wchar_t *wcschr(const wchar_t *s, wchar_t c);
wchar_t *wcsrchr(const wchar_t *s, wchar_t c);
wchar_t *wmemcpy(wchar_t *d, const wchar_t *s, size_t n);
wchar_t *wmemmove(wchar_t *d, const wchar_t *s, size_t n);
wchar_t *wmemset(wchar_t *d, wchar_t c, size_t n);
int wmemcmp(const wchar_t *a, const wchar_t *b, size_t n);
size_t mbstowcs(wchar_t *d, const char *s, size_t n);
size_t wcstombs(char *d, const wchar_t *s, size_t n);
size_t mbrtowc(wchar_t *pwc, const char *s, size_t n, mbstate_t *ps);
size_t wcrtomb(char *s, wchar_t wc, mbstate_t *ps);
int mbtowc(wchar_t *pwc, const char *s, size_t n);
int wctomb(char *s, wchar_t wc);
size_t mbrlen(const char *s, size_t n, mbstate_t *ps);
int mbsinit(const mbstate_t *ps);
int swprintf(wchar_t *s, size_t n, const wchar_t *fmt, ...);
size_t wcsxfrm(wchar_t *d, const wchar_t *s, size_t n);
#endif
