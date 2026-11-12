/* dircolors - color setup for ls. */
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    int csh = 0, print_db = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--csh") == 0) csh = 1;
        else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--print-database") == 0) print_db = 1;
    }
    if (print_db) { printf("DIR 01;34\nLINK 01;36\nFIFO 40;33\nEXEC 01;32\n"); return 0; }
    const char *val = "rs=0:di=01;34:ln=01;36:pi=40;33:ex=01;32:";
    if (csh) printf("setenv LS_COLORS '%s'\n", val);
    else printf("LS_COLORS='%s';\nexport LS_COLORS\n", val);
    return 0;
}
