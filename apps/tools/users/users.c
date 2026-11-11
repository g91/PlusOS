/* users - list the accounts in /etc/passwd. */
#include <plusos/plusos.h>
#include <plusos/syscall.h>

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    struct plusos_users_args u = { 0 };
    u.op = PLUSOS_USERS_COUNT;
    int n = (int)plusos_syscall(SYS_users, &u);
    if (n <= 0) { printf("(no accounts)\n"); return 0; }
    printf("UID   NAME             HOME                    SUDO\n");
    for (int i = 0; i < n; i++) {
        struct plusos_users_args r = { 0 };
        r.op = PLUSOS_USERS_AT; r.index = i;
        if (plusos_syscall(SYS_users, &r) != 0) continue;
        printf("%-5d %-16s %-23s %s\n", r.uid, r.name, r.home,
               (r.flags & 0x01) ? "yes" : "no");
    }
    return 0;
}
