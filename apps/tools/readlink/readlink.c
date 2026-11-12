/* readlink - print resolved symbolic links or canonical file names. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    int canon = 0, no_nl = 0, quiet = 0, i = 1;
    char term = '\n';
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        for (const char *p = argv[i] + 1; *p; p++) {
            if (*p == 'f' || *p == 'e' || *p == 'm') canon = 1;
            else if (*p == 'n') no_nl = 1;
            else if (*p == 'q' || *p == 's') quiet = 1;
            else if (*p == 'z') term = '\0';
        }
    }
    if (i >= argc) { fprintf(stderr, "readlink: missing operand\n"); return 1; }
    int rc = 0;
    for (; i < argc; i++) {
        char out[192];
        if (canon) {
            if (!realpath(argv[i], out)) {
                if (!quiet) fprintf(stderr, "readlink: %s: %s\n", argv[i], strerror(errno));
                rc = 1; continue;
            }
        } else {
            ssize_t n = readlink(argv[i], out, sizeof out - 1);
            if (n < 0) { rc = 1; continue; }
            out[n] = '\0';
        }
        if (no_nl) printf("%s", out);
        else printf("%s%c", out, term);
    }
    return rc;
}
