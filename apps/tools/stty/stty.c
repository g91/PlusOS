/* stty - change and print terminal line settings. */
#include <stdio.h>
#include <string.h>
#include <termios.h>

int main(int argc, char **argv)
{
    struct termios t;
    if (tcgetattr(0, &t) != 0) { perror("stty"); return 1; }
    if (argc == 1 || (argc == 2 && !strcmp(argv[1], "-a"))) {
        printf("speed %lu baud; rows %u; columns %u; line = 0;\n",
               (unsigned long)t.baud, t.rows, t.cols);
        printf("%sicanon %secho %sisig %sicrnl %sopost\n",
               (t.c_lflag & ICANON) ? "" : "-", (t.c_lflag & ECHO) ? "" : "-",
               (t.c_lflag & ISIG) ? "" : "-", (t.c_iflag & ICRNL) ? "" : "-",
               (t.c_oflag & OPOST) ? "" : "-");
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "size")) { printf("%u %u\n", t.rows, t.cols); return 0; }
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "echo")) t.c_lflag |= ECHO;
        else if (!strcmp(argv[i], "-echo")) t.c_lflag &= ~ECHO;
        else if (!strcmp(argv[i], "icanon")) t.c_lflag |= ICANON;
        else if (!strcmp(argv[i], "-icanon")) t.c_lflag &= ~ICANON;
        else if (!strcmp(argv[i], "isig")) t.c_lflag |= ISIG;
        else if (!strcmp(argv[i], "-isig")) t.c_lflag &= ~ISIG;
        else if (!strcmp(argv[i], "raw")) t.c_lflag &= ~(ECHO | ICANON | ISIG);
        else if (!strcmp(argv[i], "sane") || !strcmp(argv[i], "cooked")) t.c_lflag |= (ECHO | ICANON | ISIG);
    }
    return tcsetattr(0, TCSANOW, &t) == 0 ? 0 : 1;
}
