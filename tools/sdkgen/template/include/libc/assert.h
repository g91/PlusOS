/* assert.h - PlusOS libc */
#ifndef PLUSOS_ASSERT_H
#define PLUSOS_ASSERT_H
#ifdef __cplusplus
extern "C" {
#endif
__attribute__((noreturn)) void __plusos_assert_fail(const char *expr, const char *file, int line);
#ifdef __cplusplus
}
#endif
#endif

#undef assert
#ifdef NDEBUG
#define assert(e) ((void)0)
#else
#define assert(e) ((e) ? (void)0 : __plusos_assert_fail(#e, __FILE__, __LINE__))
#endif
