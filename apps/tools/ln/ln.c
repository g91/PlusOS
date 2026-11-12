/* ln - make links between files. */
#include <stdio.h>
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
    int sym = 0, force = 0, verbose = 0, i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        for (const char *p = argv[i] + 1; *p; p++) {
            if (*p == 's') sym = 1; else if (*p == 'f') force = 1; else if (*p == 'v') verbose = 1;
        }
    }
    int narg = argc - i;
    if (narg < 1) { fprintf(stderr, "ln: missing file operand\n"); return 1; }
    const char *target = argv[i];
    char dstbuf[192];
    const char *linkname = NULL;
    if (narg == 1) linkname = base_name(target);
    else if (narg == 2) {
        struct stat st;
        if (stat(argv[i + 1], &st) == 0 && S_ISDIR(st.st_mode)) {
            join_p(dstbuf, sizeof dstbuf, argv[i + 1], base_name(target));
            linkname = dstbuf;
        } else linkname = argv[i + 1];
    } else {
        const char *dir = argv[argc - 1];
        int rc = 0;
        for (int k = i; k < argc - 1; k++) {
            join_p(dstbuf, sizeof dstbuf, dir, base_name(argv[k]));
            if (force) unlink(dstbuf);
            if ((sym ? symlink(argv[k], dstbuf) : link(argv[k], dstbuf)) != 0) {
                fprintf(stderr, "ln: '%s': %s\n", dstbuf, strerror(errno)); rc = 1;
            } else if (verbose) printf("'%s' -> '%s'\n", dstbuf, argv[k]);
        }
        return rc;
    }
    if (force) unlink(linkname);
    if ((sym ? symlink(target, linkname) : link(target, linkname)) != 0) {
        fprintf(stderr, "ln: failed to create %s link '%s': %s\n",
                sym ? "symbolic" : "hard", linkname, strerror(errno));
        return 1;
    }
    if (verbose) printf("'%s' -> '%s'\n", linkname, target);
    return 0;
}
