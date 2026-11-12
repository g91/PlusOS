/* unexpand - convert spaces to tabs. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    int tab = 8, all = 0, i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) { tab = atoi(argv[++i]); continue; }
        if (strcmp(argv[i], "-a") == 0) all = 1;
    }
    if (tab <= 0) tab = 8;
    int nf = argc - i, rc = 0;
    for (int k = 0; k < (nf > 0 ? nf : 1); k++) {
        FILE *f = (nf > 0 && strcmp(argv[i + k], "-") != 0) ? fopen(argv[i + k], "rb") : stdin;
        if (!f) { rc = 1; continue; }
        int col = 0, leading = 1, spaces = 0, c;
        while ((c = fgetc(f)) != EOF) {
            if (c == ' ' && (leading || all)) {
                spaces++; col++;
                if (col % tab == 0) { putchar('\t'); spaces = 0; }
            } else {
                while (spaces > 0) { putchar(' '); spaces--; }
                if (c == '\n') { col = 0; leading = 1; }
                else if (c == '\t') col += tab - (col % tab);
                else { col++; leading = 0; }
                putchar(c);
            }
        }
        while (spaces > 0) { putchar(' '); spaces--; }
        if (f != stdin) fclose(f);
    }
    return rc;
}
