/* wc - print newline, word, and byte counts for each file. */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

int main(int argc, char **argv)
{
    int do_l = 0, do_w = 0, do_c = 0, do_m = 0, do_L = 0, i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        for (const char *p = argv[i] + 1; *p; p++) {
            if (*p == 'l') do_l = 1; else if (*p == 'w') do_w = 1;
            else if (*p == 'c') do_c = 1; else if (*p == 'm') do_m = 1;
            else if (*p == 'L') do_L = 1;
        }
    }
    if (!do_l && !do_w && !do_c && !do_m && !do_L) do_l = do_w = do_c = 1;
    unsigned long long tl = 0, tw = 0, tc = 0, tL = 0;
    int nf = argc - i, rc = 0;
    for (int k = 0; k < (nf > 0 ? nf : 1); k++) {
        const char *name = nf > 0 ? argv[i + k] : NULL;
        FILE *f = (name && strcmp(name, "-") != 0) ? fopen(name, "rb") : stdin;
        if (!f) { fprintf(stderr, "wc: %s: %s\n", name, strerror(errno)); rc = 1; continue; }
        unsigned long long lines = 0, words = 0, bytes = 0, maxl = 0, curl = 0;
        int in_word = 0, c;
        while ((c = fgetc(f)) != EOF) {
            bytes++;
            if (c == '\n') { lines++; if (curl > maxl) maxl = curl; curl = 0; }
            else if (c != '\r') curl++;
            if (isspace((unsigned char)c)) in_word = 0;
            else if (!in_word) { in_word = 1; words++; }
        }
        if (curl > maxl) maxl = curl;
        tl += lines; tw += words; tc += bytes; if (maxl > tL) tL = maxl;
        int first = 1;
        #define PCOL(val) do { printf("%s%7llu", first ? "" : " ", (val)); first = 0; } while(0)
        if (do_l) PCOL(lines); if (do_w) PCOL(words); if (do_c || do_m) PCOL(bytes); if (do_L) PCOL(maxl);
        if (name) printf(" %s", name);
        putchar('\n');
        if (f != stdin) fclose(f);
    }
    if (nf > 1) {
        int first = 1;
        if (do_l) PCOL(tl); if (do_w) PCOL(tw); if (do_c || do_m) PCOL(tc); if (do_L) PCOL(tL);
        printf(" total\n");
    }
    #undef PCOL
    return rc;
}
