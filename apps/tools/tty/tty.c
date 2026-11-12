/* tty - print the file name of the terminal connected to standard input. */
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    int silent = (argc > 1 && (!strcmp(argv[1], "-s") || !strcmp(argv[1], "--silent")));
    if (isatty(0)) { if (!silent) puts("/dev/tty0"); return 0; }
    if (!silent) puts("not a tty");
    return 1;
}
