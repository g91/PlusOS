/* timeout - run a command with a time limit. */
#include <plusos/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

int main(int argc, char **argv)
{
    int i = 1;
    while (i < argc && argv[i][0] == '-') {
        if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "-k")) i += 2; else i++;
    }
    if (argc - i < 2) { fprintf(stderr, "timeout: missing operand\n"); return 125; }
    unsigned long secs = strtoul(argv[i++], NULL, 10);
    struct plusos_posix_args pa = { 0 };
    pa.op = PLUSOS_POSIX_SPAWN_REDIR; pa.name = argv[i];
    pa.argc = argc - i; pa.argv = &argv[i];
    pa.fds[0] = -1; pa.fds[1] = -1; pa.fds[2] = -1;
    long pid = plusos_syscall(SYS_posix, &pa);
    if (pid < 0) return 127;
    unsigned long ticks = secs * 60;
    for (unsigned long t = 0; t < ticks || ticks == 0; t += 2) {
        struct plusos_procinfo pi[16];
        int np = (int)plusos_syscall(SYS_proclist, pi);
        int alive = 0;
        for (int k = 0; k < np; k++) if (pi[k].pid == (int)pid) { alive = 1; break; }
        if (!alive) return 0;
        if (ticks > 0 && t + 2 >= ticks) break;
        plusos_syscall(SYS_sleep, (void *)(intptr_t)2);
    }
    if (ticks > 0) {
        struct plusos_posix_args ka = { 0 };
        ka.op = PLUSOS_POSIX_KILL_SIG; ka.a = (int)pid; ka.b = SIGTERM;
        plusos_syscall(SYS_posix, &ka);
        return 124;
    }
    return 0;
}
