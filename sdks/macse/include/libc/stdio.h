/* stdio.h - PlusOS libc. Output goes to the PlusOS text console. */
#ifndef PLUSOS_STDIO_H
#define PLUSOS_STDIO_H
#include <stddef.h>
#include <stdarg.h>
#ifdef __cplusplus
extern "C" {
#endif

#define EOF (-1)
#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

typedef struct __plusos_file FILE;

extern FILE *const stdin;
extern FILE *const stdout;
extern FILE *const stderr;

int putchar(int c);
int puts(const char *s);                 /* writes s and a newline */
int printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
int vprintf(const char *fmt, va_list ap);
int snprintf(char *buf, size_t size, const char *fmt, ...) __attribute__((format(printf, 3, 4)));
int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap);
int sprintf(char *buf, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

int getchar(void);                       /* blocks for one key (ASCII) */
char *gets_s(char *buf, size_t size);    /* line editor with echo; returns buf */

FILE  *fopen(const char *path, const char *mode);
FILE  *fdopen(int fd, const char *mode);
FILE  *freopen(const char *path, const char *mode, FILE *stream);
int    fclose(FILE *stream);
int    fflush(FILE *stream);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
int    fgetc(FILE *stream);
#define getc(f) fgetc(f)
char  *fgets(char *s, int size, FILE *stream);
int    fputc(int c, FILE *stream);
#define putc(c, f) fputc((c), (f))
int    fputs(const char *s, FILE *stream);
int    ungetc(int c, FILE *stream);
int    fprintf(FILE *stream, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
int    vfprintf(FILE *stream, const char *fmt, va_list ap);
int    fseek(FILE *stream, long offset, int whence);
long   ftell(FILE *stream);
void   rewind(FILE *stream);
int    feof(FILE *stream);
int    ferror(FILE *stream);
void   clearerr(FILE *stream);
int    fileno(FILE *stream);
int    remove(const char *path);
int    rename(const char *oldpath, const char *newpath);
void   perror(const char *s);
long   getline(char **lineptr, size_t *n, FILE *stream);

#ifdef __cplusplus
}
#endif
#endif
