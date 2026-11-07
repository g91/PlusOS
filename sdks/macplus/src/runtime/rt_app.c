/* rt_app.c - app side of the runtime: entry from crt0, exit(), and the libc
 * back-end. A PlusOS app is a ring-2 (user mode) process: it exits through the
 * syscall gate, and its blocking/process/power operations go through the
 * service table entries the kernel points at syscall trampolines. Console,
 * memory and ROM calls are reached directly. */
#include <plusos/plusos.h>
#include <plusos/syscall.h>

const PlusOSAPI *plusos;

extern int main(int argc, char **argv);
extern void __plusos_run_ctors(void);
extern void __plusos_run_dtors(void);

/* Entry from crt0 with (argc, argv, api). Runs main, then exits via syscall;
 * does not return to crt0. */
int __plusos_app_start(int argc, char **argv, const PlusOSAPI *api)
{
    plusos = api;
    if (api && api->magic == PLUSOS_API_MAGIC) {
        __plusos_run_ctors();
        int rc = main(argc, argv);
        __plusos_run_dtors();
        exit(rc);
    }
    exit(-999);                         /* not started by a PlusOS kernel */
    return 0;
}

void exit(int code)
{
    __plusos_run_dtors();
    plusos_syscall(SYS_exit, (void *)(intptr_t)code);     /* kernel ends this process */
    for (;;) {}                          /* unreachable */
}

void abort(void)
{
    puts("\n*** abort() ***");
    exit(134);
}

/* ---- libc back-end (forwards to the kernel's service table) ---- */
void __plusos_con_putc(int c)                    { plusos->con_putc(c); }
void __plusos_con_write(const char *s, size_t n) { plusos->con_write(s, n); }
int  __plusos_key_get(void)                      { return plusos->key_get(); }
int  __plusos_key_line(char *buf, int size)      { return plusos->key_line(buf, size); }
void *__plusos_mem_alloc(uint32_t n)             { return plusos->mem_alloc(n); }
void  __plusos_mem_free(void *p)                 { plusos->mem_free(p); }
