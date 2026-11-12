/* nice - run a program with modified scheduling priority. */
#include <plusos/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    int inc = 10, i = 1;
    if (i < argc && !strcmp(argv[i], "-n") && i + 1 < argc) { inc = atoi(argv[++i]); i++; }
    nice(inc);
    if (i >= argc) { printf("%d\n", nice(0)); return 0; }
    int pid = spawnv(argv[i], &argv[i]);
    if (pid < 0) { fprintf(stderr, "nice: '%s': No such file or directory\n", argv[i]); return 127; }
    return (int)plusos_syscall(SYS_wait, (void *)(intptr_t)pid);
}
