/* syscall.h - kernel side of the PlusOS syscall gate. The ABI (numbers,
 * argument structs, the TRAP #1 stub) is shared with userland in
 * <plusos/syscall.h>. */
#ifndef PLUSOS_KERNEL_SYSCALL_H
#define PLUSOS_KERNEL_SYSCALL_H
#include <plusos/syscall.h>

long syscall_dispatch(long num, void *arg);     /* called from trap.S */
void syscall_init(void);                         /* install the TRAP #1 vector */

/* Syscall trampolines: tiny TRAP #1 stubs the service table points at, so an
 * app's blocking/process/power calls become syscalls whether it is in user or
 * supervisor mode. */
int  ktrap_getkey(void);
int  ktrap_readline(char *buf, int size);
int  ktrap_spawn(const char *name, int argc, char **argv);
__attribute__((noreturn)) void ktrap_reboot(void);
__attribute__((noreturn)) void ktrap_shutdown(void);
void *ktrap_malloc(uint32_t n);
void  ktrap_free(void *p);
uint32_t ktrap_memfree(void);
long  console_op(void *arg);
void  ktrap_con_putc(int ch);
void  ktrap_con_write(const char *s, size_t n);
void  ktrap_con_clear(void);
void  ktrap_con_goto(int c, int r);
void  ktrap_con_getpos(int *c, int *r);
void  ktrap_con_invert(int on);
void  ktrap_con_cursor(int on);
int   ktrap_pollkey(void);
void  ktrap_sleep(int ticks);
int   ktrap_blk_read(uint32_t b, uint32_t n, void *buf);
int   ktrap_blk_write(uint32_t b, uint32_t n, const void *buf);

#endif
