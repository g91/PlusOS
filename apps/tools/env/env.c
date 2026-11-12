/* env - run a program in a modified environment. */
#include <plusos/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    int ignore = 0, nul = 0, i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (!strcmp(argv[i], "--")) { i++; break; }
        if (!strcmp(argv[i], "-i")) { ignore = 1; continue; }
        if (!strcmp(argv[i], "-0")) { nul = 1; continue; }
        if (!strcmp(argv[i], "-u") && i + 1 < argc) { unsetenv(argv[++i]); continue; }
    }
    if (ignore) {
        struct plusos_posix_args pa = { 0 };
        pa.op = PLUSOS_POSIX_CLEARENV;
        plusos_syscall(SYS_posix, &pa);
    }
    while (i < argc && strchr(argv[i], '=')) {
        char *eq = strchr(argv[i], '=');
        *eq = '\0'; setenv(argv[i], eq + 1, 1); *eq = '=';
        i++;
    }
    if (i < argc) {
        int pid = spawnv(argv[i], &argv[i]);
        if (pid < 0) { fprintf(stderr, "env: '%s': No such file or directory\n", argv[i]); return 127; }
        return (int)plusos_syscall(SYS_wait, (void *)(intptr_t)pid);
    }
    char *buf = malloc(1024);
    if (!buf) return 1;
    struct plusos_posix_args pa = { 0 };
    pa.op = PLUSOS_POSIX_GETENV_ALL; pa.ptr = buf; pa.size = 1024;
    long n = plusos_syscall(SYS_posix, &pa);
    if (n > 0) {
        if (nul) for (long k = 0; k < n; k++) if (buf[k] == '\n') buf[k] = '\0';
        fwrite(buf, 1, (size_t)n, stdout);
    }
    free(buf);
    return 0;
}
