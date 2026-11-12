/* stddef.h - P68 freestanding header */
#ifndef _P68_STDDEF_H
#define _P68_STDDEF_H
typedef unsigned long size_t;
typedef long ptrdiff_t;
#ifndef __cplusplus
typedef long wchar_t;
#define NULL ((void *)0)
#else
#define NULL 0
#endif
typedef long max_align_t;
#define offsetof(type, member) ((size_t)&((type *)0)->member)
#endif
