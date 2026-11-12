/* basename - strip directory and suffix from filenames. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_base(const char *path, const char *suffix, char term)
{
    char *tmp = strdup(path ? path : "");
    if (!tmp) return;
    size_t len = strlen(tmp);
    while (len > 1 && tmp[len - 1] == '/') tmp[--len] = '\0';
    if (strcmp(tmp, "/") == 0) {
        printf("/%c", term);
        free(tmp);
        return;
    }
    char *sl = strrchr(tmp, '/');
    const char *base = sl ? sl + 1 : tmp;
    if (suffix && *suffix) {
        size_t bl = strlen(base), slen = strlen(suffix);
        if (bl > slen && strcmp(base + bl - slen, suffix) == 0)
            ((char *)base)[bl - slen] = '\0';
    }
    printf("%s%c", base, term);
    free(tmp);
}

int main(int argc, char **argv)
{
    int multi = 0;
    char term = '\n';
    const char *suffix = NULL;
    int i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--multiple") == 0) { multi = 1; continue; }
        if (strcmp(argv[i], "-z") == 0 || strcmp(argv[i], "--zero") == 0) { term = '\0'; continue; }
        if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) { suffix = argv[++i]; multi = 1; continue; }
        if (strncmp(argv[i], "--suffix=", 9) == 0) { suffix = argv[i] + 9; multi = 1; continue; }
        fprintf(stderr, "basename: invalid option -- '%s'\n", argv[i]);
        return 1;
    }
    if (i >= argc) { fprintf(stderr, "basename: missing operand\n"); return 1; }
    if (!multi && i + 1 < argc) {
        print_base(argv[i], argv[i + 1], term);
        return 0;
    }
    for (; i < argc; i++) print_base(argv[i], suffix, term);
    return 0;
}
