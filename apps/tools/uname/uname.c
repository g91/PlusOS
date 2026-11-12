/* uname - print system information. */
#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>

int main(int argc, char **argv)
{
    int s = 0, n = 0, r = 0, v = 0, m = 0, p = 0, o = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--all")) { s = n = r = v = m = p = o = 1; continue; }
        if (argv[i][0] != '-') continue;
        for (const char *q = argv[i] + 1; *q; q++) {
            if (*q == 'a') s = n = r = v = m = p = o = 1;
            else if (*q == 's') s = 1; else if (*q == 'n') n = 1; else if (*q == 'r') r = 1;
            else if (*q == 'v') v = 1; else if (*q == 'm') m = 1;
            else if (*q == 'p' || *q == 'i') p = 1; else if (*q == 'o') o = 1;
        }
    }
    if (!s && !n && !r && !v && !m && !p && !o) s = 1;
    struct utsname u;
    if (uname(&u) != 0) return 1;
    int first = 1;
    #define EMIT(cond, val) do { if (cond) { printf("%s%s", first ? "" : " ", val); first = 0; } } while(0)
    EMIT(s, u.sysname); EMIT(n, u.nodename); EMIT(r, u.release); EMIT(v, u.version);
    EMIT(m, u.machine); EMIT(p, u.machine); EMIT(o, "PlusOS");
    #undef EMIT
    putchar('\n');
    return 0;
}
