/* rm - remove files or directories. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

static void join_p(char *out, size_t cap, const char *dir, const char *name)
{
    size_t d = strlen(dir);
    snprintf(out, cap, "%s%s%s", dir, (d && dir[d - 1] != '/' && dir[d - 1] != ':') ? "/" : "", name);
}

static int remove_tree(const char *path, int rec, int force, int dir_empty, int verbose)
{
    struct stat st;
    if (lstat(path, &st) != 0) {
        if (force && errno == ENOENT) return 0;
        fprintf(stderr, "rm: cannot remove '%s': %s\n", path, strerror(errno));
        return 1;
    }
    if (S_ISDIR(st.st_mode) && !S_ISLNK(st.st_mode)) {
        if (rec) {
            DIR *d = opendir(path);
            if (!d) {
                if (!force) fprintf(stderr, "rm: cannot open '%s': %s\n", path, strerror(errno));
                return force ? 0 : 1;
            }
            /* Collect child names before deleting so HFS index shifts do not skip entries. */
            size_t cap = 16, n = 0;
            char **kids = malloc(cap * sizeof *kids);
            struct dirent *de;
            while (kids && (de = readdir(d)) != NULL) {
                if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
                if (n >= cap) {
                    cap *= 2;
                    char **nk = realloc(kids, cap * sizeof *kids);
                    if (!nk) break;
                    kids = nk;
                }
                kids[n++] = strdup(de->d_name);
            }
            closedir(d);
            int rc = 0;
            for (size_t i = 0; i < n; i++) {
                if (!kids[i]) continue;
                char *sub = malloc(192);
                if (sub) {
                    join_p(sub, 192, path, kids[i]);
                    rc |= remove_tree(sub, 1, force, 0, verbose);
                    free(sub);
                } else rc = 1;
                free(kids[i]);
            }
            free(kids);
            if (rmdir(path) != 0) {
                if (!force) fprintf(stderr, "rm: cannot remove '%s': %s\n", path, strerror(errno));
                return 1;
            }
            if (verbose) printf("removed directory '%s'\n", path);
            return rc;
        }
        if (dir_empty) {
            if (rmdir(path) != 0) { fprintf(stderr, "rm: cannot remove '%s': %s\n", path, strerror(errno)); return 1; }
            if (verbose) printf("removed directory '%s'\n", path);
            return 0;
        }
        fprintf(stderr, "rm: cannot remove '%s': Is a directory\n", path);
        return 1;
    }
    if (unlink(path) != 0) {
        if (force && errno == ENOENT) return 0;
        fprintf(stderr, "rm: cannot remove '%s': %s\n", path, strerror(errno));
        return 1;
    }
    if (verbose) printf("removed '%s'\n", path);
    return 0;
}

int main(int argc, char **argv)
{
    int rec = 0, force = 0, dir_empty = 0, verbose = 0, i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        for (const char *p = argv[i] + 1; *p; p++) {
            if (*p == 'r' || *p == 'R') rec = 1;
            else if (*p == 'f') force = 1;
            else if (*p == 'd') dir_empty = 1;
            else if (*p == 'v') verbose = 1;
        }
    }
    if (i >= argc) {
        if (force) return 0;
        fprintf(stderr, "rm: missing operand\n");
        return 1;
    }
    int rc = 0;
    for (; i < argc; i++) {
        if (strcmp(argv[i], "/") == 0) {
            fprintf(stderr, "rm: it is dangerous to operate recursively on '/'\n");
            rc = 1; continue;
        }
        rc |= remove_tree(argv[i], rec, force, dir_empty, verbose);
    }
    return rc;
}
