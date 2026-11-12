/* mknod - make block or character special files. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

int main(int argc, char **argv)
{
    mode_t mode = 0666;
    const char *ms = NULL;
    int i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) { ms = argv[++i]; continue; }
    }
    if (ms) mode = (mode_t)strtoul(ms, NULL, 8) & 07777;
    if (i + 1 >= argc) { fprintf(stderr, "mknod: missing operand\n"); return 1; }
    const char *name = argv[i];
    char type = argv[i + 1][0];
    mode_t kind = S_IFIFO;
    if (type == 'p') kind = S_IFIFO;
    else if (type == 'c' || type == 'u') kind = S_IFCHR;
    else if (type == 'b') kind = S_IFBLK;
    else { fprintf(stderr, "mknod: invalid device type '%s'\n", argv[i + 1]); return 1; }
    unsigned maj = (i + 2 < argc) ? (unsigned)strtoul(argv[i + 2], NULL, 0) : 0;
    unsigned min = (i + 3 < argc) ? (unsigned)strtoul(argv[i + 3], NULL, 0) : 0;
    dev_t dev = ((maj & 0xffu) << 8) | (min & 0xffu);
    if (mknod(name, kind | mode, dev) != 0) {
        fprintf(stderr, "mknod: '%s': %s\n", name, strerror(errno));
        return 1;
    }
    return 0;
}
