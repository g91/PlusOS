/* touch - change file timestamps. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <utime.h>

static int parse_touch_t(const char *s, time_t *out)
{
    struct tm tm;
    time_t now = time(NULL);
    struct tm *cur = localtime(&now);
    if (cur) tm = *cur; else memset(&tm, 0, sizeof tm);
    const char *dot = strchr(s, '.');
    int main_len = dot ? (int)(dot - s) : (int)strlen(s);
    int sec = (dot && dot[1]) ? atoi(dot + 1) : 0;
    int pos = 0;
    if (main_len == 12) {
        int cc = (s[0] - '0') * 10 + (s[1] - '0');
        int yy = (s[2] - '0') * 10 + (s[3] - '0');
        tm.tm_year = cc * 100 + yy - 1900; pos = 4;
    } else if (main_len == 10) {
        int yy = (s[0] - '0') * 10 + (s[1] - '0');
        tm.tm_year = (yy >= 69 ? yy : 100 + yy); pos = 2;
    } else if (main_len != 8) return -1;
    tm.tm_mon  = (s[pos] - '0') * 10 + (s[pos + 1] - '0') - 1;
    tm.tm_mday = (s[pos + 2] - '0') * 10 + (s[pos + 3] - '0');
    tm.tm_hour = (s[pos + 4] - '0') * 10 + (s[pos + 5] - '0');
    tm.tm_min  = (s[pos + 6] - '0') * 10 + (s[pos + 7] - '0');
    tm.tm_sec  = sec;
    *out = mktime(&tm);
    return 0;
}

static int parse_date_str(const char *s, time_t *out)
{
    if (!s || !*s) return -1;
    if (s[0] == '@') { *out = (time_t)strtoul(s + 1, NULL, 10); return 0; }
    if (strlen(s) >= 10 && isdigit((unsigned char)s[0]) && s[4] == '-' && s[7] == '-') {
        int y = atoi(s), mo = atoi(s + 5), d = atoi(s + 8), h = 0, mi = 0, sc = 0;
        const char *p = s;
        if (p[10] == ' ' || p[10] == 'T') {
            p += 11; h = atoi(p);
            const char *c1 = strchr(p, ':');
            if (c1) { mi = atoi(c1 + 1); const char *c2 = strchr(c1 + 1, ':'); if (c2) sc = atoi(c2 + 1); }
        }
        struct tm tm; memset(&tm, 0, sizeof tm);
        tm.tm_year = y - 1900; tm.tm_mon = mo - 1; tm.tm_mday = d;
        tm.tm_hour = h; tm.tm_min = mi; tm.tm_sec = sc;
        *out = mktime(&tm);
        return 0;
    }
    return -1;
}

int main(int argc, char **argv)
{
    int no_create = 0, use_ref = 0, use_stamp = 0, i = 1;
    time_t stamp = time(NULL);
    const char *ref_file = NULL;
    for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        if (strcmp(argv[i], "-r") == 0 && i + 1 < argc) { ref_file = argv[++i]; use_ref = 1; continue; }
        if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            if (parse_date_str(argv[++i], &stamp) != 0) { fprintf(stderr, "touch: invalid date '%s'\n", argv[i]); return 1; }
            use_stamp = 1; continue;
        }
        if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            if (parse_touch_t(argv[++i], &stamp) != 0) { fprintf(stderr, "touch: invalid time '%s'\n", argv[i]); return 1; }
            use_stamp = 1; continue;
        }
        for (const char *p = argv[i] + 1; *p; p++) if (*p == 'c') no_create = 1;
    }
    if (use_ref) {
        struct stat st;
        if (stat(ref_file, &st) != 0) { fprintf(stderr, "touch: '%s': %s\n", ref_file, strerror(errno)); return 1; }
        stamp = st.st_mtime; use_stamp = 1;
    }
    if (i >= argc) { fprintf(stderr, "touch: missing file operand\n"); return 1; }
    int rc = 0;
    for (; i < argc; i++) {
        struct stat st;
        if (stat(argv[i], &st) != 0) {
            if (no_create) continue;
            int fd = open(argv[i], O_WRONLY | O_CREAT, 0666);
            if (fd < 0) { fprintf(stderr, "touch: cannot touch '%s': %s\n", argv[i], strerror(errno)); rc = 1; continue; }
            close(fd);
        }
        if (use_stamp) { struct utimbuf ut = { stamp, stamp }; utime(argv[i], &ut); }
        else utime(argv[i], NULL);
    }
    return rc;
}
