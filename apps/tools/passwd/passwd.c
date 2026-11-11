/* passwd - change a user's password.
 *
 * Non-root users may only change their own password, and must prove the old
 * one first. Root can change anyone's with no old-password prompt. */
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

static int streq(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return *a == 0 && *b == 0;
}

int main(int argc, char **argv)
{
    struct plusos_users_args u = { 0 };
    u.op = PLUSOS_USERS_WHOAMI;
    int my_uid = (int)plusos_syscall(SYS_users, &u);
    u.op = PLUSOS_USERS_FIND_UID; u.uid = my_uid;
    if (plusos_syscall(SYS_users, &u) != 0) { printf("passwd: not logged in\n"); return 1; }
    char me[16]; int i;
    for (i = 0; i < 15 && u.name[i]; i++) me[i] = u.name[i]; me[i] = 0;

    const char *target = (argc >= 2) ? argv[1] : me;

    /* Non-root targeting someone else: refused */
    if (!streq(target, me) && my_uid != 0) {
        printf("passwd: only root can change another user's password.\n");
        return 1;
    }

    /* Non-root changing own password: prove the old one first */
    if (my_uid != 0) {
        char old_pw[32];
        printf("(current) password: ");
        read_line(old_pw, sizeof old_pw, 0);
        struct plusos_users_args a = { 0 };
        a.op = PLUSOS_USERS_AUTH;
        for (int j = 0; j < 15 && me[j]; j++) a.name[j] = me[j];
        for (int j = 0; j < 31 && old_pw[j]; j++) a.pw[j] = old_pw[j];
        int ok = (int)plusos_syscall(SYS_users, &a);
        for (int j = 0; j < 32; j++) old_pw[j] = 0;
        if (!ok) { printf("passwd: authentication failed\n"); return 1; }
    }

    char pw1[32], pw2[32];
    printf("New password: ");
    read_line(pw1, sizeof pw1, 0);
    printf("Retype new password: ");
    read_line(pw2, sizeof pw2, 0);
    if (!streq(pw1, pw2)) { printf("passwd: passwords do not match\n"); return 1; }

    struct plusos_users_args s = { 0 };
    s.op = PLUSOS_USERS_SETPW;
    for (int j = 0; j < 15 && target[j]; j++) s.name[j] = target[j];
    for (int j = 0; j < 31 && pw1[j]; j++) s.pw[j] = pw1[j];
    int rc = (int)plusos_syscall(SYS_users, &s);
    for (int j = 0; j < 32; j++) { pw1[j] = 0; pw2[j] = 0; }
    if (rc != 0) { printf("passwd: update failed\n"); return 1; }
    printf("passwd: password updated for '%s'\n", target);
    return 0;
}
