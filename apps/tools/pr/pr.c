/* pr - convert text files for printing. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    int page_len = 66, omit_hdr = 0, num_lines = 0, i = 1;
    const char *header = NULL;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        if (strcmp(argv[i], "-l") == 0 && i + 1 < argc) { page_len = atoi(argv[++i]); continue; }
        if (strcmp(argv[i], "-h") == 0 && i + 1 < argc) { header = argv[++i]; continue; }
        if (strcmp(argv[i], "-t") == 0) omit_hdr = 1;
        else if (strcmp(argv[i], "-n") == 0) num_lines = 1;
    }
    if (page_len < 12) page_len = 66;
    int nf = argc - i, rc = 0; char *line = NULL; size_t cap = 0;
    for (int k = 0; k < (nf > 0 ? nf : 1); k++) {
        const char *name = nf > 0 ? argv[i + k] : "";
        FILE *f = (nf > 0 && strcmp(name, "-") != 0) ? fopen(name, "rb") : stdin;
        if (!f) { rc = 1; continue; }
        int lno = 1, pg = 1, body = omit_hdr ? page_len : page_len - 10, on_page = 0;
        while (read_line(f, &line, &cap) >= 0) {
            if (on_page == 0 && !omit_hdr) printf("\n\n%s  Page %d\n\n\n", header ? header : name, pg++);
            if (num_lines) printf("%5d\t%s", lno++, line); else fputs(line, stdout);
            on_page++;
            if (on_page >= body && !omit_hdr) { printf("\n\n\n\n\n"); on_page = 0; }
        }
        if (f != stdin) fclose(f);
    }
    free(line); return rc;
}
