/* sleep - delay for a specified amount of time. */
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc < 2) { fprintf(stderr, "sleep: missing operand\n"); return 1; }
    unsigned long long total_us = 0;
    for (int i = 1; i < argc; i++) {
        const char *s = argv[i];
        unsigned long long whole = 0, frac = 0, div = 1;
        while (isdigit((unsigned char)*s)) whole = whole * 10 + (unsigned)(*s++ - '0');
        if (*s == '.') {
            s++;
            while (isdigit((unsigned char)*s) && div < 1000000ULL) { frac = frac * 10 + (unsigned)(*s++ - '0'); div *= 10; }
            while (isdigit((unsigned char)*s)) s++;
        }
        unsigned long long mul = 1;
        if (*s == 'm') mul = 60; else if (*s == 'h') mul = 3600; else if (*s == 'd') mul = 86400;
        total_us += (whole * 1000000ULL + (frac * 1000000ULL) / div) * mul;
    }
    unsigned int sec = (unsigned int)(total_us / 1000000ULL);
    unsigned long rem_us = (unsigned long)(total_us % 1000000ULL);
    if (sec) sleep(sec);
    if (rem_us) usleep(rem_us);
    return 0;
}
