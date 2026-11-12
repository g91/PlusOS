/* logname - print user's login name (communicates with whoami). */
#include <plusos/syscall.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    char *av[] = { "whoami", NULL };
    int pid = spawnv("whoami", av);
    if (pid < 0) return 1;
    return (int)plusos_syscall(SYS_wait, (void *)(intptr_t)pid);
}
