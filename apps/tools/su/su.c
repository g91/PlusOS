/* su - switch user.
 *
 * Root switches without a password. Everyone else is prompted for the target
 * user's password. On success the running process's uid/gid change and HOME
 * / USER / SHELL env vars are reset; any program the shell then launches
 * inherits the new identity. */
#include <plusos/plusos.h>
#include <plusos/syscall.h>

static int read_line(char *buf, int cap, int echo)
{
    int n = 0;
    for (;;) {
        int k = plusos->key_get();
        if (k == 0x0D || k == 0x0A) { printf("\n"); buf[n] = 0; return n; }
        if (k == 0x08 || k == 0x7F) { if (n > 0) { n--; printf("\b \b"); } continue; }
        if (k < 0x20 || k > 0x7E) continue;
        if (n >= cap - 1) continue;
        buf[n++] = (char)k;
        printf("%c", echo ? k : '*');
    }
}

int main(int argc, char **argv)
{
    const char *target = (argc >= 2) ? argv[1] : "root";

    struct plusos_users_args t = { 0 };
    t.op = PLUSOS_USERS_FIND;
    for (int j = 0; j < 15 && target[j]; j++) t.name[j] = target[j];
    if (plusos_syscall(SYS_users, &t) != 0) {
        printf("su: no such user '%s'\n", target);
        return 1;
    }

    struct plusos_users_args me = { 0 };
    me.op = PLUSOS_USERS_WHOAMI;
    int my_uid = (int)plusos_syscall(SYS_users, &me);

    if (my_uid != 0) {
        char pw[32];
        printf("Password: ");
        read_line(pw, sizeof pw, 0);
        struct plusos_users_args a = { 0 };
        a.op = PLUSOS_USERS_AUTH;
        for (int j = 0; j < 15 && target[j]; j++) a.name[j] = target[j];
        for (int j = 0; j < 31 && pw[j]; j++) a.pw[j] = pw[j];
        int ok = (int)plusos_syscall(SYS_users, &a);
        for (int j = 0; j < 32; j++) pw[j] = 0;
        if (!ok) { printf("su: authentication failed\n"); return 1; }
    }

    /* Hand the switch to the kernel - it updates both current_uid and
     * proc_current->uid so children we spawn inherit the new identity. */
    struct plusos_users_args s = { 0 };
    s.op = PLUSOS_USERS_SETCURRENT;
    s.uid = t.uid;
    if (plusos_syscall(SYS_users, &s) != 0) {
        printf("su: kernel refused the switch\n");
        return 1;
    }

    /* Freshen env vars for the running shell. */
    struct plusos_env_args e;
    e.op = PLUSOS_ENV_SET;
    e.key = "HOME";  e.value = t.home;  plusos_syscall(SYS_setenv, &e);
    e.key = "USER";  e.value = t.name;  plusos_syscall(SYS_setenv, &e);
    e.key = "SHELL"; e.value = t.shell; plusos_syscall(SYS_setenv, &e);

    printf("Switched to %s (uid %d). HOME=%s\n", t.name, t.uid, t.home);
    return 0;
}
