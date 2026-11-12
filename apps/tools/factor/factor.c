/* factor - factor numbers. */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

static void print_factors(unsigned long long n)
{
    printf("%llu:", n);
    unsigned long long t = n;
    for (unsigned long long d = 2; d * d <= t; d += (d == 2 ? 1 : 2)) {
        while (t % d == 0) { printf(" %llu", d); t /= d; }
    }
    if (t > 1) printf(" %llu", t);
    putchar('\n');
}

int main(int argc, char **argv)
{
    if (argc > 1) {
        for (int i = 1; i < argc; i++) print_factors(strtoull(argv[i], NULL, 10));
        return 0;
    }
    char line[256];
    while (fgets(line, sizeof line, stdin)) {
        char *p = line;
        while (*p) {
            while (*p && isspace((unsigned char)*p)) p++;
            if (!*p) break;
            print_factors(strtoull(p, &p, 10));
        }
    }
    return 0;
}
