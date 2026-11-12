/* hostid - print the numeric identifier for the current host. */
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    printf("%08lx\n", (unsigned long)gethostid());
    return 0;
}
