/* dirname - strip last component from file name. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    char term = '\n';
    int i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        if (strcmp(argv[i], "-z") == 0 || strcmp(argv[i], "--zero") == 0) { term = '\0'; continue; }
        fprintf(stderr, "dirname: invalid option -- '%s'\n", argv[i]);
        return 1;
    }
    if (i >= argc) { fprintf(stderr, "dirname: missing operand\n"); return 1; }
    for (; i < argc; i++) {
        char *tmp = strdup(argv[i]);
        if (!tmp) return 1;
        size_t len = strlen(tmp);
        while (len > 1 && tmp[len - 1] == '/') tmp[--len] = '\0';
        char *sl = strrchr(tmp, '/');
        if (!sl) printf(".%c", term);
        else {
            while (sl > tmp && *sl == '/') sl--;
            if (sl == tmp && *sl == '/') printf("/%c", term);
            else { sl[1] = '\0'; printf("%s%c", tmp, term); }
        }
        free(tmp);
    }
    return 0;
}
