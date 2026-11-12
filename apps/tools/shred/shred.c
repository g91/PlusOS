/* shred - overwrite a file to hide its contents, and optionally delete it. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

static char g_buf[1024];

int main(int argc, char **argv)
{
    int passes = 3, zero_pass = 0, remove_file = 0, verbose = 0, i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) { passes = atoi(argv[++i]); continue; }
        for (const char *p = argv[i] + 1; *p; p++) {
            if (*p == 'u') remove_file = 1; else if (*p == 'z') zero_pass = 1;
            else if (*p == 'v') verbose = 1;
        }
    }
    if (i >= argc) { fprintf(stderr, "shred: missing file operand\n"); return 1; }
    srand((unsigned)time(NULL));
    int rc = 0;
    for (; i < argc; i++) {
        struct stat st;
        if (stat(argv[i], &st) != 0) { fprintf(stderr, "shred: '%s': %s\n", argv[i], strerror(errno)); rc = 1; continue; }
        int fd = open(argv[i], O_WRONLY);
        if (fd < 0) { fprintf(stderr, "shred: '%s': %s\n", argv[i], strerror(errno)); rc = 1; continue; }
        off_t sz = st.st_size > 0 ? st.st_size : 512;
        int total_passes = passes + (zero_pass ? 1 : 0);
        for (int p = 0; p < total_passes; p++) {
            int is_zero = (zero_pass && p == total_passes - 1);
            lseek(fd, 0, SEEK_SET);
            if (is_zero) memset(g_buf, 0, sizeof g_buf);
            else for (size_t k = 0; k < sizeof g_buf; k++) g_buf[k] = (char)(rand() & 0xff);
            off_t rem = sz;
            while (rem > 0) {
                size_t chunk = rem > (off_t)sizeof g_buf ? sizeof g_buf : (size_t)rem;
                write(fd, g_buf, chunk); rem -= (off_t)chunk;
            }
            if (verbose) fprintf(stderr, "shred: %s: pass %d/%d (%s)...\n",
                                 argv[i], p + 1, total_passes, is_zero ? "000000" : "random");
        }
        close(fd);
        if (remove_file) unlink(argv[i]);
    }
    return rc;
}
