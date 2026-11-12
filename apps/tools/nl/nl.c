/* nl - number lines of files. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static int read_line(FILE *f, char **buf, size_t *cap)
{
    if (!*buf) { *cap = 128; *buf = malloc(*cap); if (!*buf) return -1; }
    size_t len = 0; int c;
    while ((c = fgetc(f)) != EOF) {
        if (len + 2 >= *cap) { char *nb = realloc(*buf, (*cap) *= 2); if (!nb) return -1; *buf = nb; }
        (*buf)[len++] = (char)c;
        if (c == '\n') break;
    }
    if (len == 0) return -1;
    (*buf)[len] = '\0';
    return (int)len;
}

int main(int argc, char **argv)
{
    char body = 't', fmt = 'r';
    long start = 1, incr = 1;
    int width = 6, i = 1;
    const char *sep = "\t";
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) { body = argv[++i][0]; continue; }
        if (strcmp(argv[i], "-v") == 0 && i + 1 < argc) { start = atol(argv[++i]); continue; }
        if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) { incr = atol(argv[++i]); continue; }
        if (strcmp(argv[i], "-w") == 0 && i + 1 < argc) { width = atoi(argv[++i]); continue; }
        if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) { sep = argv[++i]; continue; }
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            const char *nf = argv[++i];
            fmt = !strcmp(nf, "ln") ? 'l' : !strcmp(nf, "rz") ? 'z' : 'r';
        }
    }
    int nf = argc - i, rc = 0; long lno = start;
    char *line = NULL; size_t lcap = 0;
    for (int k = 0; k < (nf > 0 ? nf : 1); k++) {
        FILE *f = (nf > 0 && strcmp(argv[i + k], "-") != 0) ? fopen(argv[i + k], "rb") : stdin;
        if (!f) { fprintf(stderr, "nl: %s: %s\n", argv[i + k], strerror(errno)); rc = 1; continue; }
        int len;
        while ((len = read_line(f, &line, &lcap)) >= 0) {
            int empty = (len == 0 || (len == 1 && line[0] == '\n'));
            if (body == 'a' || (body == 't' && !empty)) {
                if (fmt == 'l') printf("%-*ld%s", width, lno, sep);
                else if (fmt == 'z') printf("%0*ld%s", width, lno, sep);
                else printf("%*ld%s", width, lno, sep);
                lno += incr;
            } else printf("%*s ", width, "");
            fputs(line, stdout);
        }
        if (f != stdin) fclose(f);
    }
    free(line);
    return rc;
}
