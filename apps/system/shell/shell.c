/* shell.c - the PlusOS command shell, a ring-2 (userland) application.
 *
 * The shell is only a launcher. It has no built-in commands except the few that
 * must control the shell itself (exit, jobs, fg, cd). Every other command -
 * ls, echo, ps, kill, ... - is a separate program in the "bin" folder, run as
 * its own process. A trailing "&" runs a program in the background, so several
 * programs run at once; the shell reaps finished background jobs and prints a
 * note. Two routes reach this shell: it is the default program at boot, and the
 * desktop opens it; `exit` returns to the desktop.
 */
#include <plusos/plusos.h>
#include <plusos/syscall.h>

#define MAXARGS 24
#define MAXJOBS 16

static int streq(const char *a, const char *b)
{
    while (*a && *a == *b) { a++; b++; }
    return *a == 0 && *b == 0;
}

/* ---- background jobs ---------------------------------------------------- */
typedef struct { int pid; int num; char name[16]; } Job;
static Job jobs[MAXJOBS];
static int next_job = 1;

static void job_add(int pid, const char *name)
{
    for (int i = 0; i < MAXJOBS; i++)
        if (!jobs[i].pid) {
            jobs[i].pid = pid;
            jobs[i].num = next_job++;
            int j = 0;
            for (; name[j] && j < 15; j++) jobs[i].name[j] = name[j];
            jobs[i].name[j] = 0;
            printf("[%d] pid %d  %s &\n", jobs[i].num, pid, name);
            return;
        }
    printf("too many background jobs\n");
}

/* reap finished background jobs and report them */
static void reap_jobs(void)
{
    for (;;) {
        struct plusos_reap r = { 0 };
        int code = (int)plusos_syscall(SYS_reapany, &r);
        if (!r.pid) break;
        for (int i = 0; i < MAXJOBS; i++)
            if (jobs[i].pid == r.pid) {
                printf("[%d] done   %s (exit %d)\n", jobs[i].num, jobs[i].name, code);
                jobs[i].pid = 0;
                break;
            }
    }
}

static void cmd_jobs(void)
{
    int any = 0;
    for (int i = 0; i < MAXJOBS; i++)
        if (jobs[i].pid) { printf("[%d] pid %d  %s\n", jobs[i].num, jobs[i].pid, jobs[i].name); any = 1; }
    if (!any) printf("no background jobs\n");
}

static void cmd_fg(int argc, char **argv)
{
    int num = argc >= 2 ? atoi(argv[1]) : 0;
    for (int i = 0; i < MAXJOBS; i++)
        if (jobs[i].pid && (num == 0 || jobs[i].num == num)) {
            int pid = jobs[i].pid;
            jobs[i].pid = 0;
            int code = (int)plusos_syscall(SYS_wait, (void *)(intptr_t)pid);
            if (code) printf("[%s exited with %d]\n", jobs[i].name, code);
            return;
        }
    printf("no such job\n");
}

static void cmd_history(int argc, char **argv)
{
    struct plusos_history_args ha;
    if (argc >= 2 && (streq(argv[1], "-c") || streq(argv[1], "clear"))) {
        ha.op = PLUSOS_HIST_CLEAR;
        plusos_syscall(SYS_history, &ha);
        printf("command history cleared.\n");
        return;
    }
    ha.op = PLUSOS_HIST_COUNT;
    int n = (int)plusos_syscall(SYS_history, &ha);
    if (n <= 0) { printf("(history is empty)\n"); return; }
    char buf[80];
    for (int i = 0; i < n; i++) {
        ha.op = PLUSOS_HIST_GET;
        ha.index = i;
        ha.buf = buf;
        ha.size = sizeof buf;
        if (plusos_syscall(SYS_history, &ha) == 0)
            printf("  %2d  %s\n", i + 1, buf);
    }
}

/* ---- run a program ----------------------------------------------------- */
static void run(int argc, char **argv, int background)
{
    int pid = plusos->run(argv[0], argc, argv);      /* loader searches bin/ */
    if (pid == -1001) { printf("%s: not found\n", argv[0]); return; }
    if (pid < 0) { printf("%s: cannot run (%d)\n", argv[0], pid); return; }
    if (background) { job_add(pid, argv[0]); return; }
    int code = (int)plusos_syscall(SYS_wait, (void *)(intptr_t)pid);
    if (streq(argv[0], "desktop")) con_clear();
    else if (code) printf("[%s exited with %d]\n", argv[0], code);
}

/* returns 1 when the shell should quit */
static int execute(char *line)
{
    char *argv[MAXARGS + 1];
    int argc = 0, background = 0;
    char *p = line;
    while (*p && argc < MAXARGS) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;
        argv[argc++] = p;
        while (*p && *p != ' ' && *p != '\t') p++;
        if (*p) *p++ = 0;
    }
    argv[argc] = 0;
    if (argc && streq(argv[argc - 1], "&")) { background = 1; argv[--argc] = 0; }
    else if (argc) {                                 /* trailing & stuck to the word */
        char *w = argv[argc - 1];
        int n = 0; while (w[n]) n++;
        if (n > 1 && w[n - 1] == '&') { w[n - 1] = 0; background = 1; }
    }
    if (!argc) return 0;

    /* The only built-ins are the ones that act on the shell itself. */
    if (streq(argv[0], "exit") || streq(argv[0], "logout")) return 1;
    if (streq(argv[0], "jobs")) { cmd_jobs(); return 0; }
    if (streq(argv[0], "fg")) { cmd_fg(argc, argv); return 0; }
    if (streq(argv[0], "history")) { cmd_history(argc, argv); return 0; }
    if (streq(argv[0], "cls") || streq(argv[0], "clear")) { con_clear(); return 0; }
    run(argc, argv, background);
    return 0;
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    char line[128];
    printf("\nPlusOS shell. Programs live in bin - try 'help'.\n");
    for (;;) {
        reap_jobs();
        printf("plus> ");
        plusos->key_line(line, sizeof line);
        if (execute(line)) break;
    }
    return 0;
}
