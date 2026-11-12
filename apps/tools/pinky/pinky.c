/* pinky - lightweight finger. */
#include <plusos/syscall.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    int short_fmt = 1, i = 1;
    for (; i < argc && argv[i][0] == '-'; i++) {
        if (!strcmp(argv[i], "-l")) short_fmt = 0; else if (!strcmp(argv[i], "-s")) short_fmt = 1;
    }
    struct plusos_users_args u = { 0 };
    if (i < argc) { u.op = PLUSOS_USERS_FIND; strlcpy(u.name, argv[i], sizeof u.name); }
    else {
        u.op = PLUSOS_USERS_WHOAMI; int uid = (int)plusos_syscall(SYS_users, &u);
        memset(&u, 0, sizeof u); u.op = PLUSOS_USERS_FIND_UID; u.uid = uid;
    }
    if (plusos_syscall(SYS_users, &u) != 0) return 1;
    if (!short_fmt) printf("Login name: %-20s\nDirectory: %-20s  Shell: /bin/shell\n", u.name, u.home);
    else {
        printf("Login    Name                 TTY      Idle   When             Where\n");
        printf("%-8s %-20s tty0     00:00  now              console\n", u.name, u.home);
    }
    return 0;
}
