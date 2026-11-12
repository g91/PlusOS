/* cp - copy files and directories. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <utime.h>

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

static int copy_file(const char *src, const char *dst, int noclobber, int update, int verbose)
{
    struct stat sst, dst_st;
    if (lstat(src, &sst) != 0) { fprintf(stderr, "cp: cannot stat '%s': %s\n", src, strerror(errno)); return -1; }
    if (S_ISLNK(sst.st_mode)) {
        char lt[160];
        ssize_t n = readlink(src, lt, sizeof lt - 1);
        if (n >= 0) {
            lt[n] = '\0'; unlink(dst);
            if (symlink(lt, dst) == 0) { if (verbose) printf("'%s' -> '%s'\n", src, dst); return 0; }
        }
    }
    if (stat(dst, &dst_st) == 0) {
        if (noclobber) return 0;
        if (update && dst_st.st_mtime >= sst.st_mtime) return 0;
    }
    int in = open(src, O_RDONLY);
    if (in < 0) { fprintf(stderr, "cp: cannot open '%s': %s\n", src, strerror(errno)); return -1; }
    int out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, sst.st_mode & 0777);
    if (out < 0) { fprintf(stderr, "cp: cannot create '%s': %s\n", dst, strerror(errno)); close(in); return -1; }
    ssize_t n; int err = 0;
    while ((n = read(in, g_buf, sizeof g_buf)) > 0) {
        if (write(out, g_buf, (size_t)n) != n) { err = -1; break; }
    }
    if (n < 0) err = -1;
    close(in); close(out);
    if (err == 0 && verbose) printf("'%s' -> '%s'\n", src, dst);
    return err;
}

static int copy_tree(const char *src, const char *dst, int rec, int force,
                     int noclobber, int preserve, int update, int verbose)
{
    struct stat st;
    if (lstat(src, &st) != 0) { fprintf(stderr, "cp: cannot stat '%s': %s\n", src, strerror(errno)); return 1; }
    if (S_ISDIR(st.st_mode)) {
        if (!rec) { fprintf(stderr, "cp: -r not specified; omitting directory '%s'\n", src); return 1; }
        struct stat dst_st;
        if (stat(dst, &dst_st) != 0) {
            if (mkdir(dst, preserve ? (st.st_mode & 0777) : 0755) != 0) {
                fprintf(stderr, "cp: cannot create directory '%s': %s\n", dst, strerror(errno));
                return 1;
            }
            if (verbose) printf("'%s' -> '%s'\n", src, dst);
        }
        DIR *d = opendir(src);
        if (!d) return 1;
        int rc = 0; struct dirent *de;
        while ((de = readdir(d)) != NULL) {
            if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) continue;
            char *ns = malloc(192), *nd = malloc(192);
            if (!ns || !nd) { free(ns); free(nd); rc = 1; break; }
            join_p(ns, 192, src, de->d_name); join_p(nd, 192, dst, de->d_name);
            rc |= copy_tree(ns, nd, 1, force, noclobber, preserve, update, verbose);
            free(ns); free(nd);
        }
        closedir(d);
        return rc;
    }
    if (force) {
        struct stat dst_st;
        if (lstat(dst, &dst_st) == 0 && !S_ISDIR(dst_st.st_mode)) unlink(dst);
    }
    int rc = copy_file(src, dst, noclobber, update, verbose);
    if (rc == 0 && preserve) {
        struct utimbuf ut = { st.st_atime, st.st_mtime };
        utime(dst, &ut); chmod(dst, st.st_mode & 0777);
    }
    return rc == 0 ? 0 : 1;
}

int main(int argc, char **argv)
{
    int rec = 0, force = 0, noclobber = 0, preserve = 0, update = 0, verbose = 0, i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        for (const char *p = argv[i] + 1; *p; p++) {
            if (*p == 'r' || *p == 'R') rec = 1;
            else if (*p == 'a') { rec = 1; preserve = 1; }
            else if (*p == 'f') force = 1;
            else if (*p == 'n') noclobber = 1;
            else if (*p == 'p') preserve = 1;
            else if (*p == 'u') update = 1;
            else if (*p == 'v') verbose = 1;
        }
    }
    int narg = argc - i;
    if (narg < 2) { fprintf(stderr, "cp: missing file operand\n"); return 1; }
    const char *dst = argv[argc - 1];
    struct stat dst_st;
    int dst_is_dir = (stat(dst, &dst_st) == 0 && S_ISDIR(dst_st.st_mode));
    if (narg > 2 && !dst_is_dir) { fprintf(stderr, "cp: target '%s' is not a directory\n", dst); return 1; }
    int rc = 0;
    for (int k = i; k < argc - 1; k++) {
        char *target = malloc(192);
        if (!target) return 1;
        if (dst_is_dir) join_p(target, 192, dst, base_name(argv[k]));
        else strlcpy(target, dst, 192);
        rc |= copy_tree(argv[k], target, rec, force, noclobber, preserve, update, verbose);
        free(target);
    }
    return rc;
}
