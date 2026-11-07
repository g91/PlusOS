/* setjmp.h - PlusOS libc. Saves d2-d7/a2-a7 and the return PC. */
#ifndef PLUSOS_SETJMP_H
#define PLUSOS_SETJMP_H
#ifdef __cplusplus
extern "C" {
#endif

typedef long jmp_buf[13];   /* d2-d7 (6), a2-a7 (6), pc (1) */

int  setjmp(jmp_buf env) __attribute__((returns_twice));
__attribute__((noreturn)) void longjmp(jmp_buf env, int val);

#ifdef __cplusplus
}
#endif
#endif
