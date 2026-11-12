/* sha1sum - compute and check SHA1 message digest. */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

static uint8_t io[1024];
static uint32_t rol32(uint32_t x, int n) { return (x << n) | (x >> (32 - n)); }

static void sha1_block(uint32_t s[5], const uint8_t b[64])
{
    uint32_t w[80];
    for (int i = 0; i < 16; i++)
        w[i] = ((uint32_t)b[i*4]<<24)|((uint32_t)b[i*4+1]<<16)|((uint32_t)b[i*4+2]<<8)|b[i*4+3];
    for (int i = 16; i < 80; i++) w[i] = rol32(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);
    uint32_t a=s[0], B=s[1], c=s[2], d=s[3], e=s[4];
    for (int i = 0; i < 80; i++) {
        uint32_t f, k;
        if (i < 20) { f = (B & c) | ((~B) & d); k = 0x5a827999u; }
        else if (i < 40) { f = B ^ c ^ d; k = 0x6ed9eba1u; }
        else if (i < 60) { f = (B & c) | (B & d) | (c & d); k = 0x8f1bbcdcu; }
        else { f = B ^ c ^ d; k = 0xca62c1d6u; }
        uint32_t t = rol32(a, 5) + f + e + k + w[i]; e = d; d = c; c = rol32(B, 30); B = a; a = t;
    }
    s[0]+=a; s[1]+=B; s[2]+=c; s[3]+=d; s[4]+=e;
}

static void sha1_hex(FILE *f, char hex[41])
{
    uint32_t st[5] = { 0x67452301u, 0xefcdab89u, 0x98badcfeu, 0x10325476u, 0xc3d2e1f0u };
    uint8_t buf[64]; size_t len = 0, n; uint64_t bits = 0;
    while ((n = fread(io, 1, sizeof io, f)) > 0) {
        bits += (uint64_t)n * 8; size_t pos = 0;
        while (pos < n) {
            size_t t = 64 - len; if (t > n - pos) t = n - pos;
            memcpy(buf + len, io + pos, t); len += t; pos += t;
            if (len == 64) { sha1_block(st, buf); len = 0; }
        }
    }
    buf[len++] = 0x80;
    if (len > 56) { while (len < 64) buf[len++] = 0; sha1_block(st, buf); len = 0; }
    while (len < 56) buf[len++] = 0;
    for (int i = 0; i < 8; i++) buf[56 + i] = (uint8_t)(bits >> (56 - i * 8));
    sha1_block(st, buf);
    for (int i = 0; i < 5; i++) sprintf(hex + i * 8, "%08lx", (unsigned long)st[i]);
    hex[40] = '\0';
}

int main(int argc, char **argv)
{
    int check = 0, bin = 0, tag = 0, quiet = 0, status = 0, i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (!strcmp(argv[i], "--")) { i++; break; }
        if (!strcmp(argv[i], "--check") || !strcmp(argv[i], "-c")) check = 1;
        else if (!strcmp(argv[i], "--tag")) tag = 1;
        else if (!strcmp(argv[i], "--quiet")) quiet = 1;
        else if (!strcmp(argv[i], "--status")) status = 1;
        else if (!strcmp(argv[i], "-b")) bin = 1; else if (!strcmp(argv[i], "-t")) bin = 0;
    }
    int nf = argc - i, rc = 0; char hex[41];
    if (check) {
        int bad = 0;
        for (int k = 0; k < (nf > 0 ? nf : 1); k++) {
            FILE *cf = (nf > 0 && strcmp(argv[i + k], "-")) ? fopen(argv[i + k], "rb") : stdin;
            if (!cf) return 1;
            char line[256];
            while (fgets(line, sizeof line, cf)) {
                size_t l = strlen(line); while (l && (line[l - 1] == '\n' || line[l - 1] == '\r')) line[--l] = '\0';
                char *sp = strchr(line, ' '); if (!sp) continue;
                *sp++ = '\0'; while (*sp == ' ' || *sp == '*') sp++;
                FILE *tf = strcmp(sp, "-") ? fopen(sp, "rb") : stdin;
                if (!tf) { if (!status) printf("%s: FAILED open or read\n", sp); bad++; continue; }
                sha1_hex(tf, hex); if (tf != stdin) fclose(tf);
                if (strcasecmp(line, hex) == 0) { if (!quiet && !status) printf("%s: OK\n", sp); }
                else { if (!status) printf("%s: FAILED\n", sp); bad++; }
            }
            if (cf != stdin) fclose(cf);
        }
        return bad ? 1 : 0;
    }
    for (int k = 0; k < (nf > 0 ? nf : 1); k++) {
        const char *name = nf > 0 ? argv[i + k] : "-";
        FILE *f = strcmp(name, "-") ? fopen(name, "rb") : stdin;
        if (!f) { fprintf(stderr, "sha1sum: %s: %s\n", name, strerror(errno)); rc = 1; continue; }
        sha1_hex(f, hex); if (f != stdin) fclose(f);
        if (tag) printf("SHA1 (%s) = %s\n", name, hex);
        else printf("%s %c%s\n", hex, bin ? '*' : ' ', name);
    }
    return rc;
}
