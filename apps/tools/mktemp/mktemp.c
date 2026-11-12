/* mktemp - create a temporary file or directory. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

int main(int argc, char **argv)
{
    int make_dir = 0, dry_run = 0, quiet = 0, use_tmpdir = 0, i = 1;
    const char *tmpdir = getenv("TMPDIR");
    if (!tmpdir || !*tmpdir) tmpdir = "/tmp";
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) { tmpdir = argv[++i]; use_tmpdir = 1; continue; }
        for (const char *p = argv[i] + 1; *p; p++) {
            if (*p == 'd') make_dir = 1; else if (*p == 'u') dry_run = 1;
            else if (*p == 'q') quiet = 1; else if (*p == 't') use_tmpdir = 1;
        }
    }
    const char *tmpl = (i < argc) ? argv[i] : "tmp.XXXXXX";
    if (i >= argc) use_tmpdir = 1;

    char path[192];
    if (use_tmpdir && tmpl[0] != '/') {
        size_t dl = strlen(tmpdir);
        snprintf(path, sizeof path, "%s%s%s", tmpdir, (dl && tmpdir[dl - 1] == '/') ? "" : "/", tmpl);
    } else strlcpy(path, tmpl, sizeof path);

    if (dry_run) {
        srand((unsigned)time(NULL) ^ (unsigned)getpid());
        static const char al[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        for (char *p = path; *p; p++) if (*p == 'X') *p = al[rand() % (int)(sizeof al - 1)];
        printf("%s\n", path);
        return 0;
    }
    if (make_dir) {
        if (!mkdtemp(path)) {
            if (!quiet) fprintf(stderr, "mktemp: failed to create directory via '%s': %s\n", path, strerror(errno));
            return 1;
        }
    } else {
        int fd = mkstemp(path);
        if (fd < 0) {
            if (!quiet) fprintf(stderr, "mktemp: failed to create file via '%s': %s\n", path, strerror(errno));
            return 1;
        }
        close(fd);
    }
    printf("%s\n", path);
    return 0;
}
