/* test / [ - check file types and compare values. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

static char **g_av;
static int g_pos, g_end;
static int test_or(void);

static int test_unary(const char *op, const char *arg)
{
    struct stat st;
    if (!strcmp(op, "-n")) return arg[0] != '\0';
    if (!strcmp(op, "-z")) return arg[0] == '\0';
    if (!strcmp(op, "-e")) return stat(arg, &st) == 0;
    if (!strcmp(op, "-f")) return stat(arg, &st) == 0 && S_ISREG(st.st_mode);
    if (!strcmp(op, "-d")) return stat(arg, &st) == 0 && S_ISDIR(st.st_mode);
    if (!strcmp(op, "-L") || !strcmp(op, "-h")) return lstat(arg, &st) == 0 && S_ISLNK(st.st_mode);
    if (!strcmp(op, "-p")) return stat(arg, &st) == 0 && S_ISFIFO(st.st_mode);
    if (!strcmp(op, "-c")) return stat(arg, &st) == 0 && S_ISCHR(st.st_mode);
    if (!strcmp(op, "-b")) return stat(arg, &st) == 0 && S_ISBLK(st.st_mode);
    if (!strcmp(op, "-s")) return stat(arg, &st) == 0 && st.st_size > 0;
    if (!strcmp(op, "-r")) return access(arg, R_OK) == 0;
    if (!strcmp(op, "-w")) return access(arg, W_OK) == 0;
    if (!strcmp(op, "-x")) return access(arg, X_OK) == 0;
    return 0;
}

static int test_prim(void)
{
    if (g_pos >= g_end) return 0;
    const char *t = g_av[g_pos];
    if (!strcmp(t, "!")) { g_pos++; return !test_prim(); }
    if (!strcmp(t, "(")) {
        g_pos++; int v = test_or();
        if (g_pos < g_end && !strcmp(g_av[g_pos], ")")) g_pos++;
        return v;
    }
    if (g_pos + 1 < g_end && t[0] == '-' && t[1] && !t[2]) { g_pos += 2; return test_unary(t, g_av[g_pos - 1]); }
    if (g_pos + 2 < g_end) {
        const char *op = g_av[g_pos + 1], *rhs = g_av[g_pos + 2];
        if (!strcmp(op, "=") || !strcmp(op, "==")) { g_pos += 3; return strcmp(t, rhs) == 0; }
        if (!strcmp(op, "!=")) { g_pos += 3; return strcmp(t, rhs) != 0; }
        if (!strcmp(op, "-eq")) { g_pos += 3; return strtoll(t, NULL, 10) == strtoll(rhs, NULL, 10); }
        if (!strcmp(op, "-ne")) { g_pos += 3; return strtoll(t, NULL, 10) != strtoll(rhs, NULL, 10); }
        if (!strcmp(op, "-lt")) { g_pos += 3; return strtoll(t, NULL, 10) <  strtoll(rhs, NULL, 10); }
        if (!strcmp(op, "-le")) { g_pos += 3; return strtoll(t, NULL, 10) <= strtoll(rhs, NULL, 10); }
        if (!strcmp(op, "-gt")) { g_pos += 3; return strtoll(t, NULL, 10) >  strtoll(rhs, NULL, 10); }
        if (!strcmp(op, "-ge")) { g_pos += 3; return strtoll(t, NULL, 10) >= strtoll(rhs, NULL, 10); }
        struct stat s1, s2;
        if (!strcmp(op, "-nt")) { g_pos += 3; return stat(t, &s1) == 0 && stat(rhs, &s2) == 0 && s1.st_mtime > s2.st_mtime; }
        if (!strcmp(op, "-ot")) { g_pos += 3; return stat(t, &s1) == 0 && stat(rhs, &s2) == 0 && s1.st_mtime < s2.st_mtime; }
    }
    g_pos++; return t[0] != '\0';
}

static int test_and(void) { int v = test_prim(); while (g_pos < g_end && !strcmp(g_av[g_pos], "-a")) { g_pos++; int r = test_prim(); v = v && r; } return v; }
static int test_or(void)  { int v = test_and();  while (g_pos < g_end && !strcmp(g_av[g_pos], "-o")) { g_pos++; int r = test_and();  v = v || r; } return v; }

int main(int argc, char **argv)
{
    int end = argc;
    if (argv[0]) {
        const char *leaf = argv[0];
        for (const char *s = argv[0]; *s; s++) if (*s == '/' || *s == ':') leaf = s + 1;
        if (strcmp(leaf, "[") == 0) {
            if (argc < 2 || strcmp(argv[argc - 1], "]") != 0) { fprintf(stderr, "[: missing ']'\n"); return 2; }
            end = argc - 1;
        }
    }
    if (end <= 1) return 1;
    g_av = argv; g_pos = 1; g_end = end;
    return test_or() ? 0 : 1;
}
