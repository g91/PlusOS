/* tac - concatenate and print files in reverse. */
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
    int i = 1, rc = 0;
    if (i < argc && strcmp(argv[i], "--") == 0) i++;
    int nf = argc - i;
    for (int k = 0; k < (nf > 0 ? nf : 1); k++) {
        FILE *f = (nf > 0 && strcmp(argv[i + k], "-") != 0) ? fopen(argv[i + k], "rb") : stdin;
        if (!f) { fprintf(stderr, "tac: %s: %s\n", argv[i + k], strerror(errno)); rc = 1; continue; }
        size_t cap = 32, nl = 0; char **lines = malloc(cap * sizeof *lines);
        char *buf = NULL; size_t bcap = 0;
        while (read_line(f, &buf, &bcap) >= 0) {
            if (nl >= cap) lines = realloc(lines, (cap *= 2) * sizeof *lines);
            lines[nl++] = strdup(buf);
        }
        free(buf);
        for (size_t j = nl; j > 0; j--) printf("%s", lines[j - 1]);
        for (size_t j = 0; j < nl; j++) free(lines[j]);
        free(lines);
        if (f != stdin) fclose(f);
    }
    return rc;
}
