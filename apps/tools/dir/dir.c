/* dir - list directory contents (invokes ls -C). */
#include <plusos/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    char **nav = malloc((size_t)(argc + 3) * sizeof(char *));
    if (!nav) return 1;
    nav[0] = "ls";
    nav[1] = "-C";
    for (int i = 1; i < argc; i++) nav[i + 1] = argv[i];
    nav[argc + 1] = NULL;
    int pid = spawnv("ls", nav);
    free(nav);
    if (pid < 0) { fprintf(stderr, "dir: failed to invoke ls\n"); return 1; }
    return (int)plusos_syscall(SYS_wait, (void *)(intptr_t)pid);
}
