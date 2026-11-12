/* unlink - call the unlink function to remove the specified file. */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc != 2) { fprintf(stderr, "unlink: expected 1 argument\n"); return 1; }
    if (unlink(argv[1]) != 0) {
        fprintf(stderr, "unlink: cannot unlink '%s': %s\n", argv[1], strerror(errno));
        return 1;
    }
    return 0;
}
