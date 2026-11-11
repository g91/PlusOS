/* userdel - remove a user account. Root only.
 *
 * Usage: userdel NAME
 *
 * The user's /home/<name> is left on disk (classic Unix userdel keeps the
 * data; use rm manually to remove it). The account is removed from
 * /etc/passwd, /etc/shadow, and /etc/sudoers. Root cannot be removed. */
#include <plusos/plusos.h>
#include <plusos/syscall.h>

int main(int argc, char **argv)
{
    if (argc < 2) { printf("usage: userdel NAME\n"); return 1; }

    struct plusos_users_args me = { 0 };
    me.op = PLUSOS_USERS_WHOAMI;
    if ((int)plusos_syscall(SYS_users, &me) != 0) {
        printf("userdel: must be run as root (try: sudo userdel ...)\n");
        return 1;
    }

    struct plusos_users_args d = { 0 };
    d.op = PLUSOS_USERS_DELETE;
    for (int i = 0; i < 15 && argv[1][i]; i++) d.name[i] = argv[1][i];
    int rc = (int)plusos_syscall(SYS_users, &d);
    if (rc != 0) {
        printf("userdel: cannot remove '%s' (root, unknown user, or disk error)\n", argv[1]);
        return 1;
    }
    printf("userdel: account '%s' removed. /home/%s was left on disk.\n", argv[1], argv[1]);
    return 0;
}
