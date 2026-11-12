/* sum - checksum and count the blocks in a file. */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

int main(int argc, char **argv)
{
    int sysv = 0, i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        for (const char *p = argv[i] + 1; *p; p++) { if (*p == 's') sysv = 1; else if (*p == 'r') sysv = 0; }
    }
    int nf = argc - i, rc = 0;
    for (int k = 0; k < (nf > 0 ? nf : 1); k++) {
        const char *name = nf > 0 ? argv[i + k] : NULL;
        FILE *f = (name && strcmp(name, "-") != 0) ? fopen(name, "rb") : stdin;
        if (!f) { fprintf(stderr, "sum: %s: %s\n", name, strerror(errno)); rc = 1; continue; }
        uint32_t bsd = 0, sv = 0; unsigned long long bytes = 0; int c;
        while ((c = fgetc(f)) != EOF) {
            bytes++;
            bsd = ((bsd >> 1) + ((bsd & 1) << 15) + (uint32_t)(c & 0xff)) & 0xffffu;
            sv += (uint32_t)(c & 0xff);
        }
        if (sysv) {
            sv = (sv & 0xffffu) + (sv >> 16); sv = (sv & 0xffffu) + (sv >> 16);
            printf("%lu %llu", (unsigned long)sv, (bytes + 511) / 512);
        } else printf("%05lu %5llu", (unsigned long)bsd, (bytes + 1023) / 1024);
        if (name && (nf > 1 || sysv)) printf(" %s", name);
        putchar('\n');
        if (f != stdin) fclose(f);
    }
    return rc;
}
