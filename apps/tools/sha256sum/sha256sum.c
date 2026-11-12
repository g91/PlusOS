/* sha256sum - compute and check SHA256 message digest. */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

static uint8_t io[1024];
static uint32_t ror32(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

static const uint32_t sha256_k[64] = {
    0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
    0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
    0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
    0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
    0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
    0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
    0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
    0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
};

static void sha256_block(uint32_t s[8], const uint8_t b[64])
{
    uint32_t w[64];
    for (int i = 0; i < 16; i++)
        w[i] = ((uint32_t)b[i*4]<<24)|((uint32_t)b[i*4+1]<<16)|((uint32_t)b[i*4+2]<<8)|b[i*4+3];
    for (int i = 16; i < 64; i++) {
        uint32_t s0 = ror32(w[i-15], 7) ^ ror32(w[i-15], 18) ^ (w[i-15] >> 3);
        uint32_t s1 = ror32(w[i-2], 17) ^ ror32(w[i-2], 19) ^ (w[i-2] >> 10);
        w[i] = w[i-16] + s0 + w[i-7] + s1;
    }
    uint32_t a=s[0],B=s[1],c=s[2],d=s[3],e=s[4],f=s[5],g=s[6],h=s[7];
    for (int i = 0; i < 64; i++) {
        uint32_t S1 = ror32(e, 6) ^ ror32(e, 11) ^ ror32(e, 25);
        uint32_t ch = (e & f) ^ ((~e) & g);
        uint32_t t1 = h + S1 + ch + sha256_k[i] + w[i];
        uint32_t S0 = ror32(a, 2) ^ ror32(a, 13) ^ ror32(a, 22);
        uint32_t maj = (a & B) ^ (a & c) ^ (B & c);
        uint32_t t2 = S0 + maj;
        h = g; g = f; f = e; e = d + t1; d = c; c = B; B = a; a = t1 + t2;
    }
    s[0]+=a; s[1]+=B; s[2]+=c; s[3]+=d; s[4]+=e; s[5]+=f; s[6]+=g; s[7]+=h;
}

static void sha256_hex(FILE *f, char hex[65])
{
    uint32_t st[8] = {0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u};
    uint8_t buf[64]; size_t len = 0, n; uint64_t bits = 0;
    while ((n = fread(io, 1, sizeof io, f)) > 0) {
        bits += (uint64_t)n * 8; size_t pos = 0;
        while (pos < n) {
            size_t t = 64 - len; if (t > n - pos) t = n - pos;
            memcpy(buf + len, io + pos, t); len += t; pos += t;
            if (len == 64) { sha256_block(st, buf); len = 0; }
        }
    }
    buf[len++] = 0x80;
    if (len > 56) { while (len < 64) buf[len++] = 0; sha256_block(st, buf); len = 0; }
    while (len < 56) buf[len++] = 0;
    for (int i = 0; i < 8; i++) buf[56 + i] = (uint8_t)(bits >> (56 - i * 8));
    sha256_block(st, buf);
    for (int i = 0; i < 8; i++) sprintf(hex + i * 8, "%08lx", (unsigned long)st[i]);
    hex[64] = '\0';
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
    int nf = argc - i, rc = 0; char hex[65];
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
                sha256_hex(tf, hex); if (tf != stdin) fclose(tf);
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
        if (!f) { fprintf(stderr, "sha256sum: %s: %s\n", name, strerror(errno)); rc = 1; continue; }
        sha256_hex(f, hex); if (f != stdin) fclose(f);
        if (tag) printf("SHA256 (%s) = %s\n", name, hex);
        else printf("%s %c%s\n", hex, bin ? '*' : ' ', name);
    }
    return rc;
}
