/* pathchk - check whether file names are valid or portable. */
#include <stdio.h>
#include <string.h>
#include <ctype.h>

int main(int argc, char **argv)
{
    int posix_mode = 0, extra = 0, i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        for (const char *p = argv[i] + 1; *p; p++) {
            if (*p == 'p') posix_mode = 1;
            else if (*p == 'P') extra = 1;
        }
    }
    if (i >= argc) { fprintf(stderr, "pathchk: missing operand\n"); return 1; }
    int rc = 0;
    for (; i < argc; i++) {
        const char *s = argv[i];
        if (extra && !*s) { fprintf(stderr, "pathchk: empty file name\n"); rc = 1; continue; }
        if (posix_mode && strlen(s) > 255) { fprintf(stderr, "pathchk: limit exceeded for '%s'\n", s); rc = 1; continue; }
        const char *seg = s;
        while (*seg) {
            while (*seg == '/') seg++;
            const char *end = seg;
            while (*end && *end != '/') {
                unsigned char c = (unsigned char)*end;
                if (posix_mode && !isalnum(c) && c != '.' && c != '_' && c != '-') {
                    fprintf(stderr, "pathchk: nonportable character '%c' in '%s'\n", c, s);
                    rc = 1; break;
                }
                end++;
            }
            if ((size_t)(end - seg) > (posix_mode ? 14u : 63u)) {
                fprintf(stderr, "pathchk: component too long in '%s'\n", s);
                rc = 1; break;
            }
            if (extra && *seg == '-') {
                fprintf(stderr, "pathchk: leading '-' in '%s'\n", s);
                rc = 1; break;
            }
            seg = end;
        }
    }
    return rc;
}
