/* mv - move (rename) files. */
#include <plusos/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

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

int main(int argc, char **argv)
{
    int force = 0, noclobber = 0, update = 0, verbose = 0, i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        for (const char *p = argv[i] + 1; *p; p++) {
            if (*p == 'f') force = 1; else if (*p == 'n') noclobber = 1;
            else if (*p == 'u') update = 1; else if (*p == 'v') verbose = 1;
        }
    }
    int narg = argc - i;
    if (narg < 2) { fprintf(stderr, "mv: missing file operand\n"); return 1; }
    const char *dst = argv[argc - 1];
    struct stat dst_st;
    int dst_is_dir = (stat(dst, &dst_st) == 0 && S_ISDIR(dst_st.st_mode));
    if (narg > 2 && !dst_is_dir) { fprintf(stderr, "mv: target '%s' is not a directory\n", dst); return 1; }
    int rc = 0;
    for (int k = i; k < argc - 1; k++) {
        char *target = malloc(192);
        if (!target) return 1;
        if (dst_is_dir) join_p(target, 192, dst, base_name(argv[k]));
        else strlcpy(target, dst, 192);
        struct stat tst, sst;
        if (stat(target, &tst) == 0) {
            if (noclobber) { free(target); continue; }
            if (update && stat(argv[k], &sst) == 0 && tst.st_mtime >= sst.st_mtime) { free(target); continue; }
            if (force && !S_ISDIR(tst.st_mode)) unlink(target);
        }
        if (rename(argv[k], target) != 0) {
            char *cp_av[] = { "cp", "-a", argv[k], target, NULL };
            int cpid = spawnv("cp", cp_av);
            int crc = (cpid >= 0) ? (int)plusos_syscall(SYS_wait, (void *)(intptr_t)cpid) : -1;
            if (crc == 0) {
                char *rm_av[] = { "rm", "-rf", argv[k], NULL };
                int rpid = spawnv("rm", rm_av);
                if (rpid >= 0) plusos_syscall(SYS_wait, (void *)(intptr_t)rpid);
                if (verbose) printf("renamed '%s' -> '%s'\n", argv[k], target);
            } else {
                fprintf(stderr, "mv: cannot move '%s' to '%s': %s\n", argv[k], target, strerror(errno));
                rc = 1;
            }
        } else if (verbose) {
            printf("renamed '%s' -> '%s'\n", argv[k], target);
        }
        free(target);
    }
    return rc;
}
