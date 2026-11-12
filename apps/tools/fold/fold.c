/* fold - wrap each input line to fit in specified width. */
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
        (*buf)[len++] = (char)c;
        if (c == '\n') break;
    }
    if (len == 0) return -1;
    (*buf)[len] = '\0';
    return (int)len;
}

int main(int argc, char **argv)
{
    int width = 80, spaces = 0, i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        if (strcmp(argv[i], "-w") == 0 && i + 1 < argc) { width = atoi(argv[++i]); continue; }
        if (strcmp(argv[i], "-s") == 0) spaces = 1;
        else if (isdigit((unsigned char)argv[i][1])) width = atoi(argv[i] + 1);
    }
    if (width < 1) width = 80;
    int nf = argc - i, rc = 0; char *line = NULL; size_t cap = 0;
    for (int k = 0; k < (nf > 0 ? nf : 1); k++) {
        FILE *f = (nf > 0 && strcmp(argv[i + k], "-") != 0) ? fopen(argv[i + k], "rb") : stdin;
        if (!f) { rc = 1; continue; }
        int len;
        while ((len = read_line(f, &line, &cap)) >= 0) {
            if (len > 0 && line[len - 1] == '\n') line[--len] = '\0';
            int pos = 0;
            while (pos < len) {
                int take = len - pos;
                if (take > width) {
                    take = width;
                    if (spaces) {
                        int sp = take - 1;
                        while (sp > 0 && line[pos + sp] != ' ' && line[pos + sp] != '\t') sp--;
                        if (sp > 0) take = sp + 1;
                    }
                }
                printf("%.*s\n", take, line + pos); pos += take;
            }
            if (len == 0) putchar('\n');
        }
        if (f != stdin) fclose(f);
    }
    free(line);
    return rc;
}
