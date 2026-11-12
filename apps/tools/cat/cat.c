/* cat - concatenate files and print on the standard output. */
#include <stdio.h>
#include <string.h>
#include <errno.h>

static FILE *open_arg(const char *path)
{
    if (!path || strcmp(path, "-") == 0) return stdin;
    FILE *f = fopen(path, "rb");
    if (!f) fprintf(stderr, "cat: %s: %s\n", path, strerror(errno));
    return f;
}

static void cat_fp(FILE *f, int number, int number_nb, int squeeze,
                   int ends, int tabs, int nonprint,
                   unsigned long *lno, int *at_sol, int *prev_blank)
{
    int c;
    while ((c = fgetc(f)) != EOF) {
        if (*at_sol) {
            if (c == '\n') {
                if (squeeze && *prev_blank) continue;
                *prev_blank = 1;
                if (number && !number_nb) printf("%6lu\t", (*lno)++);
            } else {
                *prev_blank = 0;
                if (number || number_nb) printf("%6lu\t", (*lno)++);
            }
            *at_sol = 0;
        }
        if (c == '\n') { if (ends) putchar('$'); putchar('\n'); *at_sol = 1; continue; }
        if (c == '\t') { if (tabs) { putchar('^'); putchar('I'); } else putchar('\t'); }
        else if (nonprint) {
            unsigned char uc = (unsigned char)c;
            if (uc >= 128) { printf("M-"); uc &= 127; }
            if (uc < 32) { putchar('^'); putchar(uc + '@'); }
            else if (uc == 127) { putchar('^'); putchar('?'); }
            else putchar(uc);
        } else putchar(c);
    }
}

int main(int argc, char **argv)
{
    int number = 0, number_nb = 0, squeeze = 0, ends = 0, tabs = 0, nonprint = 0, i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        for (const char *p = argv[i] + 1; *p; p++) {
            switch (*p) {
            case 'n': number = 1; break;
            case 'b': number_nb = 1; number = 0; break;
            case 's': squeeze = 1; break;
            case 'E': ends = 1; break;
            case 'T': tabs = 1; break;
            case 'v': nonprint = 1; break;
            case 'A': nonprint = 1; ends = 1; tabs = 1; break;
            case 'e': nonprint = 1; ends = 1; break;
            case 't': nonprint = 1; tabs = 1; break;
            case 'u': break;
            default: fprintf(stderr, "cat: invalid option -- '%c'\n", *p); return 1;
            }
        }
    }
    unsigned long lno = 1; int at_sol = 1, prev_blank = 0, rc = 0;
    if (i >= argc) { cat_fp(stdin, number, number_nb, squeeze, ends, tabs, nonprint, &lno, &at_sol, &prev_blank); return 0; }
    for (; i < argc; i++) {
        FILE *f = open_arg(argv[i]);
        if (!f) { rc = 1; continue; }
        cat_fp(f, number, number_nb, squeeze, ends, tabs, nonprint, &lno, &at_sol, &prev_blank);
        if (f != stdin) fclose(f);
    }
    return rc;
}
