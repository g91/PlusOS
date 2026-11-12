/* seq - print a sequence of numbers. */
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static int parse_dec(const char *s, long long *num, int *scale)
{
    int neg = 0, sc = 0;
    if (*s == '-') { neg = 1; s++; } else if (*s == '+') s++;
    long long v = 0;
    while (isdigit((unsigned char)*s)) v = v * 10 + (*s++ - '0');
    if (*s == '.') { s++; while (isdigit((unsigned char)*s)) { v = v * 10 + (*s++ - '0'); sc++; } }
    *num = neg ? -v : v; *scale = sc; return *s ? -1 : 0;
}

static long long pow10_ll(int e) { long long r = 1; while (e-- > 0) r *= 10; return r; }

int main(int argc, char **argv)
{
    const char *sep = "\n"; int equal_w = 0, i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        if (isdigit((unsigned char)argv[i][1]) || argv[i][1] == '.') break;
        if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) { sep = argv[++i]; continue; }
        if (strcmp(argv[i], "-w") == 0) { equal_w = 1; continue; }
    }
    int narg = argc - i;
    if (narg < 1 || narg > 3) { fprintf(stderr, "seq: missing or extra operand\n"); return 1; }
    const char *sf = "1", *si = "1", *sl = NULL;
    if (narg == 1) sl = argv[i];
    else if (narg == 2) { sf = argv[i]; sl = argv[i + 1]; }
    else { sf = argv[i]; si = argv[i + 1]; sl = argv[i + 2]; }

    long long a = 0, d = 0, z = 0; int sa = 0, sd = 0, sz = 0;
    if (parse_dec(sf, &a, &sa) || parse_dec(si, &d, &sd) || parse_dec(sl, &z, &sz)) {
        fprintf(stderr, "seq: invalid floating point argument\n"); return 1;
    }
    int sc = sa > sd ? sa : sd; if (sz > sc) sc = sz;
    a *= pow10_ll(sc - sa); d *= pow10_ll(sc - sd); z *= pow10_ll(sc - sz);
    if (d == 0) { fprintf(stderr, "seq: zero increment\n"); return 1; }
    long long p10 = pow10_ll(sc); int maxw = 0;
    if (equal_w && sc == 0) {
        char b1[32], b2[32]; snprintf(b1, sizeof b1, "%lld", a); snprintf(b2, sizeof b2, "%lld", z);
        int w1 = (int)strlen(b1), w2 = (int)strlen(b2); maxw = w1 > w2 ? w1 : w2;
    }
    int first = 1;
    for (long long cur = a; d > 0 ? (cur <= z) : (cur >= z); cur += d) {
        if (!first) fputs(sep, stdout);
        first = 0;
        if (sc == 0) { if (equal_w) printf("%0*lld", maxw, cur); else printf("%lld", cur); }
        else { long long av = cur < 0 ? -cur : cur; printf("%s%lld.%0*lld", (cur < 0 && cur / p10 == 0) ? "-" : "", cur / p10, sc, av % p10); }
    }
    if (!first) putchar('\n');
    return 0;
}
