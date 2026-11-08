/* dmesg - PlusOS Kernel Ring Buffer Log Viewer */
#include <plusos/plusos.h>
#include <plusos/syscall.h>

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    static char buf[4096];
    struct plusos_dmesg_args d;
    memset(&d, 0, sizeof d);
    d.buf = buf;
    d.max_len = sizeof(buf);
    d.offset = 0;

    int n = (int)plusos_syscall(SYS_dmesg, &d);
    if (n <= 0) {
        printf("(kernel event log is empty)\n");
        return 0;
    }

    /* Print log output */
    printf("%s", buf);
    return 0;
}
