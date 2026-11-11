/* chown - change the owner (and optional group) of a file.
 *
 * Usage: chown USER PATH         (group stays what it was)
 *        chown USER:GROUP PATH
 *        chown UID PATH          (numeric uid)
 *        chown UID:GID PATH
 *
 * Root only. */
#include <plusos/plusos.h>
#include <plusos/syscall.h>

static int is_num(const char *s) {
    if (!*s) return 0;
    for (const char *p = s; *p; p++) if (*p < '0' || *p > '9') return 0;
    return 1;
}
static int atoi_safe(const char *s) { int v = 0; while (*s >= '0' && *s <= '9') { v = v*10 + (*s-'0'); s++; } return v; }

/* Resolve a user spec ("name" or "123") to a uid, or -1. */
static int resolve_uid(const char *spec)
{
    if (is_num(spec)) return atoi_safe(spec);
    struct plusos_users_args u = { 0 };
    u.op = PLUSOS_USERS_FIND;
    for (int i = 0; i < 15 && spec[i]; i++) u.name[i] = spec[i];
    if (plusos_syscall(SYS_users, &u) != 0) return -1;
    return u.uid;
}

int main(int argc, char **argv)
{
    if (argc < 3) { printf("usage: chown USER[:GROUP] PATH\n"); return 1; }
    char user_part[24] = "", group_part[24] = "";
    int split = -1;
    for (int i = 0; argv[1][i]; i++) if (argv[1][i] == ':') { split = i; break; }
    if (split < 0) {
        for (int i = 0; argv[1][i] && i < 23; i++) user_part[i] = argv[1][i];
    } else {
        for (int i = 0; i < split && i < 23; i++) user_part[i] = argv[1][i];
        for (int i = split + 1, j = 0; argv[1][i] && j < 23; i++, j++) group_part[j] = argv[1][i];
    }

    int uid = resolve_uid(user_part);
    if (uid < 0) { printf("chown: unknown user '%s'\n", user_part); return 1; }

    /* If no group given, keep whatever the file currently has. */
    int gid = -1;
    if (group_part[0]) gid = is_num(group_part) ? atoi_safe(group_part) : resolve_uid(group_part);
    if (gid < 0) {
        struct plusos_perms_args g = { 0 };
        g.op = PLUSOS_PERMS_GET;
        for (int i = 0; i < 47 && argv[2][i]; i++) g.path[i] = argv[2][i];
        if (plusos_syscall(SYS_perms, &g) == 0) gid = g.gid;
        else                                     gid = uid;   /* new entry: gid = uid */
    }

    struct plusos_perms_args p = { 0 };
    p.op  = PLUSOS_PERMS_CHOWN;
    p.uid = uid; p.gid = gid;
    for (int i = 0; i < 47 && argv[2][i]; i++) p.path[i] = argv[2][i];
    if (plusos_syscall(SYS_perms, &p) != 0) {
        printf("chown: %s: permission denied (root only) or write failed\n", argv[2]);
        return 1;
    }
    printf("chown: %d:%d %s\n", uid, gid, argv[2]);
    return 0;
}
