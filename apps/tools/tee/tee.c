/* tee - read from standard input and write to standard output and files. */
#include <stdio.h>
#include <string.h>
#include <errno.h>

int main(int argc, char **argv)
{
    int append = 0, i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        for (const char *p = argv[i] + 1; *p; p++) if (*p == 'a') append = 1;
    }
    FILE *outs[16]; int nf = 0;
    for (; i < argc && nf < 16; i++) {
        outs[nf] = fopen(argv[i], append ? "a" : "w");
        if (!outs[nf]) fprintf(stderr, "tee: %s: %s\n", argv[i], strerror(errno));
        else nf++;
    }
    int c;
    while ((c = fgetc(stdin)) != EOF) {
        putchar(c);
        for (int k = 0; k < nf; k++) fputc(c, outs[k]);
    }
    for (int k = 0; k < nf; k++) fclose(outs[k]);
    return 0;
}
