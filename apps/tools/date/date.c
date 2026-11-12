/* date - print or set the system date and time. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char **argv)
{
    int utc = 0; const char *fmt = NULL;
    time_t now = time(NULL);
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-u") || !strcmp(argv[i], "--utc")) utc = 1;
        else if (!strcmp(argv[i], "-R")) fmt = "%a, %d %b %Y %H:%M:%S +0000";
        else if (!strcmp(argv[i], "-I")) fmt = "%Y-%m-%d";
        else if (!strcmp(argv[i], "-d") && i + 1 < argc) {
            const char *ds = argv[++i];
            if (ds[0] == '@') now = (time_t)strtoul(ds + 1, NULL, 10);
        } else if (!strcmp(argv[i], "-s") && i + 1 < argc) {
            const char *ss = argv[++i];
            if (ss[0] == '@') {
                time_t nt = (time_t)strtoul(ss + 1, NULL, 10);
                if (stime(&nt) != 0) { perror("date"); return 1; }
                now = nt;
            }
        } else if (argv[i][0] == '+') fmt = argv[i] + 1;
    }
    if (!fmt) fmt = "%a %b %e %H:%M:%S UTC %Y";
    struct tm *tm = utc ? gmtime(&now) : localtime(&now);
    char out[96];
    if (tm && strftime(out, sizeof out, fmt, tm) > 0) puts(out);
    else printf("%lu\n", (unsigned long)now);
    return 0;
}
