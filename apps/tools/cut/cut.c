/* cut - remove sections from each line of files. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

typedef struct { int lo, hi; } Range;

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

static int parse_ranges(const char *s, Range *r, int maxr)
{
    int n = 0;
    while (*s && n < maxr) {
        int lo = 1, hi = 0x7fffffff;
        if (*s == '-') { s++; hi = atoi(s); while (isdigit((unsigned char)*s)) s++; }
        else {
            lo = atoi(s); while (isdigit((unsigned char)*s)) s++;
            if (*s == '-') { s++; if (isdigit((unsigned char)*s)) { hi = atoi(s); while (isdigit((unsigned char)*s)) s++; } }
            else hi = lo;
        }
        r[n].lo = lo; r[n].hi = hi; n++;
        if (*s == ',') s++;
    }
    return n;
}

static int in_ranges(int idx1, const Range *r, int nr, int comp)
{
    int hit = 0;
    for (int i = 0; i < nr; i++) if (idx1 >= r[i].lo && idx1 <= r[i].hi) { hit = 1; break; }
    return comp ? !hit : hit;
}

int main(int argc, char **argv)
{
    int mode = 0, only_delim = 0, comp = 0, i = 1;
    char delim = '\t';
    const char *odelim = NULL, *spec = NULL;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        if (strcmp(argv[i], "--complement") == 0) { comp = 1; continue; }
        if (strncmp(argv[i], "--output-delimiter=", 19) == 0) { odelim = argv[i] + 19; continue; }
        if ((strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "-c") == 0) && i + 1 < argc) { mode = 1; spec = argv[++i]; continue; }
        if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) { mode = 2; spec = argv[++i]; continue; }
        if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) { delim = argv[++i][0]; continue; }
        if (strcmp(argv[i], "-s") == 0) { only_delim = 1; continue; }
        if (argv[i][1] == 'f' && argv[i][2]) { mode = 2; spec = argv[i] + 2; continue; }
        if ((argv[i][1] == 'b' || argv[i][1] == 'c') && argv[i][2]) { mode = 1; spec = argv[i] + 2; continue; }
        if (argv[i][1] == 'd' && argv[i][2]) { delim = argv[i][2]; continue; }
    }
    if (!mode || !spec) { fprintf(stderr, "cut: specify -b, -c, or -f\n"); return 1; }
    Range ranges[32]; int nr = parse_ranges(spec, ranges, 32);
    char dstr[2] = { delim, '\0' }; if (!odelim) odelim = dstr;

    int nf = argc - i, rc = 0; char *line = NULL; size_t lcap = 0;
    for (int k = 0; k < (nf > 0 ? nf : 1); k++) {
        FILE *f = (nf > 0 && strcmp(argv[i + k], "-") != 0) ? fopen(argv[i + k], "rb") : stdin;
        if (!f) { fprintf(stderr, "cut: %s: %s\n", argv[i + k], strerror(errno)); rc = 1; continue; }
        int len;
        while ((len = read_line(f, &line, &lcap)) >= 0) {
            int had_nl = (len > 0 && line[len - 1] == '\n');
            if (had_nl) line[--len] = '\0';
            if (mode == 1) {
                for (int idx = 0; idx < len; idx++) if (in_ranges(idx + 1, ranges, nr, comp)) putchar(line[idx]);
                if (had_nl) putchar('\n');
            } else {
                if (!strchr(line, delim)) {
                    if (!only_delim) { fputs(line, stdout); if (had_nl) putchar('\n'); }
                    continue;
                }
                int fld = 1, printed = 0; char *p = line;
                for (;;) {
                    char *nx = strchr(p, delim); if (nx) *nx = '\0';
                    if (in_ranges(fld, ranges, nr, comp)) { if (printed) fputs(odelim, stdout); fputs(p, stdout); printed = 1; }
                    if (!nx) break;
                    p = nx + 1;
                    fld++;
                }
                if (had_nl) putchar('\n');
            }
        }
        if (f != stdin) fclose(f);
    }
    free(line);
    return rc;
}
