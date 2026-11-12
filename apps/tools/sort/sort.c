/* sort - sort lines of text files. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static int g_rev = 0, g_num = 0, g_icase = 0, g_blank = 0, g_key = 0;
static char g_delim = 0;

static void get_field(const char *line, int field1, char delim, char *out, size_t cap)
{
    const char *p = line; int cur = 1; out[0] = '\0';
    if (!delim) {
        while (*p) {
            while (*p == ' ' || *p == '\t') p++;
            if (!*p) break;
            const char *s = p;
            while (*p && *p != ' ' && *p != '\t') p++;
            if (cur == field1) { size_t l = (size_t)(p - s); if (l >= cap) l = cap - 1; memcpy(out, s, l); out[l] = '\0'; return; }
            cur++;
        }
    } else {
        while (*p) {
            const char *nx = strchr(p, delim);
            size_t l = nx ? (size_t)(nx - p) : strlen(p);
            if (cur == field1) { if (l >= cap) l = cap - 1; memcpy(out, p, l); out[l] = '\0'; return; }
            if (!nx) break;
            p = nx + 1;
            cur++;
        }
    }
}

static const char *extract_key(const char *s, char *buf, size_t cap)
{
    if (g_blank) while (*s == ' ' || *s == '\t') s++;
    if (g_key <= 0) return s;
    get_field(s, g_key, g_delim, buf, cap);
    return buf;
}

static int sort_cmp(const void *pa, const void *pb)
{
    const char *a = *(const char *const *)pa, *b = *(const char *const *)pb;
    char ka_b[160], kb_b[160];
    const char *ka = extract_key(a, ka_b, sizeof ka_b);
    const char *kb = extract_key(b, kb_b, sizeof kb_b);
    int r = 0;
    if (g_num) { long va = strtol(ka, NULL, 10), vb = strtol(kb, NULL, 10); r = (va < vb) ? -1 : (va > vb) ? 1 : 0; }
    if (r == 0) r = g_icase ? strcasecmp(ka, kb) : strcmp(ka, kb);
    if (r == 0 && g_key > 0) r = g_icase ? strcasecmp(a, b) : strcmp(a, b);
    return g_rev ? -r : r;
}

int main(int argc, char **argv)
{
    int uniq_flag = 0, check_only = 0, i = 1;
    const char *out_path = NULL;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) { g_delim = argv[++i][0]; continue; }
        if (strcmp(argv[i], "-k") == 0 && i + 1 < argc) { g_key = atoi(argv[++i]); continue; }
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) { out_path = argv[++i]; continue; }
        for (const char *p = argv[i] + 1; *p; p++) {
            if (*p == 'r') g_rev = 1;
            else if (*p == 'n' || *p == 'g' || *p == 'h') g_num = 1;
            else if (*p == 'f') g_icase = 1;
            else if (*p == 'b') g_blank = 1;
            else if (*p == 'u') uniq_flag = 1;
            else if (*p == 'c' || *p == 'C') check_only = 1;
            else if (*p == 'k' && p[1]) { g_key = atoi(p + 1); break; }
            else if (*p == 't' && p[1]) { g_delim = p[1]; break; }
        }
    }
    size_t cap = 64, n = 0, bcap = 128;
    char **lines = malloc(cap * sizeof *lines);
    char *buf = malloc(bcap);
    int nf = argc - i;
    for (int k = 0; k < (nf > 0 ? nf : 1); k++) {
        FILE *f = (nf > 0 && strcmp(argv[i + k], "-") != 0) ? fopen(argv[i + k], "rb") : stdin;
        if (!f) { fprintf(stderr, "sort: %s: %s\n", argv[i + k], strerror(errno)); return 2; }
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
        if (f != stdin) fclose(f);
    }
    free(buf);
    if (check_only) {
        for (size_t k = 1; k < n; k++) {
            if (sort_cmp(&lines[k - 1], &lines[k]) > 0) {
                fprintf(stderr, "sort: disorder: %s\n", lines[k]); return 1;
            }
        }
        return 0;
    }
    if (n > 1) qsort(lines, n, sizeof *lines, sort_cmp);
    FILE *out = out_path ? fopen(out_path, "w") : stdout;
    if (!out) { fprintf(stderr, "sort: %s: %s\n", out_path, strerror(errno)); return 2; }
    for (size_t k = 0; k < n; k++) {
        if (uniq_flag && k > 0 && sort_cmp(&lines[k - 1], &lines[k]) == 0) continue;
        fprintf(out, "%s\n", lines[k]);
    }
    if (out != stdout) fclose(out);
    for (size_t k = 0; k < n; k++) free(lines[k]);
    free(lines);
    return 0;
}
