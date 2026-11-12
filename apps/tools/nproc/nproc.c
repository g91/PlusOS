/* nproc - print the number of processing units available. */
#include <plusos/syscall.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    struct plusos_posix_args pa = { 0 };
    pa.op = PLUSOS_POSIX_NPROC;
    long n = plusos_syscall(SYS_posix, &pa);
    printf("%ld\n", n > 0 ? n : 1L);
    return 0;
}
