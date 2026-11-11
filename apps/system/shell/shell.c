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
    if (streq(argv[0], "cd")) {
        /* The kernel has no CWD, so cd just tracks a $PWD env var that other
         * programs (prompt, ls, etc.) consult.
         *   cd            -> $HOME
         *   cd ~          -> $HOME
         *   cd ~/sub      -> $HOME/sub
         *   cd /abs/path  -> /abs/path
         *   cd ..         -> drop last segment of $PWD
         *   cd foo        -> $PWD/foo                          (relative)
         * No real validation: if the directory doesn't exist, you only
         * notice when the next command complains. */
        const char *dst = (argc >= 2) ? argv[1] : "~";
        char expanded[128]; int o = 0;
        if (dst[0] == '~' && (dst[1] == 0 || dst[1] == '/')) {
            char home[64] = "";
            struct plusos_env_args eg = { PLUSOS_ENV_GET, "HOME", home, sizeof home };
            plusos_syscall(SYS_getenv, &eg);
            for (int i = 0; home[i] && o < (int)sizeof expanded - 1; i++) expanded[o++] = home[i];
            dst++;
        } else if (dst[0] != '/') {
            /* Relative: start from $PWD. */
            char pwd[96] = "/";
            struct plusos_env_args eg = { PLUSOS_ENV_GET, "PWD", pwd, sizeof pwd };
            plusos_syscall(SYS_getenv, &eg);
            for (int i = 0; pwd[i] && o < (int)sizeof expanded - 1; i++) expanded[o++] = pwd[i];
            if (o == 0 || expanded[o - 1] != '/') if (o < (int)sizeof expanded - 1) expanded[o++] = '/';
        }
        while (*dst && o < (int)sizeof expanded - 1) expanded[o++] = *dst++;
        expanded[o] = 0;

        /* Resolve ".." and "." segments. In-place rewrite. */
        char tidy[128]; int ti = 0;
        int i = 0;
        if (expanded[0] == '/') { tidy[ti++] = '/'; i = 1; }
        while (expanded[i]) {
            while (expanded[i] == '/') i++;
            if (!expanded[i]) break;
            char seg[32]; int s = 0;
            while (expanded[i] && expanded[i] != '/' && s < 31) seg[s++] = expanded[i++];
            seg[s] = 0;
            if (streq(seg, ".")) continue;
            if (streq(seg, "..")) {
                /* strip last segment of tidy */
                if (ti > 1) {
                    ti--;                            /* drop trailing '/' */
                    while (ti > 0 && tidy[ti - 1] != '/') ti--;
                }
                continue;
            }
            for (int k = 0; seg[k] && ti < (int)sizeof tidy - 2; k++) tidy[ti++] = seg[k];
            if (expanded[i] == '/') tidy[ti++] = '/';
        }
        if (ti == 0) { tidy[0] = '/'; ti = 1; }
        /* Strip a trailing '/' unless this is the root. */
        if (ti > 1 && tidy[ti - 1] == '/') ti--;
        tidy[ti] = 0;

        struct plusos_env_args es = { PLUSOS_ENV_SET, "PWD", tidy, 0 };
        plusos_syscall(SYS_setenv, &es);
        return 0;
    }
    run(argc, argv, background);
    return 0;
}

/* Short, Unix-style form of PWD for the prompt: $HOME -> "~", everything
 * else as-is. Returns a pointer into pwd (or "~" or "/"). */
static void make_prompt(char *out, int cap)
{
    char user[16] = "user", home[64] = "", pwd[96] = "/";
    struct plusos_env_args e;
    e.op = PLUSOS_ENV_GET; e.key = "USER"; e.value = user; e.val_size = sizeof user; plusos_syscall(SYS_getenv, &e);
    e.op = PLUSOS_ENV_GET; e.key = "HOME"; e.value = home; e.val_size = sizeof home; plusos_syscall(SYS_getenv, &e);
    e.op = PLUSOS_ENV_GET; e.key = "PWD";  e.value = pwd;  e.val_size = sizeof pwd;  plusos_syscall(SYS_getenv, &e);

    /* Collapse $HOME prefix to ~ for a tidy prompt. */
    const char *disp = pwd;
    char tilde[96];
    if (home[0]) {
        int i = 0;
        while (home[i] && home[i] == pwd[i]) i++;
        if (home[i] == 0 && (pwd[i] == 0 || pwd[i] == '/')) {
            tilde[0] = '~';
            int o = 1;
            while (pwd[i] && o < (int)sizeof tilde - 1) tilde[o++] = pwd[i++];
            tilde[o] = 0;
            disp = tilde;
        }
    }
    /* Root gets a '#' sigil, everyone else '$', as Unix does. */
    const char *sigil = streq(user, "root") ? "#" : "$";
    int o = 0;
    for (int i = 0; user[i] && o < cap - 1; i++) out[o++] = user[i];
    if (o < cap - 1) out[o++] = ':';
    for (int i = 0; disp[i] && o < cap - 1; i++) out[o++] = disp[i];
    for (int i = 0; sigil[i] && o < cap - 1; i++) out[o++] = sigil[i];
    if (o < cap - 1) out[o++] = ' ';
    out[o] = 0;
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;

    /* First time the shell runs after login, seed $PWD from $HOME so the
     * user starts "in" their home directory, as a Unix login shell does. */
    char pwd_check[96] = "";
    struct plusos_env_args eg = { PLUSOS_ENV_GET, "PWD", pwd_check, sizeof pwd_check };
    plusos_syscall(SYS_getenv, &eg);
    if (!pwd_check[0]) {
        char home[64] = "/";
        struct plusos_env_args eh = { PLUSOS_ENV_GET, "HOME", home, sizeof home };
        plusos_syscall(SYS_getenv, &eh);
        if (!home[0]) { home[0] = '/'; home[1] = 0; }
        struct plusos_env_args es = { PLUSOS_ENV_SET, "PWD", home, 0 };
        plusos_syscall(SYS_setenv, &es);
    }

    char line[128], prompt[128];
    printf("\nPlusOS shell by the1Domo. Programs live in bin - try 'help'.\n");
    for (;;) {
        reap_jobs();
        make_prompt(prompt, sizeof prompt);
        printf("%s", prompt);
        plusos->key_line(line, sizeof line);
        if (execute(line)) break;
    }
    return 0;
}
