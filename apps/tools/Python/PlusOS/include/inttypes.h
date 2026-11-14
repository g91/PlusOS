#ifndef PLUSOS_INTTYPES_H
#define PLUSOS_INTTYPES_H
#include <stdint.h>
#define PRId8 "d"
#define PRId16 "d"
#define PRId32 "ld"
#define PRId64 "lld"
#define PRIu32 "lu"
#define PRIu64 "llu"
#define PRIx32 "lx"
#define PRIx64 "llx"
#define PRIdPTR "ld"
#define PRIuPTR "lu"
#define PRIxPTR "lx"
#define PRIdMAX "lld"
#define PRIuMAX "llu"
typedef long long intmax_t_dummy;
#endif
