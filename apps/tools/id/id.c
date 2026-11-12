/* id - print real and effective user and group IDs. */
#include <plusos/syscall.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int find_user(const char *opt_name, struct plusos_users_args *out)
{
    memset(out, 0, sizeof *out);
    if (opt_name && *opt_name) {
        out->op = PLUSOS_USERS_FIND; strlcpy(out->name, opt_name, sizeof out->name);
        return (int)plusos_syscall(SYS_users, out);
    }
    out->op = PLUSOS_USERS_WHOAMI;
    int uid = (int)plusos_syscall(SYS_users, out);
    memset(out, 0, sizeof *out);
    out->op = PLUSOS_USERS_FIND_UID; out->uid = uid;
    return (int)plusos_syscall(SYS_users, out);
}

int main(int argc, char **argv)
{
    int uflag = 0, gflag = 0, Gflag = 0, nflag = 0, i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        for (const char *p = argv[i] + 1; *p; p++) {
            if (*p == 'u') uflag = 1; else if (*p == 'g') gflag = 1;
            else if (*p == 'G') Gflag = 1; else if (*p == 'n') nflag = 1;
        }
    }
    struct plusos_users_args u;
    if (find_user(i < argc ? argv[i] : NULL, &u) != 0) {
        if (i < argc) { fprintf(stderr, "id: '%s': no such user\n", argv[i]); return 1; }
        printf("uid=%u gid=%u\n", (unsigned)getuid(), (unsigned)getgid());
        return 0;
    }
    if (uflag) { if (nflag) puts(u.name); else printf("%d\n", u.uid); return 0; }
    if (gflag || Gflag) { if (nflag) puts(u.name); else printf("%d\n", u.gid); return 0; }
    printf("uid=%d(%s) gid=%d(%s) groups=%d(%s)%s\n",
           u.uid, u.name, u.gid, u.name, u.gid, u.name, (u.flags & 0x01) ? ",0(sudo)" : "");
    return 0;
}
