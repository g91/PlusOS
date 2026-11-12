/* hostname - show or set the system's host name. */
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc >= 2 && argv[1][0] != '-') {
        if (sethostname(argv[1], strlen(argv[1])) != 0) { perror("hostname"); return 1; }
        return 0;
    }
    char h[64];
    if (gethostname(h, sizeof h) != 0) return 1;
    puts(h);
    return 0;
}
