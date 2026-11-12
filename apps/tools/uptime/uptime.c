/* uptime - tell how long the system has been running. */
#include <plusos/syscall.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

int main(int argc, char **argv)
{
    int pretty = 0, since = 0, telemetry = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-p")) pretty = 1;
        else if (!strcmp(argv[i], "-s")) since = 1;
        else if (!strcmp(argv[i], "-v")) telemetry = 1;
    }
    struct plusos_sysinfo si = { 0 }; plusos_syscall(SYS_sysinfo, &si);
    struct plusos_perf_metrics pm = { 0 }; plusos_syscall(SYS_perfmetrics, &pm);
    unsigned long up = si.ticks / 60;
    unsigned long days = up / 86400, hrs = (up % 86400) / 3600, mins = (up % 3600) / 60;
    if (since) {
        time_t boot_t = time(NULL) - (time_t)up;
        struct tm *tm = localtime(&boot_t); char tb[32];
        if (tm && strftime(tb, sizeof tb, "%Y-%m-%d %H:%M:%S", tm) > 0) puts(tb);
        return 0;
    }
    if (pretty) {
        printf("up");
        if (days) printf(" %lu day%s,", days, days == 1 ? "" : "s");
        if (hrs) printf(" %lu hour%s,", hrs, hrs == 1 ? "" : "s");
        printf(" %lu minute%s\n", mins, mins == 1 ? "" : "s");
        return 0;
    }
    time_t now = time(NULL); struct tm *tm = localtime(&now); char clk[16] = "00:00:00";
    if (tm) strftime(clk, sizeof clk, "%H:%M:%S", tm);
    unsigned long l1i = pm.load_1m_fixed >> 16, l1f = ((pm.load_1m_fixed & 0xffff) * 100) >> 16;
    unsigned long l5i = pm.load_5m_fixed >> 16, l5f = ((pm.load_5m_fixed & 0xffff) * 100) >> 16;
    printf(" %s up ", clk);
    if (days) printf("%lu day%s, ", days, days == 1 ? "" : "s");
    printf("%2lu:%02lu,  1 user,  load average: %lu.%02lu, %lu.%02lu, %lu.%02lu\n",
           hrs, mins, l1i, l1f, l5i, l5f, l5i, l5f);
    if (telemetry) {
        printf("Telemetry: %lu context switches, %lu syscalls, %lu VBL interrupts\n",
               (unsigned long)pm.context_switches, (unsigned long)pm.syscall_count, (unsigned long)pm.interrupt_ticks);
    }
    return 0;
}
