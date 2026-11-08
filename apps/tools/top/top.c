/* top - PlusOS Real-Time Interactive System Monitor */
#include <plusos/plusos.h>
#include <plusos/syscall.h>

static const char *st_names[] = { "-", "ready", "run", "block", "zombie" };

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    char line[32];

    for (;;) {
        struct plusos_procinfo p[16];
        struct plusos_proclist_args pl = { p, 16 };
        int n = (int)plusos_syscall(SYS_proclist, &pl);

        struct plusos_sysinfo si;
        plusos_syscall(SYS_sysinfo, &si);

        struct plusos_perf_metrics pm;
        memset(&pm, 0, sizeof pm);
        plusos_syscall(SYS_perfmetrics, &pm);

        unsigned long up = si.ticks ? si.ticks : 1;
        con_clear();

        unsigned long l1_int = pm.load_1m_fixed >> 16;
        unsigned long l1_frac = ((pm.load_1m_fixed & 0xFFFF) * 100) >> 16;
        unsigned long l5_int = pm.load_5m_fixed >> 16;
        unsigned long l5_frac = ((pm.load_5m_fixed & 0xFFFF) * 100) >> 16;

        printf("PlusOS Top - %s  up %lu:%02lu:%02lu  Load: %lu.%02lu, %lu.%02lu\n",
               si.machine,
               (up / 60) / 3600, ((up / 60) / 60) % 60, (up / 60) % 60,
               l1_int, l1_frac, l5_int, l5_frac);
        printf("RAM: %luK total, %luK free  Syscalls: %lu  Switches: %lu\n\n",
               (unsigned long)(si.ram_bytes >> 10),
               (unsigned long)(si.free_bytes >> 10),
               (unsigned long)pm.syscall_count,
               (unsigned long)pm.context_switches);

        printf("PID  PPID  PRIO  PGID  STATE    CPU%%   RAM   NAME\n");
        printf("---  ----  ----  ----  -----    ----   ---   ----\n");
        for (int i = 0; i < n; i++) {
            printf("%3d  %4d  %4d  %4d  %-7s  %3lu%%  %3luK  %s\n",
                   p[i].pid,
                   p[i].parent,
                   p[i].priority,
                   p[i].pgid,
                   st_names[p[i].state < 5 ? p[i].state : 0],
                   p[i].cpu_ticks * 100 / up,
                   p[i].mem_bytes >> 10,
                   p[i].name);
        }

        printf("\n[k] kill  [n] nice/priority  [q] quit  [space] refresh\n");

        int k = 0;
        for (int t = 0; t < 60 && !(k = plusos->key_poll()); t++) {
            plusos_syscall(SYS_sleep, (void *)1);
        }

        if (k == 'q' || k == 'Q' || k == 0x1B) break;
        if (k == 'k' || k == 'K') {
            printf("PID to kill: ");
            plusos->key_line(line, sizeof line);
            int pid = atoi(line);
            if (pid > 0) plusos_syscall(SYS_kill, (void *)(intptr_t)pid);
        } else if (k == 'n' || k == 'N') {
            printf("PID to renice: ");
            plusos->key_line(line, sizeof line);
            int pid = atoi(line);
            printf("New priority (-20 to 19): ");
            plusos->key_line(line, sizeof line);
            int prio = atoi(line);
            struct plusos_prio_args pa = { PLUSOS_PRIO_SET, pid, prio };
            plusos_syscall(SYS_priority, &pa);
        }
    }

    con_clear();
    return 0;
}
