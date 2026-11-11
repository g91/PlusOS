/* id - print uid, gid, home, and sudo status of the current user. */
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
        printf("uid=%d (unknown)\n", uid);
        return 1;
    }
    char env_home[64] = "";
    struct plusos_env_args e = { PLUSOS_ENV_GET, "HOME", env_home, sizeof env_home };
    plusos_syscall(SYS_getenv, &e);
    printf("uid=%d(%s) gid=%d home=%s%s\n", u.uid, u.name, u.gid,
           env_home[0] ? env_home : u.home,
           (u.flags & 0x01) ? " sudo=yes" : "");
    return 0;
}
