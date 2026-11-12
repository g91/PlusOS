/* uniq - report or omit repeated lines. */
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

static const char *skip_f(const char *s, int fskip, int sskip)
{
    for (int f = 0; f < fskip && *s; f++) {
        while (*s == ' ' || *s == '\t') s++;
        while (*s && *s != ' ' && *s != '\t') s++;
    }
    while (*s && sskip-- > 0) s++;
    return s;
}

int main(int argc, char **argv)
{
    int count = 0, dup_only = 0, uniq_only = 0, icase = 0, fskip = 0, sskip = 0, i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) { fskip = atoi(argv[++i]); continue; }
        if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) { sskip = atoi(argv[++i]); continue; }
        for (const char *p = argv[i] + 1; *p; p++) {
            if (*p == 'c') count = 1; else if (*p == 'd' || *p == 'D') dup_only = 1;
            else if (*p == 'u') uniq_only = 1; else if (*p == 'i') icase = 1;
        }
    }
    FILE *in = (i < argc && strcmp(argv[i], "-") != 0) ? fopen(argv[i], "rb") : stdin;
    if (!in) { fprintf(stderr, "uniq: %s: %s\n", argv[i], strerror(errno)); return 1; }
    FILE *out = (i + 1 < argc) ? fopen(argv[i + 1], "w") : stdout;
    if (!out) { if (in != stdin) fclose(in); return 1; }
    char *prev = NULL, *cur = NULL; size_t ccap = 0;
    unsigned long run = 0; int len;
    while ((len = read_line(in, &cur, &ccap)) >= 0) {
        if (len > 0 && cur[len - 1] == '\n') cur[--len] = '\0';
        if (!prev) { prev = strdup(cur); run = 1; }
        else {
            const char *kp = skip_f(prev, fskip, sskip), *kc = skip_f(cur, fskip, sskip);
            int eq = icase ? (strcasecmp(kp, kc) == 0) : (strcmp(kp, kc) == 0);
            if (eq) run++;
            else {
                int show = (!dup_only && !uniq_only) || (dup_only && run > 1) || (uniq_only && run == 1);
                if (show) { if (count) fprintf(out, "%7lu %s\n", run, prev); else fprintf(out, "%s\n", prev); }
                free(prev); prev = strdup(cur); run = 1;
            }
        }
    }
    if (prev) {
        int show = (!dup_only && !uniq_only) || (dup_only && run > 1) || (uniq_only && run == 1);
        if (show) { if (count) fprintf(out, "%7lu %s\n", run, prev); else fprintf(out, "%s\n", prev); }
        free(prev);
    }
    free(cur);
    if (in != stdin) fclose(in); if (out != stdout) fclose(out);
    return 0;
}
