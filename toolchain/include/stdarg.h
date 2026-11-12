/* stdarg.h - P68 freestanding header */
#ifndef _P68_STDARG_H
#define _P68_STDARG_H
typedef __builtin_va_list va_list;
typedef __builtin_va_list __gnuc_va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type)   __builtin_va_arg(ap, type)
#define va_end(ap)         ((void)0)
#define va_copy(dst, src)  ((dst) = (src))
#define __va_copy(dst, src) ((dst) = (src))
#endif
