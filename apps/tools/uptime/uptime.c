/* uptime - PlusOS System Metrics & Uptime Utility */
#include <plusos/plusos.h>
#include <plusos/syscall.h>

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    struct plusos_sysinfo si;
    plusos_syscall(SYS_sysinfo, &si);

    struct plusos_perf_metrics pm;
    memset(&pm, 0, sizeof pm);
    plusos_syscall(SYS_perfmetrics, &pm);

    struct plusos_ramdisk_args rd;
    memset(&rd, 0, sizeof rd);
    rd.op = PLUSOS_RAMDISK_INFO;
    plusos_syscall(SYS_ramdisk, &rd);

    unsigned long up = si.ticks / 60;
    unsigned long days = up / 86400;
    unsigned long hours = (up % 86400) / 3600;
    unsigned long mins = (up % 3600) / 60;
    unsigned long secs = up % 60;

    unsigned long l1_int = pm.load_1m_fixed >> 16;
    unsigned long l1_frac = ((pm.load_1m_fixed & 0xFFFF) * 100) >> 16;
    unsigned long l5_int = pm.load_5m_fixed >> 16;
    unsigned long l5_frac = ((pm.load_5m_fixed & 0xFFFF) * 100) >> 16;

    if (days > 0) {
        printf("up %lu day%s, %lu:%02lu:%02lu",
               days, days == 1 ? "" : "s", hours, mins, secs);
    } else {
        printf("up %lu:%02lu:%02lu", hours, mins, secs);
    }

    printf(", %d processes, load average: %lu.%02lu, %lu.%02lu\n",
           si.nprocs, l1_int, l1_frac, l5_int, l5_frac);
    printf("Telemetry: %lu context switches, %lu syscalls, %lu VBL interrupts\n",
           (unsigned long)pm.context_switches,
           (unsigned long)pm.syscall_count,
           (unsigned long)pm.interrupt_ticks);
    printf("RAM Disk:  %lu / %lu blocks used (%lu KB)\n",
           (unsigned long)rd.used_blocks,
           (unsigned long)rd.total_blocks,
           (unsigned long)((rd.total_blocks * 512) >> 10));

    return 0;
}
