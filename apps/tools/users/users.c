/* users - print user names currently logged in (-l lists configured accounts). */
#include <plusos/syscall.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    if (argc > 1 && !strcmp(argv[1], "-l")) {
        struct plusos_users_args u = { 0 }; u.op = PLUSOS_USERS_COUNT;
        int n = (int)plusos_syscall(SYS_users, &u);
        for (int i = 0; i < n; i++) {
            struct plusos_users_args r = { 0 }; r.op = PLUSOS_USERS_AT; r.index = i;
            if (plusos_syscall(SYS_users, &r) == 0) printf("%s%s", i ? " " : "", r.name);
        }
        putchar('\n');
        return 0;
    }
    struct plusos_users_args u = { 0 }; u.op = PLUSOS_USERS_WHOAMI;
    int uid = (int)plusos_syscall(SYS_users, &u);
    memset(&u, 0, sizeof u); u.op = PLUSOS_USERS_FIND_UID; u.uid = uid;
    if (plusos_syscall(SYS_users, &u) == 0 && u.name[0]) puts(u.name);
    else puts("root");
    return 0;
}
