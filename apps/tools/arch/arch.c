/* arch - print machine hardware name (communicates with uname -m). */
#include <plusos/syscall.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    char *av[] = { "uname", "-m", NULL };
    int pid = spawnv("uname", av);
    if (pid < 0) { fprintf(stderr, "arch: failed to invoke uname\n"); return 1; }
    return (int)plusos_syscall(SYS_wait, (void *)(intptr_t)pid);
}
