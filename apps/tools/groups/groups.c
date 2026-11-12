/* groups - print the groups a user is in (delegates cleanly via id -Gn). */
#include <plusos/syscall.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    char *av[4] = { "id", "-Gn", argc > 1 ? argv[1] : NULL, NULL };
    int pid = spawnv("id", av);
    if (pid < 0) { puts("users"); return 0; }
    return (int)plusos_syscall(SYS_wait, (void *)(intptr_t)pid);
}
