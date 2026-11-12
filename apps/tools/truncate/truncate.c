/* truncate - shrink or extend the size of a file to the specified size. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

static int parse_size(const char *s, unsigned long long *out)
{
    if (!s || !*s) return -1;
    char *end = NULL;
    unsigned long long v = strtoull(s, &end, 10);
    if (end == s) return -1;
    unsigned long long mul = 1;
    if (*end) {
        int base = 1024;
        if ((end[1] == 'B' || end[1] == 'b') && !end[2] && tolower((unsigned char)end[0]) != 'i') base = 1000;
        switch (tolower((unsigned char)*end)) {
        case 'b': mul = 512ULL; end++; break;
        case 'c': mul = 1ULL; end++; break;
        case 'w': mul = 2ULL; end++; break;
        case 'k': mul = (unsigned long long)base; end++; break;
        case 'm': mul = (unsigned long long)base * base; end++; break;
        case 'g': mul = (unsigned long long)base * base * base; end++; break;
        default: return -1;
        }
        if (*end == 'B' || *end == 'b') end++;
        else if ((*end == 'i' || *end == 'I') && (end[1] == 'B' || end[1] == 'b')) end += 2;
        if (*end) return -1;
    }
    *out = v * mul;
    return 0;
}

int main(int argc, char **argv)
{
    int no_create = 0, i = 1;
    const char *size_spec = NULL, *ref_file = NULL;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) { size_spec = argv[++i]; continue; }
        if (strcmp(argv[i], "-r") == 0 && i + 1 < argc) { ref_file = argv[++i]; continue; }
        for (const char *p = argv[i] + 1; *p; p++) {
            if (*p == 'c') no_create = 1;
            else if (*p == 's' && p[1]) { size_spec = p + 1; break; }
        }
    }
    if ((!size_spec && !ref_file) || i >= argc) {
        fprintf(stderr, "truncate: usage: truncate -s SIZE FILE...\n");
        return 1;
    }
    off_t base_ref = 0;
    if (ref_file) {
        struct stat st;
        if (stat(ref_file, &st) != 0) { fprintf(stderr, "truncate: '%s': %s\n", ref_file, strerror(errno)); return 1; }
        base_ref = st.st_size;
    }
    int rc = 0;
    for (; i < argc; i++) {
        struct stat st;
        if (stat(argv[i], &st) != 0) {
            if (no_create) continue;
            int fd = open(argv[i], O_WRONLY | O_CREAT, 0666);
            if (fd < 0) { fprintf(stderr, "truncate: '%s': %s\n", argv[i], strerror(errno)); rc = 1; continue; }
            close(fd);
            st.st_size = 0;
        }
        off_t target = base_ref;
        if (size_spec) {
            char op = 0; const char *ns = size_spec;
            if (ns[0] == '+' || ns[0] == '-' || ns[0] == '<' || ns[0] == '>' || ns[0] == '/' || ns[0] == '%') {
                op = ns[0]; ns++;
            }
            unsigned long long val = 0;
            if (parse_size(ns, &val) != 0) { fprintf(stderr, "truncate: invalid size '%s'\n", size_spec); return 1; }
            off_t cur = ref_file ? base_ref : st.st_size;
            if (op == '+') target = cur + (off_t)val;
            else if (op == '-') target = cur > (off_t)val ? cur - (off_t)val : 0;
            else if (op == '<') target = cur < (off_t)val ? cur : (off_t)val;
            else if (op == '>') target = cur > (off_t)val ? cur : (off_t)val;
            else if (op == '/' && val) target = (off_t)((cur / (off_t)val) * (off_t)val);
            else if (op == '%' && val) target = (off_t)(((cur + (off_t)val - 1) / (off_t)val) * (off_t)val);
            else target = (off_t)val;
        }
        if (truncate(argv[i], target) != 0) { fprintf(stderr, "truncate: '%s': %s\n", argv[i], strerror(errno)); rc = 1; }
    }
    return rc;
}
