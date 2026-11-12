/* cksum - checksum and count the bytes in a file. */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

static uint32_t tab[256];
static uint8_t io[1024];

int main(int argc, char **argv)
{
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i << 24;
        for (int b = 0; b < 8; b++) c = (c & 0x80000000u) ? ((c << 1) ^ 0x04C11DB7u) : (c << 1);
        tab[i] = c;
    }
    int i = 1; if (i < argc && strcmp(argv[i], "--") == 0) i++;
    int nf = argc - i, rc = 0;
    for (int k = 0; k < (nf > 0 ? nf : 1); k++) {
        const char *name = nf > 0 ? argv[i + k] : NULL;
        FILE *f = (name && strcmp(name, "-") != 0) ? fopen(name, "rb") : stdin;
        if (!f) { fprintf(stderr, "cksum: %s: %s\n", name, strerror(errno)); rc = 1; continue; }
        uint32_t crc = 0; unsigned long long len = 0; size_t n;
        while ((n = fread(io, 1, sizeof io, f)) > 0) {
            len += n;
            for (size_t j = 0; j < n; j++) crc = (crc << 8) ^ tab[((crc >> 24) ^ io[j]) & 0xff];
        }
        unsigned long long t = len;
        while (t > 0) { crc = (crc << 8) ^ tab[((crc >> 24) ^ (uint8_t)(t & 0xff)) & 0xff]; t >>= 8; }
        crc = ~crc;
        if (name) printf("%lu %llu %s\n", (unsigned long)crc, len, name);
        else printf("%lu %llu\n", (unsigned long)crc, len);
        if (f != stdin) fclose(f);
    }
    return rc;
}
