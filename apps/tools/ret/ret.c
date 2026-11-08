/* ret.c - the smallest possible user program: no I/O, just an exit code.
 * Used to isolate the ring-2 start-up/exit path from everything else. */
#include <plusos/plusos.h>

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    return 42;
}
