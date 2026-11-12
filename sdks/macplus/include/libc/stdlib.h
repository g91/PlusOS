/* stdlib.h - PlusOS libc. malloc/free sit on the ROM Memory Manager (NewPtr). */
#ifndef PLUSOS_STDLIB_H
#define PLUSOS_STDLIB_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define RAND_MAX     0x7FFF

void *malloc(size_t n);
void *calloc(size_t count, size_t size);
void *realloc(void *p, size_t n);
void  free(void *p);

int   atoi(const char *s);
char *itoa(int value, char *buf, int base);     /* base 2..36 */
char *ltoa(long value, char *buf, int base);
long  atol(const char *s);
long  strtol(const char *s, char **end, int base);
unsigned long strtoul(const char *s, char **end, int base);
long long strtoll(const char *s, char **end, int base);
unsigned long long strtoull(const char *s, char **end, int base);
int   abs(int v);
long  labs(long v);

int   rand(void);
void  srand(unsigned seed);
void  qsort(void *base, size_t n, size_t size, int (*cmp)(const void *, const void *));
void *bsearch(const void *key, const void *base, size_t n, size_t size, int (*cmp)(const void *, const void *));

char *getenv(const char *name);
int   setenv(const char *name, const char *value, int overwrite);
int   unsetenv(const char *name);
int   mkstemp(char *tmpl);
char *mkdtemp(char *tmpl);
char *realpath(const char *path, char *resolved_path);
int   system(const char *command);

__attribute__((noreturn)) void exit(int code);
__attribute__((noreturn)) void abort(void);
int   atexit(void (*fn)(void));

#ifdef __cplusplus
}
#endif
#endif
