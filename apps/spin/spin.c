/* spin - burn the CPU in a tight loop that never yields, for N seconds
 * (default: until killed). A test of preemptive multitasking: while it runs,
 * the shell, the desktop and everything else must stay responsive.
 *
 *   spin 20 &      then try ps, taskman, the desktop - and kill it
 */
#include <plusos/plusos.h>

int main(int argc, char **argv)
{
    volatile uint32_t *ticks60 = (volatile uint32_t *)0x16A;   /* read directly: no syscall */
    uint32_t end = argc > 1 ? *ticks60 + (uint32_t)atoi(argv[1]) * 60 : 0;
    unsigned long n = 0;
    for (;;) {
        n++;
        if (end && *ticks60 >= end) break;
    }
    printf("spin: %lu loops\n", n);
    return 0;
}
