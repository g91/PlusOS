/* sysinfo - machine, memory and uptime. */
#include <plusos/plusos.h>
#include <plusos/syscall.h>

int main(void)
{
    struct plusos_sysinfo si;
    plusos_syscall(SYS_sysinfo, &si);
    unsigned long up = si.ticks / 60;
    printf("%s, %s, %luK RAM\n", si.machine, si.cpu, si.ram_bytes >> 10);
    printf("boot volume \"%s\", %luK free in the ROM heap\n", si.volume, si.free_bytes >> 10);
    printf("up %lu:%02lu:%02lu, %d processes\n", up / 3600, (up / 60) % 60, up % 60, si.nprocs);
    if (si.is_se) {
        if (si.startup_id >= 0) printf("SCSI startup disk: ID %d\n", si.startup_id);
        else printf("SCSI startup disk: not set (highest ID boots)\n");
    }
    return 0;
}
