/* comm - compare two sorted files line by line. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char **read_lines(FILE *f, size_t *out_n)
{
    size_t cap = 32, n = 0, bcap = 128; char **lines = malloc(cap * sizeof *lines);
    char *buf = malloc(bcap);
    for (;;) {
        size_t len = 0; int c;
        while ((c = fgetc(f)) != EOF) {
            if (len + 2 >= bcap) buf = realloc(buf, (bcap *= 2));
            buf[len++] = (char)c;
            if (c == '\n') break;
        }
        if (len == 0) break;
        if (buf[len - 1] == '\n') buf[--len] = '\0'; else buf[len] = '\0';
        if (n >= cap) lines = realloc(lines, (cap *= 2) * sizeof *lines);
        lines[n++] = strdup(buf);
    }
    free(buf); *out_n = n; return lines;
}

int main(int argc, char **argv)
{
    int sup1 = 0, sup2 = 0, sup3 = 0, i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        for (const char *p = argv[i] + 1; *p; p++) {
            if (*p == '1') sup1 = 1; else if (*p == '2') sup2 = 1; else if (*p == '3') sup3 = 1;
        }
    }
    if (argc - i < 2) { fprintf(stderr, "comm: missing operand\n"); return 1; }
    FILE *fp1 = strcmp(argv[i], "-") ? fopen(argv[i], "rb") : stdin;
    FILE *fp2 = strcmp(argv[i + 1], "-") ? fopen(argv[i + 1], "rb") : stdin;
    if (!fp1 || !fp2) { fprintf(stderr, "comm: cannot open files\n"); return 1; }
    size_t n1 = 0, n2 = 0; char **l1 = read_lines(fp1, &n1), **l2 = read_lines(fp2, &n2);
    if (fp1 != stdin) fclose(fp1);
    if (fp2 != stdin) fclose(fp2);
    size_t a = 0, b = 0;
    while (a < n1 || b < n2) {
        int c = (a >= n1) ? 1 : (b >= n2) ? -1 : strcmp(l1[a], l2[b]);
        if (c < 0) { if (!sup1) puts(l1[a]); a++; }
        else if (c > 0) { if (!sup2) printf("%s%s\n", sup1 ? "" : "\t", l2[b]); b++; }
        else { if (!sup3) printf("%s%s%s\n", sup1 ? "" : "\t", sup2 ? "" : "\t", l1[a]); a++; b++; }
    }
    for (size_t k = 0; k < n1; k++) free(l1[k]);
    free(l1);
    for (size_t k = 0; k < n2; k++) free(l2[k]);
    free(l2);
    return 0;
}
