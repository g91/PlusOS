/* printenv - print all or part of environment. */
#include <plusos/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    char term = '\n'; int i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) if (!strcmp(argv[i], "-0")) term = '\0';
    if (i >= argc) {
        char *buf = malloc(1024);
        if (!buf) return 1;
        struct plusos_posix_args pa = { 0 };
        pa.op = PLUSOS_POSIX_GETENV_ALL; pa.ptr = buf; pa.size = 1024;
        long n = plusos_syscall(SYS_posix, &pa);
        if (n > 0) fwrite(buf, 1, (size_t)n, stdout);
        free(buf);
        return 0;
    }
    int rc = 0;
    for (; i < argc; i++) {
        const char *v = getenv(argv[i]);
        if (v) printf("%s%c", v, term); else rc = 1;
    }
    return rc;
}
