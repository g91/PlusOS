/* yes - output a string repeatedly until killed. */
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    int i = 1;
    if (i < argc && strcmp(argv[i], "--") == 0) i++;
    for (;;) {
        if (i >= argc) {
            if (putchar('y') == EOF || putchar('\n') == EOF) return 0;
        } else {
            for (int k = i; k < argc; k++) printf("%s%s", argv[k], k + 1 < argc ? " " : "");
            if (putchar('\n') == EOF) return 0;
        }
    }
}
