/* chown - change file owner and group. */
#include <plusos/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

static int name_to_uid(const char *spec, uid_t *out)
{
    if (!spec || !*spec) return -1;
    int all_dig = 1;
    for (const char *p = spec; *p; p++) if (*p < '0' || *p > '9') { all_dig = 0; break; }
    if (all_dig) { *out = (uid_t)strtoul(spec, NULL, 10); return 0; }
    struct plusos_users_args u = { 0 };
    u.op = PLUSOS_USERS_FIND; strlcpy(u.name, spec, sizeof u.name);
    if (plusos_syscall(SYS_users, &u) != 0) return -1;
    *out = (uid_t)u.uid;
    return 0;
}

static int apply_chown(const char *path, uid_t uid, gid_t gid, int rec, int verbose)
{
    if (chown(path, uid, gid) != 0) {
        fprintf(stderr, "chown: changing ownership of '%s': %s\n", path, strerror(errno));
        return 1;
    }
    if (verbose) printf("ownership of '%s' changed\n", path);
    int rc = 0;
    if (rec) {
        struct stat st;
        if (lstat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
            DIR *d = opendir(path);
            if (d) {
                struct dirent *de;
                while ((de = readdir(d)) != NULL) {
                    if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
                    char *sub = malloc(192);
                    if (!sub) { rc = 1; break; }
                    size_t pl = strlen(path);
                    snprintf(sub, 192, "%s%s%s", path, (pl && path[pl - 1] != '/') ? "/" : "", de->d_name);
                    rc |= apply_chown(sub, uid, gid, 1, verbose);
                    free(sub);
                }
                closedir(d);
            }
        }
    }
    return rc;
}

int main(int argc, char **argv)
{
    int rec = 0, verbose = 0, i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        for (const char *p = argv[i] + 1; *p; p++) {
            if (*p == 'R') rec = 1; else if (*p == 'v' || *p == 'c') verbose = 1;
        }
    }
    if (argc - i < 2) { fprintf(stderr, "chown: missing operand\n"); return 1; }
    const char *spec = argv[i++];
    char us[32] = "", gs[32] = "";
    const char *colon = strchr(spec, ':');
    if (!colon) colon = strchr(spec, '.');
    if (!colon) strlcpy(us, spec, sizeof us);
    else {
        size_t ul = (size_t)(colon - spec);
        if (ul >= sizeof us) ul = sizeof us - 1;
        memcpy(us, spec, ul); us[ul] = '\0';
        strlcpy(gs, colon + 1, sizeof gs);
    }
    uid_t uid = (uid_t)-1; gid_t gid = (gid_t)-1;
    if (us[0] && name_to_uid(us, &uid) != 0) { fprintf(stderr, "chown: invalid user: '%s'\n", us); return 1; }
    if (gs[0] && name_to_uid(gs, &gid) != 0) { fprintf(stderr, "chown: invalid group: '%s'\n", gs); return 1; }
    int rc = 0;
    for (; i < argc; i++) rc |= apply_chown(argv[i], uid, gid, rec, verbose);
    return rc;
}
