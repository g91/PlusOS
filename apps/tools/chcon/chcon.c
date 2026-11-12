/* chcon - change file security context. */
#include <stdio.h>

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    fprintf(stderr, "chcon: SELinux security context is not enabled on this kernel\n");
    return 1;
}
