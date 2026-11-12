/* paste - merge lines of files. */
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
    const char *delims = "\t";
    int serial = 0, i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        if (strcmp(argv[i], "-s") == 0) { serial = 1; continue; }
        if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) { delims = argv[++i]; continue; }
    }
    size_t dlen = strlen(delims);
    if (dlen == 0) { delims = ""; dlen = 1; }
    int nf = argc - i;
    char *line = NULL; size_t cap = 0;
    if (nf <= 0) {
        int len, first = 1; size_t di = 0;
        while ((len = read_line(stdin, &line, &cap)) >= 0) {
            if (len > 0 && line[len - 1] == '\n') line[--len] = '\0';
            if (serial) { if (!first) { char dc = delims[di++ % dlen]; if (dc) putchar(dc); } fputs(line, stdout); first = 0; }
            else puts(line);
        }
        if (serial && !first) putchar('\n');
        free(line);
        return 0;
    }
    FILE *fps[16]; if (nf > 16) nf = 16;
    for (int k = 0; k < nf; k++) {
        fps[k] = strcmp(argv[i + k], "-") ? fopen(argv[i + k], "rb") : stdin;
        if (!fps[k]) { fprintf(stderr, "paste: %s: %s\n", argv[i + k], strerror(errno)); return 1; }
    }
    if (serial) {
        for (int k = 0; k < nf; k++) {
            int first = 1, len; size_t di = 0;
            while ((len = read_line(fps[k], &line, &cap)) >= 0) {
                if (len > 0 && line[len - 1] == '\n') line[--len] = '\0';
                if (!first) { char dc = delims[di++ % dlen]; if (dc) putchar(dc); }
                fputs(line, stdout); first = 0;
            }
            putchar('\n');
        }
    } else {
        for (;;) {
            int any = 0; size_t di = 0; char *row[16];
            for (int k = 0; k < nf; k++) {
                int len = read_line(fps[k], &line, &cap);
                if (len >= 0) { if (len > 0 && line[len - 1] == '\n') line[--len] = '\0'; row[k] = strdup(line); any = 1; }
                else row[k] = NULL;
            }
            if (!any) break;
            for (int k = 0; k < nf; k++) {
                if (k > 0) { char dc = delims[di++ % dlen]; if (dc) putchar(dc); }
                if (row[k]) { fputs(row[k], stdout); free(row[k]); }
            }
            putchar('\n');
        }
    }
    free(line);
    for (int k = 0; k < nf; k++) if (fps[k] != stdin) fclose(fps[k]);
    return 0;
}
