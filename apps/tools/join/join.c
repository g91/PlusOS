/* join - join lines of two files on a common field. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static char **read_lines(FILE *f, size_t *out_n)
{
    size_t cap = 32, n = 0; char **lines = malloc(cap * sizeof *lines);
    char *buf = NULL; size_t bcap = 0; int len;
    while ((len = read_line(f, &buf, &bcap)) >= 0) {
        if (len > 0 && buf[len - 1] == '\n') buf[--len] = '\0';
        if (n >= cap) lines = realloc(lines, (cap *= 2) * sizeof *lines);
        lines[n++] = strdup(buf);
    }
    free(buf); *out_n = n; return lines;
}

static void get_field(const char *line, int field1, char delim, char *out, size_t cap)
{
    const char *p = line; int cur = 1; out[0] = '\0';
    if (!delim) {
        while (*p) {
            while (*p == ' ' || *p == '\t') p++;
            if (!*p) break;
            const char *s = p;
            while (*p && *p != ' ' && *p != '\t') p++;
            if (cur == field1) {
                size_t l = (size_t)(p - s); if (l >= cap) l = cap - 1;
                memcpy(out, s, l); out[l] = '\0'; return;
            }
            cur++;
        }
    } else {
        while (*p) {
            const char *nx = strchr(p, delim);
            size_t l = nx ? (size_t)(nx - p) : strlen(p);
            if (cur == field1) { if (l >= cap) l = cap - 1; memcpy(out, p, l); out[l] = '\0'; return; }
            if (!nx) break;
            p = nx + 1;
            cur++;
        }
    }
}

int main(int argc, char **argv)
{
    int f1 = 1, f2 = 1, icase = 0, i = 1; char delim = 0;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        if (strcmp(argv[i], "-1") == 0 && i + 1 < argc) { f1 = atoi(argv[++i]); continue; }
        if (strcmp(argv[i], "-2") == 0 && i + 1 < argc) { f2 = atoi(argv[++i]); continue; }
        if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) { delim = argv[++i][0]; continue; }
        if (strcmp(argv[i], "-i") == 0) icase = 1;
    }
    if (argc - i < 2) { fprintf(stderr, "join: missing operand\n"); return 1; }
    FILE *fp1 = strcmp(argv[i], "-") ? fopen(argv[i], "rb") : stdin;
    FILE *fp2 = strcmp(argv[i + 1], "-") ? fopen(argv[i + 1], "rb") : stdin;
    if (!fp1 || !fp2) { fprintf(stderr, "join: cannot open input files\n"); return 1; }
    size_t n1 = 0, n2 = 0;
    char **l1 = read_lines(fp1, &n1), **l2 = read_lines(fp2, &n2);
    if (fp1 != stdin) fclose(fp1);
    if (fp2 != stdin) fclose(fp2);
    char k1[128], k2[128], sep = delim ? delim : ' ';
    for (size_t a = 0; a < n1; a++) {
        get_field(l1[a], f1, delim, k1, sizeof k1);
        for (size_t b = 0; b < n2; b++) {
            get_field(l2[b], f2, delim, k2, sizeof k2);
            if ((icase ? strcasecmp(k1, k2) : strcmp(k1, k2)) == 0) {
                fputs(k1, stdout);
                char fld[128];
                for (int f = 1; f < 16; f++) { if (f == f1) continue; get_field(l1[a], f, delim, fld, sizeof fld); if (!fld[0]) break; printf("%c%s", sep, fld); }
                for (int f = 1; f < 16; f++) { if (f == f2) continue; get_field(l2[b], f, delim, fld, sizeof fld); if (!fld[0]) break; printf("%c%s", sep, fld); }
                putchar('\n');
            }
        }
    }
    for (size_t a = 0; a < n1; a++) free(l1[a]);
    free(l1);
    for (size_t b = 0; b < n2; b++) free(l2[b]);
    free(l2);
    return 0;
}
