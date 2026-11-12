/* b2sum - compute and check BLAKE2b message digest. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

static uint8_t io[1024];
static uint64_t ror64(uint64_t x, int n) { return (x >> n) | (x << (64 - n)); }

static void b2b_compress(uint64_t h[8], const uint8_t block[128], uint64_t t0, int last)
{
    static const uint64_t iv[8] = {
        0x6a09e667f3bcc908ULL,0xbb67ae8584caa73bULL,0x3c6ef372fe94f82bULL,0xa54ff53a5f1d36f1ULL,
        0x510e527fade682d1ULL,0x9b05688c2b3e6c1fULL,0x1f83d9abfb41bd6bULL,0x5be0cd19137e2179ULL
    };
    static const uint8_t s[12][16] = {
        {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15},{14,10,4,8,9,15,13,6,1,12,0,2,11,7,5,3},
        {11,8,12,0,5,2,15,13,10,14,3,6,7,1,9,4},{7,9,3,1,13,12,11,14,2,6,5,10,4,0,15,8},
        {9,0,5,7,2,4,10,15,14,1,11,12,6,8,3,13},{2,12,6,10,0,11,8,3,4,13,7,5,15,14,1,9},
        {12,5,1,15,14,13,4,10,0,7,6,3,9,2,8,11},{13,11,7,14,12,1,3,9,5,0,15,4,8,6,2,10},
        {6,15,14,9,11,3,0,8,12,2,13,7,1,4,10,5},{10,2,8,4,7,6,1,5,15,11,9,14,3,12,13,0},
        {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15},{14,10,4,8,9,15,13,6,1,12,0,2,11,7,5,3}
    };
    uint64_t *m = malloc(16 * sizeof(uint64_t)), *v = malloc(16 * sizeof(uint64_t));
    if (!m || !v) { free(m); free(v); return; }
    for (int i = 0; i < 16; i++) { uint64_t x = 0; for (int j = 7; j >= 0; j--) x = (x << 8) | block[i*8 + j]; m[i] = x; }
    for (int i = 0; i < 8; i++) { v[i] = h[i]; v[i + 8] = iv[i]; }
    v[12] ^= t0; if (last) v[14] = ~v[14];
    #define G(r,i,a,b,c,d) do { \
        v[a] += v[b] + m[s[r][2*(i)]]; v[d] = ror64(v[d] ^ v[a], 32); \
        v[c] += v[d]; v[b] = ror64(v[b] ^ v[c], 24); \
        v[a] += v[b] + m[s[r][2*(i)+1]]; v[d] = ror64(v[d] ^ v[a], 16); \
        v[c] += v[d]; v[b] = ror64(v[b] ^ v[c], 63); \
    } while(0)
    for (int r = 0; r < 12; r++) {
        G(r,0,0,4,8,12); G(r,1,1,5,9,13); G(r,2,2,6,10,14); G(r,3,3,7,11,15);
        G(r,4,0,5,10,15); G(r,5,1,6,11,12); G(r,6,2,7,8,13); G(r,7,3,4,9,14);
    }
    #undef G
    for (int i = 0; i < 8; i++) h[i] ^= v[i] ^ v[i + 8];
    free(m); free(v);
}

static void b2b_hex(FILE *f, char hex[129])
{
    uint64_t h[8] = {
        0x6a09e667f3bcc908ULL ^ 0x01010040ULL, 0xbb67ae8584caa73bULL, 0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL,
        0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL, 0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL
    };
    uint8_t *buf = calloc(1, 128); size_t len = 0, n; uint64_t total = 0;
    while ((n = fread(io, 1, sizeof io, f)) > 0) {
        for (size_t pos = 0; pos < n; pos++) {
            if (len == 128) { total += 128; b2b_compress(h, buf, total, 0); len = 0; }
            buf[len++] = io[pos];
        }
    }
    total += len; while (len < 128) buf[len++] = 0;
    b2b_compress(h, buf, total, 1); free(buf);
    for (int i = 0; i < 8; i++) for (int j = 0; j < 8; j++) sprintf(hex + (i*8 + j) * 2, "%02x", (uint8_t)(h[i] >> (j * 8)));
    hex[128] = '\0';
}

int main(int argc, char **argv)
{
    int bin = 0, tag = 0, i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (!strcmp(argv[i], "--")) { i++; break; }
        if (!strcmp(argv[i], "--tag")) tag = 1; else if (!strcmp(argv[i], "-b")) bin = 1;
    }
    int nf = argc - i, rc = 0; char hex[129];
    for (int k = 0; k < (nf > 0 ? nf : 1); k++) {
        const char *name = nf > 0 ? argv[i + k] : "-";
        FILE *f = strcmp(name, "-") ? fopen(name, "rb") : stdin;
        if (!f) { fprintf(stderr, "b2sum: %s: %s\n", name, strerror(errno)); rc = 1; continue; }
        b2b_hex(f, hex); if (f != stdin) fclose(f);
        if (tag) printf("BLAKE2b (%s) = %s\n", name, hex);
        else printf("%s %c%s\n", hex, bin ? '*' : ' ', name);
    }
    return rc;
}
