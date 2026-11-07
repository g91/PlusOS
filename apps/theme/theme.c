/* theme.c - PlusOS Theme Manager utility (CLI & interactive selector).
 *
 * Usage:
 *   theme                Show current active theme and list all 6 OS themes
 *   theme list           List all available themes with descriptions
 *   theme <name|id>      Switch to Mac, Amiga, Win95, Win98, Aero/Vista/7, or Hacker
 *   theme next           Cycle to the next OS theme
 */
#include <plusos/plusos.h>
#include <plusos/syscall.h>
#include <plusos/theme.h>

static void print_themes(int cur)
{
    con_invert(1);
    printf(" PlusOS Multi-OS Theme System                                  \n");
    con_invert(0);
    printf("\n");
    for (int i = 0; i < PLUSOS_THEME_COUNT; i++) {
        printf("  %s [%d] %-8s  %-22s  %s\n",
               (i == cur) ? "*" : " ",
               i + 1,
               plusos_theme_key(i),
               plusos_theme_name(i),
               plusos_theme_desc(i));
    }
    printf("\n");
    printf("Usage: theme <mac|amiga|win95|win98|aero|hacker|1..6|next>\n");
}

int main(int argc, char **argv)
{
    int cur = plusos_theme_get();

    if (argc < 2 || !strcmp(argv[1], "list") || !strcmp(argv[1], "-l") || !strcmp(argv[1], "--help")) {
        print_themes(cur);
        return 0;
    }

    int next = -1;
    if (!strcmp(argv[1], "next") || !strcmp(argv[1], "cycle")) {
        next = (cur + 1) % PLUSOS_THEME_COUNT;
    } else {
        next = plusos_theme_parse(argv[1]);
    }

    if (next < 0 || next >= PLUSOS_THEME_COUNT) {
        printf("theme: unknown theme \"%s\"\n\n", argv[1]);
        print_themes(cur);
        return 1;
    }

    plusos_theme_set(next);

    /* When switching to Hacker Mode, enable the Matrix rain wallpaper in registry */
    struct plusos_reg_args r;
    memset(&r, 0, sizeof r);
    r.key = "desktop.wallpaper";
    r.value = (next == PLUSOS_THEME_HACKER) ? "matrix" : "gray";
    plusos_syscall(SYS_regset, &r);

    printf("Active PlusOS theme set to: %s (%s)\n",
           plusos_theme_name(next), plusos_theme_desc(next));
    return 0;
}
