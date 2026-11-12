/* whoami - print effective user name. */
#include <plusos/syscall.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    struct plusos_users_args u = { 0 };
    u.op = PLUSOS_USERS_WHOAMI;
    int uid = (int)plusos_syscall(SYS_users, &u);
    memset(&u, 0, sizeof u);
    u.op = PLUSOS_USERS_FIND_UID; u.uid = uid;
    if (plusos_syscall(SYS_users, &u) == 0 && u.name[0]) { puts(u.name); return 0; }
    puts("root");
    return 0;
}
