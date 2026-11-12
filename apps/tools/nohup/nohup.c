/* nohup - run a command immune to hangups. */
#include <plusos/syscall.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc < 2) { fprintf(stderr, "nohup: missing operand\n"); return 125; }
    signal(SIGHUP, SIG_IGN);
    int pid = spawnv(argv[1], &argv[1]);
    if (pid < 0) { fprintf(stderr, "nohup: failed to run '%s'\n", argv[1]); return 127; }
    return (int)plusos_syscall(SYS_wait, (void *)(intptr_t)pid);
}
