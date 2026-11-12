/* shell.c - PlusOS Unix-capable command shell (pipelines, redirection, quoting, jobs). */
#include <plusos/plusos.h>
#include <plusos/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

#define MAXARGS   24
#define MAXSTAGES 6
#define MAXJOBS   16

static int last_status = 0;

static int streq(const char *a, const char *b) { return strcmp(a, b) == 0; }

/* ---- background jobs ---------------------------------------------------- */
typedef struct { int pid; int num; char name[16]; } Job;
static Job jobs[MAXJOBS];
static int next_job = 1;

static void job_add(int pid, const char *name)
{
    for (int i = 0; i < MAXJOBS; i++) {
        if (!jobs[i].pid) {
            jobs[i].pid = pid;
            jobs[i].num = next_job++;
            strlcpy(jobs[i].name, name, sizeof jobs[i].name);
            printf("[%d] pid %d  %s &\n", jobs[i].num, pid, name);
            return;
        }
    }
    printf("too many background jobs\n");
}

static void reap_jobs(void)
{
    for (;;) {
        struct plusos_reap r = { 0 };
        int code = (int)plusos_syscall(SYS_reapany, &r);
        if (!r.pid) break;
        for (int i = 0; i < MAXJOBS; i++) {
            if (jobs[i].pid == r.pid) {
                printf("[%d] done   %s (exit %d)\n", jobs[i].num, jobs[i].name, code);
                jobs[i].pid = 0;
                break;
            }
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
    for (int i = 0; i < MAXJOBS; i++) {
        if (jobs[i].pid && (num == 0 || jobs[i].num == num)) {
            int pid = jobs[i].pid;
            jobs[i].pid = 0;
            int code = (int)plusos_syscall(SYS_wait, (void *)(intptr_t)pid);
            last_status = code;
            if (code) printf("[%s exited with %d]\n", jobs[i].name, code);
            return;
        }
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

/* ---- variable & tilde expansion ----------------------------------------- */
static void expand_token(const char *src, char *out, int cap)
{
    int o = 0;
    int i = 0;
    /* Note: quotes have already been stripped at parse time or passed via markers;
     * for simple unquoted/double-quoted tokens this handles $VAR, $?, $$ and ~. */
    if (src[0] == '~' && (src[1] == 0 || src[1] == '/')) {
        const char *home = getenv("HOME");
        if (!home || !home[0]) home = "/";
        for (int k = 0; home[k] && o < cap - 1; k++) out[o++] = home[k];
        i = 1;
    }
    while (src[i] && o < cap - 1) {
        if (src[i] == '$') {
            if (src[i + 1] == '?') {
                char num[16];
                snprintf(num, sizeof num, "%d", last_status);
                for (int k = 0; num[k] && o < cap - 1; k++) out[o++] = num[k];
                i += 2;
                continue;
            }
            if (src[i + 1] == '$') {
                char num[16];
                snprintf(num, sizeof num, "%d", (int)getpid());
                for (int k = 0; num[k] && o < cap - 1; k++) out[o++] = num[k];
                i += 2;
                continue;
            }
            if ((src[i + 1] >= 'A' && src[i + 1] <= 'Z') ||
                (src[i + 1] >= 'a' && src[i + 1] <= 'z') ||
                src[i + 1] == '_' || src[i + 1] == '{') {
                int braced = (src[i + 1] == '{');
                i += braced ? 2 : 1;
                char kbuf[32]; int kl = 0;
                while (src[i] && ((braced && src[i] != '}') ||
                       (!braced && ((src[i] >= 'A' && src[i] <= 'Z') ||
                        (src[i] >= 'a' && src[i] <= 'z') ||
                        (src[i] >= '0' && src[i] <= '9') || src[i] == '_'))) &&
                       kl < (int)sizeof kbuf - 1) {
                    kbuf[kl++] = src[i++];
                }
                kbuf[kl] = 0;
                if (braced && src[i] == '}') i++;
                const char *val = getenv(kbuf);
                if (val) {
                    for (int k = 0; val[k] && o < cap - 1; k++) out[o++] = val[k];
                }
                continue;
            }
        }
        out[o++] = src[i++];
    }
    out[o] = 0;
}

typedef struct {
    char *argv[MAXARGS + 1];
    int   argc;
    char *in_file;
    char *out_file;
    int   out_append;
    char *err_file;
    int   err_append;
    int   err_to_out;
} Stage;

/* Tokenize one command segment (between ; / && / ||) with quotes & redirections */
static int parse_pipeline(char *cmd, Stage stages[MAXSTAGES], char *pool, int pool_cap, int *bg)
{
    int ns = 0;
    int pp = 0;
    *bg = 0;
    memset(stages, 0, sizeof(Stage) * MAXSTAGES);

    char *p = cmd;
    while (*p) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p || *p == '#') break;
        if (*p == '|') {
            if (stages[ns].argc == 0 || ns + 1 >= MAXSTAGES) return -1;
            ns++;
            p++;
            continue;
        }
        if (*p == '&' && p[1] == 0) { *bg = 1; break; }

        /* Redirection tokens */
        int redir = 0; /* 1=<, 2=>, 3=>>, 4=2>, 5=2>>, 6=2>&1 */
        if (p[0] == '2' && p[1] == '>' && p[2] == '&' && p[3] == '1') {
            stages[ns].err_to_out = 1;
            p += 4;
            continue;
        }
        if (p[0] == '2' && p[1] == '>' && p[2] == '>') { redir = 5; p += 3; }
        else if (p[0] == '2' && p[1] == '>') { redir = 4; p += 2; }
        else if (p[0] == '>' && p[1] == '>') { redir = 3; p += 2; }
        else if (p[0] == '>') { redir = 2; p++; }
        else if (p[0] == '<') { redir = 1; p++; }

        while (*p == ' ' || *p == '\t') p++;
        if (!*p && redir) return -1;

        char raw[128], exp[128];
        int rl = 0;
        int squoted = 0;
        while (*p && rl < (int)sizeof raw - 1) {
            if (*p == ' ' || *p == '\t' || *p == '|' || *p == '<' || *p == '>') break;
            if (*p == '\'') {
                squoted = 1;
                p++;
                while (*p && *p != '\'' && rl < (int)sizeof raw - 1) raw[rl++] = *p++;
                if (*p == '\'') p++;
            } else if (*p == '"') {
                p++;
                while (*p && *p != '"' && rl < (int)sizeof raw - 1) {
                    if (*p == '\\' && (p[1] == '"' || p[1] == '\\' || p[1] == '$')) p++;
                    raw[rl++] = *p++;
                }
                if (*p == '"') p++;
            } else if (*p == '\\' && p[1]) {
                p++;
                raw[rl++] = *p++;
            } else if (*p == '&' && p[1] == 0) {
                *bg = 1;
                p++;
                break;
            } else {
                raw[rl++] = *p++;
            }
        }
        raw[rl] = 0;

        if (!rl && redir) return -1;
        if (!rl && !redir) continue;

        if (squoted) strlcpy(exp, raw, sizeof exp);
        else expand_token(raw, exp, sizeof exp);

        int len = (int)strlen(exp) + 1;
        if (pp + len > pool_cap) return -1;
        char *dst = pool + pp;
        memcpy(dst, exp, (size_t)len);
        pp += len;

        if (redir == 1) stages[ns].in_file = dst;
        else if (redir == 2) { stages[ns].out_file = dst; stages[ns].out_append = 0; }
        else if (redir == 3) { stages[ns].out_file = dst; stages[ns].out_append = 1; }
        else if (redir == 4) { stages[ns].err_file = dst; stages[ns].err_append = 0; }
        else if (redir == 5) { stages[ns].err_file = dst; stages[ns].err_append = 1; }
        else if (stages[ns].argc < MAXARGS) {
            stages[ns].argv[stages[ns].argc++] = dst;
            stages[ns].argv[stages[ns].argc] = 0;
        }
    }
    return (stages[0].argc > 0) ? (ns + 1) : 0;
}

static int run_builtin(Stage *s, int *should_quit)
{
    char **argv = s->argv;
    int argc = s->argc;
    if (argc == 0) return 1;

    if (streq(argv[0], "exit") || streq(argv[0], "logout")) {
        *should_quit = 1;
        last_status = (argc >= 2) ? atoi(argv[1]) : 0;
        return 1;
    }
    if (streq(argv[0], "jobs"))    { cmd_jobs(); last_status = 0; return 1; }
    if (streq(argv[0], "fg"))      { cmd_fg(argc, argv); return 1; }
    if (streq(argv[0], "history")) { cmd_history(argc, argv); last_status = 0; return 1; }
    if (streq(argv[0], "cls") || streq(argv[0], "clear")) { con_clear(); last_status = 0; return 1; }
    if (streq(argv[0], "cd")) {
        const char *dst = (argc >= 2) ? argv[1] : getenv("HOME");
        if (!dst || !dst[0]) dst = "/";
        if (chdir(dst) < 0) {
            printf("cd: %s: no such directory\n", dst);
            last_status = 1;
        } else {
            last_status = 0;
        }
        return 1;
    }
    if (streq(argv[0], "export")) {
        for (int i = 1; i < argc; i++) {
            char *eq = strchr(argv[i], '=');
            if (eq) { *eq = 0; setenv(argv[i], eq + 1, 1); *eq = '='; }
        }
        last_status = 0;
        return 1;
    }
    if (streq(argv[0], "unset")) {
        for (int i = 1; i < argc; i++) unsetenv(argv[i]);
        last_status = 0;
        return 1;
    }
    if (streq(argv[0], "umask")) {
        if (argc < 2) {
            struct plusos_posix_args pa = { PLUSOS_POSIX_GETUMASK };
            printf("%04o\n", (unsigned)plusos_syscall(SYS_posix, &pa));
        } else {
            mode_t m = (mode_t)strtoul(argv[1], 0, 8);
            umask(m);
        }
        last_status = 0;
        return 1;
    }
    return 0;
}

static int spawn_redir(const char *name, int argc, char **argv, int in_fd, int out_fd, int err_fd)
{
    struct plusos_posix_args pa;
    memset(&pa, 0, sizeof pa);
    pa.op = PLUSOS_POSIX_SPAWN_REDIR;
    pa.name = name;
    pa.argc = argc;
    pa.argv = argv;
    pa.fds[0] = in_fd;
    pa.fds[1] = out_fd;
    pa.fds[2] = err_fd;
    return (int)plusos_syscall(SYS_posix, &pa);
}

static int run_pipeline(char *cmd, int *should_quit)
{
    Stage stages[MAXSTAGES];
    char pool[512];
    int bg = 0;
    int n = parse_pipeline(cmd, stages, pool, sizeof pool, &bg);
    if (n < 0) { printf("syntax error\n"); last_status = 2; return 2; }
    if (n == 0) return 0;

    if (n == 1 && !stages[0].in_file && !stages[0].out_file && !bg) {
        if (run_builtin(&stages[0], should_quit)) return last_status;
    }

    int pipes[MAXSTAGES - 1][2];
    for (int i = 0; i < n - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            for (int k = 0; k < i; k++) { close(pipes[k][0]); close(pipes[k][1]); }
            printf("shell: pipe failed\n");
            last_status = 1;
            return 1;
        }
    }

    int pids[MAXSTAGES];
    for (int i = 0; i < n; i++) pids[i] = -1;

    for (int i = 0; i < n; i++) {
        int fd_in = (i == 0) ? -1 : pipes[i - 1][0];
        int fd_out = (i == n - 1) ? -1 : pipes[i][1];
        int fd_err = -1;
        int open_in = -1, open_out = -1, open_err = -1;

        if (stages[i].in_file) {
            open_in = open(stages[i].in_file, O_RDONLY);
            if (open_in < 0) { printf("%s: cannot open\n", stages[i].in_file); last_status = 1; goto next_close; }
            fd_in = open_in;
        }
        if (stages[i].out_file) {
            int flags = O_WRONLY | O_CREAT | (stages[i].out_append ? O_APPEND : O_TRUNC);
            open_out = open(stages[i].out_file, flags, 0644);
            if (open_out < 0) { printf("%s: cannot write\n", stages[i].out_file); last_status = 1; goto next_close; }
            fd_out = open_out;
        }
        if (stages[i].err_file) {
            int flags = O_WRONLY | O_CREAT | (stages[i].err_append ? O_APPEND : O_TRUNC);
            open_err = open(stages[i].err_file, flags, 0644);
            if (open_err >= 0) fd_err = open_err;
        } else if (stages[i].err_to_out) {
            fd_err = fd_out;
        }

        pids[i] = spawn_redir(stages[i].argv[0], stages[i].argc, stages[i].argv, fd_in, fd_out, fd_err);
        if (pids[i] == -1001) printf("%s: not found\n", stages[i].argv[0]);
        else if (pids[i] < 0) printf("%s: cannot run (%d)\n", stages[i].argv[0], pids[i]);

    next_close:
        if (open_in >= 0)  close(open_in);
        if (open_out >= 0) close(open_out);
        if (open_err >= 0) close(open_err);
        if (i > 0)         close(pipes[i - 1][0]);
        if (i < n - 1)     close(pipes[i][1]);
    }

    if (bg && pids[n - 1] > 0) {
        for (int i = 0; i < n - 1; i++)
            if (pids[i] > 0) job_add(pids[i], stages[i].argv[0]);
        job_add(pids[n - 1], stages[n - 1].argv[0]);
        return 0;
    }

    int final_code = 127;
    for (int i = 0; i < n; i++) {
        if (pids[i] > 0) {
            int code = (int)plusos_syscall(SYS_wait, (void *)(intptr_t)pids[i]);
            if (i == n - 1) final_code = code;
        }
    }
    if (n == 1 && streq(stages[0].argv[0], "desktop")) con_clear();
    else if (final_code && !bg) printf("[%s exited with %d]\n", stages[n - 1].argv[0], final_code);
    last_status = final_code;
    return final_code;
}

static int execute(char *line)
{
    int should_quit = 0;
    char *p = line;
    while (*p && !should_quit) {
        char *seg = p;
        int op = 0; /* 0=end, 1=;, 2=&&, 3=|| */
        int in_sq = 0, in_dq = 0;
        while (*p) {
            if (*p == '\'' && !in_dq) in_sq = !in_sq;
            else if (*p == '"' && !in_sq) in_dq = !in_dq;
            else if (!in_sq && !in_dq) {
                if (*p == ';') { op = 1; *p++ = 0; break; }
                if (p[0] == '&' && p[1] == '&') { op = 2; p[0] = p[1] = 0; p += 2; break; }
                if (p[0] == '|' && p[1] == '|') { op = 3; p[0] = p[1] = 0; p += 2; break; }
            }
            p++;
        }
        run_pipeline(seg, &should_quit);
        if (should_quit) break;
        if (op == 2 && last_status != 0) break;
        if (op == 3 && last_status == 0) break;
    }
    return should_quit ? 1 : 0;
}

static void make_prompt(char *out, int cap)
{
    const char *user = getenv("USER"); if (!user || !user[0]) user = "user";
    char ucopy[16]; strlcpy(ucopy, user, sizeof ucopy);
    const char *home = getenv("HOME"); char hcopy[64] = ""; if (home) strlcpy(hcopy, home, sizeof hcopy);
    char pwd[96] = "/"; getcwd(pwd, sizeof pwd);

    const char *disp = pwd;
    char tilde[96];
    if (hcopy[0]) {
        int i = 0;
        while (hcopy[i] && hcopy[i] == pwd[i]) i++;
        if (hcopy[i] == 0 && (pwd[i] == 0 || pwd[i] == '/')) {
            tilde[0] = '~';
            strlcpy(tilde + 1, pwd + i, sizeof tilde - 1);
            disp = tilde;
        }
    }
    const char *sigil = streq(ucopy, "root") ? "#" : "$";
    snprintf(out, (size_t)cap, "%s:%s%s ", ucopy, disp, sigil);
}

int main(int argc, char **argv)
{
    if (argc >= 3 && streq(argv[1], "-c")) {
        char buf[256];
        strlcpy(buf, argv[2], sizeof buf);
        execute(buf);
        return last_status;
    }
    if (argc >= 2 && !streq(argv[1], "-i")) {
        FILE *f = fopen(argv[1], "r");
        if (!f) { perror(argv[1]); return 1; }
        char line[256];
        while (fgets(line, sizeof line, f)) {
            int l = (int)strlen(line);
            while (l > 0 && (line[l - 1] == '\n' || line[l - 1] == '\r')) line[--l] = 0;
            if (execute(line)) break;
        }
        fclose(f);
        return last_status;
    }

    char pwd_check[96] = "";
    if (!getcwd(pwd_check, sizeof pwd_check) || !pwd_check[0]) chdir("/");
    const char *home = getenv("HOME");
    if (home && home[0] && streq(pwd_check, "/")) chdir(home);

    char line[256], prompt[128];
    printf("\nPlusOS shell by the1Domo. Programs live in bin - try 'help'.\n");
    for (;;) {
        reap_jobs();
        make_prompt(prompt, sizeof prompt);
        printf("%s", prompt);
        plusos->key_line(line, sizeof line);
        if (execute(line)) break;
    }
    return last_status;
}
