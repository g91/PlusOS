/* du - estimate file space usage. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

static void fmt_human(unsigned long long bytes, char *out, size_t cap, int si)
{
    static const char *const u[] = { "", "K", "M", "G", "T" };
    unsigned long base = si ? 1000UL : 1024UL;
    if (bytes < base) { snprintf(out, cap, "%llu", bytes); return; }
    int idx = 0; unsigned long long w = bytes; unsigned long rem = 0;
    while (w >= base && idx < 4) { rem = (unsigned long)(w % base); w /= base; idx++; }
    if (w < 10) snprintf(out, cap, "%llu.%lu%s", w, (rem * 10UL) / base, u[idx]);
    else snprintf(out, cap, "%llu%s", w, u[idx]);
}

static unsigned long long walk(const char *path, int all, int sum, int hum, int si, int app, int dep, int maxd)
{
    struct stat st;
    if (lstat(path, &st) != 0) { fprintf(stderr, "du: '%s': %s\n", path, strerror(errno)); return 0; }
    unsigned long long bytes = app ? (unsigned long long)(st.st_size > 0 ? st.st_size : 0)
                                   : (unsigned long long)((st.st_size + 511) & ~511);
    if (S_ISDIR(st.st_mode)) {
        if (bytes == 0 && !app) bytes = 512;
        DIR *d = opendir(path);
        if (d) {
            struct dirent *de;
            while ((de = readdir(d)) != NULL) {
                if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
                char *sub = malloc(192);
                if (!sub) break;
                size_t pl = strlen(path);
                snprintf(sub, 192, "%s%s%s", path, (pl && path[pl - 1] != '/') ? "/" : "", de->d_name);
                bytes += walk(sub, all, sum, hum, si, app, dep + 1, maxd);
                free(sub);
            }
            closedir(d);
        }
    }
    int show = (dep == 0) || (!sum && (maxd < 0 || dep <= maxd) && (all || S_ISDIR(st.st_mode)));
    if (show) {
        if (hum) { char hb[16]; fmt_human(bytes, hb, sizeof hb, si); printf("%s\t%s\n", hb, path); }
        else printf("%llu\t%s\n", (bytes + 1023ULL) / 1024ULL, path);
    }
    return bytes;
}

int main(int argc, char **argv)
{
    int all = 0, sum = 0, hum = 0, si = 0, app = 0, tot = 0, maxd = -1, i = 1;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) { maxd = atoi(argv[++i]); continue; }
        for (const char *p = argv[i] + 1; *p; p++) {
            if (*p == 'a') all = 1; else if (*p == 's') sum = 1;
            else if (*p == 'h') hum = 1; else if (*p == 'H') { hum = 1; si = 1; }
            else if (*p == 'b') app = 1; else if (*p == 'c') tot = 1;
        }
    }
    unsigned long long grand = 0;
    if (i >= argc) grand += walk(".", all, sum, hum, si, app, 0, maxd);
    else for (; i < argc; i++) grand += walk(argv[i], all, sum, hum, si, app, 0, maxd);
    if (tot) {
        if (hum) { char hb[16]; fmt_human(grand, hb, sizeof hb, si); printf("%s\ttotal\n", hb); }
        else printf("%llu\ttotal\n", (grand + 1023ULL) / 1024ULL);
    }
    return 0;
}
