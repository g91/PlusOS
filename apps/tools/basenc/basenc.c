/* basenc - encode/decode data (delegates base64/base32 cleanly via spawnv or handles base64url/base16). */
#include <plusos/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <unistd.h>

static int b64url_val(int c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '-') return 62;
    if (c == '_') return 63;
    return -1;
}

int main(int argc, char **argv)
{
    int mode = 0, dec = 0, ign = 0, wrap = 76, i = 1; /* 0=b64, 1=b64url, 2=b32, 3=b16 */
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (!strcmp(argv[i], "--")) { i++; break; }
        if (!strcmp(argv[i], "--base64")) mode = 0;
        else if (!strcmp(argv[i], "--base64url")) mode = 1;
        else if (!strcmp(argv[i], "--base32")) mode = 2;
        else if (!strcmp(argv[i], "--base16")) mode = 3;
        else if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--decode")) dec = 1;
        else if (!strcmp(argv[i], "-i")) ign = 1;
        else if (!strcmp(argv[i], "-w") && i + 1 < argc) wrap = atoi(argv[++i]);
    }
    if (mode == 0 || mode == 2) {
        char **nav = malloc((size_t)(argc + 2) * sizeof(char *));
        if (!nav) return 1;
        int n = 0; nav[n++] = (mode == 0) ? "base64" : "base32";
        for (int k = 1; k < argc; k++) {
            if (!strcmp(argv[k], "--base64") || !strcmp(argv[k], "--base32")) continue;
            nav[n++] = argv[k];
        }
        nav[n] = NULL;
        int pid = spawnv(nav[0], nav);
        free(nav);
        return pid < 0 ? 1 : (int)plusos_syscall(SYS_wait, (void *)(intptr_t)pid);
    }
    FILE *f = (i < argc && strcmp(argv[i], "-")) ? fopen(argv[i], "rb") : stdin;
    if (!f) return 1;
    if (mode == 3) {
        if (!dec) {
            int col = 0, c;
            while ((c = fgetc(f)) != EOF) { printf("%02X", c & 0xff); col += 2; if (wrap > 0 && col >= wrap) { putchar('\n'); col = 0; } }
            if (col > 0) putchar('\n');
        } else {
            int hi = -1, c;
            while ((c = fgetc(f)) != EOF) {
                if (isspace((unsigned char)c)) continue;
                int v = (c>='0'&&c<='9')?(c-'0'):(c>='A'&&c<='F')?(c-'A'+10):(c>='a'&&c<='f')?(c-'a'+10):-1;
                if (v < 0) continue;
                if (hi < 0) hi = v; else { putchar((hi << 4) | v); hi = -1; }
            }
        }
    } else {
        static const char tab[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
        if (!dec) {
            int col = 0; uint8_t in[3];
            for (;;) {
                int n = 0; while (n < 3) { int c = fgetc(f); if (c == EOF) break; in[n++] = (uint8_t)c; }
                if (!n) break;
                uint32_t v = ((uint32_t)in[0]<<16) | (n>1 ? ((uint32_t)in[1]<<8) : 0) | (n>2 ? in[2] : 0);
                char out[4] = { tab[(v>>18)&63], tab[(v>>12)&63], n>1 ? tab[(v>>6)&63] : '=', n>2 ? tab[v&63] : '=' };
                for (int k = 0; k < 4; k++) { putchar(out[k]); col++; if (wrap > 0 && col >= wrap) { putchar('\n'); col = 0; } }
            }
            if (wrap == 0 || col > 0) putchar('\n');
        } else {
            int q[4], qn = 0, pad = 0, c;
            while ((c = fgetc(f)) != EOF) {
                if (c == '\n' || c == '\r' || c == ' ' || c == '\t') continue;
                if (c == '=') { q[qn++] = 0; pad++; }
                else { int v = b64url_val(c); if (v < 0) { if (ign) continue; return 1; } q[qn++] = v; }
                if (qn == 4) {
                    uint32_t v = ((uint32_t)q[0]<<18)|((uint32_t)q[1]<<12)|((uint32_t)q[2]<<6)|(uint32_t)q[3];
                    putchar((int)((v>>16)&0xff)); if (pad<2) putchar((int)((v>>8)&0xff)); if (pad<1) putchar((int)(v&0xff));
                    qn = 0; pad = 0;
                }
            }
        }
    }
    if (f != stdin) fclose(f);
    return 0;
}
