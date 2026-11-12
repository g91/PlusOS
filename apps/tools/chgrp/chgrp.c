/* chgrp - change group ownership (delegates cleanly via chown :GROUP). */
#include <plusos/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc < 3) { fprintf(stderr, "chgrp: missing operand\n"); return 1; }
    int idx = 1;
    while (idx < argc && argv[idx][0] == '-' && argv[idx][1]) idx++;
    if (idx + 1 >= argc) { fprintf(stderr, "chgrp: missing operand\n"); return 1; }
    char grp_spec[48];
    snprintf(grp_spec, sizeof grp_spec, ":%s", argv[idx]);
    argv[idx] = grp_spec;
    argv[0] = "chown";
    int pid = spawnv("chown", argv);
    if (pid < 0) { fprintf(stderr, "chgrp: failed to invoke chown\n"); return 1; }
    return (int)plusos_syscall(SYS_wait, (void *)(intptr_t)pid);
}
