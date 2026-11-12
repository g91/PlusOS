/* base32 - base32 encode/decode data and print to standard output. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <errno.h>

int main(int argc, char **argv)
{
    int dec = 0, ign = 0, wrap = 76, i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (!strcmp(argv[i], "--")) { i++; break; }
        if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--decode")) dec = 1;
        else if (!strcmp(argv[i], "-i")) ign = 1;
        else if (!strcmp(argv[i], "-w") && i + 1 < argc) wrap = atoi(argv[++i]);
        else if (!strncmp(argv[i], "--wrap=", 7)) wrap = atoi(argv[i] + 7);
    }
    FILE *f = (i < argc && strcmp(argv[i], "-")) ? fopen(argv[i], "rb") : stdin;
    if (!f) { fprintf(stderr, "base32: %s: %s\n", argv[i], strerror(errno)); return 1; }
    static const char tab[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    if (!dec) {
        int col = 0; uint8_t in[5];
        for (;;) {
            int n = 0; while (n < 5) { int c = fgetc(f); if (c == EOF) break; in[n++] = (uint8_t)c; }
            if (!n) break;
            for (int k = n; k < 5; k++) in[k] = 0;
            uint64_t v = ((uint64_t)in[0]<<32)|((uint64_t)in[1]<<24)|((uint64_t)in[2]<<16)|((uint64_t)in[3]<<8)|in[4];
            int chars = (n == 1) ? 2 : (n == 2) ? 4 : (n == 3) ? 5 : (n == 4) ? 7 : 8;
            for (int k = 0; k < 8; k++) { putchar(k < chars ? tab[(v >> (35 - k * 5)) & 31] : '='); col++; if (wrap > 0 && col >= wrap) { putchar('\n'); col = 0; } }
        }
        if (wrap == 0 || col > 0) putchar('\n');
    } else {
        int q[8], qn = 0, pad = 0, c;
        while ((c = fgetc(f)) != EOF) {
            if (c == '\n' || c == '\r' || c == ' ' || c == '\t') continue;
            if (c == '=') { q[qn++] = 0; pad++; }
            else {
                int uc = toupper((unsigned char)c);
                int v = (uc >= 'A' && uc <= 'Z') ? (uc - 'A') : (uc >= '2' && uc <= '7') ? (uc - '2' + 26) : -1;
                if (v < 0) { if (ign) continue; if (f != stdin) fclose(f); return 1; }
                q[qn++] = v;
            }
            if (qn == 8) {
                uint64_t v = 0; for (int k = 0; k < 8; k++) v = (v << 5) | (uint64_t)q[k];
                int nb = (pad >= 6) ? 1 : (pad >= 4) ? 2 : (pad >= 3) ? 3 : (pad >= 1) ? 4 : 5;
                for (int k = 0; k < nb; k++) putchar((int)((v >> (32 - k * 8)) & 0xff));
                qn = 0; pad = 0;
            }
        }
    }
    if (f != stdin) fclose(f);
    return 0;
}
