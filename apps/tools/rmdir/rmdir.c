/* rmdir - remove empty directories. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    int parents = 0, verbose = 0, i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        for (const char *p = argv[i] + 1; *p; p++) {
            if (*p == 'p') parents = 1;
            else if (*p == 'v') verbose = 1;
        }
    }
    if (i >= argc) { fprintf(stderr, "rmdir: missing operand\n"); return 1; }
    int rc = 0;
    for (; i < argc; i++) {
        char *cur = strdup(argv[i]);
        if (!cur) return 1;
        if (rmdir(cur) != 0) {
            fprintf(stderr, "rmdir: failed to remove '%s': %s\n", cur, strerror(errno));
            rc = 1; free(cur); continue;
        }
        if (verbose) printf("rmdir: removing directory, '%s'\n", cur);
        while (parents) {
            size_t len = strlen(cur);
            while (len > 1 && cur[len - 1] == '/') cur[--len] = '\0';
            char *sl = strrchr(cur, '/');
            if (!sl || sl == cur) break;
            *sl = '\0';
            if (rmdir(cur) != 0) break;
            if (verbose) printf("rmdir: removing directory, '%s'\n", cur);
        }
        free(cur);
    }
    return rc;
}
