/* install - copy files and set attributes. */
#include <plusos/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

static char g_buf[2048];

static const char *base_name(const char *path)
{
    const char *p = path;
    for (const char *s = path; *s; s++) if (*s == '/' || *s == ':') p = s + 1;
    return p;
}

static void join_p(char *out, size_t cap, const char *dir, const char *name)
{
    size_t d = strlen(dir);
    snprintf(out, cap, "%s%s%s", dir, (d && dir[d - 1] != '/' && dir[d - 1] != ':') ? "/" : "", name);
}

static int name_to_uid(const char *spec, uid_t *out)
{
    if (!spec || !*spec) return -1;
    int all_dig = 1;
    for (const char *p = spec; *p; p++) if (*p < '0' || *p > '9') { all_dig = 0; break; }
    if (all_dig) { *out = (uid_t)strtoul(spec, NULL, 10); return 0; }
    struct plusos_users_args u = { 0 };
    u.op = PLUSOS_USERS_FIND;
    strlcpy(u.name, spec, sizeof u.name);
    if (plusos_syscall(SYS_users, &u) != 0) return -1;
    *out = (uid_t)u.uid;
    return 0;
}

static int mkdir_p(const char *path, mode_t mode, int verbose)
{
    char *tmp = strdup(path);
    if (!tmp) return -1;
    size_t len = strlen(tmp);
    while (len > 1 && tmp[len - 1] == '/') tmp[--len] = '\0';
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            struct stat st;
            if (stat(tmp, &st) != 0) {
                if (mkdir(tmp, mode) != 0 && errno != EEXIST) { free(tmp); return -1; }
                if (verbose) printf("install: creating directory '%s'\n", tmp);
            }
            *p = '/';
        }
    }
    struct stat st;
    int rc = (stat(tmp, &st) == 0 && S_ISDIR(st.st_mode)) ? 0 : mkdir(tmp, mode);
    free(tmp);
    return rc;
}

static int copy_inst(const char *src, const char *dst, mode_t mode, int verbose)
{
    int in = open(src, O_RDONLY);
    if (in < 0) { fprintf(stderr, "install: '%s': %s\n", src, strerror(errno)); return -1; }
    int out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, mode);
    if (out < 0) { fprintf(stderr, "install: '%s': %s\n", dst, strerror(errno)); close(in); return -1; }
    ssize_t n; int err = 0;
    while ((n = read(in, g_buf, sizeof g_buf)) > 0) if (write(out, g_buf, (size_t)n) != n) { err = -1; break; }
    if (n < 0) err = -1;
    close(in); close(out);
    if (err == 0 && verbose) printf("'%s' -> '%s'\n", src, dst);
    return err;
}

int main(int argc, char **argv)
{
    int make_dirs = 0, mk_parent = 0, verbose = 0, i = 1;
    mode_t mode = 0755;
    const char *owner = NULL, *group = NULL, *ms = NULL;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) { ms = argv[++i]; continue; }
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) { owner = argv[++i]; continue; }
        if (strcmp(argv[i], "-g") == 0 && i + 1 < argc) { group = argv[++i]; continue; }
        for (const char *p = argv[i] + 1; *p; p++) {
            if (*p == 'd') make_dirs = 1; else if (*p == 'D') mk_parent = 1; else if (*p == 'v') verbose = 1;
        }
    }
    if (ms) mode = (mode_t)strtoul(ms, NULL, 8) & 07777;
    uid_t uid = (uid_t)-1; gid_t gid = (gid_t)-1;
    if (owner && name_to_uid(owner, &uid) != 0) { fprintf(stderr, "install: invalid user '%s'\n", owner); return 1; }
    if (group && name_to_uid(group, &gid) != 0) { fprintf(stderr, "install: invalid group '%s'\n", group); return 1; }
    if (make_dirs) {
        int rc = 0;
        for (; i < argc; i++) {
            if (mkdir_p(argv[i], mode, verbose) != 0) rc = 1;
            else { chmod(argv[i], mode); if (uid != (uid_t)-1 || gid != (gid_t)-1) chown(argv[i], uid, gid); }
        }
        return rc;
    }
    if (argc - i < 2) { fprintf(stderr, "install: missing file operand\n"); return 1; }
    const char *dst = argv[argc - 1];
    struct stat dst_st;
    int dst_is_dir = (stat(dst, &dst_st) == 0 && S_ISDIR(dst_st.st_mode));
    if (mk_parent && !dst_is_dir) {
        char *dup_d = strdup(dst);
        if (dup_d) { char *sl = strrchr(dup_d, '/'); if (sl && sl != dup_d) { *sl = '\0'; mkdir_p(dup_d, 0755, 0); } free(dup_d); }
    }
    int rc = 0;
    for (int k = i; k < argc - 1; k++) {
        char *target = malloc(192);
        if (!target) return 1;
        if (dst_is_dir) join_p(target, 192, dst, base_name(argv[k]));
        else strlcpy(target, dst, 192);
        if (copy_inst(argv[k], target, mode, verbose) != 0) rc = 1;
        else { chmod(target, mode); if (uid != (uid_t)-1 || gid != (gid_t)-1) chown(target, uid, gid); }
        free(target);
    }
    return rc;
}
