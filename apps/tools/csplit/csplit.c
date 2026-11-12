/* csplit - split a file into sections determined by context lines. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static int match_here(const char *re, const char *txt);
static int match_star(const char *re, int ch, const char *txt)
{
    const char *t = txt;
    while (*t && (ch == '.' || *t == ch)) t++;
    while (t >= txt) { if (match_here(re, t)) return 1; t--; }
    return 0;
}
static int match_here(const char *re, const char *txt)
{
    if (!re[0]) return 1;
    if (re[1] == '*') return match_star(re + 2, (unsigned char)re[0], txt);
    if (re[0] == '$' && !re[1]) return *txt == '\0';
    if (*txt && (re[0] == '.' || re[0] == *txt)) return match_here(re + 1, txt + 1);
    return 0;
}
static int simple_rx(const char *re, const char *txt)
{
    if (re[0] == '^') return match_here(re + 1, txt);
    for (const char *t = txt; ; t++) { if (match_here(re, t)) return 1; if (!*t) break; }
    return 0;
}

int main(int argc, char **argv)
{
    const char *prefix = "xx"; int quiet = 0, i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) { prefix = argv[++i]; continue; }
        if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "-s") == 0) quiet = 1;
    }
    if (argc - i < 2) { fprintf(stderr, "csplit: missing operand\n"); return 1; }
    FILE *f = strcmp(argv[i], "-") ? fopen(argv[i], "rb") : stdin;
    if (!f) { fprintf(stderr, "csplit: %s: %s\n", argv[i], strerror(errno)); return 1; }
    i++;
    size_t cap = 32, nl = 0, bcap = 128; char **lines = malloc(cap * sizeof *lines);
    char *buf = malloc(bcap);
    for (;;) {
        size_t len = 0; int c;
        while ((c = fgetc(f)) != EOF) {
            if (len + 2 >= bcap) buf = realloc(buf, (bcap *= 2));
            buf[len++] = (char)c; if (c == '\n') break;
        }
        if (len == 0) break;
        if (buf[len - 1] == '\n') buf[--len] = '\0'; else buf[len] = '\0';
        if (nl >= cap) lines = realloc(lines, (cap *= 2) * sizeof *lines);
        lines[nl++] = strdup(buf);
    }
    free(buf); if (f != stdin) fclose(f);

    size_t pos = 0; int part = 0;
    for (; i < argc; i++) {
        size_t end = pos;
        if (argv[i][0] == '/') {
            char pat[128]; strlcpy(pat, argv[i] + 1, sizeof pat);
            char *sl = strrchr(pat, '/'); if (sl) *sl = '\0';
            while (end < nl && !simple_rx(pat, lines[end])) end++;
        } else {
            size_t t = (size_t)atol(argv[i]);
            end = (t > 0 && t - 1 <= nl) ? t - 1 : nl;
        }
        char outname[160]; snprintf(outname, sizeof outname, "%s%02d", prefix, part++);
        FILE *out = fopen(outname, "w"); unsigned long bytes = 0;
        if (out) { for (size_t k = pos; k < end && k < nl; k++) { fprintf(out, "%s\n", lines[k]); bytes += (unsigned long)strlen(lines[k]) + 1; } fclose(out); }
        if (!quiet) printf("%lu\n", bytes);
        pos = end;
    }
    if (pos < nl) {
        char outname[160]; snprintf(outname, sizeof outname, "%s%02d", prefix, part++);
        FILE *out = fopen(outname, "w"); unsigned long bytes = 0;
        if (out) { for (size_t k = pos; k < nl; k++) { fprintf(out, "%s\n", lines[k]); bytes += (unsigned long)strlen(lines[k]) + 1; } fclose(out); }
        if (!quiet) printf("%lu\n", bytes);
    }
    for (size_t k = 0; k < nl; k++) free(lines[k]);
    free(lines);
    return 0;
}
