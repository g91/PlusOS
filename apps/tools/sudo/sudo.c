/* sudo - run a command as root.
 *
 * Usage: sudo COMMAND [ARGS...]
 *
 * The caller must be listed in /etc/sudoers. We prompt for the caller's own
 * password (not root's), authenticate it, then ask the kernel to flip
 * proc_current->uid to 0 for this process only. The child inherits uid 0 at
 * spawn, runs, and exits; this sudo process exits too and its (elevated)
 * state goes with it, so the shell that invoked sudo is unaffected. */
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
    if (argc < 2) { printf("usage: sudo COMMAND [ARGS...]\n"); return 1; }

    /* Who am I? */
    struct plusos_users_args me = { 0 };
    me.op = PLUSOS_USERS_WHOAMI;
    int uid = (int)plusos_syscall(SYS_users, &me);
    me.op = PLUSOS_USERS_FIND_UID; me.uid = uid;
    if (plusos_syscall(SYS_users, &me) != 0) { printf("sudo: not logged in\n"); return 1; }

    /* Already root? Just spawn without prompting. */
    if (uid == 0) {
        int pid = plusos->run(argv[1], argc - 1, &argv[1]);
        if (pid == -1001) { printf("sudo: %s: not found\n", argv[1]); return 1; }
        if (pid < 0)      { printf("sudo: %s: cannot run (%d)\n", argv[1], pid); return 1; }
        return (int)plusos_syscall(SYS_wait, (void *)(intptr_t)pid);
    }

    /* Sudoer? */
    struct plusos_users_args s = { 0 };
    s.op = PLUSOS_USERS_IS_SUDOER;
    for (int i = 0; i < 15 && me.name[i]; i++) s.name[i] = me.name[i];
    if (plusos_syscall(SYS_users, &s) != 1) {
        printf("sudo: %s is not in the sudoers file.\n", me.name);
        return 1;
    }

    /* Verify the caller's own password. One retry. */
    int ok = 0;
    for (int attempt = 0; attempt < 3 && !ok; attempt++) {
        char pw[32];
        printf("[sudo] password for %s: ", me.name);
        read_line(pw, sizeof pw, 0);
        struct plusos_users_args a = { 0 };
        a.op = PLUSOS_USERS_AUTH;
        for (int i = 0; i < 15 && me.name[i]; i++) a.name[i] = me.name[i];
        for (int i = 0; i < 31 && pw[i]; i++) a.pw[i] = pw[i];
        ok = (int)plusos_syscall(SYS_users, &a);
        for (int i = 0; i < 32; i++) pw[i] = 0;
        if (!ok) printf("Sorry, try again.\n");
    }
    if (!ok) { printf("sudo: 3 incorrect password attempts\n"); return 1; }

    /* Elevate. */
    struct plusos_users_args e = { 0 };
    e.op = PLUSOS_USERS_SUDO_ELEVATE;
    if (plusos_syscall(SYS_users, &e) != 0) {
        printf("sudo: kernel refused to elevate\n");
        return 1;
    }

    /* Spawn the command; it inherits uid 0. */
    int pid = plusos->run(argv[1], argc - 1, &argv[1]);
    if (pid == -1001) { printf("sudo: %s: not found\n", argv[1]); return 1; }
    if (pid < 0)      { printf("sudo: %s: cannot run (%d)\n", argv[1], pid); return 1; }
    return (int)plusos_syscall(SYS_wait, (void *)(intptr_t)pid);
}
