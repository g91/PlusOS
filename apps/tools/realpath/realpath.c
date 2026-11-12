/* realpath - print the resolved path. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

int main(int argc, char **argv)
{
    char term = '\n';
    int quiet = 0, no_symlinks = 0, must_exist = 0, i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        for (const char *p = argv[i] + 1; *p; p++) {
            if (*p == 'z') term = '\0';
            else if (*p == 'q') quiet = 1;
            else if (*p == 's' || *p == 'm') no_symlinks = 1;
            else if (*p == 'e') must_exist = 1;
        }
    }
    if (i >= argc) { fprintf(stderr, "realpath: missing operand\n"); return 1; }
    int rc = 0;
    for (; i < argc; i++) {
        char out[192];
        if (!realpath(argv[i], out)) {
            if (!quiet) fprintf(stderr, "realpath: %s: %s\n", argv[i], strerror(errno));
            rc = 1; continue;
        }
        if (must_exist) {
            struct stat st;
            if ((no_symlinks ? lstat(out, &st) : stat(out, &st)) != 0) {
                if (!quiet) fprintf(stderr, "realpath: %s: %s\n", argv[i], strerror(errno));
                rc = 1; continue;
            }
        }
        printf("%s%c", out, term);
    }
    return rc;
}
