/* tr - translate or delete characters. */
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static int build_set(const char *s, unsigned char out[256], int *len_out)
{
    int len = 0;
    while (*s && len < 256) {
        if (strncmp(s, "[:alnum:]", 9) == 0) { for (int c = 0; c < 128 && len < 256; c++) if (isalnum(c)) out[len++] = (unsigned char)c; s += 9; continue; }
        if (strncmp(s, "[:alpha:]", 9) == 0) { for (int c = 0; c < 128 && len < 256; c++) if (isalpha(c)) out[len++] = (unsigned char)c; s += 9; continue; }
        if (strncmp(s, "[:digit:]", 9) == 0) { for (int c = '0'; c <= '9' && len < 256; c++) out[len++] = (unsigned char)c; s += 9; continue; }
        if (strncmp(s, "[:lower:]", 9) == 0) { for (int c = 'a'; c <= 'z' && len < 256; c++) out[len++] = (unsigned char)c; s += 9; continue; }
        if (strncmp(s, "[:upper:]", 9) == 0) { for (int c = 'A'; c <= 'Z' && len < 256; c++) out[len++] = (unsigned char)c; s += 9; continue; }
        if (strncmp(s, "[:space:]", 9) == 0) { const char *sp = "\t\n\v\f\r "; for (const char *q = sp; *q && len < 256; q++) out[len++] = (unsigned char)*q; s += 9; continue; }
        if (strncmp(s, "[:blank:]", 9) == 0) { out[len++] = ' '; out[len++] = '\t'; s += 9; continue; }
        unsigned char ch;
        if (*s == '\\' && s[1]) {
            s++;
            if (*s == 'n') ch = '\n'; else if (*s == 't') ch = '\t'; else if (*s == 'r') ch = '\r';
            else if (*s >= '0' && *s <= '7') {
                int v = 0; for (int k = 0; k < 3 && *s >= '0' && *s <= '7'; k++) v = v * 8 + (*s++ - '0');
                s--; ch = (unsigned char)v;
            } else ch = (unsigned char)*s;
            s++;
        } else ch = (unsigned char)*s++;
        if (s[0] == '-' && s[1]) {
            unsigned char endc = (unsigned char)s[1]; s += 2;
            for (int c = ch; c <= endc && len < 256; c++) out[len++] = (unsigned char)c;
        } else out[len++] = ch;
    }
    *len_out = len;
    return 0;
}

int main(int argc, char **argv)
{
    int del = 0, squeeze = 0, comp = 0, i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        for (const char *p = argv[i] + 1; *p; p++) {
            if (*p == 'd') del = 1; else if (*p == 's') squeeze = 1; else if (*p == 'c' || *p == 'C') comp = 1;
        }
    }
    if (i >= argc) { fprintf(stderr, "tr: missing operand\n"); return 1; }
    unsigned char s1[256], s2[256]; int l1 = 0, l2 = 0;
    build_set(argv[i++], s1, &l1);
    if (i < argc) build_set(argv[i++], s2, &l2);
    unsigned char in1[256]; memset(in1, 0, sizeof in1);
    for (int k = 0; k < l1; k++) in1[s1[k]] = 1;
    if (comp) {
        for (int k = 0; k < 256; k++) in1[k] = !in1[k];
        l1 = 0; for (int k = 0; k < 256; k++) if (in1[k]) s1[l1++] = (unsigned char)k;
    }
    unsigned char map[256], sq_set[256];
    for (int k = 0; k < 256; k++) { map[k] = (unsigned char)k; sq_set[k] = 0; }
    if (!del && l2 > 0) for (int k = 0; k < l1; k++) map[s1[k]] = s2[k < l2 ? k : l2 - 1];
    if (squeeze) {
        if (l2 > 0) for (int k = 0; k < l2; k++) sq_set[s2[k]] = 1;
        else for (int k = 0; k < l1; k++) sq_set[s1[k]] = 1;
    }
    int prev = -1, c;
    while ((c = fgetc(stdin)) != EOF) {
        unsigned char uc = (unsigned char)c;
        if (del && in1[uc]) continue;
        unsigned char outc = (!del && l2 > 0) ? map[uc] : uc;
        if (squeeze && sq_set[outc] && outc == prev) continue;
        putchar(outc); prev = outc;
    }
    return 0;
}
