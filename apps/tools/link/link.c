/* link - call the link function to create a link to a file. */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc != 3) { fprintf(stderr, "link: expected 2 arguments\n"); return 1; }
    if (link(argv[1], argv[2]) != 0) {
        fprintf(stderr, "link: cannot create link '%s' to '%s': %s\n", argv[2], argv[1], strerror(errno));
        return 1;
    }
    return 0;
}
