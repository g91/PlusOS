/* chroot - run command or interactive shell with special root directory. */
#include <plusos/syscall.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc < 2) { fprintf(stderr, "chroot: missing operand\n"); return 1; }
    if (chroot(argv[1]) != 0) {
        fprintf(stderr, "chroot: cannot change root directory to '%s': %s\n", argv[1], strerror(errno));
        return 125;
    }
    if (argc >= 3) {
        int pid = spawnv(argv[2], &argv[2]);
        if (pid < 0) { fprintf(stderr, "chroot: failed to run '%s'\n", argv[2]); return 127; }
        return (int)plusos_syscall(SYS_wait, (void *)(intptr_t)pid);
    }
    char *sh[] = { "shell", NULL };
    int pid = spawnv("shell", sh);
    return pid < 0 ? 127 : (int)plusos_syscall(SYS_wait, (void *)(intptr_t)pid);
}
