/* split - split a file into pieces. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

static int parse_sz(const char *s, unsigned long long *out)
{
    char *e = NULL; unsigned long long v = strtoull(s, &e, 10);
    if (e == s) return -1;
    unsigned long long mul = 1;
    if (*e) {
        switch (tolower((unsigned char)*e)) {
        case 'k': mul = 1024ULL; break; case 'm': mul = 1024ULL * 1024ULL; break; case 'g': mul = 1024ULL * 1024ULL * 1024ULL; break;
        default: return -1;
        }
    }
    *out = v * mul; return 0;
}

int main(int argc, char **argv)
{
    long lines_per = 1000; long long bytes_per = 0; int suf_len = 2, i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        if (strcmp(argv[i], "-l") == 0 && i + 1 < argc) { lines_per = atol(argv[++i]); bytes_per = 0; continue; }
        if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) { unsigned long long u = 0; parse_sz(argv[++i], &u); bytes_per = (long long)u; continue; }
        if (strcmp(argv[i], "-a") == 0 && i + 1 < argc) { suf_len = atoi(argv[++i]); continue; }
    }
    const char *inpath = (i < argc) ? argv[i++] : "-";
    const char *prefix = (i < argc) ? argv[i++] : "x";
    FILE *in = strcmp(inpath, "-") ? fopen(inpath, "rb") : stdin;
    if (!in) { fprintf(stderr, "split: %s: %s\n", inpath, strerror(errno)); return 1; }
    if (suf_len < 1 || suf_len > 6) suf_len = 2;
    int part = 0, c; long cur_l = 0; long long cur_b = 0; FILE *out = NULL; char outname[160];
    while ((c = fgetc(in)) != EOF) {
        if (!out) {
            char suf[8]; int t = part++;
            for (int k = suf_len - 1; k >= 0; k--) { suf[k] = (char)('a' + (t % 26)); t /= 26; }
            suf[suf_len] = '\0'; snprintf(outname, sizeof outname, "%s%s", prefix, suf);
            out = fopen(outname, "w"); if (!out) { if (in != stdin) fclose(in); return 1; }
            cur_l = 0; cur_b = 0;
        }
        fputc(c, out); cur_b++; if (c == '\n') cur_l++;
        if ((bytes_per > 0 && cur_b >= bytes_per) || (!bytes_per && cur_l >= lines_per)) { fclose(out); out = NULL; }
    }
    if (out) fclose(out);
    if (in != stdin) fclose(in);
    return 0;
}
