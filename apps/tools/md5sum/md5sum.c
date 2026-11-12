/* md5sum - compute and check MD5 message digest. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

static uint8_t io[1024];
static uint32_t rol32(uint32_t x, int n) { return (x << n) | (x >> (32 - n)); }

static void md5_block(uint32_t s[4], const uint8_t b[64])
{
    static const uint8_t r[64] = {
        7,12,17,22,7,12,17,22,7,12,17,22,7,12,17,22, 5,9,14,20,5,9,14,20,5,9,14,20,5,9,14,20,
        4,11,16,23,4,11,16,23,4,11,16,23,4,11,16,23, 6,10,15,21,6,10,15,21,6,10,15,21,6,10,15,21
    };
    static const uint32_t k[64] = {
        0xd76aa478u,0xe8c7b756u,0x242070dbu,0xc1bdceeeu,0xf57c0fafu,0x4787c62au,0xa8304613u,0xfd469501u,
        0x698098d8u,0x8b44f7afu,0xffff5bb1u,0x895cd7beu,0x6b901122u,0xfd987193u,0xa679438eu,0x49b40821u,
        0xf61e2562u,0xc040b340u,0x265e5a51u,0xe9b6c7aau,0xd62f105du,0x02441453u,0xd8a1e681u,0xe7d3fbc8u,
        0x21e1cde6u,0xc33707d6u,0xf4d50d87u,0x455a14edu,0xa9e3e905u,0xfcefa3f8u,0x676f02d9u,0x8d2a4c8au,
        0xfffa3942u,0x8771f681u,0x6d9d6122u,0xfde5380cu,0xa4beea44u,0x4bdecfa9u,0xf6bb4b60u,0xbebfbc70u,
        0x289b7ec6u,0xeaa127fau,0xd4ef3085u,0x04881d05u,0xd9d4d039u,0xe6db99e5u,0x1fa27cf8u,0xc4ac5665u,
        0xf4292244u,0x432aff97u,0xab9423a7u,0xfc93a039u,0x655b59c3u,0x8f0ccc92u,0xffeff47du,0x85845dd1u,
        0x6fa87e4fu,0xfe2ce6e0u,0xa3014314u,0x4e0811a1u,0xf7537e82u,0xbd3af235u,0x2ad7d2bbu,0xeb86d391u
    };
    uint32_t w[16];
    for (int i = 0; i < 16; i++)
        w[i] = (uint32_t)b[i*4] | ((uint32_t)b[i*4+1]<<8) | ((uint32_t)b[i*4+2]<<16) | ((uint32_t)b[i*4+3]<<24);
    uint32_t a=s[0], B=s[1], c=s[2], d=s[3];
    for (int i = 0; i < 64; i++) {
        uint32_t f, g;
        if (i < 16) { f = (B & c) | ((~B) & d); g = (uint32_t)i; }
        else if (i < 32) { f = (d & B) | ((~d) & c); g = (uint32_t)((5*i + 1) & 15); }
        else if (i < 48) { f = B ^ c ^ d; g = (uint32_t)((3*i + 5) & 15); }
        else { f = c ^ (B | (~d)); g = (uint32_t)((7*i) & 15); }
        uint32_t tmp = d; d = c; c = B; B = B + rol32(a + f + k[i] + w[g], r[i]); a = tmp;
    }
    s[0]+=a; s[1]+=B; s[2]+=c; s[3]+=d;
}

static void md5_hex(FILE *f, char hex[33])
{
    uint32_t st[4] = { 0x67452301u, 0xefcdab89u, 0x98badcfeu, 0x10325476u };
    uint8_t buf[64]; size_t len = 0, n; uint64_t bits = 0;
    while ((n = fread(io, 1, sizeof io, f)) > 0) {
        bits += (uint64_t)n * 8; size_t pos = 0;
        while (pos < n) {
            size_t t = 64 - len; if (t > n - pos) t = n - pos;
            memcpy(buf + len, io + pos, t); len += t; pos += t;
            if (len == 64) { md5_block(st, buf); len = 0; }
        }
    }
    buf[len++] = 0x80;
    if (len > 56) { while (len < 64) buf[len++] = 0; md5_block(st, buf); len = 0; }
    while (len < 56) buf[len++] = 0;
    for (int i = 0; i < 8; i++) buf[56 + i] = (uint8_t)(bits >> (i * 8));
    md5_block(st, buf);
    uint8_t out[16];
    for (int i = 0; i < 4; i++) {
        out[i*4]=(uint8_t)st[i]; out[i*4+1]=(uint8_t)(st[i]>>8);
        out[i*4+2]=(uint8_t)(st[i]>>16); out[i*4+3]=(uint8_t)(st[i]>>24);
    }
    for (int i = 0; i < 16; i++) sprintf(hex + i * 2, "%02x", out[i]);
    hex[32] = '\0';
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
        else if (!strcmp(argv[i], "-b")) bin = 1;
        else if (!strcmp(argv[i], "-t")) bin = 0;
    }
    int nf = argc - i, rc = 0; char hex[33];
    if (check) {
        int bad = 0;
        for (int k = 0; k < (nf > 0 ? nf : 1); k++) {
            FILE *cf = (nf > 0 && strcmp(argv[i + k], "-")) ? fopen(argv[i + k], "rb") : stdin;
            if (!cf) { fprintf(stderr, "md5sum: %s: %s\n", argv[i + k], strerror(errno)); return 1; }
            char line[256];
            while (fgets(line, sizeof line, cf)) {
                size_t l = strlen(line); while (l && (line[l - 1] == '\n' || line[l - 1] == '\r')) line[--l] = '\0';
                char *sp = strchr(line, ' '); if (!sp) continue;
                *sp++ = '\0'; while (*sp == ' ' || *sp == '*') sp++;
                FILE *tf = strcmp(sp, "-") ? fopen(sp, "rb") : stdin;
                if (!tf) { if (!status) printf("%s: FAILED open or read\n", sp); bad++; continue; }
                md5_hex(tf, hex); if (tf != stdin) fclose(tf);
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
        if (!f) { fprintf(stderr, "md5sum: %s: %s\n", name, strerror(errno)); rc = 1; continue; }
        md5_hex(f, hex); if (f != stdin) fclose(f);
        if (tag) printf("MD5 (%s) = %s\n", name, hex);
        else printf("%s %c%s\n", hex, bin ? '*' : ' ', name);
    }
    return rc;
}
