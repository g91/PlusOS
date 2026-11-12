/* chmod - change file mode bits. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

static int parse_mode(const char *spec, mode_t cur_mode, int is_dir, mode_t *out)
{
    if (!spec || !*spec) return -1;
    if (spec[0] >= '0' && spec[0] <= '7') {
        char *end = NULL;
        unsigned long v = strtoul(spec, &end, 8);
        if (*end) return -1;
        *out = (mode_t)(v & 07777);
        return 0;
    }
    mode_t m = cur_mode & 07777;
    const char *p = spec;
    while (*p) {
        mode_t who = 0;
        while (*p == 'u' || *p == 'g' || *p == 'o' || *p == 'a') {
            if (*p == 'u') who |= 0700; else if (*p == 'g') who |= 0070;
            else if (*p == 'o') who |= 0007; else who |= 0777;
            p++;
        }
        if (!who) who = 0777;
        while (*p == '+' || *p == '-' || *p == '=') {
            char op = *p++; mode_t bits = 0;
            while (*p == 'r' || *p == 'w' || *p == 'x' || *p == 'X') {
                if (*p == 'r') bits |= 0444;
                else if (*p == 'w') bits |= 0222;
                else if (*p == 'x' || (*p == 'X' && (is_dir || (m & 0111)))) bits |= 0111;
                p++;
            }
            bits &= who;
            if (op == '+') m |= bits; else if (op == '-') m &= ~bits; else m = (m & ~who) | bits;
        }
        if (*p == ',') { p++; continue; }
        if (*p) return -1;
    }
    *out = m;
    return 0;
}

static int apply_chmod(const char *path, const char *spec, int rec, int verbose)
{
    struct stat st;
    if (stat(path, &st) != 0) { fprintf(stderr, "chmod: '%s': %s\n", path, strerror(errno)); return 1; }
    mode_t nmode = 0;
    if (parse_mode(spec, st.st_mode & 07777, S_ISDIR(st.st_mode), &nmode) != 0) {
        fprintf(stderr, "chmod: invalid mode: '%s'\n", spec); return 1;
    }
    if (chmod(path, nmode) != 0) { fprintf(stderr, "chmod: '%s': %s\n", path, strerror(errno)); return 1; }
    if (verbose) printf("mode of '%s' changed to %04o\n", path, (unsigned)nmode);
    int rc = 0;
    if (rec && S_ISDIR(st.st_mode)) {
        DIR *d = opendir(path);
        if (d) {
            struct dirent *de;
            while ((de = readdir(d)) != NULL) {
                if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
                char *sub = malloc(192);
                if (!sub) { rc = 1; break; }
                size_t pl = strlen(path);
                snprintf(sub, 192, "%s%s%s", path, (pl && path[pl - 1] != '/') ? "/" : "", de->d_name);
                rc |= apply_chmod(sub, spec, 1, verbose);
                free(sub);
            }
            closedir(d);
        }
    }
    return rc;
}

int main(int argc, char **argv)
{
    int rec = 0, verbose = 0, i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        int ok = 1;
        for (const char *p = argv[i] + 1; *p; p++)
            if (*p != 'R' && *p != 'v' && *p != 'f' && *p != 'c') { ok = 0; break; }
        if (!ok) break;
        for (const char *p = argv[i] + 1; *p; p++) {
            if (*p == 'R') rec = 1; else if (*p == 'v' || *p == 'c') verbose = 1;
        }
    }
    if (argc - i < 2) { fprintf(stderr, "chmod: missing operand\n"); return 1; }
    const char *spec = argv[i++];
    int rc = 0;
    for (; i < argc; i++) rc |= apply_chmod(argv[i], spec, rec, verbose);
    return rc;
}
