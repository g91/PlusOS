/* od - dump files in octal and other formats. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <errno.h>

int main(int argc, char **argv)
{
    char addr_radix = 'o', out_type = 'o';
    int unit_sz = 2, i = 1;
    long skip = 0, limit = -1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        if (strcmp(argv[i], "-A") == 0 && i + 1 < argc) { addr_radix = argv[++i][0]; continue; }
        if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            const char *ts = argv[++i]; out_type = ts[0];
            if (isdigit((unsigned char)ts[1])) unit_sz = ts[1] - '0';
            else if (out_type == 'c' || out_type == 'a') unit_sz = 1;
            continue;
        }
        if (strcmp(argv[i], "-j") == 0 && i + 1 < argc) { skip = atol(argv[++i]); continue; }
        if (strcmp(argv[i], "-N") == 0 && i + 1 < argc) { limit = atol(argv[++i]); continue; }
        for (const char *p = argv[i] + 1; *p; p++) {
            if (*p == 'x') { out_type = 'x'; unit_sz = 2; }
            else if (*p == 'o') { out_type = 'o'; unit_sz = 2; }
            else if (*p == 'd') { out_type = 'u'; unit_sz = 2; }
            else if (*p == 'c') { out_type = 'c'; unit_sz = 1; }
            else if (*p == 'b') { out_type = 'o'; unit_sz = 1; }
        }
    }
    int nf = argc - i;
    unsigned long offset = 0, read_total = 0;
    unsigned char buf[16]; int bpos = 0;
    for (int k = 0; k < (nf > 0 ? nf : 1); k++) {
        FILE *f = (nf > 0 && strcmp(argv[i + k], "-") != 0) ? fopen(argv[i + k], "rb") : stdin;
        if (!f) { fprintf(stderr, "od: %s: %s\n", argv[i + k], strerror(errno)); return 1; }
        int c;
        while ((c = fgetc(f)) != EOF) {
            if (skip > 0) { skip--; offset++; continue; }
            if (limit >= 0 && (long)read_total >= limit) break;
            buf[bpos++] = (unsigned char)c; read_total++;
            if (bpos == 16) {
                if (addr_radix == 'x') printf("%07lx", offset);
                else if (addr_radix == 'd') printf("%07lu", offset);
                else if (addr_radix != 'n') printf("%07lo", offset);
                for (int j = 0; j < 16; j += unit_sz) {
                    if (out_type == 'c') {
                        unsigned char ch = buf[j];
                        if (ch == '\n') printf("  \\n");
                        else if (ch == '\t') printf("  \\t");
                        else if (ch >= 32 && ch < 127) printf("   %c", ch);
                        else printf(" %03o", ch);
                    } else if (unit_sz == 1) {
                        if (out_type == 'x') printf(" %02x", buf[j]);
                        else if (out_type == 'd' || out_type == 'u') printf(" %3u", buf[j]);
                        else printf(" %03o", buf[j]);
                    } else if (unit_sz == 2) {
                        uint16_t w = (uint16_t)((buf[j] << 8) | buf[j + 1]);
                        if (out_type == 'x') printf(" %04x", w);
                        else if (out_type == 'd') printf(" %6d", (int16_t)w);
                        else if (out_type == 'u') printf(" %5u", w);
                        else printf(" %06o", w);
                    } else {
                        uint32_t w = ((uint32_t)buf[j] << 24) | ((uint32_t)buf[j + 1] << 16) | ((uint32_t)buf[j + 2] << 8) | buf[j + 3];
                        if (out_type == 'x') printf(" %08lx", (unsigned long)w);
                        else printf(" %10lu", (unsigned long)w);
                    }
                }
                putchar('\n'); offset += 16; bpos = 0;
            }
        }
        if (f != stdin) fclose(f);
    }
    if (bpos > 0) {
        if (addr_radix == 'x') printf("%07lx", offset);
        else if (addr_radix == 'd') printf("%07lu", offset);
        else if (addr_radix != 'n') printf("%07lo", offset);
        for (int j = 0; j < bpos; j += unit_sz) {
            if (out_type == 'c') {
                unsigned char ch = buf[j];
                if (ch == '\n') printf("  \\n"); else if (ch >= 32 && ch < 127) printf("   %c", ch); else printf(" %03o", ch);
            } else if (unit_sz == 1 || j + 1 >= bpos) {
                if (out_type == 'x') printf(" %02x", buf[j]); else printf(" %03o", buf[j]);
            } else {
                uint16_t w = (uint16_t)((buf[j] << 8) | buf[j + 1]);
                if (out_type == 'x') printf(" %04x", w); else printf(" %06o", w);
            }
        }
        putchar('\n'); offset += (unsigned long)bpos;
    }
    if (addr_radix == 'x') printf("%07lx\n", offset);
    else if (addr_radix == 'd') printf("%07lu\n", offset);
    else if (addr_radix != 'n') printf("%07lo\n", offset);
    return 0;
}
