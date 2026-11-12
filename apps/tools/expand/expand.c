/* expand - convert tabs to spaces. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    int tab = 8, initial_only = 0, i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) { tab = atoi(argv[++i]); continue; }
        if (strcmp(argv[i], "-i") == 0) initial_only = 1;
    }
    if (tab <= 0) tab = 8;
    int nf = argc - i, rc = 0;
    for (int k = 0; k < (nf > 0 ? nf : 1); k++) {
        FILE *f = (nf > 0 && strcmp(argv[i + k], "-") != 0) ? fopen(argv[i + k], "rb") : stdin;
        if (!f) { rc = 1; continue; }
        int col = 0, leading = 1, c;
        while ((c = fgetc(f)) != EOF) {
            if (c == '\t' && (!initial_only || leading)) {
                int nsp = tab - (col % tab);
                for (int s = 0; s < nsp; s++) putchar(' ');
                col += nsp;
            } else {
                if (c != ' ') leading = 0;
                if (c == '\n') { col = 0; leading = 1; } else col++;
                putchar(c);
            }
        }
        if (f != stdin) fclose(f);
    }
    return rc;
}
