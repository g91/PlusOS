/* cat - print a text file from the boot volume.
 *
 * Accepts both HFS colon paths (":Desktop:notes") and Unix-style paths
 * ("/etc/passwd", "~/notes"). "~" expands to $HOME. */
#include <plusos/plusos.h>
#include <plusos/syscall.h>
#include <plusrom/FileMgr.h>

/* Translate /foo/bar -> :foo:bar, ~/foo -> $HOME:foo, foo -> $PWD:foo.
 * Writes at most cap bytes including the trailing NUL. */
static void translate(const char *in, char *out, int cap)
{
    const char *src = in;
    int o = 0;
    char env[96] = "";
    if (src[0] == '~' && (src[1] == 0 || src[1] == '/')) {
        struct plusos_env_args e = { PLUSOS_ENV_GET, "HOME", env, sizeof env };
        plusos_syscall(SYS_getenv, &e);
        src++;
    } else if (src[0] != '/') {
        /* Relative - prepend $PWD. */
        struct plusos_env_args e = { PLUSOS_ENV_GET, "PWD", env, sizeof env };
        plusos_syscall(SYS_getenv, &e);
        if (!env[0]) { env[0] = '/'; env[1] = 0; }
        /* make sure there's a separator between pwd and the relative part */
        int n = 0; while (env[n]) n++;
        if (n > 0 && env[n - 1] != '/' && n < (int)sizeof env - 1) { env[n] = '/'; env[n + 1] = 0; }
    }
    for (int i = 0; env[i] && o < cap - 1; i++)
        out[o++] = (env[i] == '/') ? ':' : env[i];

    int last_sep = (o > 0 && out[o - 1] == ':');
    while (*src && o < cap - 1) {
        char c = *src++;
        if (c == '/') { if (last_sep) continue; out[o++] = ':'; last_sep = 1; }
        else          { out[o++] = c;              last_sep = 0; }
    }
    out[o] = 0;
}

static void p(const char *name, INTEGER vref)
{
    char hfs[128];
    translate(name, hfs, sizeof hfs);

    /* Advisory permission check: let a well-behaved user program refuse to
     * display a file the current user has no read bit on. The kernel cannot
     * physically enforce this on HFS calls, so a hostile program could still
     * read the file by calling FSOpen directly; see kernel/services/perms.h. */
    struct plusos_perms_args pr = { 0 };
    pr.op = PLUSOS_PERMS_CHECK; pr.want = 4; /* PERMS_R */
    /* Pass the Unix form of the path, since /etc/perms.db stores Unix paths. */
    for (int i = 0; i < 47 && name[i]; i++) pr.path[i] = name[i];
    if (plusos_syscall(SYS_perms, &pr) == 0) {
        printf("cat: %s: permission denied\n", name);
        return;
    }

    unsigned char pn[256];
    int n = 0;
    while (hfs[n] && n < 254) { pn[n + 1] = hfs[n]; n++; }
    pn[0] = n;
    INTEGER ref;
    if (FSOpen(pn, vref, &ref) != 0) { printf("cat: %s: not found\n", name); return; }
    char buf[256];
    for (;;) {
        LONGINT got = sizeof buf;
        OSErr e = FSRead(ref, &got, buf);
        if (got > 0) plusos->con_write(buf, (size_t)got);
        if (e != 0) break;                  /* 0 = ok; -39 (eofErr) ends it */
    }
    FSClose(ref);
}

int main(int argc, char **argv)
{
    if (argc < 2) { printf("usage: cat <file>\n"); return 1; }
    INTEGER vref = (INTEGER)plusos->boot_vrefnum;
    for (int i = 1; i < argc; i++) p(argv[i], vref);
    return 0;
}
