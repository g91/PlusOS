/* chmod - change the advisory permission mode of a file.
 *
 * Usage: chmod MODE PATH
 * MODE is in octal (e.g. 0644, 0755, 0600). */
#include <plusos/plusos.h>
#include <plusos/syscall.h>

static int parse_octal(const char *s)
{
    int v = 0;
    while (*s >= '0' && *s <= '7') { v = v * 8 + (*s - '0'); s++; }
    return v;
}

int main(int argc, char **argv)
{
    if (argc < 3) { printf("usage: chmod MODE PATH\n"); return 1; }
    int mode = parse_octal(argv[1]);
    struct plusos_perms_args p = { 0 };
    p.op   = PLUSOS_PERMS_CHMOD;
    p.mode = mode;
    for (int i = 0; i < 47 && argv[2][i]; i++) p.path[i] = argv[2][i];
    if (plusos_syscall(SYS_perms, &p) != 0) {
        printf("chmod: %s: permission denied or write failed\n", argv[2]);
        return 1;
    }
    printf("chmod: %04o %s\n", mode, argv[2]);
    return 0;
}
