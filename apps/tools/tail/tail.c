/* tail - output the last part of files. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
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
    long long count = 10;
    int by_bytes = 0, from_start = 0, quiet = 0, verbose = 0, i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        if (isdigit((unsigned char)argv[i][1])) { count = atol(argv[i] + 1); continue; }
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            const char *v = argv[++i]; if (v[0] == '+') { from_start = 1; v++; }
            count = strtoll(v, NULL, 10); by_bytes = 0; continue;
        }
        if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            const char *v = argv[++i]; if (v[0] == '+') { from_start = 1; v++; }
            count = strtoll(v, NULL, 10); by_bytes = 1; continue;
        }
        for (const char *p = argv[i] + 1; *p; p++) {
            if (*p == 'q') quiet = 1; else if (*p == 'v') verbose = 1;
            else if (*p == 'n' && p[1]) { const char *v = p + 1; if (*v == '+') { from_start = 1; v++; } count = strtoll(v, NULL, 10); by_bytes = 0; break; }
            else if (*p == 'c' && p[1]) { const char *v = p + 1; if (*v == '+') { from_start = 1; v++; } count = strtoll(v, NULL, 10); by_bytes = 1; break; }
        }
    }
    int nfiles = argc - i, rc = 0;
    for (int k = 0; k < (nfiles > 0 ? nfiles : 1); k++) {
        const char *name = nfiles > 0 ? argv[i + k] : "standard input";
        FILE *f = (nfiles > 0 && strcmp(name, "-") != 0) ? fopen(name, "rb") : stdin;
        if (!f) { fprintf(stderr, "tail: %s: %s\n", name, strerror(errno)); rc = 1; continue; }
        if (!quiet && (verbose || nfiles > 1)) { if (k > 0) putchar('\n'); printf("==> %s <==\n", name); }
        if (by_bytes) {
            if (from_start) {
                long long pos = 1; int c;
                while ((c = fgetc(f)) != EOF) { if (pos >= count) putchar(c); pos++; }
            } else {
                size_t cap = (size_t)(count > 0 ? count : 1);
                char *ring = malloc(cap);
                if (ring) {
                    size_t tot = 0; int c;
                    while ((c = fgetc(f)) != EOF) ring[tot++ % cap] = (char)c;
                    size_t start = tot > cap ? tot - cap : 0;
                    for (size_t j = start; j < tot; j++) putchar((unsigned char)ring[j % cap]);
                    free(ring);
                }
            }
        } else {
            size_t cap = 32, nl = 0; char **lines = malloc(cap * sizeof *lines);
            char *buf = NULL; size_t bcap = 0;
            while (read_line(f, &buf, &bcap) >= 0) {
                if (nl >= cap) lines = realloc(lines, (cap *= 2) * sizeof *lines);
                lines[nl++] = strdup(buf);
            }
            free(buf);
            size_t start = from_start ? (count > 1 ? (size_t)(count - 1) : 0)
                                      : ((size_t)count < nl ? nl - (size_t)count : 0);
            for (size_t j = start; j < nl; j++) printf("%s", lines[j]);
            for (size_t j = 0; j < nl; j++) free(lines[j]);
            free(lines);
        }
        if (f != stdin) fclose(f);
    }
    return rc;
}
