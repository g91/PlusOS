/* stdbuf - run command with modified buffering operations for its standard streams. */
#include <plusos/syscall.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    int i = 1;
    while (i < argc && argv[i][0] == '-') {
        if ((!strcmp(argv[i], "-i") || !strcmp(argv[i], "-o") || !strcmp(argv[i], "-e")) && i + 1 < argc) i += 2;
        else if (!strncmp(argv[i], "-i", 2) || !strncmp(argv[i], "-o", 2) || !strncmp(argv[i], "-e", 2)) i++;
        else break;
    }
    if (i >= argc) { fprintf(stderr, "stdbuf: missing operand\n"); return 125; }
    int pid = spawnv(argv[i], &argv[i]);
    if (pid < 0) return 127;
    return (int)plusos_syscall(SYS_wait, (void *)(intptr_t)pid);
}
