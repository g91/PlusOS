/* base64 - base64 encode/decode data and print to standard output. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

static int b64_val(int c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

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
    if (!f) { fprintf(stderr, "base64: %s: %s\n", argv[i], strerror(errno)); return 1; }
    static const char tab[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    if (!dec) {
        int col = 0; uint8_t in[3];
        for (;;) {
            int n = 0; while (n < 3) { int c = fgetc(f); if (c == EOF) break; in[n++] = (uint8_t)c; }
            if (!n) break;
            uint32_t v = ((uint32_t)in[0]<<16) | (n > 1 ? ((uint32_t)in[1]<<8) : 0) | (n > 2 ? in[2] : 0);
            char out[4] = { tab[(v>>18)&63], tab[(v>>12)&63], n>1 ? tab[(v>>6)&63] : '=', n>2 ? tab[v&63] : '=' };
            for (int k = 0; k < 4; k++) { putchar(out[k]); col++; if (wrap > 0 && col >= wrap) { putchar('\n'); col = 0; } }
        }
        if (wrap == 0 || col > 0) putchar('\n');
    } else {
        int q[4], qn = 0, pad = 0, c;
        while ((c = fgetc(f)) != EOF) {
            if (c == '\n' || c == '\r' || c == ' ' || c == '\t') continue;
            if (c == '=') { q[qn++] = 0; pad++; }
            else {
                int v = b64_val(c);
                if (v < 0) { if (ign) continue; fprintf(stderr, "base64: invalid input\n"); if (f != stdin) fclose(f); return 1; }
                q[qn++] = v;
            }
            if (qn == 4) {
                uint32_t v = ((uint32_t)q[0]<<18) | ((uint32_t)q[1]<<12) | ((uint32_t)q[2]<<6) | (uint32_t)q[3];
                putchar((int)((v >> 16) & 0xff));
                if (pad < 2) putchar((int)((v >> 8) & 0xff));
                if (pad < 1) putchar((int)(v & 0xff));
                qn = 0; pad = 0;
            }
        }
    }
    if (f != stdin) fclose(f);
    return 0;
}
