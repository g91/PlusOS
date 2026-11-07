/* startup - show or set which program PlusOS runs at boot. */
#include <plusos/plusos.h>
#include <plusos/syscall.h>

static int streq(const char *a, const char *b)
{
    while (*a && *a == *b) { a++; b++; }
    return *a == 0 && *b == 0;
}

int main(int argc, char **argv)
{
    if (argc >= 2) {
        int v = streq(argv[1], "desktop") ? PLUSOS_BOOT_DESKTOP
              : streq(argv[1], "shell")   ? PLUSOS_BOOT_SHELL : -1;
        if (v < 0) { printf("usage: startup [shell|desktop]\n"); return 1; }
        struct plusos_cfg_args c = { PLUSOS_CFG_BOOT, v };
        if (plusos_syscall(SYS_setcfg, &c) < 0) { printf("could not save the setting\n"); return 1; }
    }
    int cur = (int)plusos_syscall(SYS_getcfg, (void *)PLUSOS_CFG_BOOT);
    printf("At boot PlusOS starts the %s.\n", cur == PLUSOS_BOOT_DESKTOP ? "desktop" : "shell");
    if (argc < 2) printf("(startup shell  or  startup desktop  to change)\n");
    return 0;
}
