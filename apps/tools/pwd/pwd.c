/* pwd - print name of current/working directory. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    int logical = 1;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-P") == 0 || strcmp(argv[i], "--physical") == 0) logical = 0;
        else if (strcmp(argv[i], "-L") == 0 || strcmp(argv[i], "--logical") == 0) logical = 1;
    }
    if (logical) {
        const char *p = getenv("PWD");
        if (p && p[0] == '/') { printf("%s\n", p); return 0; }
    }
    char buf[192];
    if (!getcwd(buf, sizeof buf)) { perror("pwd"); return 1; }
    printf("%s\n", buf);
    return 0;
}
