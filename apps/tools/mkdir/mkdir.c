/* mkdir - make directories. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

static int parse_mode_str(const char *spec, mode_t cur_mode, mode_t *out)
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
                if (*p == 'r') bits |= 0444; else if (*p == 'w') bits |= 0222; else bits |= 0111;
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

static int mkdir_p(const char *path, mode_t mode, int verbose)
{
    char *tmp = strdup(path);
    if (!tmp) return -1;
    size_t len = strlen(tmp);
    while (len > 1 && tmp[len - 1] == '/') tmp[--len] = '\0';
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            struct stat st;
            if (stat(tmp, &st) != 0) {
                if (mkdir(tmp, mode) != 0 && errno != EEXIST) { free(tmp); return -1; }
                if (verbose) printf("mkdir: created directory '%s'\n", tmp);
            }
            *p = '/';
        }
    }
    struct stat st;
    if (stat(tmp, &st) == 0) {
        free(tmp);
        if (!S_ISDIR(st.st_mode)) { errno = ENOTDIR; return -1; }
        return 0;
    }
    int rc = mkdir(tmp, mode);
    if (rc == 0 && verbose) printf("mkdir: created directory '%s'\n", tmp);
    free(tmp);
    return rc;
}

int main(int argc, char **argv)
{
    int parents = 0, verbose = 0, mode_set = 0, i = 1;
    mode_t um = umask(0); umask(um);
    mode_t mode = 0777 & ~(um & 0777);
    const char *mode_spec = NULL;

    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) { mode_spec = argv[++i]; continue; }
        for (const char *p = argv[i] + 1; *p; p++) {
            if (*p == 'p') parents = 1;
            else if (*p == 'v') verbose = 1;
            else if (*p == 'm' && *(p + 1)) { mode_spec = p + 1; break; }
            else { fprintf(stderr, "mkdir: invalid option -- '%c'\n", *p); return 1; }
        }
    }
    if (mode_spec) {
        if (parse_mode_str(mode_spec, 0777, &mode) != 0) {
            fprintf(stderr, "mkdir: invalid mode '%s'\n", mode_spec);
            return 1;
        }
        mode_set = 1;
    }
    if (i >= argc) { fprintf(stderr, "mkdir: missing operand\n"); return 1; }
    int rc = 0;
    for (; i < argc; i++) {
        int err = parents ? mkdir_p(argv[i], mode, verbose) : mkdir(argv[i], mode);
        if (!parents && err == 0 && verbose) printf("mkdir: created directory '%s'\n", argv[i]);
        if (err != 0) {
            fprintf(stderr, "mkdir: cannot create directory '%s': %s\n", argv[i], strerror(errno));
            rc = 1;
        } else if (mode_set) chmod(argv[i], mode);
    }
    return rc;
}
