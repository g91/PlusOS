/* echo - print the arguments. A bin program, run by the shell. */
#include <plusos/plusos.h>

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) printf("%s%s", argv[i], i + 1 < argc ? " " : "");
    printf("\n");
    return 0;
}
