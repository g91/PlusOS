/* kill - end a process by pid (see ps for the list). */
#include <plusos/plusos.h>
#include <plusos/syscall.h>

int main(int argc, char **argv)
{
    if (argc < 2) { printf("usage: kill <pid>\n"); return 1; }
    for (int i = 1; i < argc; i++) {
        int pid = atoi(argv[i]);
        long rc = plusos_syscall(SYS_kill, (void *)(intptr_t)pid);
        if (rc == 0) printf("killed %d\n", pid);
        else printf("kill %d: no such process, or it is protected\n", pid);
    }
    return 0;
}
