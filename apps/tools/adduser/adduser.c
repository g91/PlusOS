/* adduser - interactively create a new user account. Root only.
 *
 * Usage: adduser NAME
 *   or:  adduser              (prompts for the name) */
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

static int streq(const char *a, const char *b)
{ while (*a && *a == *b) { a++; b++; } return *a == 0 && *b == 0; }

static int yes_no(const char *prompt, int def_yes)
{
    printf("%s [%s]: ", prompt, def_yes ? "Y/n" : "y/N");
    char a[8]; int n = read_line(a, sizeof a, 1);
    if (n == 0) return def_yes;
    char c = a[0];
    if (c >= 'A' && c <= 'Z') c = (char)(c + 32);
    return c == 'y' ? 1 : c == 'n' ? 0 : def_yes;
}

int main(int argc, char **argv)
{
    /* Check we're root. */
    struct plusos_users_args me = { 0 };
    me.op = PLUSOS_USERS_WHOAMI;
    if ((int)plusos_syscall(SYS_users, &me) != 0) {
        printf("adduser: must be run as root (try: sudo adduser ...)\n");
        return 1;
    }

    char name[16];
    if (argc >= 2) {
        for (int i = 0; i < 15 && argv[1][i]; i++) name[i] = argv[1][i];
        name[15] = 0;
        if (argv[1][0] == 0 || argv[1][15]) name[15] = 0;
        int w = 0;
        for (int i = 0; name[i]; i++) if (name[i] > ' ') name[w++] = name[i];
        name[w] = 0;
    } else {
        printf("Username: ");
        read_line(name, sizeof name, 1);
    }
    if (!name[0]) { printf("adduser: empty username\n"); return 1; }
    if (streq(name, "root")) { printf("adduser: 'root' already exists\n"); return 1; }

    /* Check it's free. */
    struct plusos_users_args f = { 0 };
    f.op = PLUSOS_USERS_FIND;
    for (int i = 0; i < 15 && name[i]; i++) f.name[i] = name[i];
    if (plusos_syscall(SYS_users, &f) == 0) {
        printf("adduser: user '%s' already exists\n", name);
        return 1;
    }

    /* Password. */
    char pw1[32], pw2[32];
    int want_pw = yes_no("Set a password?", 1);
    if (want_pw) {
        for (;;) {
            printf("New password: "); read_line(pw1, sizeof pw1, 0);
            printf("Confirm:      "); read_line(pw2, sizeof pw2, 0);
            if (streq(pw1, pw2) && pw1[0]) break;
            printf("Passwords did not match (or empty); try again.\n");
        }
    } else {
        pw1[0] = 0;
    }

    int sudoer = yes_no("Grant sudo (root) permissions to this account?", 0);

    struct plusos_users_args a = { 0 };
    a.op = PLUSOS_USERS_ADD;
    a.flags = sudoer ? 0x01 : 0;
    a.is_root = 0;
    for (int i = 0; i < 15 && name[i]; i++) a.name[i] = name[i];
    for (int i = 0; i < 31 && pw1[i]; i++) a.pw[i]  = pw1[i];
    int rc = (int)plusos_syscall(SYS_users, &a);
    for (int i = 0; i < 32; i++) { pw1[i] = 0; pw2[i] = 0; }
    if (rc != 0) { printf("adduser: kernel refused (disk full? too many users?)\n"); return 1; }

    printf("adduser: account '%s' created%s.\n", name, sudoer ? " with sudo" : "");
    printf("  Home: /home/%s   (Desktop, Documents, Settings seeded)\n", name);
    return 0;
}
