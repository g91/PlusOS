/* numfmt - convert numbers from/to human-readable strings. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static int parse_sz(const char *s, unsigned long long *out)
{
    char *e = NULL; unsigned long long v = strtoull(s, &e, 10);
    if (e == s) return -1;
    unsigned long long mul = 1;
    if (*e && !isspace((unsigned char)*e)) {
        switch (tolower((unsigned char)*e)) {
        case 'k': mul = 1024ULL; break; case 'm': mul = 1024ULL * 1024ULL; break; case 'g': mul = 1024ULL * 1024ULL * 1024ULL; break;
        default: break;
        }
    }
    *out = v * mul; return 0;
}

static void fmt_hum(unsigned long long bytes, char *out, size_t cap, int si)
{
    static const char *const u[] = { "", "K", "M", "G", "T" };
    unsigned long base = si ? 1000UL : 1024UL;
    if (bytes < base) { snprintf(out, cap, "%llu", bytes); return; }
    int idx = 0; unsigned long long w = bytes; unsigned long rem = 0;
    while (w >= base && idx < 4) { rem = (unsigned long)(w % base); w /= base; idx++; }
    if (w < 10) snprintf(out, cap, "%llu.%lu%s", w, (rem * 10UL) / base, u[idx]);
    else snprintf(out, cap, "%llu%s", w, u[idx]);
}

int main(int argc, char **argv)
{
    int to_mode = 0, i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (!strcmp(argv[i], "--to=iec") || !strcmp(argv[i], "--to=iec-i")) to_mode = 1;
        else if (!strcmp(argv[i], "--to=si")) to_mode = 2;
    }
    char out[24];
    if (i < argc) {
        for (; i < argc; i++) {
            unsigned long long v = 0; parse_sz(argv[i], &v);
            if (to_mode) { fmt_hum(v, out, sizeof out, to_mode == 2); puts(out); }
            else printf("%llu\n", v);
        }
        return 0;
    }
    char line[128];
    while (fgets(line, sizeof line, stdin)) {
        unsigned long long v = 0; parse_sz(line, &v);
        if (to_mode) { fmt_hum(v, out, sizeof out, to_mode == 2); puts(out); }
        else printf("%llu\n", v);
    }
    return 0;
}
