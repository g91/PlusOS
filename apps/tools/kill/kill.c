/* kill - send a signal to a process. */
#include <plusos/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>

static int parse_sig(const char *s)
{
    if (isdigit((unsigned char)s[0])) return atoi(s);
    if (!strncasecmp(s, "SIG", 3)) s += 3;
    if (!strcasecmp(s, "HUP")) return SIGHUP;
    if (!strcasecmp(s, "INT")) return SIGINT;
    if (!strcasecmp(s, "QUIT")) return SIGQUIT;
    if (!strcasecmp(s, "KILL")) return SIGKILL;
    if (!strcasecmp(s, "TERM")) return SIGTERM;
    if (!strcasecmp(s, "USR1")) return SIGUSR1;
    if (!strcasecmp(s, "USR2")) return SIGUSR2;
    if (!strcasecmp(s, "CONT")) return SIGCONT;
    if (!strcasecmp(s, "STOP")) return SIGSTOP;
    return -1;
}

int main(int argc, char **argv)
{
    int sig = SIGTERM, i = 1;
    if (argc < 2) { fprintf(stderr, "kill: usage: kill [-s sig | -sig] pid...\n"); return 1; }
    if (!strcmp(argv[i], "-l") || !strcmp(argv[i], "--list")) {
        if (i + 1 < argc) {
            int n = atoi(argv[i + 1]);
            puts(n==1?"HUP":n==2?"INT":n==3?"QUIT":n==9?"KILL":"TERM");
        } else {
            printf(" 1) SIGHUP\t 2) SIGINT\t 3) SIGQUIT\t 9) SIGKILL\n10) SIGUSR1\t12) SIGUSR2\t15) SIGTERM\t18) SIGCONT\t19) SIGSTOP\n");
        }
        return 0;
    }
    if (!strcmp(argv[i], "-s") && i + 1 < argc) { sig = parse_sig(argv[++i]); i++; }
    else if (argv[i][0] == '-' && argv[i][1] && strcmp(argv[i], "--")) { sig = parse_sig(argv[i] + 1); i++; }
    if (sig < 0) { fprintf(stderr, "kill: invalid signal\n"); return 1; }
    if (i < argc && !strcmp(argv[i], "--")) i++;
    if (i >= argc) { fprintf(stderr, "kill: missing pid\n"); return 1; }
    int rc = 0;
    for (; i < argc; i++) {
        int pid = atoi(argv[i]);
        struct plusos_posix_args pa = { 0 };
        pa.op = PLUSOS_POSIX_KILL_SIG; pa.a = pid; pa.b = sig;
        if (plusos_syscall(SYS_posix, &pa) != 0) { fprintf(stderr, "kill: (%d) - No such process\n", pid); rc = 1; }
    }
    return rc;
}
