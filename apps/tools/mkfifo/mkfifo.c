/* mkfifo - make FIFOs (named pipes). */
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
    if (i >= argc) { fprintf(stderr, "mkfifo: missing operand\n"); return 1; }
    int rc = 0;
    for (; i < argc; i++) {
        if (mkfifo(argv[i], mode) != 0) {
            fprintf(stderr, "mkfifo: cannot create fifo '%s': %s\n", argv[i], strerror(errno));
            rc = 1;
        }
    }
    return rc;
}
