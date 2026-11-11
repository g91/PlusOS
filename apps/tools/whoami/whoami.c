/* whoami - print the name of the currently logged-in user. */
#include <plusos/plusos.h>
#include <plusos/syscall.h>

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    struct plusos_users_args u = { 0 };
    u.op = PLUSOS_USERS_WHOAMI;
    int uid = (int)plusos_syscall(SYS_users, &u);
    u.op = PLUSOS_USERS_FIND_UID;
    u.uid = uid;
    if (plusos_syscall(SYS_users, &u) != 0) {
        printf("(not logged in)\n");
        return 1;
    }
    printf("%s\n", u.name);
    return 0;
}
