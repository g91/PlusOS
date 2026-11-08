/* ps - list the running processes (a one-shot task list; see also taskman). */
#include <plusos/plusos.h>
#include <plusos/syscall.h>

static const char *st[] = { "-", "ready", "run", "block", "zombie" };

int main(void)
{
    struct plusos_procinfo p[16];
    struct plusos_proclist_args a = { p, 16 };
    int n = (int)plusos_syscall(SYS_proclist, &a);
    struct plusos_sysinfo si;
    plusos_syscall(SYS_sysinfo, &si);
    unsigned long up = si.ticks ? si.ticks : 1;
    printf("PID PPID TYPE  STATE  CPU%%  MEM  NAME\n");
    for (int i = 0; i < n; i++) {
        unsigned long cpu = p[i].cpu_ticks * 100 / up;
        printf("%3d %4d %-4s  %-6s %3lu%% %3luK  %s\n", p[i].pid, p[i].parent,
               p[i].ring == 0 ? "sys" : "user",
               (p[i].state >= 0 && p[i].state < 5) ? st[p[i].state] : "?",
               cpu, p[i].mem_bytes >> 10, p[i].name);
    }
    return 0;
}
