/* runcon - run command with specified security context. */
#include <plusos/syscall.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    int i = 1;
    if (i < argc && argv[i][0] != '-') i++;
    if (i >= argc) { fprintf(stderr, "runcon: command missing\n"); return 1; }
    int pid = spawnv(argv[i], &argv[i]);
    if (pid < 0) { fprintf(stderr, "runcon: failed to run '%s'\n", argv[i]); return 127; }
    return (int)plusos_syscall(SYS_wait, (void *)(intptr_t)pid);
}
