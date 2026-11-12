/* ptx - produce a permuted index of file contents. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static int read_line(FILE *f, char **buf, size_t *cap)
{
    if (!*buf) { *cap = 128; *buf = malloc(*cap); if (!*buf) return -1; }
    size_t len = 0; int c;
    while ((c = fgetc(f)) != EOF) {
        if (len + 2 >= *cap) { char *nb = realloc(*buf, (*cap) *= 2); if (!nb) return -1; *buf = nb; }
        (*buf)[len++] = (char)c; if (c == '\n') break;
    }
    if (len == 0) return -1;
    (*buf)[len] = '\0'; return (int)len;
}

int main(int argc, char **argv)
{
    int i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "-w") == 0 && i + 1 < argc) i++;
    }
    FILE *f = (i < argc && strcmp(argv[i], "-") != 0) ? fopen(argv[i], "rb") : stdin;
    if (!f) return 1;
    char *line = NULL; size_t cap = 0; int len;
    while ((len = read_line(f, &line, &cap)) >= 0) {
        if (len > 0 && line[len - 1] == '\n') line[--len] = '\0';
        for (char *p = line; *p; p++) {
            if ((p == line || isspace((unsigned char)p[-1])) && isalnum((unsigned char)*p)) {
                printf("%35.*s  %s\n", (int)(p - line), line, p);
            }
        }
    }
    free(line);
    if (f != stdin) fclose(f);
    return 0;
}
