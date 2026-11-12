/* shuf - generate random permutations. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

int main(int argc, char **argv)
{
    int echo_mode = 0, has_range = 0, i = 1;
    long head_n = -1, lo = 0, hi = -1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) { head_n = atol(argv[++i]); continue; }
        if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            const char *rs = argv[++i]; lo = atol(rs);
            const char *dash = strchr(rs, '-'); if (dash) hi = atol(dash + 1);
            has_range = 1; continue;
        }
        if (strcmp(argv[i], "-e") == 0) echo_mode = 1;
    }
    srand((unsigned)time(NULL) ^ (unsigned)getpid());
    size_t n = 0; char **items = NULL;
    if (has_range && hi >= lo) {
        n = (size_t)(hi - lo + 1); if (n > 2048) n = 2048;
        items = malloc(n * sizeof *items);
        for (size_t k = 0; k < n; k++) { items[k] = malloc(24); snprintf(items[k], 24, "%ld", lo + (long)k); }
    } else if (echo_mode) {
        n = (size_t)(argc - i); items = malloc((n ? n : 1) * sizeof *items);
        for (size_t k = 0; k < n; k++) items[k] = strdup(argv[i + (int)k]);
    } else {
        FILE *f = (i < argc && strcmp(argv[i], "-") != 0) ? fopen(argv[i], "rb") : stdin;
        if (!f) { fprintf(stderr, "shuf: %s: %s\n", argv[i], strerror(errno)); return 1; }
        size_t cap = 32, bcap = 128; items = malloc(cap * sizeof *items); char *buf = malloc(bcap);
        for (;;) {
            size_t len = 0; int c;
            while ((c = fgetc(f)) != EOF) {
                if (len + 2 >= bcap) buf = realloc(buf, (bcap *= 2));
                buf[len++] = (char)c; if (c == '\n') break;
            }
            if (len == 0) break;
            if (buf[len - 1] == '\n') buf[--len] = '\0'; else buf[len] = '\0';
            if (n >= cap) items = realloc(items, (cap *= 2) * sizeof *items);
            items[n++] = strdup(buf);
        }
        free(buf); if (f != stdin) fclose(f);
    }
    for (size_t k = n; k > 1; k--) {
        size_t j = (size_t)rand() % k;
        char *t = items[k - 1]; items[k - 1] = items[j]; items[j] = t;
    }
    size_t out_n = (head_n >= 0 && (size_t)head_n < n) ? (size_t)head_n : n;
    for (size_t k = 0; k < out_n; k++) puts(items[k]);
    for (size_t k = 0; k < n; k++) free(items[k]);
    free(items);
    return 0;
}
