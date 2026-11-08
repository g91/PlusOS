/* sleep - wait for a number of seconds (handy with "&", e.g. sleep 10 &). */
#include <plusos/plusos.h>
#include <plusos/syscall.h>

int main(int argc, char **argv)
{
    int secs = argc >= 2 ? atoi(argv[1]) : 1;
    if (secs < 0) secs = 0;
    plusos_syscall(SYS_sleep, (void *)(intptr_t)(secs * 60));
    return 0;
}
